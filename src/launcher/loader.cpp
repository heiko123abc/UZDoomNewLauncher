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

	// helper to better sort the data
	struct WadInfo
	{
		std::string title;
		std::string author;
		std::string releaseDate;
		bool        isIWAD = false;
	};

	// list 99% of versions is enough
	static const std::unordered_map<std::string, WadInfo> wadDatabase = {
		// DOOM Shareware
		{"90facab21eede7981be10790e3f82da2",{"Doom (Shareware 1.0)", "id Software", "10-12-1993", true}                                            },
		{"cea4989df52b65f4d481b706234a3dca",
	     {"Doom (Shareware 1.1)", "id Software", "15-12-1993", true}                                                 }, //  there are two versions
		{"52cbc8882f445573ce421fa5453513c1",              {"Doom (Shareware 1.1)", "id Software", "16-12-1993", true}},
		{"2a380f28e813fb0989cae5e4762ebb4c",              {"Doom (Shareware 1.2)", "id Software", "04-02-1994", true}},
		{"30aa5beb9e5ebfbbe1e1765561c08f38",
	     {"Doom (Shareware 1.2)", "id Software", "17-02-1994", true}                                                 }, //  there are two versions
		{"17aebd6b5f2ed8ce07aa526a32af8d99",             {"Doom (Shareware 1.25)", "id Software", "21-04-1994", true}},
		{"a21ae40c388cb6f2c3cc1b95589ee693",              {"Doom (Shareware 1.4)", "id Software", "28-06-1994", true}},
		{"e280233d533dcc28c1acd6ccdc7742d4",              {"Doom (Shareware 1.5)", "id Software", "08-07-1994", true}},
		{"762fd6d4b960d4b759730f01387a50a1",              {"Doom (Shareware 1.6)", "id Software", "03-08-1994", true}},
		{"c428ea394dc52835f2580d5bfd50d76f",            {"Doom (Shareware 1.666)", "id Software", "30-08-1994", true}},
		{"5f4eb849b1af12887dec04a2a12e5e62",              {"Doom (Shareware 1.8)", "id Software", "10-12-1994", true}},
		{"f0cefca49926d00903cf57551d901abe",              {"Doom (Shareware 1.9)", "id Software", "01-02-1995", true}},

		// DOOM Registered
		{"740901119ba2953e3c7f3764eca6e128",             {"Doom (Registered 0.2)", "id Software", "04-02-1993", true}},
		{"dae9b1eea1a8e090fdfa5707187f4a43",             {"Doom (Registered 0.3)", "id Software", "28-02-1993", true}},
		{"b6afa12a8b22e2726a8ff5bd249223de",             {"Doom (Registered 0.4)", "id Software", "03-04-1993", true}},
		{"9c877480b8ef33b7074f1f0c07ed6487",             {"Doom (Registered 0.5)", "id Software", "23-05-1993", true}},
		{"049e32f18d9c9529630366cfc72726ea",            {"Doom (Registered Beta)", "id Software", "04-10-1993", true}},

		{"981b03e6d1dc033301aa3095acc437ce",             {"Doom (Registered 1.1)", "id Software", "16-12-1993", true}},
		{"792fd1fea023d61210857089a7c1e351",             {"Doom (Registered 1.2)", "id Software", "17-02-1994", true}},
		{"54978d12de87f162b9bcc011676cb3c0",           {"Doom (Registered 1.666)", "id Software", "01-09-1994", true}},
		{"11e1cd216801ea2657723abc86ecb01f",             {"Doom (Registered 1.8)", "id Software", "20-01-1995", true}},
		{"1cd63c5ddff1bf8ce844237f580e9cf3",             {"Doom (Registered 1.9)", "id Software", "01-02-1995", true}},
		{"c4fe9fd920207691a9f493668e0a2083",                 {"The Ultimate Doom", "id Software", "25-05-1995", true}},
		{"fb35c4a5a9fd49ec29ab6e900572c524",                {"Doom (BFG Edition)", "id Software", "16-10-2012", true}},
		{"8517c4e8f0eef90b82852667d345eb86",
	     {"The Ultimate Doom (Unity/Bethesda)", "id Software", "09-01-2020	", true}                                  },

		// DOOM II: HELL ON EARTH
		{"d9153ced9fd5b898b36cc5844e35b520",            {"Doom II (1.666 German)", "id Software", "29-08-1994", true}},
		{"30e3c2d0350b67bfbf47271970b74b2f",                   {"Doom II (1.666)", "id Software", "29-08-1994", true}},
		{"ea74a47a791fdef2e9f2ea8b8a9da13b",                     {"Doom II (1.7)", "id Software", "21-09-1994", true}},
		{"d7a07e5d3f4625074312bc299d7ed33f",                    {"Doom II (1.7a)", "id Software", "18-10-1994", true}},
		{"3cb02349b3df649c86290907eed64e7b",              {"Doom II (1.8 French)", "id Software", "01-12-1994", true}},
		{"c236745bb01d89bbb866c8fed81b6f8c",                     {"Doom II (1.8)", "id Software", "20-01-1995", true}},
		{"25e1459ca71d321525f84628f45ca8cd",                     {"Doom II (1.9)", "id Software", "01-02-1995", true}},
		{"b96683d113c4f4e9a916e1c7d1d71ffd",                   {"Doom II (PC-98)", "id Software", "01-02-1995", true}},
		{"c3bea40570c23e511a7ed3ebcd9865f7",
	     {"Doom II (BFG Edition)", "id Software", "16-10-2012", true}                                                }, //  appreantly its incorretly marked as PWAD
		{"8ab6d0527a29efdc1ef200e5687b5cae",          {"Doom II (Unity/Bethesda)", "id Software", "09-01-2020", true}},

		// TNT
		{"4e158d9953c79ccf97bd0663244cc6b6",       {"Final Doom: TNT Evilution (1.9)", "TeamTNT", "10-06-1996", true}},
		{"1d39e405bf6ee3df69a8d2646c8d5c49", {"Final Doom: TNT Evilution (Anthology)", "TeamTNT", "14-11-1996", true}},
		{"f5528f6fd55cf9629141d79eda169630",
	     {"Final Doom: TNT Evilution (Unity/Bethesda)", "TeamTNT", "03-09-2020", true}                               },

		// Plutonia
		{"75c8cf89566741fa9d22447604053bd7",
	     {"Final Doom: Plutonia Experiment", "Casali Brothers", "10-06-1996", true}                                  },
		{"3493be7e1e2588bc9c8b31eab2587a04",
	     {"Final Doom: Plutonia Experiment (Anthology)", "Casali Brothers", "21-11-1996", true}                      },
		{"ae76c20366ff685d3bb9fab11b148b84",
	     {"Final Doom: Plutonia Experiment (Unity/Bethesda)", "Casali Brothers", "03-09-2020", true}                 },

		// Heretic
		{"ae779722390ec32fa37b0d361f7d82f8",        {"Heretic (Shareware 1.2)", "Raven Software", "28-06-1995", true}},
		{"023b52175d2f260c3bdc5528df5d0a8c",        {"Heretic (Shareware 1.0)", "Raven Software", "24-12-1994", true}},
		{"fc7eab659f6ee522bb57acc1a946912f",       {"Heretic (Shareware Beta)", "Raven Software", "23-12-1994", true}},

		{"66d686b1ed6d35ff103f15dbd30e0341",                  {"Heretic (1.3)", "Raven Software", "22-03-1996", true}},
		{"1e4cb4ef075ad344dd63971637307e04",                  {"Heretic (1.2)", "Raven Software", "28-06-1995", true}},
		{"3117e399cdb4298eaa3941625f4b2923",                  {"Heretic (1.0)", "Raven Software", "27-12-1994", true}},

		// Hexen
		{"abb033caf81e26f12a2103e1fa25453f",    {"Hexen: Beyond Heretic (1.1)", "Raven Software", "14-03-1996", true}},
		{"b2543a03521365261d0a0f74d5dd90f0",    {"Hexen: Beyond Heretic (1.0)", "Raven Software", "13-10-1995", true}},
		{"c88a2bb3d783e2ad7b599a8e301e099e",   {"Hexen: Beyond Heretic (Beta)", "Raven Software", "27-09-1995", true}},
		{"876a5a44c7b68f04b3bb9bc7a5bd69d6",   {"Hexen: Beyond Heretic (Demo)", "Raven Software", "18-10-1995", true}},
		{"9178a32a496ff5befebfe6c47dac106c",
	     {"Hexen: Beyond Heretic (Demo Beta)", "Raven Software", "02-10-1995", true}                                 },

		{"78d5898e99e220e4de64edaa0e479593",
	     {"Hexen: Deathkings of the Dark Citadel (1.1)", "Raven Software", "09-05-1996",
         false}																									  }, // Expansion , unrunnable without hexen
		{"1077432e2690d390c256ac908b5f4efa",
	     {"Hexen: Deathkings of the Dark Citadel (1.0)", "Raven Software", "22-03-1996",
         false}																									  }, // Expansion , unrunnable without hexen

		// Strife
		{"bb545b9c4eca0ff92c14d466b3294023",       {"Strife (Teaser 1.1)", "Rogue Entertainment", "14-03-1996", true}},
		{"de2c8dcad7cca206292294bdab524292",       {"Strife (Teaser 1.0)", "Rogue Entertainment", "22-02-1996", true}},
		{"2fed2031a5b03892106e0f117f17901f",       {"Strife (1.2 - 1.31)", "Rogue Entertainment", "23-05-1996", true}},
		{"8f2d3a6a289f5d2f2f9c1eec02b47299",              {"Strife (1.1)", "Rogue Entertainment", "18-04-1996", true}},
		{"082234d6a3f7086424856478b5aa9e95",
	     {"Strife (Voices)", "Rogue Entertainment", "18-04-1996", false}                                             }, //  just voices, needs game

		// Chex Quest 3
		{"bce163d06521f9d15f9686786e64df13",             {"Chex Quest 3 (1.4)", "Charles Jacobi", "24-06-2009", true}},
		{"148367e53ff7f4f814e54b5ac9ff0ab3",             {"Chex Quest 3 (1.3)", "Charles Jacobi", "12-06-2009", true}},
		{"26a8998ecdaa983f8e6c363b4b95bf55",             {"Chex Quest 3 (1.2)", "Charles Jacobi", "02-05-2009", true}},
		{"f85944f55fff094f2ffbd3ecef3fa255",             {"Chex Quest 3 (1.1)", "Charles Jacobi", "22-04-2009", true}},
		{"cb001c34e424687191f299cc1dff4d68",             {"Chex Quest 3 (1.0)", "Charles Jacobi", "12-11-2008", true}},
		{"59c985995db55cd2623c1893550d82b3",
	     {"Chex Quest 3 (1.0 unoffical PWAD)", "Charles Jacobi", "24-06-2009", true}                                 },

		// Action Doom 2: Urban Brawl
		{"1914b280b0a4b517214523bc2270e758",
	     {"Action Doom 2: Urban Brawl (1.0)", "Stephen Browning et al.", "??-??-????", true}                         },
		{"c106a4e0a96f299954b073d5f97240be",
	     {"Action Doom 2: Urban Brawl (1.1)", "Stephen Browning et al.", "26-12-2013", true}                         },

		// Hacx (Standalone 1.2)
		{"402ca45bb90520bfef0dec6baac5889e",            {"Hacx (1.0 verified)", "Banjo Software", "08-10-1997", true}},
		{"1511a7032ebc834a3884cf390d7f186e",          {"Hacx (1.0 unverified)", "Banjo Software", "09-10-1997", true}},
		{"b7fd2f43f3382cf012dc6b097a3cb182",                     {"Hacx (1.1)", "Banjo Software", "16-09-1997", true}},
		{"65ed74d522bdf6649c2831b13b9e02b4",                     {"Hacx (1.2)", "Banjo Software", "09-10-2010", true}},
		{"793f07ebadb3d7353ee5b6b6429d9afa",
	     {"Hacx (2.0)", "Banjo Software et al.", "09-10-2010", true}                                                 }, //  from build r61

		// Harmony
		{"48ebb49b52f6a3020d174dbcc1b9aeaf",           {"Harmony (1.1)", "Thomas van der Velden", "17-02-2012", true}},
		{"fe2cce6713ddcf6c6d6f0e8154b0cb38",           {"Harmony (1.0)", "Thomas van der Velden", "10-12-2009", true}},

		// The Adventures of Square (Ep 1 & 2)
		{"f4578097c658ad3c813cd5901ec125e2",  {"The Adventures of Square (2.1)", "BigBrik Games", "22-06-2019", true}},

		// Delaweare (this hash is taken from the wad of the standalone, as thats the only one i could find)
		{"a185498bdf721b4c01dc87fa81d1580b",                      {"Delaweare", "Space Is Green", "30-06-2014", true}},

		// Rise of the Wool Ball
		{"9176043468e10eaa471ae556e8e55745",      {"Rise of the Wool Ball (1.3)", "MSPaintR0cks", "21-07-2017", true}},
		{"fb0226e3fed7a3c1e7ea4cd4da905950",      {"Rise of the Wool Ball (1.2)", "MSPaintR0cks", "14-06-2017", true}},

		// Freedoom (MUST BE UPDATE EVERY RELASE)
		{"b93be13d05148dd01614bc205a03648e",     {"Freedoom: Phase 1 (0.13)", "Freedoom Project", "30-01-2024", true}},
		{"cd666466759b5e5f63af93c5f0ffd0a1",     {"Freedoom: Phase 2 (0.13)", "Freedoom Project", "30-01-2024", true}},
		{"908dfd77a14cc490c4cea94b62d13449",                {"FreeDM (0.13)", "Freedoom Project", "30-01-2024", true}},
	};

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
			// was the archive nested
			auto nestCheck = std::filesystem::directory_iterator(path);
			const auto &firstEntry = *nestCheck;
			if (nestCheck != std::filesystem::directory_iterator())
			{
				auto nextEntry = nestCheck;
				if (firstEntry.is_directory() && ++nextEntry == std::filesystem::directory_iterator())
				{
					path = firstEntry.path().string(); //only on item and its a folder -> nested!
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
				wxMessageBox("Error: No .wad file found in the archive! (Or archive is too nested.)", "Error", wxICON_ERROR);
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
	if (newProfile.isIWAD)
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
		// prentend its an iwad for now, it will be checked once unzipped in that method
		createInitialProfile(std::string(filePath.ToUTF8()), true, true);
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
		wxMessageBox("That is not a valid IWAD or (P)WAD file.", "Error", wxOK | wxICON_ERROR);
		return;
	}
}
