#include "about.h"

#include <filesystem>
#include <fstream>
#include <rapidxml/rapidxml.hpp>
#include <string>

std::string loadFileToString(const std::string &filename)
{
	std::ifstream file(filename, std::ios::in | std::ios::binary);
	if (!file)
		return "";

	uintmax_t   fileSize = std::filesystem::file_size(filename);
	std::string result(fileSize, '\0');
	file.read(result.data(), fileSize);

	return result;
}

std::string getReleasesParsed(std::string xmlBuffer)
{
	using namespace rapidxml;

	if (xmlBuffer.empty())
		return "";

	xml_document<> doc;

	//just in case it is invalid
	try
	{
		doc.parse<0>(xmlBuffer.data());
	}
	catch (...)
	{
		return "Error: Invalid XML";
	}

	xml_node<> *root = doc.first_node("component");
	if (!root)
		return "Error: No component tag";

	xml_node<> *releasesNode = root->first_node("releases");
	if (!releasesNode)
		return "Error: No releases tag";

	std::stringstream ss;

	//iterate through as long as more releases are in file
	for (xml_node<> *rel = releasesNode->first_node("release"); rel; rel = rel->next_sibling("release"))
	{
		xml_attribute<> *ver  = rel->first_attribute("version");
		xml_attribute<> *date = rel->first_attribute("date");

		if (ver)
			ss << ver->value();
		if (date)
			ss << " (" << date->value() << ")";

		ss << "\n"; //sperated the notes
	}

	return ss.str();
}

void About::ReleaseNotesDialog(wxWindow *parent)
{

	// create the window here since above is not a constructor
	this->Create(parent, wxID_ANY, "Release Notes", wxDefaultPosition, parent->FromDIP(wxSize(1000, 800)));

	// parse release notes xml from meta
	wxString patchNotes =
		wxString::FromUTF8(getReleasesParsed(loadFileToString("./posix/freedesktop/org.zdoom.UZDoom.metainfo.xml")));

	// required to display rich text
	wxHtmlWindow *htmlWin = new wxHtmlWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO);
	htmlWin->SetPage(patchNotes);

	// The checkbox to show/hide patch notes on update
	wxCheckBox *showOnUpdateReq = new wxCheckBox(this, wxID_ANY, "Show these notes upon new update");
	showOnUpdateReq->SetValue(true); // Default to checked

	// Button to close the dialog
	wxButton *closeButton = new wxButton(this, wxID_OK, "Close");

	// Layout using a vertical box sizer for proper arrangement
	wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
	vbox->Add(htmlWin, 1, wxEXPAND | wxALL, 10);
	vbox->Add(showOnUpdateReq, 0, wxALIGN_LEFT | wxLEFT | wxBOTTOM, 10);
	vbox->Add(closeButton, 0, wxALIGN_CENTER | wxALL, 10);

	SetSizer(vbox);
	Layout();

	Center(); // force everything to center
}

void About::CreditsDialog(wxWindow *parent)
{

	// create the window here since above is not a constructor
	this->Create(parent, wxID_ANY, "Credits", wxDefaultPosition, parent->FromDIP(wxSize(1000, 800)));

	// load about + contributors
	wxString aboutTxt   = wxString::FromUTF8(loadFileToString("../wadsrc/static/about.txt"));
	wxString contribTxt = wxString::FromUTF8(loadFileToString("../CONTRIBUTORS"));

	// required to display rich text
	wxHtmlWindow *htmlWin = new wxHtmlWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO);
	htmlWin->SetPage(wxString(aboutTxt + "\n\n\n" + contribTxt));

	// Button to close the dialog
	wxButton *closeButton = new wxButton(this, wxID_OK, "Close");

	// Layout using a vertical box sizer for proper arrangement
	wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
	vbox->Add(htmlWin, 1, wxEXPAND | wxALL, 10);
	vbox->Add(closeButton, 0, wxALIGN_CENTER | wxALL, 10);

	SetSizer(vbox);
	Layout();

	Center(); // force everything to center
}
