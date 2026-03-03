/*
** loader.cpp
**
** Create profile and correctly assign if imported file is IWAD or PWAD
**
**---------------------------------------------------------------------------
**
** Copyright 2025-2026 UZDoom Maintainers and Contributors
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
**---------------------------------------------------------------------------
**
*/

#include "loader.h"
#include "gstrings.h"
#include "md5.h"
#include "miniz.h"
#include "profile.h"

#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <regex>
#include <string>
#include <vector>

using json = nlohmann::json;

enum fileType
{
	TYPE_IWAD,
	TYPE_IPK3,
	TYPE_IPK7,
	TYPE_PWAD,
	TYPE_PK3,
	TYPE_PK7, // i never seen this one myself but it seems to exist https://forum.zdoom.org/viewtopic.php?t=34552
	TYPE_UNKNOWN
};

// Adds the attributes to the profile based on hash (not good but 90% good enough)
void attributeFromFilename(Profile *p, std::string hash)
{
	// found a match? copy over
	if (wadDatabase.contains(hash))
	{
		WadInfo info = wadDatabase.at(hash);
		if (!info.title.empty())
			p->title = info.title;
		if (!info.author.empty())
			p->author = info.author;
		if (!info.releaseDate.empty())
			p->releaseDate = info.releaseDate;
		if (info.isIWAD)
			p->isIWAD = true; // fix for wrongly detected WADS
	}
}

fileType getFiletype(const std::string filepath)
{
	fileType status = TYPE_UNKNOWN;

	// we fetch the extension
	std::filesystem::path extPath(filepath);
	std::string           ext = extPath.extension().string();

	std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });

	// do a innocent check on the file to see if it's a valid IWAD
	if (ext == ".wad")
	{
		std::ifstream file(filepath, std::ios_base::in | std::ios_base::binary);

		if (!file.is_open())
		{
			// very bad case, this is not a readable file
			std::cerr << "Failed to open file: " << filepath << std::endl;
			return TYPE_UNKNOWN;
		}

		char magicHeader[4] = {0};
		file.read(magicHeader, 4);
		if (file.gcount() == 4)
		{
			// wad can be either iwad or pwad, need to be checked
			if (memcmp(magicHeader, "IWAD", 4) == 0)
			{
				return TYPE_IWAD;
			}
			else if (memcmp(magicHeader, "PWAD", 4) == 0)
			{
				return TYPE_PWAD;
			}
		}
	}

	// more checks based on clear extension
	if (ext == ".iwad")
		status = TYPE_IWAD;
	else if (ext == ".pwad" || ext == ".pwd")
		status = TYPE_PWAD;
	else if (ext == ".ipk3")
		status = TYPE_IPK3;
	else if (ext == ".ipk7")
		status = TYPE_IPK7;
	else if (ext == ".pk3")
		status = TYPE_PK3;
	else if (ext == ".pk7")
		status = TYPE_PK7;

	// return what we found (even if unknown)
	return status;
}

// Extracts zip using build-in miniz.
bool ExtractArchive(const std::string &archivePath, const std::string &targetDir)
{
	mz_zip_archive zip_archive;

	// Clear the struct to 0
	memset(&zip_archive, 0, sizeof(zip_archive));

	// Read file into memory
	std::ifstream file(archivePath, std::ios::binary | std::ios::ate);
	if (!file.is_open())
	{
		std::cerr << "miniz error: Failed to open archive " << archivePath << std::endl;
		return false;
	}

	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);
	std::vector<uint8_t> buffer(size);

	if (!file.read(reinterpret_cast<char *>(buffer.data()), size))
	{
		std::cerr << "miniz error: Failed to read archive " << archivePath << std::endl;
		return false;
	}

	// Try to open the zip file from memory
	if (!mz_zip_reader_init_mem(&zip_archive, buffer.data(), size, 0))
	{
		std::cerr << "miniz error: Failed to init zip from memory" << std::endl;
		return false;
	}

	int num_files = (int)mz_zip_reader_get_num_files(&zip_archive);

	for (int i = 0; i < num_files; i++)
	{
		mz_zip_archive_file_stat file_stat;
		if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat))
		{
			std::cerr << "miniz error: Failed to get file stat at index " << i << std::endl;
			mz_zip_reader_end(&zip_archive);
			return false;
		}

		// Safely combine the target directory with the zip's internal filename
		std::filesystem::path relativePath(file_stat.m_filename);
		if (relativePath.is_absolute())
		{
			relativePath = relativePath.relative_path(); // Strip root/drive letters
		}
		std::filesystem::path outPath = std::filesystem::path(targetDir) / relativePath;

		// Ban Zip Slip (just in case)
		std::string fname = file_stat.m_filename;
		if (fname.find("..") != std::string::npos)
		{
			continue; // Reject dangerous relative paths
		}

		if (mz_zip_reader_is_file_a_directory(&zip_archive, i))
		{
			// It's a directory, so just ensure it exists
			std::filesystem::create_directories(outPath);
		}
		else
		{
			// It's a file. Ensure its parent folder exists (in case the zip didn't explicitly list the directory first)
			std::filesystem::create_directories(outPath.parent_path());

			// Extract the actual file using heap allocation
			size_t uncomp_size = 0;
			void  *p           = mz_zip_reader_extract_to_heap(&zip_archive, i, &uncomp_size, 0);
			if (!p)
			{
				std::cerr << "miniz error: Failed to extract " << file_stat.m_filename << std::endl;
				mz_zip_reader_end(&zip_archive);
				return false;
			}

			std::ofstream outFile(outPath, std::ios::binary);
			if (outFile.is_open())
			{
				outFile.write(reinterpret_cast<const char *>(p), uncomp_size);
				outFile.close();
			}

			mz_free(p);
		}
	}

	// Cleanup and close
	mz_zip_reader_end(&zip_archive);
	return true;
}

