/*
** const.h
**
** Defines the consts to be used by the launcher
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

#include <string>
#include <string_view>
#include <wx/filename.h>
#include <wx/stdpaths.h>

// a precaution when shortcuts are used to get the actual launcher executable
inline wxString exePath;

// defined directories
inline wxString ROOT_DIR;
inline wxString PROFILE_DIR;

// config file path
inline wxString CONFIG_FILE;

// default language is english (en)
inline std::string_view DEFAULT_LANG = "en";
