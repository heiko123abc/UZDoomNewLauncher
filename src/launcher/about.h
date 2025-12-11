#pragma once
#include <wx/wx.h>
#include <wx/html/htmlwin.h> // Required for displaying rich text for the patch notes

//wxDialog because is a dialog window that blocks interaction with other windows until closed
class About : public wxDialog {

	//Class contains both the patch notes and credits windows

public:
	void ReleaseNotesDialog(wxWindow* parent);
	void CreditsDialog(wxWindow* parent);
};