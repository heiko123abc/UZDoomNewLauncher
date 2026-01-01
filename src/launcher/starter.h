/*
** starter.h
**
** Header for starter.cpp
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

class Starter : public wxApp
{
  public:
	bool OnInit();
	void OnFatalException();
};

int wxKickStarter();
