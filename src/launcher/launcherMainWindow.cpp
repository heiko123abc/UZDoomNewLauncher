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

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <vector>
#include <wx/app.h>
#include <wx/artprov.h>
#include <wx/dataview.h>
#include <wx/filedlg.h>
#include <wx/process.h>
#include <wx/spinctrl.h>
#include <wx/utils.h>

#include "about.h"
#include "const.h"
#include "loader.h"
#include "profile.h"
#include "profileSettings.h"
#include "gstrings.h"

using json = nlohmann::json;

bool     isAlreadyLaunched = false;
std::string langVar           = "";


// reuturns a string that calculates XXhXXm
using TimePoint = std::chrono::system_clock::time_point;
std::string getTimeString(std::time_t totalSeconds)
{
	long long hours   = totalSeconds / 3600;
	long long minutes = (totalSeconds % 3600) / 60;

	std::stringstream ss;
	ss << hours << "h" << minutes << "m";

	return ss.str();
}

// IDs for various events
enum
{
	ID_ADD_WAD = wxID_HIGHEST + 1,
	ID_ADD_ARCHIVE,
	ID_LANG,
	ID_THEME,
	ID_REL_NOTES,
	ID_CREDITS,
	ID_START_GAME,
	ID_JOIN_GAME,
	ID_HOST_GAME,
	ID_PROFILE_SETTINGS,
	ID_REFRESH_LIST,
	ID_MOVE_UP,
	ID_MOVE_DOWN,

	// Language IDs (Must remain sequential for range binding far below)
	ID_LANG_ENU,
	ID_LANG_ENG,
	ID_LANG_CS,
	ID_LANG_DA,
	ID_LANG_DE,
	ID_LANG_ES,
	ID_LANG_ESM,
	ID_LANG_EO,
	ID_LANG_FI,
	ID_LANG_FR,
	ID_LANG_HU,
	ID_LANG_IT,
	ID_LANG_JP,
	ID_LANG_KO,
	ID_LANG_NL,
	ID_LANG_NB,
	ID_LANG_PL,
	ID_LANG_PTG,
	ID_LANG_PT,
	ID_LANG_RO,
	ID_LANG_RU,
	ID_LANG_SR,
	ID_LANG_TR
};

// write back the vector to file
void saveConfig(LauncherMainWindow *lmw)
{
	json j;
	j["lang"]     = langVar;
	j["profiles"] = lmw->profilePaths;

	std::ofstream file(CONFIG_FILE.ToUTF8());
	if (file.is_open())
	{
		file << j.dump(4);
		file.close();
	}

	file.close();
}

void refreshList(wxDataViewListCtrl *profileList, LauncherMainWindow *lmw)
{

	profileList->DeleteAllItems(); // we dont want dupes
	lmw->profilePaths.clear();     // same with vector

	std::ifstream file(CONFIG_FILE.ToUTF8());

	if (file.is_open())
	{
		try
		{
			json j;
			file >> j;

			// Check if "profiles" key exists and is an array (just to be safe)
			if (j.contains("profiles") && j["profiles"].is_array())
			{
				lmw->profilePaths = j["profiles"].get<std::vector<std::string>>();
			}
		}
		catch (const json::parse_error &e)
		{
			// The file is corrupt or unreadable?
			wxLogError("Config JSON parse error: %s", e.what());
			return;
		}
	}

	file.close();

	// all paths collected, now parse them into the list
	for (const auto &filepaths : lmw->profilePaths)
	{
		Profile tempProfile;
		tempProfile.loadFromFile(filepaths);

		wxVector<wxVariant> rowData;
		rowData.push_back(tempProfile.isIWAD ? "IWAD" : "PWAD");
		rowData.push_back(tempProfile.title);
		rowData.push_back(tempProfile.author);
		rowData.push_back(tempProfile.releaseDate);
		rowData.push_back(tempProfile.lastPlayedDate);
		rowData.push_back(getTimeString(tempProfile.playedTime));
		rowData.push_back(tempProfile.description);
		rowData.push_back(filepaths);
		profileList->AppendItem(rowData);
	}
}

