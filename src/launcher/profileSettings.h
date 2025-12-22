#pragma once
#include "const.h"
#include "profile.h"
#include <wx/wx.h>

class ProfileSettings : public wxDialog
{

	// contains settings for user profiles

  public:
	void ProfileSettingsMenu(wxWindow *parent, const wxString &title, const std::string &profilePath);
};
