#pragma once
#include <chrono>
#include <string>
#include <vector>
#include <wx/string.h> // Include wxString

class Profile
{
  public:
	wxString title;
	wxString author      = "?";
	wxString releaseDate = "?"; // DD-MM-YYYY
	wxString lastPlayedDate =
		"-"; // Current DD-MM-YYYY set from a conveterted time to string when any 3 button is pressed to launch
	std::time_t playedTime; // stored as a time value to be converted to Xh:Ymin, starts at 0

	wxString description = "Not Available."; // Description of the WAD
	int      isIWAD;                         // Is this an IWAD -> 1 or PWAD -> 0
	wxString iwadFilePath;                   // Path to the main WAD file for this profile
	wxString pwadFilePath;                   // Path to the main (P)WAD file for this profile

	int      launchParameters; // Stores the mode. e.g Normal,mapstart etc
	int      selectedLaunchMap;
	wxString selectedLaunchSave;
	wxString selectedLaunchDemoPlayback;
	wxString selectedLaunchDemoRecord;

	int  difficultySkillRating = 2; // Custom difficulty parameters for this profile
	bool difficultyFastMonsters;
	bool difficultyRespawnMonsters;
	bool difficultyNoMonsters;

	int compatLevel = 0; // Custom difficulty parameters for this profile (more can be set in the UI)

	wxString hostPort        = "5029";
	int      hostMaxPlayers  = 8;
	wxString hostTickRate    = "25Hz";
	wxString hostGamemode    = "Cooperative";
	wxString hostNetworkMode = "Packet Server";
	wxString joinAddress;
	wxString joinPort   = "5029";
	wxString joinTeamNo = "255";

	int  DMFlags, DMFlags2, DMFlags3; // flags used for deathmatch/mp
	bool alwaysapplydmflags;
	int  compatflags, compatflags2; // flags used for compat

	wxString configFilePath;    // Path to the config file to use with this profile
	wxString saveDirPath;       // Path to the save directory to use with this profile
	wxString screenshotDirPath; // Path to the screenshot directory to use with this profile
	wxString demoDirPath;       // Path to the demo directory to use with this profile

	std::vector<std::string> modFiles; // List of mod files to load with this profile, MUST BE ORDERED THIS WAY

	bool enableFullscreen = false; // Fullscreen, Support WADs , etc
	bool enableSupportWAD = false;
	bool disableAutoload  = false;

	int renderingBackend = 0; // Rendering backend to use with this profile 0 - VK / 1 - GL / 2 - GL ES

	bool enableLights     = false; // Extra graphics parameters
	bool enableBrightmaps = false; // Extra graphics parameters
	bool enableWidescreen = false; // Extra graphics parameters

	wxString prependAdditionalParameters; // Any additional parameters not covered
	wxString appendAdditionalParameters;  // Any additional parameters not covered

	// write profile to file
	void saveToFile(const std::string &filepath);

	// load profile from file
	void loadFromFile(const std::string &filepath);

	// build launch command
	std::string giveLaunchCommand(const std::string &filepath, const std::string &mode);
};
