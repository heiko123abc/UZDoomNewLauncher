/*
** about.cpp
**
** Create the UI to display the Credits and Patch Notes
**
**---------------------------------------------------------------------------
**
** Copyright 2025 Marcus Minhorst for _ParseReleaseNotes(), _BuildReleaseNotes(), _OpenReleaseNotes(), GetReleaseNotes()
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
	// braindead html to plaintext parser

	if (!release)
		return GStrings.GetString("NOTES_FAIL"); // "Unable to parse release notes";

	auto    description = release->first_node("description");
	auto    version     = release->first_attribute("version");
	auto    date        = release->first_attribute("date");
	auto    url         = release->first_node("url");
	FString text;

	auto append = [&text](FName type, char *value) {
		if (type == "p")
		{
			text.AppendFormat("%s\n\n", value);
		}
		else if (type == "li")
		{
			text.AppendFormat(" - %s\n", value);
		}
		else if (type == "hr")
		{
			text.AppendFormat("---\n");
		}
		else
		{
			text.AppendFormat("%s", value);
		}
	};

	auto node = description;
	while (node)
	{
		// Only append the value if it's a data/text node with content
		if (node->type() == rapidxml::node_data && node->value_size() > 0)
		{
			append(node->parent()->name(), node->value());
		}

		if (node->first_node())
		{
			node = node->first_node();
		}
		else if (node->next_sibling())
		{
			node = node->next_sibling();
		}
		else
		{
			while (node->parent() && node->parent() != description && !node->parent()->next_sibling())
				node = node->parent();

			node = (node->parent() != description) ? node->parent()->next_sibling() : nullptr;
		}
	}
	text.StripRight();

	return FStringf{GAMENAME " %s %s, %s %s\n\n%s\n%s",
	                GStrings.GetString("NOTES_VERSION"),                               // "version"
	                version ? version->value() : GStrings.GetString("NOTES_UNKNOWN"),  // "Unknown"
	                GStrings.GetString("NOTES_RELEASED"),                              // "released"
	                date ? date->value() : GStrings.GetString("NOTES_UNKNOWN"),        // "Unknown"
	                description ? text.GetChars() : GStrings.GetString("NOTES_EMPTY"), // "No description provided."
	                url ? FStringf("\n%s %s",
	                               GStrings.GetString("NOTES_DETAILS"), // "For more details see:"
	                               url->value())
	                          .GetChars()
	                    : ""};
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

		text.AppendFormat("\n\n---\n\n");
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

void About::ReleaseNotesDialog(wxWindow *parent)
{

	// create the window here since above is not a constructor
	this->Create(parent, wxID_ANY, "Release Notes", wxDefaultPosition, parent->FromDIP(wxSize(1000, 800)));

	// parse release notes xml from meta
	wxString patchNotes = wxString::FromUTF8(GetReleaseNotes().GetChars());

	// required to display rich text
	wxRichTextCtrl *richTextWin = new wxRichTextCtrl(this, wxID_ANY, patchNotes, wxDefaultPosition, wxDefaultSize,
	                                                 wxVSCROLL | wxHSCROLL | wxRE_READONLY);

	// The checkbox to show/hide patch notes on update
	wxCheckBox *showOnUpdateReq = new wxCheckBox(this, wxID_ANY, "Show these notes upon new update");
	showOnUpdateReq->SetValue(true); // Default to checked

	// Button to close the dialog
	wxButton *closeButton = new wxButton(this, wxID_OK, "Close");

	// Layout using a vertical box sizer for proper arrangement
	wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
	vbox->Add(richTextWin, 1, wxEXPAND | wxALL, 10);
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

	// required to display rich text
	wxHtmlWindow *htmlWin = new wxHtmlWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO);
	htmlWin->SetPage(wxString::FromUTF8(GetAboutText().GetChars()));

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
