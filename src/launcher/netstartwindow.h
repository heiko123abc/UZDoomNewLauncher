/*
** netstartwindow.h
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

class NetStartWindow
{
  public:
	void NetInit(const char *message, bool host);
	void NetMessage(const char *message);
	void NetConnect(int client, const char *name, unsigned flags, int status);
	void NetUpdate(int client, int status);
	void NetDisconnect(int client);
	void NetProgress(int cur, int limit);
	void NetDone();
	void NetClose();
	bool ShouldStartNet();
	int  GetNetKickClient();
	int  GetNetBanClient();
	bool NetLoop(bool (*timer_callback)(void *), void *userdata);
};