// default size for the window is 1280x720
LauncherMainWindow::LauncherMainWindow(const wxString &title) : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition)
{

	// This Panel is the base for all other UI components
	wxPanel *panel = new wxPanel(this, wxID_ANY);

	// create menu item to add Archive or WAD for new profile
	wxMenu *AddMenu = new wxMenu;
	AddMenu->Append(ID_ADD_WAD, wxString::FromUTF8(GStrings.GetString("LAUNCHER_TOPBAR_FILEADDWAD")));
	AddMenu->Append(ID_ADD_ARCHIVE, wxString::FromUTF8(GStrings.GetString("LAUNCHER_TOPBAR_FILEADDARCHIVE")));
	AddMenu->AppendSeparator();
	AddMenu->Append(wxID_EXIT, wxString::FromUTF8(GStrings.GetString("LAUNCHER_TOPBAR_FILEEXIT")));

	// create menu item to change preferences e.g language, theme, etc
	wxMenu *PrefMenu = new wxMenu;

	// create toggler
	wxMenu *LangToggler = new wxMenu();
	LangToggler->Append(ID_LANG_ENU, wxString::FromUTF8("English (US)"));
	LangToggler->Append(ID_LANG_ENG, wxString::FromUTF8("English (UK)"));
	LangToggler->Append(ID_LANG_CS, wxString::FromUTF8("Česky (Czech)"));
	LangToggler->Append(ID_LANG_DA, wxString::FromUTF8("Dansk (Danish)"));
	LangToggler->Append(ID_LANG_DE, wxString::FromUTF8("Deutsch (German)"));
	LangToggler->Append(ID_LANG_ES, wxString::FromUTF8("Español (España) (Castilian Spanish)"));
	LangToggler->Append(ID_LANG_ESM, wxString::FromUTF8("Español (Latino) (Latin American Spanish)"));
	LangToggler->Append(ID_LANG_EO, wxString::FromUTF8("Esperanto"));
	LangToggler->Append(ID_LANG_FI, wxString::FromUTF8("Suomi (Finnish)"));
	LangToggler->Append(ID_LANG_FR, wxString::FromUTF8("Français (French)"));
	LangToggler->Append(ID_LANG_HU, wxString::FromUTF8("Magyar (Hungarian)"));
	LangToggler->Append(ID_LANG_IT, wxString::FromUTF8("Italiano (Italian)"));
	LangToggler->Append(ID_LANG_JP, wxString::FromUTF8("日本語 (Japanese)"));
	LangToggler->Append(ID_LANG_KO, wxString::FromUTF8("한국어 (Korean)"));
	LangToggler->Append(ID_LANG_NL, wxString::FromUTF8("Nederlands (Dutch)"));
	LangToggler->Append(ID_LANG_NB, wxString::FromUTF8("Norsk Bokmål (Norwegian)"));
	LangToggler->Append(ID_LANG_PL, wxString::FromUTF8("Polski (Polish)"));
	LangToggler->Append(ID_LANG_PTG, wxString::FromUTF8("Português (European Portuguese)"));
	LangToggler->Append(ID_LANG_PT, wxString::FromUTF8("Português do Brasil (Brazilian Portuguese)"));
	LangToggler->Append(ID_LANG_RO, wxString::FromUTF8("Română (Romanian)"));
	LangToggler->Append(ID_LANG_RU, wxString::FromUTF8("Русский (Russian)"));
	LangToggler->Append(ID_LANG_SR, wxString::FromUTF8("Српски (Serbian)"));
	LangToggler->Append(ID_LANG_TR, wxString::FromUTF8("Türkçe (Turkish)"));
	PrefMenu->AppendSubMenu(LangToggler, wxString::FromUTF8(GStrings.GetString("LAUNCHER_TOPBAR_PREFLANG")));

	// create menu item to view credits and release notes
	wxMenu *Credinfo = new wxMenu;
	Credinfo->Append(ID_REL_NOTES, wxString::FromUTF8(GStrings.GetString("LAUNCHER_TOPBAR_ABOUTNOTES")));
	Credinfo->Append(ID_CREDITS, wxString::FromUTF8(GStrings.GetString("LAUNCHER_TOPBAR_ABOUTCREDITS")));

	// create menu bar
	wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append(AddMenu, wxString::FromUTF8(GStrings.GetString("LAUNCHER_TOPBAR_FILE")));
	menuBar->Append(PrefMenu, wxString::FromUTF8(GStrings.GetString("LAUNCHER_TOPBAR_PREF")));
	menuBar->Append(Credinfo, wxString::FromUTF8(GStrings.GetString("LAUNCHER_TOPBAR_ABOUT")));
	SetMenuBar(menuBar);

	// the profile picker in a top to bottom list
	profileList = new wxDataViewListCtrl(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize);
	profileList->AppendTextColumn(wxString::FromUTF8(GStrings.GetString("LAUNCHER_PROFLIST_TYPE")), wxDATAVIEW_CELL_INERT, 60,
	                              wxALIGN_CENTER, 0);
	profileList->AppendTextColumn(wxString::FromUTF8(GStrings.GetString("LAUNCHER_PROFLIST_TITLE")), wxDATAVIEW_CELL_INERT, 350,
	                              wxALIGN_CENTER, 0);
	profileList->AppendTextColumn(wxString::FromUTF8(GStrings.GetString("LAUNCHER_PROFLIST_AUTHORS")), wxDATAVIEW_CELL_INERT, 350,
	                              wxALIGN_CENTER, 0);
	profileList->AppendTextColumn(wxString::FromUTF8(GStrings.GetString("LAUNCHER_PROFLIST_RELEASEDATE")), wxDATAVIEW_CELL_INERT, 100,
	                              wxALIGN_CENTER, 0);
	profileList->AppendTextColumn(wxString::FromUTF8(GStrings.GetString("LAUNCHER_PROFLIST_LASTPLAYED")), wxDATAVIEW_CELL_INERT, 100,
	                              wxALIGN_CENTER, 0);
	profileList->AppendTextColumn(wxString::FromUTF8(GStrings.GetString("LAUNCHER_PROFLIST_PLAYTIME")), wxDATAVIEW_CELL_INERT, 80,
	                              wxALIGN_CENTER, 0);

	// Add 4 buttons to the right of the profile list
	startGameButton  = new wxButton(panel, ID_START_GAME, wxString::FromUTF8(GStrings.GetString("LAUNCHER_PROFBUTTON_START")));
	joinServerButton = new wxButton(panel, ID_JOIN_GAME, "Join Server");
	hostServerButton = new wxButton(panel, ID_HOST_GAME, "Host Server");
	settingsButton   = new wxButton(panel, ID_PROFILE_SETTINGS, "Profile Settings ...");

	// move entry buttons + refresh
	refreshButton       = new wxButton(panel, ID_REFRESH_LIST, "Refresh");
	moveEntryUpButton   = new wxButton(panel, ID_MOVE_UP, "Move Up");
	moveEntryDownButton = new wxButton(panel, ID_MOVE_DOWN, "Move Down");

	// disable until user has clicked on item in profile list
	startGameButton->Disable();
	joinServerButton->Disable();
	hostServerButton->Disable();
	settingsButton->Disable();
	moveEntryUpButton->Disable();
	moveEntryDownButton->Disable();

	// the description box
	wxTextCtrl *descriptionBox = new wxTextCtrl(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("LAUNCHER_NO_DSC_SELECT")), wxDefaultPosition,
	                                            wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);

	// Menu Bindings
	Bind(wxEVT_MENU, &LauncherMainWindow::OnButtonClicked, this, ID_ADD_WAD);
	Bind(wxEVT_MENU, &LauncherMainWindow::OnButtonClicked, this, ID_ADD_ARCHIVE);
	Bind(wxEVT_MENU, &LauncherMainWindow::OnButtonClicked, this, wxID_EXIT);
	Bind(wxEVT_MENU, &LauncherMainWindow::OnButtonClicked, this, ID_REL_NOTES);
	Bind(wxEVT_MENU, &LauncherMainWindow::OnButtonClicked, this, ID_CREDITS);

	// This catches any menu event between ID_LANG_ENU and ID_LANG_TR (must be in one piece)
	Bind(wxEVT_MENU, &LauncherMainWindow::OnLanguageChanged, this, ID_LANG_ENU, ID_LANG_TR);

	// Button Bindings
	Bind(wxEVT_BUTTON, &LauncherMainWindow::OnButtonClicked, this, ID_START_GAME);
	Bind(wxEVT_BUTTON, &LauncherMainWindow::OnButtonClicked, this, ID_JOIN_GAME);
	Bind(wxEVT_BUTTON, &LauncherMainWindow::OnButtonClicked, this, ID_HOST_GAME);
	Bind(wxEVT_BUTTON, &LauncherMainWindow::OnButtonClicked, this, ID_PROFILE_SETTINGS);
	Bind(wxEVT_BUTTON, &LauncherMainWindow::OnButtonClicked, this, ID_REFRESH_LIST);
	Bind(wxEVT_BUTTON, &LauncherMainWindow::OnButtonClicked, this, ID_MOVE_UP);
	Bind(wxEVT_BUTTON, &LauncherMainWindow::OnButtonClicked, this, ID_MOVE_DOWN);

	// do exactly that above when event is captured and when user clicks away they will be disabled again
	profileList->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, [=, this](wxDataViewEvent &event) {
		bool hasSelection = (profileList->GetSelectedRow() != wxNOT_FOUND);

		// Toggle buttons on
		startGameButton->Enable(hasSelection);
		joinServerButton->Enable(hasSelection);
		hostServerButton->Enable(hasSelection);
		settingsButton->Enable(hasSelection);
		moveEntryUpButton->Enable(hasSelection);
		moveEntryDownButton->Enable(hasSelection);

		// change description to selected entry
		if (hasSelection)
		{
			descriptionBox->ChangeValue(profileList->GetTextValue(profileList->GetSelectedRow(), 6));
		}
	});

	refreshList(profileList, this);

	// force a layout: list on top, description box below, buttons right

	// make buttons look good
	wxBoxSizer *btnColumnSizer = new wxBoxSizer(wxVERTICAL);
	btnColumnSizer->Add(startGameButton, 0, wxEXPAND | wxBOTTOM, 5);
	btnColumnSizer->Add(joinServerButton, 0, wxEXPAND | wxBOTTOM, 5);
	btnColumnSizer->Add(hostServerButton, 0, wxEXPAND | wxBOTTOM, 5);
	btnColumnSizer->Add(settingsButton, 0, wxEXPAND | wxBOTTOM, 5);

	btnColumnSizer->AddStretchSpacer(1); // add a spacer between main buttons and move entry buttons and forces them
	                                     // down

	btnColumnSizer->Add(refreshButton, 0, wxEXPAND | wxBOTTOM, 5);
	btnColumnSizer->Add(moveEntryUpButton, 0, wxEXPAND, 0);
	btnColumnSizer->Add(moveEntryDownButton, 0, wxEXPAND, 0);

	// create the upper sizer first (list + buttons)
	wxBoxSizer *topSectionSizer = new wxBoxSizer(wxHORIZONTAL);
	topSectionSizer->Add(profileList, 1, wxEXPAND | wxRIGHT, 10);
	topSectionSizer->Add(btnColumnSizer, 0, wxEXPAND, 0);

	// build the main sizer
	wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
	mainSizer->Add(topSectionSizer, 1, wxEXPAND | wxALL, 10);
	// add the description box below
	mainSizer->Add(descriptionBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

	// create status bar for profile count
	CreateStatusBar();
	SetStatusText(wxString::Format(wxString::FromUTF8(GStrings.GetString("LAUNCHER_AVAIL_STATUS")), profileList->GetItemCount()));

	panel->SetSizer(mainSizer);
	panel->Layout(); // Force an immediate update

	// use langvar to update language
	updateLanguage();
}

