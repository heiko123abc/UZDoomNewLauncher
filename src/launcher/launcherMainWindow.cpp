/*
** launcherMainWindow.cpp
**
** Creates the main window for the launcher with the profile list and buttons
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

#include "launcherMainWindow.h"
#include "about.h"
#include "gstrings.h"
#include "loader.h"
#include "profileSettings.h"
#include "starter.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#endif

using json      = nlohmann::json;
using TimePoint = std::chrono::system_clock::time_point;

// returns a string that calculates XXhXXm
static std::string getTimeString(std::time_t totalSeconds)
{
	long long hours   = totalSeconds / 3600;
	long long minutes = (totalSeconds % 3600) / 60;

	std::stringstream ss;
	ss << hours << "h" << minutes << "m";
	return ss.str();
}

LauncherMainWindow::LauncherMainWindow()
{
	// Load config on startup
	std::ifstream configFile(CONFIG_FILE);
	if (configFile.is_open())
	{
		try
		{
			json j;
			configFile >> j;

			if (j.contains("lang") && j["lang"].is_string())
				langVar = j["lang"].get<std::string>();

			if (j.contains("theme") && j["theme"].is_string())
				themeVar = j["theme"].get<std::string>();
		}
		catch (const json::parse_error &e)
		{
			std::cerr << "Config JSON parse error: " << e.what() << std::endl;
		}
		configFile.close();
	}

	UpdateLanguage();
	ApplyTheme();
	RefreshList();
}

void LauncherMainWindow::SaveConfig()
{
	json j;
	j["lang"]     = langVar;
	j["theme"]    = themeVar;
	j["profiles"] = profilePaths;

	std::ofstream file(CONFIG_FILE);
	if (file.is_open())
	{
		file << j.dump(4);
		file.close();
	}
}

void LauncherMainWindow::RefreshList()
{
	profilePaths.clear();
	cachedProfiles.clear();

	std::ifstream file(CONFIG_FILE);
	if (file.is_open())
	{
		try
		{
			json j;
			file >> j;
			if (j.contains("profiles") && j["profiles"].is_array())
			{
				profilePaths = j["profiles"].get<std::vector<std::string>>();
			}
		}
		catch (const json::parse_error &e)
		{
			std::cerr << "Config JSON parse error: " << e.what() << std::endl;
			return;
		}
		file.close();
	}

	for (const auto &filepath : profilePaths)
	{
		Profile tempProfile;
		tempProfile.loadFromFile(filepath);
		cachedProfiles.push_back(tempProfile);
	}

	// Reset selection if it goes out of bounds
	if (selectedProfileIdx >= (int)cachedProfiles.size())
	{
		selectedProfileIdx = -1;
	}
}

void LauncherMainWindow::UpdateLanguage()
{
	GStrings.UpdateLanguage(langVar.c_str());
}

void LauncherMainWindow::ApplyTheme()
{
	// Define available themes
	static std::unordered_map<std::string, LauncherTheme> themes = {
		{"dark", LauncherTheme(LauncherTheme::BaseTheme::ImGuiDark)},
		{"light", LauncherTheme(LauncherTheme::BaseTheme::ImGuiLight)},

		// Doom: Deep hellish reds, dark charcoal, and stark text
		{"doom", LauncherTheme(0x1a1515, // bg
	                           0xdfdfdf, // text
	                           0x2b2222, // inputs
	                           0x5e1313, // interact
	                           0x8a1c1c, // hover
	                           0xc72c2c, // click
	                           0x3d2b2b  // border
	                           )},

		// Plutonia sounds green
		{"plutonia", LauncherTheme(0x1e1f1a, // bg
	                               0xdfd8c8, // text
	                               0x2d3025, // inputs
	                               0x475222, // interact
	                               0x5e6e2d, // hover
	                               0x7a8d3b, // click
	                               0x313626  // border
	                               )},

		// Classic: Retro Windows 95 / Win32 vibe
		{"classic", LauncherTheme(0xc0c0c0, // bg
	                              0x000000, // text
	                              0xffffff, // inputs
	                              0xa0a0a0, // interact
	                              0x002b80, // hover
	                              0x0000ff, // click
	                              0x808080  // border
	                              )}
    };

	if (themes.find(themeVar) != themes.end())
	{
		themes[themeVar].ApplyTheme();
	}
	else
	{
		std::cerr << "Theme not found: " << themeVar << ". Applying default." << std::endl;
		themes["Dark"].ApplyTheme();
	}
}

void LauncherMainWindow::DrawPopUp()
{
	if (showImportPopup)
	{
		ImGui::OpenPopup("UZDoom");
		showImportPopup = false; // Reset the trigger immediately so it only opens once
	}

	// ImGuiWindowFlags_AlwaysAutoResize makes the popup snap to the text size
	if (ImGui::BeginPopupModal("UZDoom", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		// Display a message based on the status enum
		switch (lastImportStatus)
		{
		case IMPORT_IWAD_SUCCESS:
			ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f),
			                   GStrings.GetString("LAUNCHER_DETECT_IWAD")); // Green text
			break;
		case IMPORT_PWAD_SUCCESS:
			ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f),
			                   GStrings.GetString("LAUNCHER_DETECT_PWAD")); // Green text
			break;
		case IMPORT_FAIL:
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
			                   GStrings.GetString("LAUNCHER_DETECT_NOWAD")); // Red text
			break;
		case IMPORT_ARCHIVE_FAIL:
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
			                   GStrings.GetString("LAUNCHER_ERROR_NOWADARCH")); // Red text
			break;
		case IMPORT_DUPLICATE:
			ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
			                   GStrings.GetString("LAUNCHER_ERROR_DUPLICATE")); // Yellow text for warning
			break;
		case IMPORT_CANCELLED:
			ImGui::Text("Import Cancelled.");
			break;
		}

		ImGui::Separator();
		ImGui::Spacing();

		// Center the OK button
		ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - 60.0f);
		if (ImGui::Button("OK", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void LauncherMainWindow::Draw()
{
	// Check if background thread signaled a refresh (e.g., game closed and we are coming back to launcher)
	if (needsRefresh && !showSettingsModal)
	{
		RefreshList();
		needsRefresh.store(false);
	}

	// Main Window
	ImGuiIO &io = ImGui::GetIO();
	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
	ImGui::SetNextWindowSize(io.DisplaySize);

	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
	                               ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;

	if (ImGui::Begin("UZDoom Launcher", nullptr, windowFlags))
	{
		DrawMenuBar();

		// Main Layout Split: Left (List) | Right (Buttons)
		ImGui::Columns(2, "MainColumns", false);
		ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() - 180.0f); // Reserve 180px for buttons to look flush

		// Top-Left: Profile List
		ImGui::BeginChild("ProfileListChild", ImVec2(0, -250.0f), true); // Increased Description Box Reserve to 250px
		DrawProfileList();
		ImGui::EndChild();

		// Bottom-Left: Description Box
		ImGui::BeginChild("DescriptionBoxChild", ImVec2(0, 0), true);
		DrawDescriptionBox();
		ImGui::EndChild();

		ImGui::NextColumn();

		// Right side: Buttons
		ImGui::BeginChild("ButtonsChild", ImVec2(0, 0));
		DrawButtons();
		ImGui::EndChild();

		ImGui::Columns(1);
	}
	ImGui::End();

	// Render the linked ImGui Modals
	About::DrawReleaseNotesDialog(&showAboutNotes, langVar);
	About::DrawCreditsDialog(&showAboutCredits, langVar);

	DrawPopUp(); // Draw the import status popup if triggered

	if (showSettingsModal && selectedProfileIdx >= 0 && selectedProfileIdx < cachedProfiles.size())
	{
		bool wasOpen = showSettingsModal;

		settingsModal.Draw(&showSettingsModal, &cachedProfiles[selectedProfileIdx], profilePaths[selectedProfileIdx]);

		// If the modal was just closed
		if (wasOpen && !showSettingsModal)
		{
			// Check if it was deleted inside the profile settings modal
			if (!std::filesystem::exists(profilePaths[selectedProfileIdx]))
			{
				profilePaths.erase(profilePaths.begin() + selectedProfileIdx);
				selectedProfileIdx = -1;
				SaveConfig();
			}

			RefreshList();
		}
	}
}

// draws the top menu bar with File, Preferences, and About
void LauncherMainWindow::DrawMenuBar()
{
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu(GStrings.GetString("LAUNCHER_TOPBAR_FILE")))
		{

			std::string defaultPath = "";

			if (ImGui::MenuItem(GStrings.GetString("LAUNCHER_TOPBAR_FILEADDWAD")))
			{
				std::string result =
					ProfileSettings::OpenPathPicker(defaultPath, false,
				                                    {
														{"WAD/PKX Files", "wad,pwd,pk3,pk7,iwad,pwad,ipk3,ipk7"}
                });

				if (!result.empty())
				{
					// Capture status and trigger popup
					lastImportStatus = Loader::ProcessWad(result);
					showImportPopup  = true;
					needsRefresh     = true;
				}
			}
			if (ImGui::MenuItem(GStrings.GetString("LAUNCHER_TOPBAR_FILEADDARCHIVE")))
			{
				std::string result = ProfileSettings::OpenPathPicker(defaultPath, false,
				                                                     {
																		 {"Zip Archives", "zip"}
                });

				if (!result.empty())
				{
					// Capture status and trigger popup
					lastImportStatus = Loader::ProcessArchive(result);
					showImportPopup  = true;
					needsRefresh     = true;
				}
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			if (ImGui::MenuItem(GStrings.GetString("LAUNCHER_TOPBAR_IMPORT")))
			{
				// Allow user to import a .zip profile

				std::string defaultPath = "";
				std::string result      = ProfileSettings::OpenPathPicker(defaultPath, false,
				                                                          {
                                                                         {"UZdoom Profiles", "uzdp"}
                });

				// Only allow valid Uzdoom profile import
				std::filesystem::path filePath(result);
				if (!result.empty() && filePath.extension().string() == ".uzdp")
				{
					ImportProfileFromZip(result);
				}
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			if (ImGui::MenuItem(GStrings.GetString("LAUNCHER_TOPBAR_FILEEXIT")))
			{
				// Shutdown SDL directly
				SDL_Event quit_event;
				quit_event.type = SDL_QUIT;
				SDL_PushEvent(&quit_event);
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(GStrings.GetString("LAUNCHER_TOPBAR_PREF")))
		{
			if (ImGui::BeginMenu(GStrings.GetString("LAUNCHER_TOPBAR_PREFLANG")))
			{
				const char *langs[] = {"default", "eng", "cs", "da", "de", "es",  "esm", "eo", "fi", "fr", "hu", "it",
				                       "jp",      "ko",  "nl", "no", "pl", "ptg", "pt",  "ro", "ru", "sr", "tr"};
				const char *names[] = {
					"English (US)",      "English (UK)",      "Česky (Czech)",      "Dansk (Danish)",
					"Deutsch (German)",  "Español (España)",  "Español (Latino)",   "Esperanto",
					"Suomi (Finnish)",   "Français (French)", "Magyar (Hungarian)", "Italiano (Italian)",
					"日本語 (Japanese)", "한국어 (Korean)",   "Nederlands (Dutch)", "Norsk Bokmål",
					"Polski (Polish)",   "Português (EU)",    "Português (BR)",     "Română (Romanian)",
					"Русский (Russian)", "Српски (Serbian)",  "Türkçe (Turkish)"};

				for (int i = 0; i < IM_ARRAYSIZE(langs); i++)
				{
					if (ImGui::MenuItem(names[i], "", langVar == langs[i]))
					{
						langVar = langs[i];
						SaveConfig();
						UpdateLanguage();
					}
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu(GStrings.GetString("LAUNCHER_TOPBAR_PREFTHEME")))
			{
				if (ImGui::MenuItem(GStrings.GetString("LAUNCHER_THEME_LIGHT"), "", themeVar == "light"))
				{
					themeVar = "light";
					ApplyTheme();
					SaveConfig();
				}
				if (ImGui::MenuItem(GStrings.GetString("LAUNCHER_THEME_DARK"), "", themeVar == "dark"))
				{
					themeVar = "dark";
					ApplyTheme();
					SaveConfig();
				}

				ImGui::Spacing();
				ImGui::Separator(); // separate "default" themes from more custom ones
				ImGui::Spacing();

				if (ImGui::MenuItem("Doom", "", themeVar == "doom"))
				{
					themeVar = "doom";
					ApplyTheme();
					SaveConfig();
				}

				if (ImGui::MenuItem("Plutonia", "", themeVar == "plutonia"))
				{
					themeVar = "plutonia";
					ApplyTheme();
					SaveConfig();
				}

				if (ImGui::MenuItem("Classic", "", themeVar == "classic"))
				{
					themeVar = "classic";
					ApplyTheme();
					SaveConfig();
				}

				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(GStrings.GetString("LAUNCHER_TOPBAR_ABOUT")))
		{
			if (ImGui::MenuItem(GStrings.GetString("LAUNCHER_TOPBAR_ABOUTNOTES")))
			{
				showAboutNotes = true;
			}
			if (ImGui::MenuItem(GStrings.GetString("LAUNCHER_TOPBAR_ABOUTCREDITS")))
			{
				showAboutCredits = true;
			}
			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}
}

void LauncherMainWindow::DrawProfileList()
{
	static ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
	                               ImGuiTableFlags_Resizable | ImGuiTableFlags_HighlightHoveredColumn;

	if (ImGui::BeginTable("Profiles", 6, flags))
	{
		ImGui::TableSetupColumn(GStrings.GetString("LAUNCHER_PROFLIST_TYPE"), ImGuiTableColumnFlags_WidthFixed, 60.0f);
		ImGui::TableSetupColumn(GStrings.GetString("LAUNCHER_PROFLIST_TITLE"), ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn(GStrings.GetString("LAUNCHER_PROFLIST_AUTHORS"), ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn(GStrings.GetString("LAUNCHER_PROFLIST_RELEASEDATE"), ImGuiTableColumnFlags_WidthFixed,
		                        100.0f);
		ImGui::TableSetupColumn(GStrings.GetString("LAUNCHER_PROFLIST_LASTPLAYED"), ImGuiTableColumnFlags_WidthFixed,
		                        100.0f);
		ImGui::TableSetupColumn(GStrings.GetString("LAUNCHER_PROFLIST_PLAYTIME"), ImGuiTableColumnFlags_WidthFixed,
		                        80.0f);
		ImGui::TableHeadersRow();

		for (int i = 0; i < cachedProfiles.size(); i++)
		{
			ImGui::PushID(profilePaths[i].c_str());

			ImGui::TableNextRow();
			bool isSelected = (selectedProfileIdx == i);

			ImGui::TableSetColumnIndex(0);
			std::string typeStr = (cachedProfiles[i].isIWAD ? "IWAD" : "PWAD");

			// Use Selectable to allow the row to be clickable across all columns
			if (ImGui::Selectable(typeStr.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns))
			{
				selectedProfileIdx = i;
			}

			ImGui::TableSetColumnIndex(1);
			ImGui::TextUnformatted(cachedProfiles[i].title.c_str());
			ImGui::TableSetColumnIndex(2);
			ImGui::TextUnformatted(cachedProfiles[i].author.c_str());
			ImGui::TableSetColumnIndex(3);
			ImGui::TextUnformatted(cachedProfiles[i].releaseDate.c_str());
			ImGui::TableSetColumnIndex(4);
			ImGui::TextUnformatted(cachedProfiles[i].lastPlayedDate.c_str());
			ImGui::TableSetColumnIndex(5);
			ImGui::TextUnformatted(getTimeString(cachedProfiles[i].playedTime).c_str());

			ImGui::PopID();
		}
		ImGui::EndTable();
	}
}

void LauncherMainWindow::DrawDescriptionBox()
{
	if (selectedProfileIdx >= 0 && selectedProfileIdx < cachedProfiles.size())
	{
		ImGui::TextWrapped("%s", cachedProfiles[selectedProfileIdx].description.c_str());
	}
	else
	{
		ImGui::TextDisabled("%s", GStrings.GetString("LAUNCHER_NO_DSC_SELECT"));
	}
}

void LauncherMainWindow::DrawButtons()
{
	ImVec2 btnSize(-FLT_MIN, 30.0f); // Stretch to column width
	bool   hasSelection = (selectedProfileIdx != -1);

	// when launched, disable all buttons
	bool disableButtons = (!hasSelection || isAlreadyLaunched);

	if (disableButtons)
		ImGui::BeginDisabled();

	if (ImGui::Button(GStrings.GetString("LAUNCHER_PROFBUTTON_START"), btnSize))
		LaunchGame("");
	if (ImGui::Button(GStrings.GetString("LAUNCHER_PROFBUTTON_JOIN"), btnSize))
		LaunchGame("join");
	if (ImGui::Button(GStrings.GetString("LAUNCHER_PROFBUTTON_HOST"), btnSize))
		LaunchGame("host");

	if (ImGui::Button(GStrings.GetString("LAUNCHER_PROFBUTTON_SETTING"), btnSize))
	{
		showSettingsModal = true;
	}

	if (disableButtons)
		ImGui::EndDisabled();

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	if (disableButtons)
		ImGui::BeginDisabled();

	if (ImGui::Button(GStrings.GetString("LAUNCHER_PROFBUTTON_MVUP"), btnSize))
		MoveSelectedEntry(-1);
	if (ImGui::Button(GStrings.GetString("LAUNCHER_PROFBUTTON_MVDOWN"), btnSize))
		MoveSelectedEntry(1);

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	if (ImGui::Button(GStrings.GetString("LAUNCHER_PROFBUTTON_REFRESH"), btnSize))
	{
		RefreshList();
	}

	if (ImGui::Button(GStrings.GetString("LAUNCHER_PROFBUTTON_CLONE"), btnSize))
		CloneSelectedProfile();

	if (disableButtons)
		ImGui::EndDisabled();

	// Available Status
	ImGui::SetCursorPosY(ImGui::GetWindowHeight() - ImGui::GetTextLineHeightWithSpacing() - 5.0f);
	ImGui::TextDisabled(GStrings.GetString("LAUNCHER_AVAIL_STATUS"), cachedProfiles.size());
}

// when user presses UP or DOWN button, adjust selected entry in table
void LauncherMainWindow::MoveSelectedEntry(int offset)
{
	int newIdx = selectedProfileIdx + offset;
	if (newIdx >= 0 && newIdx < profilePaths.size())
	{
		std::swap(profilePaths[selectedProfileIdx], profilePaths[newIdx]);
		SaveConfig();
		RefreshList();
		selectedProfileIdx = newIdx; // keep focus
	}
}

// clone the selected profile
void LauncherMainWindow::CloneSelectedProfile()
{
	if (selectedProfileIdx < 0 || selectedProfileIdx >= profilePaths.size())
		return;

	std::string           originalJsonPath = profilePaths[selectedProfileIdx];
	std::filesystem::path origDir          = std::filesystem::path(originalJsonPath).parent_path();

	// Generate new directory name with correct timestamp
	auto               now  = std::chrono::system_clock::now();
	auto               time = std::chrono::system_clock::to_time_t(now);
	std::tm            tm   = *std::localtime(&time);
	std::ostringstream oss;
	oss << std::put_time(&tm, "%Y%m%d-%H%M%S");

	std::string           newFolderName = "clone_ " + oss.str();
	std::filesystem::path newDir        = origDir.parent_path() / newFolderName;

	// copy the directory and its contents (WADs, mods, configs etc.)
	std::filesystem::copy(origDir, newDir, std::filesystem::copy_options::recursive);

	// in the copied JSON profile and update its internal paths
	std::filesystem::path copiedJsonPath = newDir / std::filesystem::path(originalJsonPath).filename();
	std::filesystem::path newJsonPath    = newDir / (newFolderName + ".json");

	// Rename the JSON file to match the new folder name
	std::filesystem::rename(copiedJsonPath, newJsonPath);

	// Update the Profile data
	Profile clonedProfile;
	clonedProfile.loadFromFile(newJsonPath.string());

	clonedProfile.title += " (Clone)";

	// Update internal paths to point to the new directory
	std::string newDirStr           = newDir.string() + (char)std::filesystem::path::preferred_separator;
	clonedProfile.configFilePath    = newDirStr + "config.ini";
	clonedProfile.saveDirPath       = newDirStr + "saves";
	clonedProfile.screenshotDirPath = newDirStr + "screenshots";
	clonedProfile.demoDirPath       = newDirStr + "demos";
	clonedProfile.modsDirPath       = newDirStr + "mods";

	// If IWAD/PWAD were inside the profile dir, update those paths too
	if (clonedProfile.iwadFilePath.find(origDir.string()) != std::string::npos)
	{
		clonedProfile.iwadFilePath = newDirStr + std::filesystem::path(clonedProfile.iwadFilePath).filename().string();
	}
	if (clonedProfile.pwadFilePath.find(origDir.string()) != std::string::npos)
	{
		clonedProfile.pwadFilePath = newDirStr + std::filesystem::path(clonedProfile.pwadFilePath).filename().string();
	}

	// Save the updated JSON
	clonedProfile.saveToFile(newJsonPath.string());

	// add to Launcher
	profilePaths.push_back(newJsonPath.string());
	SaveConfig();
	RefreshList();
}

void LauncherMainWindow::ImportProfileFromZip(const std::string &zipPath)
{
	// temporary folder name with timestamp to avoid conflicts
	auto               now  = std::chrono::system_clock::now();
	auto               time = std::chrono::system_clock::to_time_t(now);
	std::tm            tm   = *std::localtime(&time);
	std::ostringstream oss;
	oss << std::put_time(&tm, "%Y%m%d-%H%M%S");

	std::string tempFolderName = "temp_import_" + oss.str();
	std::string tempDir        = PROFILE_DIR + tempFolderName + (char)std::filesystem::path::preferred_separator;

	std::filesystem::create_directories(tempDir);

	if (Loader::ExtractArchive(zipPath, tempDir))
	{
		std::string jsonFilename = "";
		for (const auto &entry : std::filesystem::directory_iterator(tempDir))
		{
			if (entry.path().extension() == ".json")
			{
				jsonFilename = entry.path().filename().string();
				break;
			}
		}

		if (!jsonFilename.empty())
		{
			std::string originalFolderName = std::filesystem::path(jsonFilename).stem().string();
			std::string targetDir = PROFILE_DIR + originalFolderName + (char)std::filesystem::path::preferred_separator;
			std::string finalJsonPath      = targetDir + jsonFilename;

			// Check if it already exists
			bool isDuplicate = std::filesystem::exists(targetDir);

			if (!isDuplicate)
			{
				for (const std::string &existingPath : profilePaths)
				{
					if (existingPath == finalJsonPath)
					{
						isDuplicate = true;
						break;
					}
				}
			}

			// it's a duplicate -> trigger the popup and abort!
			if (isDuplicate)
			{
				std::filesystem::remove_all(tempDir);
				lastImportStatus = IMPORT_DUPLICATE;
				showImportPopup  = true;
				return;
			}

			// Move the temporary directory to the correct original folder name
			std::filesystem::rename(tempDir, targetDir);

			profilePaths.push_back(finalJsonPath);
			SaveConfig();
			needsRefresh = true;
		}
		else
		{
			// Clean up
			std::filesystem::remove_all(tempDir);
		}
	}
	else
	{
		// Clean up if extraction fails entirely
		std::filesystem::remove_all(tempDir);
	}
}

// launch selected entry
void LauncherMainWindow::LaunchGame(const std::string &mode)
{
	if (isAlreadyLaunched)
		return;
	if (selectedProfileIdx < 0)
		return;

	std::string selectedRowPath = profilePaths[selectedProfileIdx];
	Profile     tp              = cachedProfiles[selectedProfileIdx];

	std::string dispatchedCmd = tp.giveLaunchCommand(selectedRowPath, mode);
	if (dispatchedCmd == GStrings.GetString("LAUNCHER_PROF_EMPTYWAD"))
		return;

	// get executable path and prepend to command
#ifdef _WIN32
	std::string exePath = "uzdoom.exe";
#else
	std::string exePath = "./uzdoom";
#endif

	dispatchedCmd = exePath + " " + dispatchedCmd;

	isAlreadyLaunched       = true;
	TimePoint startingPoint = std::chrono::system_clock::now();

	// Detach a thread to run the process so it doesnt freeze the ImGui Main Loop
	std::thread([this, dispatchedCmd, selectedRowPath, startingPoint]() {

	// Blocking call to run the game
	// Execute the game based on the operating system
#ifdef _WIN32
		// Windows: Launch the game natively
		STARTUPINFOA        si = {sizeof(STARTUPINFOA)};
		PROCESS_INFORMATION pi;

		// CreateProcessA requires a mutable character array
		std::string cmdCopy = dispatchedCmd;

		if (CreateProcessA(NULL, cmdCopy.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
		{
			// Block the background thread until the game exits
			WaitForSingleObject(pi.hProcess, INFINITE);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
#else
		int retCode = std::system(dispatchedCmd.c_str());
#endif

		// Process finished!
		TimePoint doneTime      = std::chrono::system_clock::now();
		long long secondsPlayed = std::chrono::duration_cast<std::chrono::seconds>(doneTime - startingPoint).count();

		// Update profile disk storage
		Profile p;
		p.loadFromFile(selectedRowPath);

		auto              now       = std::chrono::system_clock::now();
		auto              in_time_t = std::chrono::system_clock::to_time_t(now);
		std::stringstream ss;
		ss << std::put_time(std::localtime(&in_time_t), "%d-%m-%Y");
		p.lastPlayedDate = ss.str();

		p.playedTime += secondsPlayed;
		p.saveToFile(selectedRowPath);

		// Tell the main thread to refresh the UI list
		needsRefresh.store(true);
		isAlreadyLaunched.store(false);
	}).detach();
}
