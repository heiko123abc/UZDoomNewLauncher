/*
** errorwindow.cpp
**
**
**
**---------------------------------------------------------------------------
**
** Copyright 2024 Magnus Norddahl
** Copyright 2024-2025 GZDoom Maintainers and Contributors
** Copyright 2025-2026 UZDoom Maintainers and Contributors
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
**---------------------------------------------------------------------------
**
*/

#include "errorwindow.h"
#include "gstrings.h"
#include "printf.h"
#include "gstrings.h"
#include "printf.h"

#include <miniz.h>
#include <wx/clipbrd.h>
#include <wx/display.h>
#include <wx/filedlg.h>
#include <wx/wfstream.h>
#include <wx/wx.h>

class CrashReporter : public wxApp
{
	std::string          text;
	std::string          log;
	std::vector<uint8_t> dump;

  public:
	CrashReporter(std::string text, std::string log, std::vector<uint8_t> dump)
		: text(std::move(text)), log(std::move(log)), dump(std::move(dump))
	{
	}

	virtual bool OnInit()
	{
		ErrorWindow *dlg = new ErrorWindow(text, log, std::move(dump));
		dlg->SetClientSize(dlg->FromDIP(wxSize(1200, 600)));

		dlg->Show(true);
		dlg->Center();
		return true;
	}
};

bool ErrorWindow::ExecModal(const std::string &text, const std::string &log, std::vector<uint8_t> minidump)
{

	int   argc   = 1;
	char *argv[] = {(char *)"UZDoom", nullptr};

	wxApp::SetInstance(new CrashReporter(text, log, std::move(minidump)));
	wxEntry(argc, argv);

	return false;
}

ErrorWindow::ErrorWindow(const std::string &text, const std::string &log, std::vector<uint8_t> initminidump)
	: wxFrame(nullptr, wxID_ANY, "Fatal Error", wxDefaultPosition), minidump(std::move(initminidump))
{
	SetTitle(wxString::FromUTF8(GStrings.GetString("CRASHREPORT_TITLE"))); // title of the crash window
	SetBackgroundColour(wxColour(38, 38, 38));

	this->SetClientSize(this->FromDIP(wxSize(1200, 700)));

	wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

	logView = new wxRichTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxRE_READONLY | wxBORDER_NONE);
	logView->SetBackgroundColour(wxColour(38, 38, 38));

	// Default style
	wxRichTextAttr defaultStyle;
	defaultStyle.SetFont(wxFont(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	defaultStyle.SetTextColour(wxColour(255, 255, 255));
	logView->SetBasicStyle(defaultStyle);

	mainSizer->Add(logView, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(20));

	// make sure the buttons are on lighter background
	wxPanel *btnPanel = new wxPanel(this, wxID_ANY);
	btnPanel->SetBackgroundColour(wxColour(204, 196, 194));

	wxBoxSizer *btnSizer = new wxBoxSizer(wxHORIZONTAL);

	btnClipboard = new wxButton(btnPanel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("CRASHREPORT_COPYCLIP")));
	btnClipboard->Bind(wxEVT_BUTTON, &ErrorWindow::OnClipboard, this);

	btnSizer->Add(btnClipboard, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxTOP | wxBOTTOM, FromDIP(20));
	btnSizer->AddStretchSpacer(1);

	// Save button (only if minidump exists)
	if (minidump.empty())
	{
		btnAction = new wxButton(btnPanel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("CRASHREPORT_NOSAVE")));
		btnAction->Disable();
	}
	else
	{
		btnAction = new wxButton(btnPanel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("CRASHREPORT_SAVE")));
		btnAction->Bind(wxEVT_BUTTON, &ErrorWindow::OnSaveReport, this);
	}
	btnSizer->Add(btnAction, 0, wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM, FromDIP(20));

	btnSizer->AddStretchSpacer(1);

	btnAction2 = new wxButton(btnPanel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("CRASHREPORT_QUIT")));
	btnAction2->Bind(wxEVT_BUTTON, &ErrorWindow::OnQuit, this);

	btnSizer->Add(btnAction2, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxTOP | wxBOTTOM, FromDIP(20));

	btnPanel->SetSizer(btnSizer);

	// Add the panel to the main sizer
	mainSizer->Add(btnPanel, 0, wxEXPAND);

	SetSizer(mainSizer);
	Layout();
	CenterOnScreen();

	ParseAndAddLog(log, text);

	logView->SetFocus();
}

