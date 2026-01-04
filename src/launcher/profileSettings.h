/*
** profileSettings.h
**
** Contains header for profileSettings.cpp
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
#include "profile.h"
#include <wx/wx.h>

class ProfileSettings : public wxDialog
{

	// contains settings for user profiles
  private:


  public:
	void ProfileSettingsMenu(wxWindow *parent, const wxString &title, const std::string &profilePath);
};
