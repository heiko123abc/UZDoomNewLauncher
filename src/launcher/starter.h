#pragma once
#include <wx/wx.h>

class Starter : public wxApp
{
public:
	bool OnInit();
	void OnFatalException();
};