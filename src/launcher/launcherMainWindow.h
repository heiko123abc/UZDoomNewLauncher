#pragma once
#include <wx/wx.h>
#include <wx/dataview.h>
#include <wx/dcclient.h>
#include <wx/artprov.h> // Required for icons
#include "Const.h"

class LauncherMainWindow : public wxFrame
{
public:

	wxDataViewListCtrl* profileList; //displays the profile paths in UI
	std::vector<std::string> profilePaths; //actual storage also used in backend

	LauncherMainWindow(const wxString& title);


private:
	void OnButtonClicked(wxCommandEvent& event);
	wxDECLARE_EVENT_TABLE();
};