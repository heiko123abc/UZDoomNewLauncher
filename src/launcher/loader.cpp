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
#include "profile.h"

#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <regex>
#include <string>
#include <wx/busyinfo.h>

using json = nlohmann::json;

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

bool isThisAnIWAD(const std::string filepath)
{

	bool status = false;

	std::ifstream file(filepath, std::ios_base::in | std::ios_base::binary);

	if (!file.is_open())
	{
		// very bad case, this is not a readable file
		std::cerr << "Failed to open file: " << filepath << std::endl;
		return false;
	}

	// do a check on the file to see if it's a valid IWAD
	// and trust no mess up in the files internally (for now?)

	char magicHeader[4];
	file.read(magicHeader, 4);

	if (memcmp(magicHeader, "IWAD", 4) == 0)
	{ // this is nowhere near reliable but as a proof of concept it will do
		status = true;
	}
	else
	{
		status = false;
	}

	file.close();

	return status;
}

// After file is selected and determined to be IWAD or PWAD, create a initial profile for it
void createInitialProfile(const std::string filepath, const bool wasIWAD, const bool wasArchive)
{

	Profile     newProfile;
	std::string path;

	if (wasIWAD)
		newProfile.isIWAD = 1;
	else
		newProfile.isIWAD = 0;

	// create a new profile container file using timestamps and create the respective folder for the profile
	std::string profileFilename = "pf_" + std::filesystem::path(filepath).stem().string() + "_" +
	                              std::format("{:%Y%m%d-%H%M%S}", std::chrono::system_clock::now());

	// we have a name for the profile, now
	std::filesystem::create_directories(std::string(PROFILE_DIR.ToUTF8()) + profileFilename);
	path = std::string(PROFILE_DIR.ToUTF8()) + profileFilename + "/";

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

		std::filesystem::copy(filepath, path);

		wxString zipp = wxString(path) + wxFileName::GetPathSeparator() + wxString(archiveName);

		Loader loaderInstance;
		bool   success = loaderInstance.wxExtractZipFiles(
            zipp, wxString(path), wxTheApp->GetTopWindow()); // let wxWidget handle the extraction (dont even try to do
		                                                       // it ourselves) - it should work on Win/Mac/Linux

		// Did user stop?
		if (!success)
		{
			wxMessageBox(GStrings.GetString("LAUNCHER_ERROR_EXTRACTION"), "UZDoom", wxICON_ERROR);
			std::filesystem::remove_all(path); // Cleanup the remains
			return;
		}

		// delete the archive after extraction
		std::filesystem::remove(std::filesystem::absolute(path).string() + archiveName);
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
		if (wasArchive)
		{
			// was the archive nested
			auto        nestCheck  = std::filesystem::directory_iterator(path);
			const auto &firstEntry = *nestCheck;
			if (nestCheck != std::filesystem::directory_iterator())
			{
				auto nextEntry = nestCheck;
				if (firstEntry.is_directory() && ++nextEntry == std::filesystem::directory_iterator())
				{
					path = firstEntry.path().string(); // only on item and its a folder -> nested!
				}
			}

			bool wadFound = false;
			for (const auto &entry : std::filesystem::directory_iterator(path))
			{

				if (entry.is_regular_file())
				{
					std::string extension = entry.path().extension().string();

					// Convert to lowercase for case-insensitive comparison (WAD vs wad)
					std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

					if (extension == ".wad")
					{
						foundWadPath = entry.path().string();
						wadFound     = true;
						break; // Stop looking after finding the first WAD (user can fix it since there are some
						       // edgecases)
					}
				}
			}

			if (!wadFound)
			{
				// The zip file didn't actually contain a WAD! do not deal with this any further -> abort
				path.pop_back(); // drop the / at the end
				wxMessageBox(GStrings.GetString("LAUNCHER_ERROR_NOWADARCH"), "UZDoom", wxICON_ERROR);
				std::filesystem::remove_all(path); // delete dir since we aborted
				return;
			}
		}
		// check again if its iwad
		isThisAnIWAD(foundWadPath) ? newProfile.isIWAD = 1 : newProfile.isIWAD = 0;
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
	std::ifstream     file(foundWadPath, std::ios_base::in | std::ios_base::binary);
	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string fileContent = buffer.str();

	// populate the IWAD basic profile data based on the md5 file hash
	uint8_t    digest[16];
	MD5Context md5;

	md5.Update((const uint8_t *)fileContent.data(), fileContent.length());
	md5.Final(digest);

	std::stringstream readableDigest;
	for (int i = 0; i < 16; ++i)
	{
		readableDigest << std::hex << std::setw(2) << std::setfill('0') << (int)digest[i];
	}

	attributeFromFilename(&newProfile, readableDigest.str());

	// pull remaining data for PWAD (only for idgames archived wads)
	if (wasArchive)
	{
		// maybe
	}

	// create the required folders
	std::filesystem::create_directories(path + "saves");
	std::filesystem::create_directories(path + "screenshots");
	std::filesystem::create_directories(path + "demos");
	std::filesystem::create_directories(path + "mods");

	// bind the paths + config
	newProfile.configFilePath    = path + profileFilename + ".ini";
	newProfile.saveDirPath       = path + "saves";
	newProfile.screenshotDirPath = path + "screenshots";
	newProfile.demoDirPath       = path + "demos";
	newProfile.modsDirPath       = path + "mods";

	// save the profile in the respective foler
	newProfile.saveToFile(path + profileFilename + ".txt");

	// add the final product to the list of profiles by adding the config file path to the launcher.cfg file

	json          j;
	std::ifstream inFile(CONFIG_FILE.ToUTF8());
	if (inFile.is_open())
	{
		try
		{
			inFile >> j;
		}
		catch (const json::parse_error &)
		{
			// Throw and error about Json being corrupted
			wxMessageBox(GStrings.GetString("LAUNCHER_ERROR_CORRUPT") + wxString(CONFIG_FILE.data()), "UZDoom",
			             wxOK | wxICON_ERROR);
		}
		inFile.close();
	}

	j["profiles"].push_back(path + profileFilename + ".txt");

	std::ofstream outFile(CONFIG_FILE.ToUTF8());
	if (outFile.is_open())
	{
		outFile << j.dump(4);
		outFile.close();
	}

	// WE ARE DONE! Tell the user what is was detected at the very end as and tell them that they can change it in the
	// profile settings later
	if (newProfile.isIWAD)
	{
		wxMessageBox(GStrings.GetString("LAUNCHER_DETECT_IWAD"), "UZDoom", wxOK | wxICON_INFORMATION);
		return;
	}
	else
	{
		wxMessageBox(GStrings.GetString("LAUNCHER_DETECT_PWAD"), "UZDoom", wxOK | wxICON_INFORMATION);
		return;
	}
}

