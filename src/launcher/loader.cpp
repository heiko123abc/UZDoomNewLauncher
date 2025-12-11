#include "Loader.h"
#include "Profile.h"

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

// Adds the attributes to the profile (not good but 90% good enough)
void attributeFromFilename(Profile *p, std::string filename)
{

	// helper to better sort the data
	struct WadInfo
	{
		std::string title;
		std::string author;
		std::string releaseDate;
		bool        isIWAD = false;
	};

	static const std::unordered_map<std::string, WadInfo> wadDatabase = {
		{    "doom2",            {"Doom II", "id Software", "10-10-1994", true}},
		{"freedoom1", {"Freedoom Phase 1", "Freedoom Team", "29-01-2024", true}}
    };

	// found a match? copy over
	if (wadDatabase.contains(filename))
	{
		WadInfo info = wadDatabase.at(filename);
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
	if (newProfile.isIWAD == 1)
	{
		std::filesystem::create_directories(std::string(IWAD_PROFILE_DIR.ToUTF8()) + profileFilename);
		path = std::string(IWAD_PROFILE_DIR.ToUTF8()) + profileFilename + "/";
	}
	else
	{
		std::filesystem::create_directories(std::string(PWAD_PROFILE_DIR.ToUTF8()) + profileFilename);
		path = std::string(PWAD_PROFILE_DIR.ToUTF8()) + profileFilename + "/";
	}

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
			wxMessageBox("Extraction was stopped.", "Error", wxICON_ERROR);
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
				wxMessageBox("Error: No .wad file found in the archive!", "Error", wxICON_ERROR);
				std::filesystem::remove_all(path); // delete dir since we aborted
				return;
			}
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

	std::string filename = std::filesystem::path(filepath).stem().string();

	for (char &c : filename)
	{
		c = std::tolower(c); // lowercase entire filename
	}

	// populate the IWAD basic profile data based on the file name (if applicable)
	attributeFromFilename(&newProfile, filename);

	// pull remaining data for PWAD (only for idgames archived wads)
	if (wasArchive)
	{
		// maybe
	}

	// create the required folders
	std::filesystem::create_directories(path + "saves");
	std::filesystem::create_directories(path + "screenshots");
	std::filesystem::create_directories(path + "demos");

	// bind the paths + config
	newProfile.configFilePath    = path + profileFilename + ".ini";
	newProfile.saveDirPath       = path + "saves";
	newProfile.screenshotDirPath = path + "screenshots";
	newProfile.demoDirPath       = path + "demos";

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
			wxMessageBox(
				"The launcher configuration file is corrupted and could not be read.\nPlease fix or delete the file: " +
					wxString(CONFIG_FILE.data()),
				"Error", wxOK | wxICON_ERROR);
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
	if (wasIWAD)
	{
		wxMessageBox("File was added as an IWAD.\n\nNot properly detected? It can be changed in the profile settings.",
		             "Success on adding IWAD", wxOK | wxICON_INFORMATION);
		return;
	}
	else
	{
		wxMessageBox("File was added as an (P)WAD.\n\nRemember to select the correct IWAD for it before "
		             "launching.\nNot properly detected? It can be changed in the profile settings.",
		             "Success on adding (P)WAD", wxOK | wxICON_INFORMATION);
		return;
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

void Loader::archiveOpener(wxWindow *window)
{
	// This one is from the New Picker Button that opens the file dialog to add a new profile

	wxFileDialog openFileDialog(window, "Select an Archive file", "", "",
	                            "Archive files (*.zip,*.ZIP)|*.zip|All files (*.*)|*.*",
	                            wxFD_OPEN | wxFD_FILE_MUST_EXIST);

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
		// At this point we have a valid archive file, now we need to check if it's an IWAD or PWAD
		if (isThisAnIWAD(std::string(filePath.ToUTF8())))
		{
			// an IWAD
			createInitialProfile(std::string(filePath.ToUTF8()), true, true);
		}
		else
		{
			// an PWAD
			createInitialProfile(std::string(filePath.ToUTF8()), false, true);
		}
	}
	else
	{
		// No? Return.
		wxMessageBox("That is not a valid archive file.", "Error", wxOK | wxICON_ERROR);
		return;
	}
}

void Loader::fileOpener(wxWindow *window)
{
	// This one is from the New Picker Button that opens the file dialog to add a new profile

	wxFileDialog openFileDialog(window, "Select a Doom (P)WAD or IWAD file", "", "",
	                            "Doom WAD files (*.wad,*.WAD)|*.wad|All files (*.*)|*.*",
	                            wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	// Wait for user input
	if (openFileDialog.ShowModal() == wxID_CANCEL)
	{
		// User clicked Cancel? Doesnt matter, just close
		return;
	}

	// User clicked Open? Get the full path
	wxString filePath = openFileDialog.GetPath();

	// Is User trying to cheat and add somthing that isn't a even a WAD?
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
		wxMessageBox("That is not a valid IWAD or (P)WAD file.", "Error", wxOK | wxICON_ERROR);
		return;
	}
}
