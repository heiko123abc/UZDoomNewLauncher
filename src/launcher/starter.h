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

#include "const.h"
#include <string>

struct FStartupSelectionInfo;
extern bool execResult;

class Starter
{
  public:
	static bool Init();

	// Runs the main ImGui rendering loop
	static void RunLoop();

	static void Shutdown();
};

// entry point from outside
bool ImGuiKickStarter();
