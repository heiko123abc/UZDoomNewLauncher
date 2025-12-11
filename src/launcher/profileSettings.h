#pragma once
#include <wx/wx.h>
#include "Const.h"
#include "Profile.h"

class ProfileSettings : public wxDialog {

	//contains settings for user profiles

public:
	void ProfileSettingsMenu(wxWindow* parent, const wxString& title, const std::string& profilePath);
};