void LauncherMainWindow::OnButtonClicked(wxCommandEvent &event)
{
	// fixated our working directory back

	if (event.GetId() == ID_REFRESH_LIST)
	{
		refreshList(profileList, this);
		return;
	}

	if (event.GetId() == ID_ADD_WAD)
	{
		Loader::fileOpener(this); // pass to loader class
		refreshList(profileList, this);
	}

	if (event.GetId() == ID_ADD_ARCHIVE)
	{
		Loader::archiveOpener(this); // pass to loader class
		refreshList(profileList, this);
	}

	if (event.GetId() == wxID_EXIT)
		Close(true); // close the main window

	if (event.GetId() == ID_REL_NOTES)
	{
		About aboutDialog;
		aboutDialog.ReleaseNotesDialog(this,langVar);
		aboutDialog.ShowModal(); // Force user to interact with notes before returning to main window
	}

	if (event.GetId() == ID_CREDITS)
	{
		About creditsDialog;
		creditsDialog.CreditsDialog(this,langVar);
		creditsDialog.ShowModal(); // Force user to interact with credits before returning to main window
	}

	//"profileList->GetSelectedRow() != wxNOT_FOUND" blocks the user from exception when clicking profile settings after
	// losing focus due to list refresh
	if (profileList->GetSelectedRow() != wxNOT_FOUND)
	{

		// startup the game
		if (event.GetId() == ID_START_GAME || event.GetId() == ID_JOIN_GAME || event.GetId() == ID_HOST_GAME)
		{
			// check if already launched
			if (isAlreadyLaunched)
			{
				wxMessageBox(wxString::FromUTF8(GStrings.GetString("LAUNCHER_ERROR_ALRRUNNING")),
				             "UZDoom", wxOK | wxICON_WARNING);
				return;
			}

			isAlreadyLaunched = true; // set flag

			Profile     tp;
			std::string dispatchedCmd;
			std::string selectedRowPath = profilePaths[profileList->GetSelectedRow()];

			TimePoint startingPoint = std::chrono::system_clock::now();

			if (event.GetId() == ID_START_GAME)
				dispatchedCmd = tp.giveLaunchCommand(selectedRowPath, "").c_str();
			if (event.GetId() == ID_JOIN_GAME)
				dispatchedCmd = tp.giveLaunchCommand(selectedRowPath, "join").c_str();
			if (event.GetId() == ID_HOST_GAME)
				dispatchedCmd = tp.giveLaunchCommand(selectedRowPath, "host").c_str();

			// below bind a listener that monitors if uzdoom closes/ends
			this->Iconize(true); // Minimize immediately

			// Track process
			wxProcess *process = new wxProcess(this);

			// Capture process end
			this->Bind(wxEVT_END_PROCESS, [this, selectedRowPath, startingPoint, process](wxProcessEvent &event) {
				TimePoint doneTime = std::chrono::system_clock::now();
				long long secondsPlayed =
					std::chrono::duration_cast<std::chrono::seconds>(doneTime - startingPoint).count();

				Profile p;
				p.loadFromFile(selectedRowPath);

				// update last played to now
				p.lastPlayedDate = std::format("{:%d-%m-%Y}", std::chrono::system_clock::now());
				p.playedTime += secondsPlayed;

				// Save back to actual file
				p.saveToFile(selectedRowPath);

				// make ui visible again
				this->Iconize(false);
				this->Raise();
				this->Show(true);

				// refresh at once for time update
				refreshList(profileList, this);

				isAlreadyLaunched = false; // reset flag

				delete process;
			});

			wxExecute(dispatchedCmd, wxEXEC_ASYNC | wxEXEC_HIDE_CONSOLE, process);
		}

		if (event.GetId() == ID_PROFILE_SETTINGS)
		{
			// This will capture the parameters from the selected entry and pass them to the profileSettings class to
			// prefill it

			ProfileSettings profileSettings;
			profileSettings.ProfileSettingsMenu(
				this, "Profile Settings", profileList->GetTextValue(profileList->GetSelectedRow(), 7).ToStdString());

			int result = profileSettings.ShowModal(); // go to the settings dialog

			// If the user clicked DELETE
			if (result == wxID_REMOVE)
			{
				profilePaths.erase(profilePaths.begin() + profileList->GetSelectedRow());
				saveConfig(this);
				refreshList(profileList, this);
			}
			// If the user clicked SAVE
			else if (result == wxID_OK)
			{
				refreshList(profileList, this);
				profileList->SelectRow(profileList->GetSelectedRow());
			}
		}

		int selectedRow = profileList->GetSelectedRow();

		if (event.GetId() == ID_MOVE_UP)
		{
			if (selectedRow > 0)
			{
				std::swap(profilePaths[selectedRow], profilePaths[selectedRow - 1]);
				saveConfig(this);
				refreshList(profileList, this);
				profileList->SelectRow(selectedRow - 1); // keep focus on item as it moves
				return;
			}
		}

		else if (event.GetId() == ID_MOVE_DOWN)
		{

			if (selectedRow < profilePaths.size() - 1)
			{
				std::swap(profilePaths[selectedRow], profilePaths[selectedRow + 1]);
				saveConfig(this);
				refreshList(profileList, this);
				profileList->SelectRow(selectedRow + 1); // keep focus on item as it moves
				return;
			}
		}
	}
}