ErrorWindow::~ErrorWindow()
{
}

void ErrorWindow::ParseAndAddLog(const std::string &log, const std::string &errorText)
{
	std::string processedLog;
	processedLog.reserve(log.size());

	for (unsigned char chr : log)
	{
		if (chr == TEXTCOLOR_ESCAPE)
			continue; // Strip color escape codes

		// Replace control range with Box Drawings Double Horizontal
		if (chr >= 0x1D && chr <= 0x1F)
		{
			processedLog += "\xE2\x95\x90";
		}
		else
		{
			processedLog += chr;
		}
	}

	// become the clipboard text
	cleanClipboardText = processedLog + "\n\nExecution could not continue.\n" + errorText + "\n";

	logView->Freeze();
	logView->Clear();

	// write the log
	logView->BeginTextColour(wxColour(255, 255, 255));
	logView->WriteText(wxString::FromUTF8(processedLog));
	logView->EndTextColour();

	logView->WriteText("\n\n");

	// simply a helper lambda to write styled text easily
	auto WriteStyled = [&](const wxString &text, const wxColour &color) {
		logView->BeginTextColour(color);
		logView->BeginFontSize(12);
		logView->WriteText(text);
		logView->EndFontSize();
		logView->EndTextColour();
	};

	// Error Header (Red)
	WriteStyled("Execution could not continue.\n", wxColour(255, 170, 170));

	// Body (Yellow)
	WriteStyled(wxString::FromUTF8(errorText), wxColour(255, 255, 170));

	logView->ShowPosition(logView->GetLastPosition());
	logView->Thaw();
}

void ErrorWindow::OnClipboard(wxCommandEvent &event)
{
	if (wxTheClipboard->Open())
	{
		// clean text only
		wxTheClipboard->SetData(new wxTextDataObject(wxString::FromUTF8(cleanClipboardText)));
		wxTheClipboard->Close();
	}
}

void ErrorWindow::OnQuit(wxCommandEvent &event)
{
	wxExit();
}

void ErrorWindow::OnSaveReport(wxCommandEvent &event)
{
	wxFileDialog dialog(this, wxString::FromUTF8(GStrings.GetString("CRASHREPORT_SAVEDIAG")), "",
	                    "UZDoomCrashReport.zip", wxString::FromUTF8(GStrings.GetString("CRASHREPORT_SAVEZIP")),
	                    wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	if (dialog.ShowModal() == wxID_OK)
	{
		std::string filename = dialog.GetPath().ToStdString();

		mz_zip_archive zip = {};
		if (mz_zip_writer_init_heap(&zip, 0, 16 * 1024 * 1024))
		{
			// Add Minidump
			mz_zip_writer_add_mem(&zip, "minidump.dmp", minidump.data(), minidump.size(), MZ_DEFAULT_COMPRESSION);

			// Add Log text
			mz_zip_writer_add_mem(&zip, "log.txt", cleanClipboardText.data(), cleanClipboardText.size(),
			                      MZ_DEFAULT_COMPRESSION);
		}

		// Finalize Zip
		void  *buffer     = nullptr;
		size_t buffersize = 0;
		mz_zip_writer_finalize_heap_archive(&zip, &buffer, &buffersize);
		mz_zip_writer_end(&zip);

		// Write to disk using wxFile
		wxFile file(filename, wxFile::write);
		if (file.IsOpened())
		{
			file.Write(buffer, buffersize);
			file.Close();
		}

		// Free the buffer allocated by miniz
		mz_free(buffer);
	}
}
