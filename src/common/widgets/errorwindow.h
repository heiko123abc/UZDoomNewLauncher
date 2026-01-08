/*
** errorwindow.h
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

#pragma once

#include <string>
#include <vector>
#include <wx/dialog.h>
#include <wx/richtext/richtextctrl.h>
#include <wx/wx.h>

class ErrorWindow : public wxDialog
{
  public:
	static bool ExecModal(const std::string &text, const std::string &log, std::vector<uint8_t> minidump);

	ErrorWindow(const std::string &text, const std::string &log, std::vector<uint8_t> minidump);
	virtual ~ErrorWindow();

  private:
	// Internal helpers
	void     ParseAndAddLog(const std::string &log, const std::string &errorText);

	// Event Handlers
	void OnClipboard(wxCommandEvent &event);
	void OnQuit(wxCommandEvent &event);
	void OnSaveReport(wxCommandEvent &event);

	// Data
	std::vector<uint8_t> minidump;
	std::string          cleanClipboardText;

	// GUI Controls
	wxRichTextCtrl *logView;
	wxButton       *btnClipboard;
	wxButton       *btnAction;
	wxButton       *btnAction2;
};
