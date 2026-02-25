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

namespace About
{
// will be called from ImGui 
void DrawReleaseNotesDialog(bool *p_open, const std::string &lang);
void DrawCreditsDialog(bool *p_open, const std::string &lang);
};
