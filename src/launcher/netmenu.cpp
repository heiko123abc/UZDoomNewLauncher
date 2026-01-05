/*
** netmenu.cpp
**
** Contains both menus for hosting and joining a multiplayer game
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

#include "netmenu.h"
#include "gstrings.h"

#include <wx/listctrl.h>

void Netmenu::hostGameLobby(wxWindow *parent, const wxString &title, int playerslots)
{

	// create the window here since above is not a constructor
	this->Create(parent, wxID_ANY, wxString::FromUTF8(GStrings.GetString("NETMENU_TITLE_HOST")), wxDefaultPosition,
	             parent->FromDIP(wxSize(500, 800)));

	wxBoxSizer   *mainSizer = new wxBoxSizer(wxVERTICAL);
	wxStaticText *lblStatus = new wxStaticText(this, wxID_ANY, wxString::FromUTF8(GStrings.GetString("NETMENU_WAIT")));
	wxStaticText *lblCount  = new wxStaticText(this, wxID_ANY, "X/X");

	// Add labels to sizer (Centered horizontally)
	mainSizer->Add(lblStatus, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP, 20);
	mainSizer->Add(lblCount, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 10);

	// wxLC_REPORT allows for columns, wxLC_SINGLE_SEL allows selecting one row at a time
	wxListCtrl *playerList = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
	                                        wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_SUNKEN);

	// Create Columns to showcase
	playerList->InsertColumn(0, "#", wxLIST_FORMAT_LEFT, 30);
	playerList->InsertColumn(1, wxString::FromUTF8(GStrings.GetString("NETMENU_LIST_INFO")), wxLIST_FORMAT_CENTER, 40);
	playerList->InsertColumn(2, wxString::FromUTF8(GStrings.GetString("NETMENU_LIST_PLAYER")), wxLIST_FORMAT_LEFT, 180);
	playerList->InsertColumn(3, wxString::FromUTF8(GStrings.GetString("NETMENU_LIST_STATUS")), wxLIST_FORMAT_LEFT, 80);

	// Add player code tbd

	// Add list to main sizer (Proportion 1 to expand and fill available space)
	mainSizer->Add(playerList, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

	// Bottom Buttons
	wxBoxSizer *btnSizer = new wxBoxSizer(wxHORIZONTAL);

	// Create the buttons
	wxButton *btnStart = new wxButton(this, wxID_ANY, wxString::FromUTF8(GStrings.GetString("NETMENU_BTN_START")));
	wxButton *btnKick  = new wxButton(this, wxID_ANY, wxString::FromUTF8(GStrings.GetString("NETMENU_BTN_KICK")));
	wxButton *btnBan   = new wxButton(this, wxID_ANY, wxString::FromUTF8(GStrings.GetString("NETMENU_BTN_BAN")));
	wxButton *btnAbort = new wxButton(this, wxID_ANY, wxString::FromUTF8(GStrings.GetString("NETMENU_BTN_ABORT")));

	// Add buttons to the horizontal sizer
	// Proportion 1 ensures they split the width equally
	btnSizer->Add(btnStart, 1, wxALL, 2);
	btnSizer->Add(btnKick, 1, wxALL, 2);
	btnSizer->Add(btnBan, 1, wxALL, 2);
	btnSizer->Add(btnAbort, 1, wxALL, 2);

	// Add button row to main sizer
	mainSizer->Add(btnSizer, 0, wxEXPAND | wxALL, 10);

	// Apply layout
	this->SetSizer(mainSizer);
	this->Layout();
	this->Centre();
	this->Show();
}

void Netmenu::joinGameLobby(wxWindow *parent, const wxString &title)
{

	// create the window here since above is not a constructor
	this->Create(parent, wxID_ANY, wxString::FromUTF8(GStrings.GetString("NETMENU_TITLE_JOIN")), wxDefaultPosition, parent->FromDIP(wxSize(500, 800)));

	wxBoxSizer   *mainSizer = new wxBoxSizer(wxVERTICAL);
	wxStaticText *lblStatus = new wxStaticText(this, wxID_ANY, wxString::FromUTF8(GStrings.GetString("NETMENU_WAIT")));
	wxStaticText *lblCount  = new wxStaticText(this, wxID_ANY, "X/X");

	// Add labels to sizer (Centered horizontally)
	mainSizer->Add(lblStatus, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP, 20);
	mainSizer->Add(lblCount, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 10);

	// wxLC_REPORT allows for columns, wxLC_SINGLE_SEL allows selecting one row at a time
	wxListCtrl *playerList = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
	                                        wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_SUNKEN);

	// Create Columns to showcase
	playerList->InsertColumn(0, "#", wxLIST_FORMAT_LEFT, 30);
	playerList->InsertColumn(1, wxString::FromUTF8(GStrings.GetString("NETMENU_LIST_INFO")), wxLIST_FORMAT_CENTER, 40);
	playerList->InsertColumn(2, wxString::FromUTF8(GStrings.GetString("NETMENU_LIST_PLAYER")), wxLIST_FORMAT_LEFT, 180);
	playerList->InsertColumn(3, wxString::FromUTF8(GStrings.GetString("NETMENU_LIST_STATUS")), wxLIST_FORMAT_LEFT, 80);

	// Add player code tbd

	// Add list to main sizer (Proportion 1 to expand and fill available space)
	mainSizer->Add(playerList, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

	// Bottom Buttons
	wxBoxSizer *btnSizer = new wxBoxSizer(wxHORIZONTAL);

	// Create the buttons (joiner has only abort button)
	wxButton *btnAbort = new wxButton(this, wxID_ANY, wxString::FromUTF8(GStrings.GetString("NETMENU_BTN_ABORT")));

	// Add buttons to the horizontal sizer
	btnSizer->Add(btnAbort, 1, wxALL, 2);

	// Add button row to main sizer
	mainSizer->Add(btnSizer, 0, wxEXPAND | wxALL, 10);

	// Apply layout
	this->SetSizer(mainSizer);
	this->Layout();
	this->Centre();
	this->Show();
}
