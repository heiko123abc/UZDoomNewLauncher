/*
** loader.h
**
** Header for loader.cpp
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
#include "gstrings.h"
#include <memory>
#include <wx/filename.h>
#include <wx/progdlg.h>
#include <wx/wfstream.h>
#include <wx/wx.h>
#include <wx/zipstrm.h>

class Loader : public wxFrame
{

  public:
	/*
	 * Method adapted from the wxWidgets Wiki
	 * Page: WxZipInputStream
	 * URL: https://wiki.wxwidgets.org/WxZipInputStream
	 * Credit: wxWidgets Wiki Contributors
	 */

	bool wxExtractZipFiles(const wxString &strZipFile, const wxString &strTargetDir, wxWindow *parent)
	{
		wxFileInputStream fis(strZipFile);

		if (!fis.IsOk())
		{
			wxLogError(wxS("Couldn't open the file '%s'."), strZipFile);
			return false;
		}

		wxZipInputStream            zis(fis);
		std::unique_ptr<wxZipEntry> upZe;

		// show the user actual progress instead of guesswork
		wxProgressDialog progress("UZDoom", wxString::FromUTF8(GStrings.GetString("LAUNCHER_EXTRAC_PREP")), 100, parent,
		                          wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_CAN_ABORT | wxPD_ELAPSED_TIME);

		// pulse the bar
		progress.Pulse();

		while (upZe.reset(zis.GetNextEntry()), upZe)
		{

			wxString statusMsg = wxString::Format("./: %s", upZe->GetName());
			bool     keepGoing = progress.Pulse(statusMsg);

			if (!keepGoing)
			{
				// User cancelled the operation -> return false
				return false;
			}

			wxString   strFileName = strTargetDir + wxFileName::GetPathSeparator() + upZe->GetName();
			int        nPermBits   = upZe->GetMode();
			wxFileName fn;

			if (upZe->IsDir())
				fn.AssignDir(strFileName);
			else
				fn.Assign(strFileName);

			// Check if the directory exists, and if not, create it recursively.
			if (!wxDirExists(fn.GetPath()))
				wxFileName::Mkdir(fn.GetPath(), nPermBits, wxPATH_MKDIR_FULL);

			if (upZe->IsDir())
				continue;

			if (!zis.CanRead())
			{
				wxLogError(wxS("Couldn't read the zip entry '%s'."), upZe->GetName());
				return false;
			}

			wxFileOutputStream fos(strFileName);

			if (!fos.IsOk())
			{
				return false;
			}

			zis.Read(fos);
		}
		return true;
	}

	static void archiveOpener(wxWindow *window);
	static void fileOpener(wxWindow *window);
};
