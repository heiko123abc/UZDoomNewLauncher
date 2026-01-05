/*
** about.cpp
**
** Create the UI to display the Credits and Patch Notes
**
**---------------------------------------------------------------------------
**
** Copyright 2025 Marcus Minhorst for _ParseReleaseNotes() (adapted), _BuildReleaseNotes(), _OpenReleaseNotes(),
*GetReleaseNotes()
** and adapted GetAboutText()
**
** Copyright 2025-2026 UZDoom Maintainers and Contributors
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
**---------------------------------------------------------------------------
**
*/

#include "about.h"
#include "filesystem.h"
#include "findfile.h"
#include "gameconfigfile.h"
#include "gstrings.h"
#include "i_interface.h"
#include "name.h"
#include "version.h"
#include "zstring.h"

#include <filesystem>
#include <fstream>
#include <rapidxml/rapidxml.hpp>
#include <regex>
#include <string>
#include <wx/richtext/richtextctrl.h>
#include <wx/utils.h>

constexpr unsigned NUMBER_OF_RELEASES_TO_DISPLAY = 3;

// Converts basic Markdown-ish text to HTML for wxHtmlWindow (really basic)
FString _SimpleMarkdownToHtml(FString input)
{
	std::string text = input.GetChars();

	text = std::regex_replace(text, std::regex(R"(^\s*#\s+(.*))"),
	                          "<h2>$1</h2>"); // we assume the first # in file is the big title
	text = std::regex_replace(text, std::regex(R"(\s*#\s+(.*))"), "<h3>$1</h3>"); // normal subtitle

	text = std::regex_replace(text, std::regex(R"(^-\s+(.*))"), "<li>$1</li>"); // turn - to bullet points

	text = std::regex_replace(text, std::regex(R"(\n)"), "<br>"); // turn newlines into html parseable br

	return FString(text.c_str());
}

FString _ParseReleaseNotes(rapidxml::xml_node<char> *release)
{
	// barebones html to plaintext parser

	if (!release)
		return GStrings.GetString("NOTES_FAIL"); // "Unable to parse release notes";

	auto description = release->first_node("description");
	auto version     = release->first_attribute("version");
	auto date        = release->first_attribute("date");
	auto url         = release->first_node("url");

	std::string descHtml;

	// simply take the already existing "html tags" inside patch notes
	std::function<void(rapidxml::xml_node<char> *)> reconstructHtml = [&](rapidxml::xml_node<char> *node) {
		for (auto child = node->first_node(); child; child = child->next_sibling())
		{
			if (child->type() == rapidxml::node_data || child->type() == rapidxml::node_cdata)
			{
				// Append text content directly
				descHtml += child->value();
			}
			else if (child->type() == rapidxml::node_element)
			{
				std::string tagName = child->name();

				// Take tags directly (e.g., <ul>, <li>, <b>, <p>)
				descHtml += "<" + tagName + ">";

				// Recursively process children
				reconstructHtml(child);

				// Close all the tags
				descHtml += "</" + tagName + ">";
			}
		}
	};

	// make sure there is even something to parse
	if (description)
	{
		reconstructHtml(description);
	}
	else
	{
		descHtml = GStrings.GetString("NOTES_EMPTY");
	}

	// get version and date strings
	std::string versionStr =
		version ? version->value() : wxString::FromUTF8(GStrings.GetString("NOTES_UNKNOWN")).ToStdString();
	std::string dateStr = date ? date->value() : wxString::FromUTF8(GStrings.GetString("NOTES_UNKNOWN")).ToStdString();
	std::string releasedStr = wxString::FromUTF8(GStrings.GetString("NOTES_RELEASED")).ToStdString();

	// build final output using std::string concatenation
	std::string finalOutput;
	finalOutput += "<h1>" GAMENAME " " + versionStr + " <small>(" + releasedStr + " " + dateStr + ")</small></h1>";

	// Body
	finalOutput += descHtml;

	// add url if present
	if (url)
	{
		std::string detailsStr = wxString::FromUTF8(GStrings.GetString("NOTES_DETAILS")).ToStdString();
		std::string urlStr     = url->value();

		finalOutput += "<p><a href=\"" + urlStr + "\">" + detailsStr + "</a></p>";
	}

	// Return converted into FString
	return FString(finalOutput.c_str());
}

FString _BuildReleaseNotes(rapidxml::xml_document<> &doc)
{
	// braindead html to plaintext parser

	// traverse to first (latest) release node
	auto release = doc.first_node("component");
	if (!release)
		return GStrings.GetString("NOTES_FAIL"); // "Unable to parse release notes";
	release = release->first_node("releases");
	if (!release)
		return GStrings.GetString("NOTES_FAIL"); // "Unable to parse release notes";
	release = release->first_node("release");

	FString text;

	for (unsigned i = 1;; i++)
	{
		release->type();
		text.AppendFormat("%s", _ParseReleaseNotes(release).GetChars());

		if (!release || i >= NUMBER_OF_RELEASES_TO_DISPLAY)
			break;

		text.AppendFormat("\n\n\n\n");
		release = release->next_sibling("release");
	}

	return text;
}

