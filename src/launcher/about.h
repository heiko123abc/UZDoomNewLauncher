/*
** about.h
**
** Header for about.cpp
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
#include <wx/html/htmlwin.h> // Required for displaying rich text for the patch notes
#include <wx/wx.h>

// wxDialog because is a dialog window that blocks interaction with other windows until closed
class About : public wxDialog
{

  public:
	void ReleaseNotesDialog(wxWindow *parent, std::string lang);
	void CreditsDialog(wxWindow *parent, std::string lang);
};
