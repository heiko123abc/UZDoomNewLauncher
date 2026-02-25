/*
** starter.cpp
**
** The launcher is STARTED here. Create required files and folder on first launch.
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

#include "starter.h"
#include "i_interface.h"
#include "launcherMainWindow.h"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <wx/debugrpt.h>
#include <wx/wx.h>

using json = nlohmann::json;
bool execResult = false;

// This is called from outside to kickstart the launcher ui and logics
bool wxKickStarter()
{
	int   argc = 1;
	char *argv[2];
	argv[0] = (char *)"UZDoom";
	argv[1] = nullptr;

	wxApp::SetInstance(new Starter()); // set this as the instance
	wxEntry(argc, argv);

	return execResult; // true if user launch via button, false if else
}


// This function is called on application startup and creates the main window
bool Starter::OnInit()
{
	wxHandleFatalExceptions(true); // allow the launcher to create crash dialogs

	wxLocale locale;
	locale.Init(wxLANGUAGE_DEFAULT);

	// DEFINE THE PATHS AND FILES WE NEED
	exePath = "./"; // we just need the folder where the executeable is located

	ROOT_DIR         = exePath + "launcher/";
	PROFILE_DIR		 = ROOT_DIR + "profiles/";
	CONFIG_FILE      = ROOT_DIR + "config.json";

	// hang on, lets see if folder for launchers profiles exists
	// if not, create them
	std::filesystem::create_directories(std::string(PROFILE_DIR.ToUTF8()));

	// and now the config file for launcher
	std::string filePath = std::string(CONFIG_FILE.ToUTF8());
	if (!std::filesystem::exists(filePath))
	{
		// Create default JSON config
		std::ofstream configFile(filePath);
		if (configFile.is_open())
		{
			json j;
			j["lang"]     = DEFAULT_LANG.data();
			j["theme"]    = "system";
			j["profiles"] = json::array(); // Ready array for later profiles

			configFile << j.dump(4); // indent for readability
			configFile.close();
		}
	}

	// Load theme from config
	std::string theme = nlohmann::json::parse(std::ifstream(filePath))["theme"];
	if (theme == "system")
	{
		SetAppearance(Appearance::System);
	}
	else if (theme == "dark")
	{
		SetAppearance(Appearance::Dark);
	}
	else
	{
		SetAppearance(Appearance::Light);
	}


	LauncherMainWindow *mainWindow = new LauncherMainWindow("UZDoom - Launcher" + wxString::Format(" %s", VERSION));

	// Windows: Manifest MUST BE SET TO DPI AWARE
	mainWindow->SetClientSize(mainWindow->FromDIP(wxSize(1280, 720)));

	mainWindow->Show(true);
	mainWindow->Center();
	return true;
}

// function is called when the application crashes
void Starter::OnFatalException()
{

	wxDebugReportCompress report; // ZIP file of the report is created

	report.AddAll(); // add all standard files: e.g stack trace etc.

	// create a dialog to tell user
	wxDebugReportPreviewStd preview;
	if (preview.Show(report) && report.Process())
		wxLogMessage("Report saved successfully.");
}