// Ensure you free returned pointer
char *_OpenReleaseNotes()
{
	auto wad = BaseFileSearch(BASEWAD, NULL, true, GameConfig);
	if (!wad)
		return nullptr;

	// we need to be free
	auto resf = FResourceFile::OpenResourceFile(wad);
	if (!resf)
		return nullptr;

	char *notes = nullptr;
	auto  lump  = resf->FindEntry("meta.xml"); // created from org.zdoom.GZDoom.metainfo.xml during build

	if (lump >= 0)
	{
		auto data = resf->Read(lump);

		// needs to be freed by caller
		notes = (char *)calloc(data.size() + 1, sizeof(char));

		if (notes)
			strncpy(notes, data.string(), data.size());
	}

	delete resf;

	return notes;
}

//==========================================================================
//
// Extract release notes from gzdoom.pk3
//
//==========================================================================
FString GetReleaseNotes()
{
	// we need to be free
	char *text = _OpenReleaseNotes();

	rapidxml::xml_document<> doc;
	if (text)
		doc.parse<rapidxml::parse_default>(text);
	FString content = _BuildReleaseNotes(doc);

	free(text);

	return content;
}

FString GetAboutText()
{
	auto wad = BaseFileSearch(BASEWAD, NULL, true, GameConfig);
	if (wad)
	{
		// we need to be free
		auto    resf = FResourceFile::OpenResourceFile(wad);
		FString text;

		auto append = [&resf, &text](const char *name) {
			auto lump = resf->FindEntry(name);
			if (lump < 0)
				return;
			auto data = resf->Read(lump);
			text.AppendCStrPart(data.string(), data.size());
		};

		int lump;
		if (resf)
		{
			append("about.txt");
			text.AppendCharacter('\n');
			append("contributors.txt");

			text.StripLeftRight();
		}

		delete resf;

		return _SimpleMarkdownToHtml(text);
	}
}

void About::ReleaseNotesDialog(wxWindow *parent, std::string lang)
{

	// update language strings
	GStrings.UpdateLanguage(lang.c_str());

	// create the window here since above is not a constructor
	this->Create(parent, wxID_ANY, wxString::FromUTF8(GStrings.GetString("LAUNCHER_TOPBAR_ABOUTNOTES")),
	             wxDefaultPosition, parent->FromDIP(wxSize(1000, 800)));

	// parse release notes xml from meta
	wxString patchNotes = wxString::FromUTF8(GetReleaseNotes().GetChars());

	// required to display rich text
	wxHtmlWindow *htmlWin = new wxHtmlWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO);

	// handle link clicks to open in default browser
	htmlWin->Bind(wxEVT_HTML_LINK_CLICKED, [](wxHtmlLinkEvent &event) {
		wxLaunchDefaultBrowser(event.GetLinkInfo().GetHref());
	});

	// The checkbox to show/hide patch notes on update
	wxCheckBox *showOnUpdateReq =
		new wxCheckBox(this, wxID_ANY, wxString::FromUTF8(GStrings.GetString("LAUNCHER_SHOW_ONUPDATED")));
	htmlWin->SetPage(patchNotes);
	showOnUpdateReq->SetValue(true); // Default to checked

	// Button to close the dialog
	wxButton *closeButton =
		new wxButton(this, wxID_OK, wxString::FromUTF8(GStrings.GetString("LAUNCHER_BUTTON_CLOSE")));

	// Layout using a vertical box sizer for proper arrangement
	wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
	vbox->Add(htmlWin, 1, wxEXPAND | wxALL, 10);
	vbox->Add(showOnUpdateReq, 0, wxALIGN_LEFT | wxLEFT | wxBOTTOM, 10);
	vbox->Add(closeButton, 0, wxALIGN_CENTER | wxALL, 10);

	SetSizer(vbox);
	Layout();

	Center(); // force everything to center
}

void About::CreditsDialog(wxWindow *parent, std::string lang)
{
	// update language strings
	GStrings.UpdateLanguage(lang.c_str());

	// create the window here since above is not a constructor
	this->Create(parent, wxID_ANY, wxString::FromUTF8(GStrings.GetString("LAUNCHER_TOPBAR_ABOUTCREDITS")),
	             wxDefaultPosition, parent->FromDIP(wxSize(1000, 800)));

	// required to display rich text
	wxHtmlWindow *htmlWin = new wxHtmlWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO);
	htmlWin->SetPage(wxString::FromUTF8(GetAboutText().GetChars()));

	// Button to close the dialog
	wxButton *closeButton =
		new wxButton(this, wxID_OK, wxString::FromUTF8(GStrings.GetString("LAUNCHER_BUTTON_CLOSE")));

	// Layout using a vertical box sizer for proper arrangement
	wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
	vbox->Add(htmlWin, 1, wxEXPAND | wxALL, 10);
	vbox->Add(closeButton, 0, wxALIGN_CENTER | wxALL, 10);

	SetSizer(vbox);
	Layout();

	Center(); // force everything to center
}
