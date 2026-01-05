/*
** launcherMainWindow.h
**
** Header for launcherMainWindow.cpp
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

#pragma once
#include "const.h"
#include <wx/artprov.h> // Required for icons
#include <wx/dataview.h>
#include <wx/dcclient.h>
#include <wx/wx.h>

class LauncherMainWindow : public wxFrame
{
  public:
	wxDataViewListCtrl      *profileList;  // displays the profile paths in UI
	std::vector<std::string> profilePaths; // actual storage also used in backend

	LauncherMainWindow(const wxString &title);

  private:
	wxButton *startGameButton;
	wxButton *joinServerButton;
	wxButton *hostServerButton;
	wxButton *settingsButton;

	wxButton *refreshButton;
	wxButton *moveEntryUpButton;
	wxButton *moveEntryDownButton;

	wxMenuBar *menuBar;

	void updateLanguage();
	void OnButtonClicked(wxCommandEvent &event);
	void OnLanguageChanged(wxCommandEvent &event);
};
