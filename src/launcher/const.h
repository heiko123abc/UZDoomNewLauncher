#pragma once

#include <string>
#include <string_view>
#include <wx/filename.h>
#include <wx/stdpaths.h>

// a precaution when shortcuts are used to get the actual launcher executable
inline wxString exePath;

// defined directories
inline wxString ROOT_DIR;
inline wxString IWAD_PROFILE_DIR;
inline wxString PWAD_PROFILE_DIR;

// config file path
inline wxString CONFIG_FILE;

// default language is english (en)
inline std::string_view DEFAULT_LANG = "en";
