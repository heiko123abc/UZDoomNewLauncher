/*
** netstartwindow.cpp
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

#include "netstartwindow.h"
#include "gstrings.h"
#include "version.h"

NetStartWindow *NetStartWindow::Instance = nullptr;

class NetMenu : public wxApp
{
  public:
	virtual bool OnInit()
	{
		return true;
	}
};

void NetStartWindow::NetInit(const char *message, bool host)
{
	// since this is called seperate from the main laucher, init wxWidgets on its own is needed

	if (!wxTheApp)
	{
		wxApp::SetInstance(new NetMenu());

		static int   argc       = 1;
		static char  progName[] = "uzdoom_net"; // Dummy program name
		static char *argv[]     = {progName, nullptr};

		if (!wxEntryStart(argc, argv))
		{
			fprintf(stderr, "Fatal Error: Failed to initialize wxWidgets.\n");
			return;
		}

		if (!wxTheApp->CallOnInit())
		{
			fprintf(stderr, "Fatal Error: Failed to initialize NetMenu App.\n");
			return;
		}
	}

	if (!Instance)
	{
		// passing nullptr as parent since this is the top-level here
		Instance = new NetStartWindow(nullptr, host);
		wxTheApp->SetTopWindow(Instance);
		Instance->Raise();
	}

	if (message)
	{
		Instance->SetMessage(message);
	}
}

void NetStartWindow::NetMessage(const char *message)
{
	if (Instance)
		Instance->SetMessage(message);
}

void NetStartWindow::NetConnect(int client, const char *name, unsigned flags, int status)
{
	if (!Instance)
		return;

	// Parse Flags
	wxString flagStr = "";
	if (flags & 1)
		flagStr += "*";
	if (flags & 2)
		flagStr += "H";

	// Parse Status
	wxString statusStr = "";
	if (status == 1)
		statusStr = wxString::FromUTF8(GStrings.GetString("NETMENU_STATUS_CONN"));
	else if (status == 2)
		statusStr = wxString::FromUTF8(GStrings.GetString("NETMENU_STATUS_WAIT"));
	else if (status == 3)
		statusStr = wxString::FromUTF8(GStrings.GetString("NETMENU_STATUS_READY"));

	long index = Instance->FindItemByClient(client);

	if (index == -1)
	{
		// Add new player
		index = Instance->playerList->InsertItem(Instance->playerList->GetItemCount(), wxString::Format("%d", client));
		Instance->playerList->SetItemData(index, client); // Store client ID
	}

	// Update columns accordingly
	Instance->playerList->SetItem(index, 1, flagStr);
	Instance->playerList->SetItem(index, 2, wxString::FromUTF8(name));
	Instance->playerList->SetItem(index, 3, statusStr);
}

void NetStartWindow::NetUpdate(int client, int status)
{
	if (!Instance)
		return;

	wxString statusStr = "";
	if (status == 1)
		statusStr = wxString::FromUTF8(GStrings.GetString("NETMENU_STATUS_CONN"));
	else if (status == 2)
		statusStr = wxString::FromUTF8(GStrings.GetString("NETMENU_STATUS_WAIT"));
	else if (status == 3)
		statusStr = wxString::FromUTF8(GStrings.GetString("NETMENU_STATUS_READY"));

	long index = Instance->FindItemByClient(client);
	if (index != -1)
	{
		Instance->playerList->SetItem(index, 3, statusStr);
	}
}

void NetStartWindow::NetDisconnect(int client)
{
	if (!Instance)
		return;

	long index = Instance->FindItemByClient(client);
	if (index != -1)
	{
		// Remove the row entirely upon disconnect
		Instance->playerList->DeleteItem(index);
	}
}

void NetStartWindow::NetProgress(int cur, int limit)
{
	if (!Instance)
		return;

	Instance->maxpos = limit;
	Instance->SetProgress(cur);

	// Ensure list has enough free slots if players haven't connected yet
	int currentCount = Instance->playerList->GetItemCount();
	for (int i = currentCount; i < limit; ++i)
	{
		long idx = Instance->playerList->InsertItem(i, wxString::Format("%d", i));
		Instance->playerList->SetItemData(idx, i);
	}
}

void NetStartWindow::NetDone()
{
	if (Instance)
	{
		Instance->Destroy();
		Instance = nullptr;
	}

	wxEntryCleanup(); // cleanup
}

void NetStartWindow::NetClose()
{
	if (Instance)
	{
		Instance->EndModal(wxID_CANCEL);
	}
}

bool NetStartWindow::ShouldStartNet()
{
	if (Instance)
		return Instance->shouldstart;
	return false;
}

int NetStartWindow::GetNetKickClient()
{
	if (!Instance || Instance->kickclients.empty())
		return -1;

	int next = Instance->kickclients.back();
	Instance->kickclients.pop_back();
	return next;
}

int NetStartWindow::GetNetBanClient()
{
	if (!Instance || Instance->banclients.empty())
		return -1;

	int next = Instance->banclients.back();
	Instance->banclients.pop_back();
	return next;
}

bool NetStartWindow::NetLoop(bool (*loopCallback)(void *), void *data)
{
	if (!Instance)
		return false;

	Instance->timer_callback    = loopCallback;
	Instance->userdata          = data;
	Instance->CallbackException = {};

	// 10ms interval gives responsive network updates
	Instance->updateTimer.Start(10);

	// ShowModal blocks execution here until EndModal is called (we wait)
	Instance->ShowModal();

	Instance->updateTimer.Stop();
	Instance->timer_callback = nullptr;
	Instance->userdata       = nullptr;

	if (Instance->CallbackException)
		std::rethrow_exception(Instance->CallbackException);

	return Instance->exitreason;
}

NetStartWindow::NetStartWindow(wxWindow *parent, bool host)
	: wxDialog(parent, wxID_ANY, wxString::FromUTF8(GAMENAME), wxDefaultPosition, wxSize(500, 600),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	  hosting(host)
{

	// Cerate Layout Setup
	wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

	// Status Label
	lblStatus         = new wxStaticText(this, wxID_ANY, wxString::FromUTF8(GStrings.GetString("NETMENU_WAIT")));
	wxFont statusFont = lblStatus->GetFont();
	statusFont.MakeBold();
	lblStatus->SetFont(statusFont);

	// Count Label
	lblCount = new wxStaticText(this, wxID_ANY, "0/0");

	mainSizer->Add(lblStatus, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP, 15);
	mainSizer->Add(lblCount, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 10);

	// Player List
	playerList = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
	                            wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_SUNKEN);

	// Columns: #, Info, Player, Status
	playerList->InsertColumn(0, "#", wxLIST_FORMAT_LEFT, FromDIP(40));
	playerList->InsertColumn(1, wxString::FromUTF8(GStrings.GetString("NETMENU_LIST_INFO")), wxLIST_FORMAT_CENTER,
	                         FromDIP(40));
	playerList->InsertColumn(2, wxString::FromUTF8(GStrings.GetString("NETMENU_LIST_PLAYER")), wxLIST_FORMAT_LEFT,
	                         FromDIP(250));
	playerList->InsertColumn(3, wxString::FromUTF8(GStrings.GetString("NETMENU_LIST_STATUS")), wxLIST_FORMAT_LEFT,
	                         FromDIP(150));

	mainSizer->Add(playerList, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

	// Buttons
	wxBoxSizer *btnSizer = new wxBoxSizer(wxHORIZONTAL);
	btnAbort             = new wxButton(this, wxID_ANY, wxString::FromUTF8(GStrings.GetString("NETMENU_BTN_ABORT")));

	if (hosting)
	{
		btnStart = new wxButton(this, wxID_ANY, wxString::FromUTF8(GStrings.GetString("NETMENU_BTN_START")));
		btnKick  = new wxButton(this, wxID_ANY, wxString::FromUTF8(GStrings.GetString("NETMENU_BTN_KICK")));
		btnBan   = new wxButton(this, wxID_ANY, wxString::FromUTF8(GStrings.GetString("NETMENU_BTN_BAN")));

		// Add to sizer (Start, Kick, Ban, Abort)
		btnSizer->Add(btnStart, 1, wxALL, FromDIP(5));
		btnSizer->Add(btnKick, 1, wxALL, FromDIP(5));
		btnSizer->Add(btnBan, 1, wxALL, FromDIP(5));
		btnSizer->Add(btnAbort, 1, wxALL, FromDIP(5));

		// Bind Host Events
		btnStart->Bind(wxEVT_BUTTON, &NetStartWindow::OnForceStart, this);
		btnKick->Bind(wxEVT_BUTTON, &NetStartWindow::OnKick, this);
		btnBan->Bind(wxEVT_BUTTON, &NetStartWindow::OnBan, this);
	}
	else
	{
		// Client only sees Abort
		btnSizer->AddStretchSpacer(FromDIP(1));
		btnSizer->Add(btnAbort, 0, wxALL, FromDIP(5));
		btnSizer->AddStretchSpacer(FromDIP(1));
	}

	btnAbort->Bind(wxEVT_BUTTON, &NetStartWindow::OnAbort, this);

	mainSizer->Add(btnSizer, 0, wxEXPAND | wxALL, FromDIP(10));

	SetSizer(mainSizer);
	this->SetClientSize(this->FromDIP(wxSize(500, 600)));
	mainSizer->SetSizeHints(this);

	Layout();
	Centre();

	// Bind Timer and Close events
	Bind(wxEVT_TIMER, &NetStartWindow::OnTimer, this);
	Bind(wxEVT_CLOSE_WINDOW, &NetStartWindow::OnCloseWindow, this);

	updateTimer.SetOwner(this);
}

NetStartWindow::~NetStartWindow()
{
	// Clean up if destroyed manually
	if (Instance == this)
		Instance = nullptr;
}

void NetStartWindow::SetMessage(const std::string &message)
{
	if (lblStatus)
	{
		lblStatus->SetLabel(wxString::FromUTF8(message));
		lblStatus->Refresh();
	}
}

void NetStartWindow::SetProgress(int newpos)
{
	if (pos != newpos && maxpos > 1)
	{
		pos = newpos;
		lblCount->SetLabel(wxString::Format("%d/%d", pos, maxpos));
		Layout();
	}
}

long NetStartWindow::FindItemByClient(int client)
{
	long itemIndex = -1;
	while ((itemIndex = playerList->GetNextItem(itemIndex, wxLIST_NEXT_ALL, wxLIST_STATE_DONTCARE)) != wxNOT_FOUND)
	{
		if (playerList->GetItemData(itemIndex) == client)
		{
			return itemIndex;
		}
	}
	return -1;
}

void NetStartWindow::OnAbort(wxCommandEvent &event)
{
	exitreason = false;
	EndModal(wxID_CANCEL);
}

void NetStartWindow::OnCloseWindow(wxCloseEvent &event)
{
	exitreason = false;
	EndModal(wxID_CANCEL);
}

void NetStartWindow::OnForceStart(wxCommandEvent &event)
{
	shouldstart = true;
}

void NetStartWindow::OnKick(wxCommandEvent &event)
{
	long item = playerList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (item != wxNOT_FOUND)
	{
		int clientID = (int)playerList->GetItemData(item);

		bool exists = false;
		for (int c : kickclients)
			if (c == clientID)
				exists = true;

		if (!exists)
			kickclients.push_back(clientID);
	}
}

void NetStartWindow::OnBan(wxCommandEvent &event)
{
	long item = playerList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (item != wxNOT_FOUND)
	{
		int clientID = (int)playerList->GetItemData(item);

		bool exists = false;
		for (int c : banclients)
			if (c == clientID)
				exists = true;

		if (!exists)
			banclients.push_back(clientID);
	}
}

void NetStartWindow::OnTimer(wxTimerEvent &event)
{
	if (timer_callback)
	{
		bool result = false;
		try
		{
			result = timer_callback(userdata);
		}
		catch (...)
		{
			CallbackException = std::current_exception();
		}

		if (result)
		{
			exitreason = true;
			EndModal(wxID_OK);
		}
	}
}
