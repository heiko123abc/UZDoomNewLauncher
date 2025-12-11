#include "LauncherMainWindow.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <vector>
#include <wx/artprov.h>
#include <wx/dataview.h>
#include <wx/filedlg.h>
#include <wx/process.h>
#include <wx/spinctrl.h>
#include <wx/utils.h>

#include "About.h"
#include "Loader.h"
#include "Profile.h"
#include "ProfileSettings.h"

using json = nlohmann::json;

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
	ID_ADD_WAD,
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
	ID_MOVE_DOWN
};

// Event Binder Table
wxBEGIN_EVENT_TABLE(LauncherMainWindow, wxFrame) EVT_MENU(ID_ADD_WAD, LauncherMainWindow::OnButtonClicked)
	EVT_MENU(ID_ADD_ARCHIVE, LauncherMainWindow::OnButtonClicked)
		EVT_MENU(wxID_EXIT, LauncherMainWindow::OnButtonClicked)
			EVT_MENU(ID_REL_NOTES, LauncherMainWindow::OnButtonClicked)
				EVT_MENU(ID_CREDITS, LauncherMainWindow::OnButtonClicked)
					EVT_BUTTON(ID_START_GAME, LauncherMainWindow::OnButtonClicked)
						EVT_BUTTON(ID_JOIN_GAME, LauncherMainWindow::OnButtonClicked)
							EVT_BUTTON(ID_HOST_GAME, LauncherMainWindow::OnButtonClicked)
								EVT_BUTTON(ID_PROFILE_SETTINGS, LauncherMainWindow::OnButtonClicked)
									EVT_BUTTON(ID_REFRESH_LIST, LauncherMainWindow::OnButtonClicked)
										EVT_BUTTON(ID_MOVE_UP, LauncherMainWindow::OnButtonClicked)
											EVT_BUTTON(ID_MOVE_DOWN, LauncherMainWindow::OnButtonClicked)
												wxEND_EVENT_TABLE()

	// write back the vector to file
	void saveConfig(LauncherMainWindow *lmw)
{
	json j;
	j["lang"]     = DEFAULT_LANG.data(); // This needs to become a variable down the line
	j["profiles"] = lmw->profilePaths;

	std::ofstream file(CONFIG_FILE.ToUTF8());
	if (file.is_open())
	{
		file << j.dump(4);
		file.close();
	}
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
	this->SetWindowStyle(wxDEFAULT_FRAME_STYLE &
	                     ~(wxRESIZE_BORDER | wxMAXIMIZE_BOX)); // ban the user from resizing and maximizing

	// create menu item to add Archive or WAD for new profile
	wxMenu *AddMenu = new wxMenu;
	AddMenu->Append(ID_ADD_WAD, "&Add WAD ...", "Load a Doom WAD file.");
	AddMenu->Append(ID_ADD_ARCHIVE, "&Add Archive ...", "Load a ZIP archive.");
	AddMenu->AppendSeparator();
	AddMenu->Append(wxID_EXIT, "&Exit");

	// create menu item to change preferences e.g language, theme, etc
	wxMenu *PrefMenu = new wxMenu;

	// create theme toggler
	wxMenu *LangToggler = new wxMenu();
	// TODO add lang support here
	PrefMenu->AppendSubMenu(LangToggler, "&Language", "Change application language.");

	// create menu item to view credits and release notes
	wxMenu *Credinfo = new wxMenu;
	Credinfo->Append(ID_REL_NOTES, "&Release Notes", "View version history of UZDoom.");
	Credinfo->Append(ID_CREDITS, "&Credits", "View Credits of UZDoom.");

	// create menu bar
	wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append(AddMenu, "&File");
	menuBar->Append(PrefMenu, "&Preferences");
	menuBar->Append(Credinfo, "&About");
	SetMenuBar(menuBar);

	// the profile picker in a top to bottom list
	profileList = new wxDataViewListCtrl(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize);
	profileList->AppendTextColumn("Type", wxDATAVIEW_CELL_INERT, 60, wxALIGN_CENTER, 0);
	profileList->AppendTextColumn("Title", wxDATAVIEW_CELL_INERT, 350, wxALIGN_CENTER, 0);
	profileList->AppendTextColumn("Author(s)", wxDATAVIEW_CELL_INERT, 350, wxALIGN_CENTER, 0);
	profileList->AppendTextColumn("Release Date", wxDATAVIEW_CELL_INERT, 100, wxALIGN_CENTER, 0);
	profileList->AppendTextColumn("Last Played", wxDATAVIEW_CELL_INERT, 100, wxALIGN_CENTER, 0);
	profileList->AppendTextColumn("Playtime", wxDATAVIEW_CELL_INERT, 80, wxALIGN_CENTER, 0);

	// Add 4 buttons to the right of the profile list
	wxButton *startGameButton  = new wxButton(panel, ID_START_GAME, "Start Game");
	wxButton *joinServerButton = new wxButton(panel, ID_JOIN_GAME, "Join Server");
	wxButton *hostServerButton = new wxButton(panel, ID_HOST_GAME, "Host Server");
	wxButton *settingsButton   = new wxButton(panel, ID_PROFILE_SETTINGS, "Profile Settings ...");

	// move entry buttons + refresh
	wxButton *refreshButton       = new wxButton(panel, ID_REFRESH_LIST, "Refresh");
	wxButton *moveEntryUpButton   = new wxButton(panel, ID_MOVE_UP, "Move Up");
	wxButton *moveEntryDownButton = new wxButton(panel, ID_MOVE_DOWN, "Move Down");

	// disable until user has clicked on item in profile list
	startGameButton->Disable();
	joinServerButton->Disable();
	hostServerButton->Disable();
	settingsButton->Disable();
	moveEntryUpButton->Disable();
	moveEntryDownButton->Disable();

	// the description box
	wxTextCtrl *descriptionBox = new wxTextCtrl(panel, wxID_ANY, wxT("Nothing Selected."), wxDefaultPosition,
	                                            wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);

	// do exactly that above when event is captured and when user clicks away they will be disabled again
	profileList->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, [=](wxDataViewEvent &event) {
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
	SetStatusText(wxString::Format("%d Profile(s) available.", profileList->GetItemCount()));

	panel->SetSizer(mainSizer);
	panel->Layout(); // Force an immediate update
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
		aboutDialog.ReleaseNotesDialog(this);
		aboutDialog.ShowModal(); // Force user to interact with notes before returning to main window
	}

	if (event.GetId() == ID_CREDITS)
	{
		About creditsDialog;
		creditsDialog.CreditsDialog(this);
		creditsDialog.ShowModal(); // Force user to interact with credits before returning to main window
	}

	//"profileList->GetSelectedRow() != wxNOT_FOUND" blocks the user from exception when clicking profile settings after
	//losing focus due to list refresh
	if (profileList->GetSelectedRow() != wxNOT_FOUND)
	{

		// startup the game
		if (event.GetId() == ID_START_GAME || event.GetId() == ID_JOIN_GAME || event.GetId() == ID_HOST_GAME)
		{

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

			// DEBUG: Show command before launch
			// wxMessageBox(dispatchedCmd, "Command to be used");

			// below bind a listener that monitors if uzdoom closes/ends
			this->Iconize(true); // Minimize immediately

			// Track process
			wxProcess *process = new wxProcess(this);

			// Capture process end
			this->Bind(wxEVT_END_PROCESS, [this, selectedRowPath, startingPoint](wxProcessEvent &event) {
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
