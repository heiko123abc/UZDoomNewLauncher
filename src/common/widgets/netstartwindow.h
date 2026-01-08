/*
** netstartwindow.h
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

#include <exception>
#include <string>
#include <vector>

#include <wx/listctrl.h>
#include <wx/wx.h>

class NetStartWindow : public wxDialog
{
  public:
	
	static void NetInit(const char *message, bool host);
	static void NetMessage(const char *message);
	static void NetConnect(int client, const char *name, unsigned flags, int status);
	static void NetUpdate(int client, int status);
	static void NetDisconnect(int client);
	static void NetProgress(int cur, int limit);
	static void NetDone();
	static void NetClose();
	static bool ShouldStartNet();
	static int  GetNetKickClient();
	static int  GetNetBanClient();
	static bool NetLoop(bool (*timer_callback)(void *), void *userdata);

	// wxWidgets Constructor
	NetStartWindow(wxWindow *parent, bool host);
	virtual ~NetStartWindow();

  protected:
	// wxWidgets Event Handlers
	void OnAbort(wxCommandEvent &event);
	void OnForceStart(wxCommandEvent &event);
	void OnKick(wxCommandEvent &event);
	void OnBan(wxCommandEvent &event);
	void OnTimer(wxTimerEvent &event);
	void OnCloseWindow(wxCloseEvent &event);

  private:
	void SetMessage(const std::string &message);
	void SetProgress(int pos);
	long FindItemByClient(int client);

	// UI Elements
	wxStaticText *lblStatus  = nullptr;
	wxStaticText *lblCount   = nullptr;
	wxListCtrl   *playerList = nullptr;

	wxButton *btnStart = nullptr;
	wxButton *btnKick  = nullptr;
	wxButton *btnBan   = nullptr;
	wxButton *btnAbort = nullptr;

	wxTimer updateTimer;

	// States
	int pos    = 0;
	int maxpos = 1;

	bool (*timer_callback)(void *) = nullptr;
	void *userdata                 = nullptr;

	bool shouldstart = false;
	bool hosting     = false;
	bool exitreason  = false;

	std::vector<int>   kickclients;
	std::vector<int>   banclients;
	std::exception_ptr CallbackException;

	static NetStartWindow *Instance;
};