void Loader::archiveOpener(wxWindow *window)
{
	// This one is from the New Picker Button that opens the file dialog to add a new profile

	wxFileDialog openFileDialog(window, GStrings.GetString("LAUNCHER_ARCHPICK_DIALOG_TITLE"), "", "",
	                            GStrings.GetString("FILETYPE_ARCH"), wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	// Wait for user input
	if (openFileDialog.ShowModal() == wxID_CANCEL)
	{
		// User clicked Cancel? Doesnt matter, just close
		return;
	}

	// User clicked Open? Get the full path
	wxString filePath = openFileDialog.GetPath();

	// Is User trying to cheat and add somthing that isn't a even an archive?
	if (filePath.EndsWith(wxString(".zip")) || filePath.EndsWith(wxString(".ZIP")))
	{
		// prentend its an iwad for now, it will be checked once unzipped in that method
		createInitialProfile(std::string(filePath.ToUTF8()), true, true);
	}
	else
	{
		// No? Return.
		wxMessageBox(GStrings.GetString("LAUNCHER_DETECT_NOTARCHIVE"), "UZDoom", wxOK | wxICON_ERROR);
		return;
	}
}

void Loader::fileOpener(wxWindow *window)
{
	// This one is from the New Picker Button that opens the file dialog to add a new profile

	wxFileDialog openFileDialog(window, GStrings.GetString("LAUNCHER_WADPICK_DIALOG_TITLE"), "", "",
	                            GStrings.GetString("FILETYPE_WAD"), wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	// Wait for user input
	if (openFileDialog.ShowModal() == wxID_CANCEL)
	{
		// User clicked Cancel? Doesnt matter, just close
		return;
	}

	// User clicked Open? Get the full path
	wxString filePath = openFileDialog.GetPath();

	// Is User trying to cheat and add somthing that isn't a even a WAD? (we assume the user doesnt mess file
	// internally)
	if (filePath.EndsWith(wxString(".wad")) || filePath.EndsWith(wxString(".WAD")))
	{
		// At this point we have a valid WAD file, now we need to check if it's an IWAD or PWAD
		if (isThisAnIWAD(std::string(filePath.ToUTF8())))
		{
			// an IWAD
			createInitialProfile(std::string(filePath.ToUTF8()), true, false);
		}
		else
		{
			// an PWAD
			createInitialProfile(std::string(filePath.ToUTF8()), false, false);
		}
	}
	else
	{
		// No? Return.
		wxMessageBox(GStrings.GetString("LAUNCHER_DETECT_NOWAD"), "UZDoom", wxOK | wxICON_ERROR);
		return;
	}
}
