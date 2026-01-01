/*
** netmenu.h
**
** Header for netmenu.cpp
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
#include <wx/wx.h>

// wxDialog because is a dialog window that blocks interaction until the game is ready to play
class Netmenu : public wxDialog
{

	// Class contains both the joining game and hosting game windows

  public:
	void hostGameLobby(wxWindow *parent, const wxString &title, int playerslots);
	void joinGameLobby(wxWindow *parent, const wxString &title);
};
