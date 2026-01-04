/*
** profile.cpp
**
** Methods for interaction (e.g save/load) of Profile files using JSON
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

#include "profile.h"

#include <nlohmann/json.hpp> //for JSON file support

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using json = nlohmann::json;

// see https://json.nlohmann.me/api/adl_serializer/ and Credit to Ziv Shahaf
// (https://github.com/nlohmann/json/issues/1592#issuecomment-488120753)
namespace nlohmann
{
template <> struct adl_serializer<wxString>
{
	static void to_json(json &j, const wxString &value)
	{
		j = value.ToStdString(); // Convert wxString -> std::string for JSON
	}
	static void from_json(const json &j, wxString &value)
	{
		value = wxString(j.get<std::string>()); // Convert std::string -> wxString
	}
};
} // namespace nlohmann

// profile file format is JSON

void Profile::saveToFile(const std::string &filepath)
{
	json j;

	// map the profile variable to JSON output (filer by tab to make it more ordered)

	j["general"]["title"]          = this->title;
	j["general"]["author"]         = this->author;
	j["general"]["releaseDate"]    = this->releaseDate;
	j["general"]["lastPlayedDate"] = this->lastPlayedDate;
	j["general"]["playedTime"]     = this->playedTime;
	j["general"]["description"]    = this->description;
	j["general"]["isIWAD"]         = this->isIWAD;
	j["general"]["iwadFilePath"]   = this->iwadFilePath;
	j["general"]["pwadFilePath"]   = this->pwadFilePath;

	j["launch"]["launchParameters"]           = this->launchParameters;
	j["launch"]["selectedLaunchMap"]          = this->selectedLaunchMap;
	j["launch"]["selectedLaunchSave"]         = this->selectedLaunchSave;
	j["launch"]["selectedLaunchDemoPlayback"] = this->selectedLaunchDemoPlayback;
	j["launch"]["selectedLaunchDemoRecord"]   = this->selectedLaunchDemoRecord;
	j["launch"]["difficultySkillRating"]      = this->difficultySkillRating;
	j["launch"]["difficultyFastMonsters"]     = this->difficultyFastMonsters;
	j["launch"]["difficultyRespawnMonsters"]  = this->difficultyRespawnMonsters;
	j["launch"]["difficultyNoMonsters"]       = this->difficultyNoMonsters;
	j["launch"]["compatLevel"]                = this->compatLevel;
	j["launch"]["playerName"]                = this->playerName;
	j["launch"]["playerClass"]                = this->playerClass;
	j["launch"]["playerGender"]                = this->playerGender;
	j["launch"]["wadLanguage"]               = this->wadLanguage;
	j["launch"]["hostPort"]                   = this->hostPort;
	j["launch"]["hostMaxPlayers"]             = this->hostMaxPlayers;
	j["launch"]["hostTickRate"]               = this->hostTickRate;
	j["launch"]["hostGamemode"]               = this->hostGamemode;
	j["launch"]["hostNetworkMode"]            = this->hostNetworkMode;
	j["launch"]["joinAddress"]                = this->joinAddress;
	j["launch"]["joinPort"]                   = this->joinPort;
	j["launch"]["joinTeamNo"]                 = this->joinTeamNo;

	j["launch"]["DMFlags"]            = this->DMFlags;
	j["launch"]["DMFlags2"]           = this->DMFlags2;
	j["launch"]["DMFlags3"]           = this->DMFlags3;
	j["launch"]["alwaysapplydmflags"] = this->alwaysapplydmflags;
	j["launch"]["compatflags"]        = this->compatflags;
	j["launch"]["compatflags2"]       = this->compatflags2;

	j["files"]["configFilePath"]    = this->configFilePath;
	j["files"]["saveDirPath"]       = this->saveDirPath;
	j["files"]["screenshotDirPath"] = this->screenshotDirPath;
	j["files"]["demoDirPath"]       = this->demoDirPath;
	j["files"]["modsDirPath"]       = this->modsDirPath;
	j["files"]["modFiles"]          = this->modFiles;

	j["output"]["enableFullscreen"] = this->enableFullscreen;
	j["output"]["enableSupportWAD"] = this->enableSupportWAD;
	j["output"]["disableAutoload"]  = this->disableAutoload;
	j["output"]["renderingBackend"] = this->renderingBackend;
	j["output"]["enableLights"]     = this->enableLights;
	j["output"]["enableBrightmaps"] = this->enableBrightmaps;
	j["output"]["enableWidescreen"] = this->enableWidescreen;

	j["advanced"]["prependAdditionalParameters"] = this->prependAdditionalParameters;
	j["advanced"]["appendAdditionalParameters"]  = this->appendAdditionalParameters;

	// Write JSON to file
	std::ofstream file(filepath);
	if (file.is_open())
	{
		file << j.dump(5);
		file.close();
	}
}

void Profile::loadFromFile(const std::string &filepath)
{

	json          j;
	std::ifstream file(filepath);
	// build a profile object based on the JSON file

	if (file.is_open())
	{

		file >> j;
		//.vaule(A,B) has a secondary fallback B if it is unable to read

		this->title          = j["general"].value("title", "#ERROR"); // thats enough to tell user something went wrong
		this->author         = j["general"].value("author", "");
		this->releaseDate    = j["general"].value("releaseDate", "");
		this->lastPlayedDate = j["general"].value("lastPlayedDate", "");
		this->playedTime     = j["general"].value("playedTime", 0);
		this->description    = j["general"].value("description", "");
		this->isIWAD         = j["general"].value("isIWAD", 0);
		this->iwadFilePath   = j["general"].value("iwadFilePath", "");
		this->pwadFilePath   = j["general"].value("pwadFilePath", "");

		this->launchParameters           = j["launch"].value("launchParameters", 0);
		this->selectedLaunchMap          = j["launch"].value("selectedLaunchMap", 1);
		this->selectedLaunchSave         = j["launch"].value("selectedLaunchSave", "");
		this->selectedLaunchDemoPlayback = j["launch"].value("selectedLaunchDemoPlayback", "");
		this->selectedLaunchDemoRecord   = j["launch"].value("selectedLaunchDemoRecord", "");
		this->difficultySkillRating      = j["launch"].value("difficultySkillRating", 3);
		this->difficultyFastMonsters     = j["launch"].value("difficultyFastMonsters", false);
		this->difficultyRespawnMonsters  = j["launch"].value("difficultyRespawnMonsters", false);
		this->difficultyNoMonsters       = j["launch"].value("difficultyNoMonsters", false);
		this->compatLevel                = j["launch"].value("compatLevel", 0);
		this->playerName                 = j["launch"].value("playerName", "Player");
		this->playerClass                = j["launch"].value("playerClass", "Fighter");
		this->playerGender               = j["launch"].value("playerGender", "male");
		this->wadLanguage                = j["launch"].value("wadLanguage", "default");
		this->hostPort                   = j["launch"].value("hostPort", "");
		this->hostMaxPlayers             = j["launch"].value("hostMaxPlayers", 8);
		this->hostTickRate               = j["launch"].value("hostTickRate", "");
		this->hostGamemode               = j["launch"].value("hostGamemode", "");
		this->hostNetworkMode            = j["launch"].value("hostNetworkMode", "");
		this->joinAddress                = j["launch"].value("joinAddress", "");
		this->joinPort                   = j["launch"].value("joinPort", "");
		this->joinTeamNo                 = j["launch"].value("joinTeamNo", "");

		this->DMFlags            = j["launch"].value("DMFlags", 0);
		this->DMFlags2           = j["launch"].value("DMFlags2", 0);
		this->DMFlags3           = j["launch"].value("DMFlags3", 0);
		this->alwaysapplydmflags = j["launch"].value("alwaysapplydmflags", 0);
		this->compatflags        = j["launch"].value("compatflags", 0);
		this->compatflags2       = j["launch"].value("compatflags2", 0);

		this->configFilePath    = j["files"].value("configFilePath", "");
		this->saveDirPath       = j["files"].value("saveDirPath", "");
		this->screenshotDirPath = j["files"].value("screenshotDirPath", "");
		this->demoDirPath       = j["files"].value("demoDirPath", "");
		this->modsDirPath       = j["files"].value("modsDirPath", "");
		this->modFiles          = j["files"].value("modFiles", std::vector<std::string>{});

		this->enableFullscreen = j["output"].value("enableFullscreen", false);
		this->enableSupportWAD = j["output"].value("enableSupportWAD", false);
		this->disableAutoload  = j["output"].value("disableAutoload", false);
		this->renderingBackend = j["output"].value("renderingBackend", 0);
		this->enableLights     = j["output"].value("enableLights", false);
		this->enableBrightmaps = j["output"].value("enableBrightmaps", false);
		this->enableWidescreen = j["output"].value("enableWidescreen", false);

		this->prependAdditionalParameters = j["advanced"].value("prependAdditionalParameters", "");
		this->appendAdditionalParameters  = j["advanced"].value("appendAdditionalParameters", "");
	}
	else
	{
		return;
	}
	file.close();
}

// WE ASSUME THE LAUNCHER WHERE UZDOOM executeable USUALLY RESIDES IN
std::string Profile::giveLaunchCommand(const std::string &filepath, const std::string &mode)
{

	// init the profile
	this->loadFromFile(filepath);

	std::stringstream cmd;

	// is there something to prepend?
	if (!prependAdditionalParameters.empty())
		cmd << prependAdditionalParameters << " ";

	// check for OS
#if defined(_WIN32)
	cmd << "uzdoom.exe ";
#elif defined(__linux__)
	cmd << "./Linux-*UZDoom-*.AppImage ";
#elif defined(__APPLE__)
	cmd << "./uzdoom.app ";
#else
	// Fallback for unknown OS (not officaly supported)
#error "Not supported operating system"
#endif

	cmd << "-iwad  \"" << this->iwadFilePath << "\" ";
	if (!this->isIWAD)
	{
		// load the (P)Wad file
		cmd << "-file \"" << this->pwadFilePath << "\" ";
	}

	// determine launch mode if it isnt normal
	if (launchParameters == 1)
	{
		cmd << "-warp " << this->selectedLaunchMap << " ";
	}
	else if (launchParameters == 2)
	{
		cmd << "-loadgame " << this->selectedLaunchSave << " ";
	}
	else if (launchParameters == 3)
	{
		cmd << "-playdemo " << this->selectedLaunchDemoPlayback << " ";
	}
	else if (launchParameters == 4)
	{
		cmd << "-record " << this->selectedLaunchDemoPlayback << " ";
	}

	// difficulty
	cmd << "+set skill " << this->difficultySkillRating << " ";
	if (this->difficultyFastMonsters)
		cmd << "-fast ";
	if (this->difficultyRespawnMonsters)
		cmd << "-respawn ";
	if (this->difficultyNoMonsters)
		cmd << "-nomonsters ";

	// are we JOINING a multiplayer game?
	if (mode == "join")
		cmd << std::format("-join {}:{} +set team {} ", this->joinAddress.ToStdString(), this->joinPort.ToStdString(),
		                   this->joinTeamNo.ToStdString());

	//... or are we HOSTING a multiplayer game.
	if (mode == "host")
	{
		cmd << "-host " << this->hostMaxPlayers << " -extratic ";
		if (this->hostNetworkMode == "Peer-to-Peer")
		{
			cmd << "-netmode 0" << " ";
		}
		else
		{
			cmd << "-netmode 1" << " ";
		}

		// nothing -> it defaults to coop
		if (this->hostGamemode == "Deathmatch")
		{
			cmd << "-deathmatch" << " ";
		}
		else if (this->hostGamemode == "Alt. Deathmatch")
		{
			cmd << "-altdeath" << " ";
		}
		else if (this->hostGamemode == "Team Deathmatch")
		{
			cmd << "-deathmatch +set teamplay 1" << " ";
		}
		else if (this->hostGamemode == "Alt. Team Deathmatch")
		{
			cmd << "-altdeath +set teamplay 1" << " ";
		}
	}
	// pass compatibility options
	if (this->compatflags != 0 && this->compatflags2 != 0)
		cmd << "+set compatmode  " << this->compatLevel << " ";

	// add compatflags and dmflags
	cmd << "+set compatflags  " << this->compatflags << " ";
	cmd << "+set compatflags2 " << this->compatflags2 << " ";
	cmd << "+set dmflags " << this->DMFlags << " ";
	cmd << "+set dmflags2 " << this->DMFlags2 << " ";
	cmd << "+set dmflags3 " << this->DMFlags3 << " ";

	// user wants dmflags elsewhere too
	if (alwaysapplydmflags)
		cmd << "+set alwaysapplydmflags 1 ";

        //apply names,class and gender (and language)
        cmd << "+set name " << this->playerName << " ";
        cmd << "+set playerclass " << this->playerClass << " ";
        cmd << "+set gender " << this->playerGender << " ";
		cmd << "+set language " << this->wadLanguage << " ";


	// pass directories
	cmd << "-config " << this->configFilePath << " ";
	cmd << "-savedir " << this->saveDirPath << " ";
	cmd << "-shotdir " << this->screenshotDirPath << " ";
	// demo does nothing right now

	// append mod files (e.g wads/pk3)
	if (!this->modFiles.empty())
	{
		cmd << "-file ";
		for (wxString item : this->modFiles)
		{
			cmd << "\"" << item << "\" ";
		}
	}

	// add general flags
	if (this->enableFullscreen)
		cmd << "+set fullscreen 1 ";
	if (this->enableSupportWAD)
		cmd << "-file ./game_support.pk3 ";
	if (this->disableAutoload)
		cmd << "-noautoload ";

	// rendering backend
	if (this->renderingBackend == 0)
	{
		cmd << "+set vid_preferbackend 1 ";
	}
	if (this->renderingBackend == 1)
	{
		cmd << "+set vid_preferbackend 0 ";
	}
	if (this->renderingBackend == 2)
	{
		cmd << "+set vid_preferbackend 3 ";
	}

	// load addional stuff
	if (this->enableLights)
		cmd << "-file ./lights.pk3 ";
	if (this->enableBrightmaps)
		cmd << "-file ./brightmaps.pk3 ";
	if (this->enableWidescreen)
		cmd << "-file ./game_widescreen_gfx.pk3 ";

	// is there something to append?
	if (!appendAdditionalParameters.empty())
		cmd << appendAdditionalParameters;

	// command strung together, return it
	return cmd.str();
}