void LauncherMainWindow::updateLanguage()
{
	// Make sure GStrings uses the correct language
	GStrings.UpdateLanguage(langVar.c_str());

	// Update all UI strings
	this->SetTitle(wxString::FromUTF8(GStrings.GetString("LAUNCHER_TITLE")) + wxString::Format(" %s", VERSION));
	startGameButton->SetLabel(wxString::FromUTF8(GStrings.GetString("LAUNCHER_PROFBUTTON_START")));
	joinServerButton->SetLabel(wxString::FromUTF8(GStrings.GetString("LAUNCHER_PROFBUTTON_JOIN")));
	hostServerButton->SetLabel(wxString::FromUTF8(GStrings.GetString("LAUNCHER_PROFBUTTON_HOST")));
	settingsButton->SetLabel(wxString::FromUTF8(GStrings.GetString("LAUNCHER_PROFBUTTON_SETTING")));

	refreshButton->SetLabel(wxString::FromUTF8(GStrings.GetString("LAUNCHER_PROFBUTTON_REFRESH")));
	moveEntryUpButton->SetLabel(wxString::FromUTF8(GStrings.GetString("LAUNCHER_PROFBUTTON_MVUP")));
	moveEntryDownButton->SetLabel(wxString::FromUTF8(GStrings.GetString("LAUNCHER_PROFBUTTON_MVDOWN")));

	// in case layout needs to be updated too
	this->Layout();
	this->Refresh();
}

