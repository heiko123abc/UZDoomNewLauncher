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
#include "const.h"

struct FStartupSelectionInfo;
extern bool execResult;

class Starter : public wxApp
{

  public:
	Starter(FStartupSelectionInfo &info);
	bool OnInit();
	void OnFatalException();

	FStartupSelectionInfo &GetStartInfo()
	{
		return hidden_info;
	}

	private:
	FStartupSelectionInfo &hidden_info;
};

bool wxKickStarter(FStartupSelectionInfo &info);
