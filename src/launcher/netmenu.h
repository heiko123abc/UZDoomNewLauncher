#pragma once
#include <wx/wx.h>

// wxDialog because is a dialog window that blocks interaction until the game is ready to play
class Netmenu : public wxDialog
{

	// Class contains both the joining game and hosting game windows

  public:
	void hostGameLobby(wxWindow *parent, const wxString &title, int playerslots);
	void joinGameLobby(wxWindow *parent, const wxString &title);
};