void LauncherMainWindow::OnLanguageChanged(wxCommandEvent &event)
{

	if (&event)
	{
		// assign correct value for language
		switch (event.GetId())
		{
		case ID_LANG_ENU:
			langVar = "default";
			break;
		case ID_LANG_ENG:
			langVar = "eng";
			break;
		case ID_LANG_CS:
			langVar = "cs";
			break;
		case ID_LANG_DA:
			langVar = "da";
			break;
		case ID_LANG_DE:
			langVar = "de";
			break;
		case ID_LANG_ES:
			langVar = "es";
			break;
		case ID_LANG_ESM:
			langVar = "esm";
			break;
		case ID_LANG_EO:
			langVar = "eo";
			break;
		case ID_LANG_FI:
			langVar = "fi";
			break;
		case ID_LANG_FR:
			langVar = "fr";
			break;
		case ID_LANG_HU:
			langVar = "hu";
			break;
		case ID_LANG_IT:
			langVar = "it";
			break;
		case ID_LANG_JP:
			langVar = "jp";
			break;
		case ID_LANG_KO:
			langVar = "ko";
			break;
		case ID_LANG_NL:
			langVar = "nl";
			break;
		case ID_LANG_NB:
			langVar = "no";
			break;
		case ID_LANG_PL:
			langVar = "pl";
			break;
		case ID_LANG_PTG:
			langVar = "ptg";
			break;
		case ID_LANG_PT:
			langVar = "pt";
			break;
		case ID_LANG_RO:
			langVar = "ro";
			break;
		case ID_LANG_RU:
			langVar = "ru";
			break;
		case ID_LANG_SR:
			langVar = "sr";
			break;
		case ID_LANG_TR:
			langVar = "tr";
			break;
		}

		saveConfig(this);
	}
	
	updateLanguage();
}