// After file is selected and determined to be IWAD or PWAD, create a initial profile for it
importStatus CreateInitialProfile(const std::string &filepath, const bool wasIWAD, const bool wasArchive)
{
	Profile     newProfile;
	std::string path;

	newProfile.isIWAD = wasIWAD ? 1 : 0;

	// create a new profile container file using timestamps and create the respective folder for the profile

	auto               now  = std::chrono::system_clock::now();
	auto               time = std::chrono::system_clock::to_time_t(now);
	std::tm            tm   = *std::localtime(&time);
	std::ostringstream oss;
	oss << std::put_time(&tm, "%Y%m%d-%H%M%S");
	std::string timestamp = oss.str();

	std::string profileFilename = "pf_" + std::filesystem::path(filepath).stem().string() + "_" + timestamp;

	std::string baseProfileDir = std::string(PROFILE_DIR);

	std::filesystem::create_directories(baseProfileDir + profileFilename);
	path = baseProfileDir + profileFilename + "/";

	// copy the file(s) to the respective folder
	if (!wasArchive)
	{
		// easy case, just a single WAD file
		std::filesystem::copy(filepath, path);
	}
	else
	{
		// copy the entire archive to the folder
		std::string archiveName = std::filesystem::path(filepath).filename().string();
		std::string targetZip   = path + archiveName;

		std::filesystem::copy(filepath, path);

		bool success = ExtractArchive(targetZip, path);

		// Did user stop or extraction fail?
		if (!success)
		{
			std::filesystem::remove_all(path); // Cleanup the remains
			return IMPORT_FAIL;
		}

		// delete the archive after extraction
		std::filesystem::remove(targetZip);
	}

	std::string foundWadPath;

	// link up the file BUT it depends on if it was an archive or not
	if (!wasArchive)
	{
		// easy case, simply link
		foundWadPath = path + std::filesystem::path(filepath).filename().string();
	}
	else
	{
		// WAIT, the user might drop a arbitary archive here, we simply stop if there isnt even a .wad file
		// was the archive nested once?
		auto nestCheck = std::filesystem::directory_iterator(path);
		if (nestCheck != std::filesystem::directory_iterator())
		{
			std::filesystem::path firstPath = nestCheck->path();
			bool                  isDir     = nestCheck->is_directory();

			int                   item_count = 0;
			std::filesystem::path singleDir;
			for (const auto &entry : std::filesystem::directory_iterator(path))
			{
				item_count++;
				singleDir = entry.path();
			}
			if (item_count == 1 && std::filesystem::is_directory(singleDir))
			{
				path = singleDir.string() + "/";
			}
		}

		bool            wadFound = false;
		std::error_code ec;
		auto            it  = std::filesystem::directory_iterator(path, ec);
		const auto      end = std::filesystem::directory_iterator();

		if (!ec) // Check if opening the dir failed
		{
			while (it != end)
			{
				const auto &entry = *it;

				if (entry.is_regular_file(ec) && !ec) // check if file causes error
				{
					std::string extension = entry.path().extension().string();
					std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

					if (extension == ".wad" || extension == ".iwad" || extension == ".pwad" || extension == ".pwd" ||
					    extension == ".ipk3" || extension == ".ipk7" || extension == ".pk3" || extension == ".pk7")
					{
						foundWadPath = entry.path().string();
						wadFound     = true;
						break;
					}
				}
				it.increment(ec);
				if (ec)
				{
					// Error moving to next file (permission denied, etc.)
					break;
				}
			}
		}

		if (!wadFound)
		{
			// The zip file didn't actually contain a WAD! do not deal with this any further -> abort
			if (!path.empty() && path.back() == '/')
				path.pop_back(); // drop the / at the end

			std::filesystem::remove_all(path); // delete dir since we aborted
			return IMPORT_FAIL;
		}

		// check again if its iwad
		fileType type = getFiletype(foundWadPath);

		if (type == TYPE_IWAD || type == TYPE_IPK3 || type == TYPE_IPK7)
		{
			newProfile.isIWAD = 1;
		}
		else
		{
			newProfile.isIWAD = 0;
		}
	}

	if (newProfile.isIWAD == 1)
	{
		newProfile.iwadFilePath = foundWadPath;
	}
	else
	{
		newProfile.pwadFilePath = foundWadPath;
	}

	newProfile.title = std::filesystem::path(filepath).stem().string(); // put in the file/archive name as Profile name
	newProfile.title[0] = std::toupper(newProfile.title[0]);            // capitlize the first letter for beautify

	// buffer entire WAD
	std::ifstream file(foundWadPath, std::ios_base::in | std::ios_base::binary);

	// populate the IWAD basic profile data based on the md5 file hash
	uint8_t    digest[16];
	MD5Context md5;

	char chunk[8192];
	while (file.read(chunk, sizeof(chunk)) || file.gcount() > 0)
	{
		md5.Update(reinterpret_cast<const uint8_t *>(chunk), file.gcount());
	}

	md5.Final(digest);

	std::stringstream readableDigest;
	for (int i = 0; i < 16; ++i)
	{
		readableDigest << std::hex << std::setw(2) << std::setfill('0') << (int)digest[i];
	}

	attributeFromFilename(&newProfile, readableDigest.str());

	// pull remaining data for PWAD (only for idgames archived wads that contain a txt file)
	if (wasArchive)
	{
		std::error_code ec;
		for (const auto &entry : std::filesystem::directory_iterator(path, ec))
		{
			if (ec)
				break; // Error with files

			if (entry.is_regular_file(ec) && !ec)
			{
				auto        txt_path  = entry.path();
				std::string extension = txt_path.extension().string();
				std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

				if (extension == ".txt")
				{
					long        fsize = std::filesystem::file_size(txt_path);
					std::string content(fsize, '\0');

					std::ifstream txtFile(txt_path, std::ios::binary);
					txtFile.read(content.data(), fsize);

					newProfile.description = content;
					break;
				}
			}
		}
	}

	// create the required folders
	std::filesystem::create_directories(path + "saves");
	std::filesystem::create_directories(path + "screenshots");
	std::filesystem::create_directories(path + "demos");
	std::filesystem::create_directories(path + "mods");

	// bind the paths + config
	newProfile.configFilePath    = path + "config.ini";
	newProfile.saveDirPath       = path + "saves";
	newProfile.screenshotDirPath = path + "screenshots";
	newProfile.demoDirPath       = path + "demos";
	newProfile.modsDirPath       = path + "mods";

	// save the profile in the respective folder
	newProfile.saveToFile(path + profileFilename + ".json");

	// add the final product to the list of profiles by adding the config file path to the launcher.cfg file
	json          j;
	std::string   configFileStr = std::string(CONFIG_FILE);
	std::ifstream inFile(configFileStr);

	if (inFile.is_open())
	{
		try
		{
			inFile >> j;
		}
		catch (const json::parse_error &)
		{
			// Throw and error about Json being corrupted
			inFile.close();
			return IMPORT_FAIL;
		}
		inFile.close();
	}

	j["profiles"].push_back(path + profileFilename + ".json");

	std::ofstream outFile(configFileStr);
	if (outFile.is_open())
	{
		outFile << j.dump(4);
		outFile.close();
	}

	// WE ARE DONE! Tell the user what is was detected at the very end
	if (newProfile.isIWAD)
	{
		return IMPORT_IWAD_SUCCESS;
	}
	else
	{
		return IMPORT_PWAD_SUCCESS;
	}
}

// Process Archive
importStatus Loader::ProcessArchive(const std::string &filePath)
{
	std::string ext = std::filesystem::path(filePath).extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	// Is User trying to cheat and add something that isn't even an archive?
	if (ext == ".zip")
	{
		// pretend it's an iwad for now, it will be checked once unzipped in that method
		return CreateInitialProfile(filePath, true, true);
	}
	else
	{
		return IMPORT_FAIL;
	}
}

// Process WAD
importStatus Loader::ProcessWad(const std::string &filePath)
{
	// At this point we have a path, check if it's an IWAD or PWAD
	fileType type = getFiletype(filePath);

	if (type == TYPE_IWAD || type == TYPE_IPK3 || type == TYPE_IPK7)
	{
		// an IWAD
		return CreateInitialProfile(filePath, true, false);
	}
	else if (type == TYPE_PWAD || type == TYPE_PK3 || type == TYPE_PK7)
	{
		// an PWAD
		return CreateInitialProfile(filePath, false, false);
	}
	else
	{
		// Not a recognized WAD
		return IMPORT_FAIL;
	}
}
