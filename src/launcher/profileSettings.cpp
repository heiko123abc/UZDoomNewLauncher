/*
** profileSettings.cpp
**
** Contains UI code and logic for the profiles own Settings tab
**
**---------------------------------------------------------------------------
**
** Copyright 2025-2026 UZDoom Maintainers and Contributors
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
**---------------------------------------------------------------------------
**
*/

#include "profileSettings.h"
#include "gstrings.h"
#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>
#include <wx/clntdata.h>
#include <wx/combobox.h>
#include <wx/filepicker.h>
#include <wx/gbsizer.h>
#include <wx/listctrl.h>
#include <wx/notebook.h>
#include <wx/spinctrl.h>
#include <wx/wx.h>

#include <wx/utils.h>
#include <wx/valgen.h>
#include <wx/valnum.h>
#include <wx/valtext.h>

// bind translation string data from a wxComboBox and proper target variable
void LinkComboData(wxComboBox *combo, wxString *targetVar)
{
	if (!combo || !targetVar)
		return;

	for (unsigned int i = 0; i < combo->GetCount(); ++i)
	{
		wxStringClientData *data = dynamic_cast<wxStringClientData *>(combo->GetClientObject(i));

		if (data && data->GetData() == *targetVar)
		{
			combo->SetSelection(i);
			break;
		}
	}

	combo->Bind(wxEVT_COMBOBOX, [combo, targetVar](wxCommandEvent &) {
		int sel = combo->GetSelection();
		if (sel != wxNOT_FOUND)
		{
			wxStringClientData *data = dynamic_cast<wxStringClientData *>(combo->GetClientObject(sel));
			if (data)
			{
				*targetVar = data->GetData();
			}
		}
	});
}

void ShowFlagEditor(Profile *currEdit, wxWindow *parent, const wxString &title, std::vector<FlagInfo> &flags,
                    int *definedVars, int varCount, bool showForceCheck = false)
{
	wxDialog dlg(parent, wxID_ANY, title, wxDefaultPosition, parent->FromDIP(wxSize(500, 600)));
	dlg.SetExtraStyle(dlg.GetExtraStyle() | wxWS_EX_VALIDATE_RECURSIVELY);
	dlg.SetWindowStyle(wxDEFAULT_DIALOG_STYLE & ~(wxRESIZE_BORDER | wxMAXIMIZE_BOX));

	wxBoxSizer       *mainSizer = new wxBoxSizer(wxVERTICAL);
	wxScrolledWindow *scrollWin =
		new wxScrolledWindow(&dlg, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL | wxBORDER_SUNKEN);
	scrollWin->SetScrollRate(0, 10);
	wxBoxSizer *scrollSizer = new wxBoxSizer(wxVERTICAL);

	// Create Checkboxes
	for (auto &f : flags)
	{
		f.ctrl = new wxCheckBox(scrollWin, wxID_ANY, f.label);
		f.ctrl->SetToolTip(f.tooltip);
		scrollSizer->Add(f.ctrl, 0, wxALL, 5);
	}
	scrollWin->SetSizer(scrollSizer);
	mainSizer->Add(scrollWin, 1, wxEXPAND | wxALL, 10);

	// Create Text Boxes
	std::vector<wxTextCtrl *> textCtrls;
	wxFlexGridSizer          *grid = new wxFlexGridSizer(2, 5, 10);

	if (showForceCheck)
	{
		// Checkbox for DMFlags
		wxCheckBox *forceBox =
			new wxCheckBox(&dlg, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_GAMEPLAY_FORCE")));
		forceBox->SetValidator(wxGenericValidator(&currEdit->alwaysapplydmflags));
		mainSizer->Add(forceBox, 0, wxALIGN_CENTER | wxBOTTOM, 5);
		dlg.TransferDataToWindow();
	}
	else
	{
		// Warning text for Compat flags
		mainSizer->Add(new wxStaticText(&dlg, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_COMP_INFO"))), 0,
		               wxALL | wxALIGN_CENTER, 5);
	}

	for (int i = 0; i < varCount; i++)
	{
		// conditional textbox rendering
		if (title == wxString::FromUTF8(GStrings.GetString("PROFSET_GAMEPLAY_TITLE")))
			grid->Add(new wxStaticText(&dlg, wxID_ANY, wxString::Format("dmflags%d:", i + 1)), 0,
			          wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
		if (title == wxString::FromUTF8(GStrings.GetString("PROFSET_COMP_TITLE")))
			grid->Add(new wxStaticText(&dlg, wxID_ANY, wxString::Format("compatflags%d:", i + 1)), 0,
			          wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
		wxTextCtrl *tc = new wxTextCtrl(&dlg, wxID_ANY, "", wxDefaultPosition, parent->FromDIP(wxSize(100, -1)));
		textCtrls.push_back(tc);
		grid->Add(tc, 0, wxEXPAND);
	}

	mainSizer->Add(grid, 0, wxALIGN_CENTER | wxBOTTOM, 20);
	mainSizer->Add(new wxButton(&dlg, wxID_OK, wxString::FromUTF8(GStrings.GetString("LAUNCHER_BUTTON_CLOSE"))), 0,
	               wxALIGN_CENTER | wxBOTTOM, 15);

	auto UpdateUI = [&]() {
		std::vector<int> currentVals(varCount, 0);
		for (const auto &f : flags)
		{
			if (f.setIdx < varCount && (f.invert ? !f.ctrl->GetValue() : f.ctrl->GetValue()))
			{
				currentVals[f.setIdx] += f.bitVal;
			}
		}
		for (int i = 0; i < varCount; i++)
			textCtrls[i]->ChangeValue(wxString::Format("%d", currentVals[i])); // Use ChangeValue to avoid loop
	};

	auto UpdateBoxes = [&]() {
		std::vector<int> vals;
		for (auto *tc : textCtrls)
		{
			long v = 0;
			tc->GetValue().ToLong(&v);
			vals.push_back((int)v);
		}
		for (const auto &f : flags)
		{
			if (f.setIdx < vals.size())
			{
				bool bitSet = (vals[f.setIdx] & f.bitVal) == f.bitVal;
				f.ctrl->SetValue(f.invert ? !bitSet : bitSet);
			}
		}
	};

	// Bindings
	for (auto &f : flags)
		f.ctrl->Bind(wxEVT_CHECKBOX, [&](wxCommandEvent &) { UpdateUI(); });
	for (auto *tc : textCtrls)
		tc->Bind(wxEVT_TEXT, [&](wxCommandEvent &) { UpdateBoxes(); });

	// Initial Load
	for (int i = 0; i < varCount; i++)
		textCtrls[i]->ChangeValue(wxString::Format("%d", definedVars[i]));
	UpdateBoxes();

	dlg.SetSizer(mainSizer);
	dlg.Layout();

	if (dlg.ShowModal() == wxID_OK)
	{
		dlg.TransferDataFromWindow(); // Save the bool validator if it exists
		for (int i = 0; i < varCount; i++)
		{
			long v = 0;
			textCtrls[i]->GetValue().ToLong(&v);
			definedVars[i] = (int)v;
		}
	}
}

void advGameplay(Profile *currEdit, wxWindow *parent)
{

	// Pass the 3 variables by reference in an array
	int vars[] = {currEdit->DMFlags, currEdit->DMFlags2, currEdit->DMFlags3};

	std::vector<FlagInfo> flags = getDMFlagsList();
	ShowFlagEditor(currEdit, parent, wxString::FromUTF8(GStrings.GetString("PROFSET_GAMEPLAY_TITLE")), flags, vars, 3,
	               true);

	// Save back results
	currEdit->DMFlags  = vars[0];
	currEdit->DMFlags2 = vars[1];
	currEdit->DMFlags3 = vars[2];
}

void advCompat(Profile *currEdit, wxWindow *parent)
{

	int vars[] = {currEdit->compatflags, currEdit->compatflags2};

	std::vector<FlagInfo> flags = getCompatFlagsList();
	ShowFlagEditor(currEdit, parent, wxString::FromUTF8(GStrings.GetString("PROFSET_COMP_TITLE")), flags, vars, 2,
	               false);

	currEdit->compatflags  = vars[0];
	currEdit->compatflags2 = vars[1];
}

// a helper to avoid duplication
void OpenPathPicker(wxWindow *parent, wxTextCtrl *targetInput, const wxString &title, bool isFolder,
                    const wxString &filter = wxString::FromUTF8(GStrings.GetString("FILETYPE_ALL")))
{
	if (isFolder)
	{
		// a folder path is needed
		wxDirDialog dirDialog(parent, title, wxGetCwd() + ROOT_DIR.ToUTF8(), wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
		if (dirDialog.ShowModal() == wxID_OK)
		{
			targetInput->SetValue(dirDialog.GetPath());
		}
	}
	else
	{
		// a file path is needed
		wxFileDialog fileDialog(parent, title, wxGetCwd() + ROOT_DIR.ToUTF8(), "", filter,
		                        wxFD_OPEN | wxFD_FILE_MUST_EXIST);
		if (fileDialog.ShowModal() == wxID_OK)
		{
			targetInput->SetValue(fileDialog.GetPath());
		}
	}
}

void RefreshModList(wxWindow *parent, Profile *currEdit, wxScrolledWindow *listWindow, wxBoxSizer *listSizer)
{

	listSizer->Clear(true); // force a redraw

	// Rebuild the list from the vector
	for (size_t i = 0; i < currEdit->modFiles.size(); ++i)
	{
		wxString item = currEdit->modFiles[i];

		// Create the row panel
		wxPanel *row = new wxPanel(listWindow);
		row->SetBackgroundColour(wxColour(220, 240, 255)); // Light blue for visibility
		wxBoxSizer *rowSizer = new wxBoxSizer(wxHORIZONTAL);

		// Mod Name (Just show file name, the path is way maybe too long)
		std::string   fname = std::filesystem::path(item.ToStdString()).filename().string();
		wxStaticText *label = new wxStaticText(row, wxID_ANY, fname);

		// Tooltip showing full path
		label->SetToolTip(item);
		rowSizer->Add(label, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);

		// UP Button
		wxButton *btnUp =
			new wxButton(row, wxID_ANY, wxString::FromUTF8("▲"), wxDefaultPosition, parent->FromDIP(wxSize(35, 30)));
		btnUp->Enable(i > 0);
		btnUp->Bind(wxEVT_BUTTON, [=](wxCommandEvent &) {
			if (i > 0)
			{
				std::swap(currEdit->modFiles[i], currEdit->modFiles[i - 1]);
				RefreshModList(parent, currEdit, listWindow, listSizer);
			}
		});
		rowSizer->Add(btnUp, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 2);

		// DOWN Button
		wxButton *btnDown =
			new wxButton(row, wxID_ANY, wxString::FromUTF8("▼"), wxDefaultPosition, parent->FromDIP(wxSize(35, 30)));
		btnDown->Enable(i < currEdit->modFiles.size() - 1);
		btnDown->Bind(wxEVT_BUTTON, [=](wxCommandEvent &) {
			if (i < currEdit->modFiles.size() - 1)
			{
				std::swap(currEdit->modFiles[i], currEdit->modFiles[i + 1]);
				RefreshModList(parent, currEdit, listWindow, listSizer);
			}
		});
		rowSizer->Add(btnDown, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 2);

		// DELETE Button
		wxButton *btnDel =
			new wxButton(row, wxID_ANY, wxString::FromUTF8("X"), wxDefaultPosition, parent->FromDIP(wxSize(35, 30)));
		btnDel->SetForegroundColour(*wxRED);
		btnDel->Bind(wxEVT_BUTTON, [=](wxCommandEvent &) {
			std::filesystem::remove(currEdit->modFiles[i]);           // Wipe the mod file itsself first
			currEdit->modFiles.erase(currEdit->modFiles.begin() + i); // Remove from vector
			RefreshModList(parent, currEdit, listWindow, listSizer);
		});
		rowSizer->Add(btnDel, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 2);

		row->SetSizer(rowSizer);
		listSizer->Add(row, 0, wxEXPAND | wxALL, 1);
	}

	listWindow->Layout();
	listWindow->FitInside(); // Crucial for scrolling to update
}

void CreateAdvancedTab(Profile *currEdit, wxPanel *panel)
{
	wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

	/*
	// prepend parameters field
	wxStaticBoxSizer *pparamGroup =
	    new wxStaticBoxSizer(wxVERTICAL, panel, wxString::FromUTF8(GStrings.GetString("PROFSET_ADVANCED_PREPEND")));
	wxTextCtrl *pparams =
	    new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, panel->FromDIP(wxSize(-1, 100)), wxTE_MULTILINE,
	                   wxTextValidator(wxFILTER_NONE, &currEdit->prependAdditionalParameters));
	pparamGroup->Add(pparams, 1, wxEXPAND);

	//mainSizer->Add(pparamGroup, 1, wxEXPAND | wxALL, 10);
	*/

	// append parameters field
	wxStaticBoxSizer *aparamGroup =
		new wxStaticBoxSizer(wxVERTICAL, panel, wxString::FromUTF8(GStrings.GetString("PROFSET_ADVANCED_APPEND")));
	wxTextCtrl *aparams =
		new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, panel->FromDIP(wxSize(-1, 100)), wxTE_MULTILINE,
	                   wxTextValidator(wxFILTER_NONE, &currEdit->appendAdditionalParameters));
	aparamGroup->Add(aparams, 1, wxEXPAND);

	mainSizer->Add(aparamGroup, 1, wxEXPAND | wxALL, 10);

	panel->SetSizer(mainSizer);
}

void CreateOutputTab(Profile *currEdit, wxPanel *panel)
{
	// capture the radio button value manually and set values (beause wxValidator with radio buttons is not suitable)
	auto setupRadio = [currEdit](wxRadioButton *rb, int index) {
		if (currEdit->renderingBackend == index)
			rb->SetValue(true);
		rb->Bind(wxEVT_RADIOBUTTON, [currEdit, index](wxCommandEvent &) { currEdit->renderingBackend = index; });
	};

	wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

	// General Settings
	wxStaticBoxSizer *generalGroup =
		new wxStaticBoxSizer(wxVERTICAL, panel, wxString::FromUTF8(GStrings.GetString("PROFSET_OUTPUT_GENERAL")));
	wxBoxSizer *genRow = new wxBoxSizer(wxHORIZONTAL);
	genRow->Add(new wxCheckBox(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_OUTPUT_FULLSCREEN")),
	                           wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator(&currEdit->enableFullscreen)),
	            0, wxRIGHT, 15);
	genRow->Add(new wxCheckBox(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_OUTPUT_SUPPORTWAD")),
	                           wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator(&currEdit->enableSupportWAD)),
	            0, wxRIGHT, 15);
	genRow->Add(new wxCheckBox(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_OUTPUT_DISAUTO")),
	                           wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator(&currEdit->disableAutoload)),
	            0);
	generalGroup->Add(genRow, 0, wxALL, 5);
	mainSizer->Add(generalGroup, 0, wxEXPAND | wxALL, 10);

	// Rendering API Group

	wxStaticBoxSizer *renderGroup =
		new wxStaticBoxSizer(wxVERTICAL, panel, wxString::FromUTF8(GStrings.GetString("PROFSET_OUTPUT_RENDER")));
	wxBoxSizer *renderRow = new wxBoxSizer(wxHORIZONTAL);

	wxRadioButton *vk =
		new wxRadioButton(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_OUTPUT_VULKAN")),
	                      wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	wxRadioButton *gl =
		new wxRadioButton(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_OUTPUT_OPENGL")),
	                      wxDefaultPosition, wxDefaultSize);
	wxRadioButton *gles =
		new wxRadioButton(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_OUTPUT_GLES")),
	                      wxDefaultPosition, wxDefaultSize);

	renderRow->Add(vk, 0, wxRIGHT, 15);
	renderRow->Add(gl, 0, wxRIGHT, 15);
	renderRow->Add(gles, 0);

	// setup bindings
	setupRadio(vk, 0);
	setupRadio(gl, 1);
	setupRadio(gles, 2);

	renderGroup->Add(renderRow, 0, wxALL, 5);
	mainSizer->Add(renderGroup, 0, wxEXPAND | wxALL, 10);

	// Extra Graphics
	wxStaticBoxSizer *graphicsGroup =
		new wxStaticBoxSizer(wxVERTICAL, panel, wxString::FromUTF8(GStrings.GetString("PROFSET_OUTPUT_EXTRA")));
	wxBoxSizer *graphRow = new wxBoxSizer(wxHORIZONTAL);
	graphRow->Add(new wxCheckBox(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_OUTPUT_LIGHTS")),
	                             wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator(&currEdit->enableLights)),
	              0, wxRIGHT, 15);
	graphRow->Add(new wxCheckBox(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_OUTPUT_BRIGHT")),
	                             wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator(&currEdit->enableBrightmaps)),
	              0, wxRIGHT, 15);
	graphRow->Add(new wxCheckBox(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_OUTPUT_WIDE")),
	                             wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator(&currEdit->enableWidescreen)),
	              0);
	graphicsGroup->Add(graphRow, 0, wxALL, 5);
	mainSizer->Add(graphicsGroup, 0, wxEXPAND | wxALL, 10);

	panel->SetSizer(mainSizer);
}

void CreateFilesTab(Profile *currEdit, wxPanel *panel)
{
	wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

	// File Selection Area
	wxFlexGridSizer *grid = new wxFlexGridSizer(4, 2, 10, 10);
	grid->AddGrowableCol(1);

	// Config File Row
	grid->Add(new wxStaticText(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_FILES_CONFIG"))), 0,
	          wxALIGN_CENTER_VERTICAL);
	wxBoxSizer *configBox = new wxBoxSizer(wxHORIZONTAL);

	wxTextCtrl *configTxt = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0,
	                                       wxTextValidator(wxFILTER_NONE, &currEdit->configFilePath));
	configTxt->SetInsertionPointEnd();

	wxButton *configButton =
		new wxButton(panel, wxID_ANY, "...", wxDefaultPosition,
	                 panel->FromDIP(wxSize(configTxt->GetBestSize().y * 1.2, configTxt->GetBestSize().y)));
	// bind it
	configButton->Bind(wxEVT_BUTTON, [panel, configTxt](wxCommandEvent &) {
		OpenPathPicker(panel, configTxt, wxString::FromUTF8(GStrings.GetString("PROFSET_FILES_CONFIGDIAG")), false,
		               wxString::FromUTF8(GStrings.GetString("FILETYPE_INI")));
	});
	configBox->Add(configTxt, 1, wxEXPAND | wxRIGHT, 5);
	configBox->Add(configButton, 0);
	grid->Add(configBox, 1, wxEXPAND);

	// Save Directory Row
	grid->Add(new wxStaticText(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_FILES_SAVE"))), 0,
	          wxALIGN_CENTER_VERTICAL);
	wxBoxSizer *saveBox = new wxBoxSizer(wxHORIZONTAL);

	wxTextCtrl *saveTxt = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0,
	                                     wxTextValidator(wxFILTER_NONE, &currEdit->saveDirPath));
	saveTxt->SetInsertionPointEnd();

	wxButton *saveDirButton =
		new wxButton(panel, wxID_ANY, "...", wxDefaultPosition,
	                 panel->FromDIP(wxSize(saveTxt->GetBestSize().y * 1.2, saveTxt->GetBestSize().y)));
	// bind it
	saveDirButton->Bind(wxEVT_BUTTON, [panel, saveTxt](wxCommandEvent &) {
		OpenPathPicker(panel, saveTxt, wxString::FromUTF8(GStrings.GetString("PROFSET_FILES_SAVEDIAG")), true);
	});
	saveBox->Add(saveTxt, 1, wxEXPAND | wxRIGHT, 5);
	saveBox->Add(saveDirButton, 0);
	grid->Add(saveBox, 1, wxEXPAND);

	// Screenshot Dir Row
	grid->Add(new wxStaticText(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_FILES_SCREEN"))), 0,
	          wxALIGN_CENTER_VERTICAL);
	wxBoxSizer *shotBox = new wxBoxSizer(wxHORIZONTAL);

	wxTextCtrl *shotTxt = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0,
	                                     wxTextValidator(wxFILTER_NONE, &currEdit->screenshotDirPath));
	shotTxt->SetInsertionPointEnd();

	wxButton *scrnDirButton =
		new wxButton(panel, wxID_ANY, "...", wxDefaultPosition,
	                 panel->FromDIP(wxSize(shotTxt->GetBestSize().y * 1.2, shotTxt->GetBestSize().y)));
	// bind it
	scrnDirButton->Bind(wxEVT_BUTTON, [panel, shotTxt](wxCommandEvent &) {
		OpenPathPicker(panel, shotTxt, wxString::FromUTF8(GStrings.GetString("PROFSET_FILES_SCREENDIAG")), true);
	});
	shotBox->Add(shotTxt, 1, wxEXPAND | wxRIGHT, 5);
	shotBox->Add(scrnDirButton, 0);
	grid->Add(shotBox, 1, wxEXPAND);

	// Demo Directory Row
	grid->Add(new wxStaticText(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_FILES_DEMO"))), 0,
	          wxALIGN_CENTER_VERTICAL);
	wxBoxSizer *demoBox = new wxBoxSizer(wxHORIZONTAL);

	wxTextCtrl *demoTxt = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0,
	                                     wxTextValidator(wxFILTER_NONE, &currEdit->demoDirPath));
	demoTxt->SetInsertionPointEnd();

	wxButton *demoDirButton =
		new wxButton(panel, wxID_ANY, "...", wxDefaultPosition,
	                 panel->FromDIP(wxSize(demoTxt->GetBestSize().y * 1.2, demoTxt->GetBestSize().y)));
	// bind it
	demoDirButton->Bind(wxEVT_BUTTON, [panel, demoTxt](wxCommandEvent &) {
		OpenPathPicker(panel, demoTxt, wxString::FromUTF8(GStrings.GetString("PROFSET_FILES_DEMODIAG")), true);
	});
	demoBox->Add(demoTxt, 1, wxEXPAND | wxRIGHT, 5);
	demoBox->Add(demoDirButton, 0);
	grid->Add(demoBox, 1, wxEXPAND);

	mainSizer->Add(grid, 0, wxEXPAND | wxALL, 15);

	// Mods Header
	wxBoxSizer *modHeader = new wxBoxSizer(wxHORIZONTAL);
	modHeader->Add(new wxStaticText(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_FILES_MODS"))), 1,
	               wxALIGN_CENTER_VERTICAL);

	wxButton *addModButton =
		new wxButton(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_FILES_ADDMOD")));
	modHeader->Add(addModButton, 0);
	mainSizer->Add(modHeader, 0, wxEXPAND | wxLEFT | wxRIGHT, 15);

	// Mods List
	wxScrolledWindow *modList =
		new wxScrolledWindow(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SUNKEN);
	modList->SetScrollRate(0, 10);

	wxBoxSizer *modListSizer = new wxBoxSizer(wxVERTICAL);
	modList->SetSizer(modListSizer);

	addModButton->Bind(wxEVT_BUTTON, [currEdit, panel, modList, modListSizer](wxCommandEvent &) {
		wxFileDialog openFileDialog(panel, "Select Mod Files", wxGetCwd(), "",
		                            wxString::FromUTF8(GStrings.GetString("FILETYPE_MOD")),
		                            wxFD_OPEN | wxFD_FILE_MUST_EXIST |
		                                wxFD_MULTIPLE); // this is a seperate case to allow for multiple selections

		if (openFileDialog.ShowModal() == wxID_CANCEL)
			return; // user clicked off -> stop

		wxArrayString paths;
		openFileDialog.GetPaths(paths);

		// Copy all selected mods over into mod folder and add to list
		for (const wxString &path : paths)
		{
			wxFileName fileName(path);
			wxCopyFile(path, currEdit->modsDirPath + "/" + fileName.GetFullName());
			currEdit->modFiles.push_back(currEdit->modsDirPath.ToStdString() + "/" +
			                             fileName.GetFullName().ToStdString());
		}

		// Force Refresh the UI
		RefreshModList(panel, currEdit, modList, modListSizer);
	});

	RefreshModList(panel, currEdit, modList, modListSizer); // redraw the ui when user comes over

	mainSizer->Add(modList, 1, wxEXPAND | wxALL, 15);

	panel->SetSizer(mainSizer);
}

void CreateLaunchTab(Profile *currEdit, wxPanel *panel)
{
	wxBoxSizer *mainSizer = new wxBoxSizer(wxHORIZONTAL);

	// left side
	wxBoxSizer *leftCol = new wxBoxSizer(wxVERTICAL);

	wxStaticBoxSizer *launchModeGroup =
		new wxStaticBoxSizer(wxVERTICAL, panel, wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_MODE")));

	// capture the radio button value manually and set values (beause wxValidator with radio buttons is not suitable)
	auto setupRadio = [currEdit](wxRadioButton *rb, int index) {
		if (currEdit->launchParameters == index)
			rb->SetValue(true);
		rb->Bind(wxEVT_RADIOBUTTON, [currEdit, index](wxCommandEvent &) { currEdit->launchParameters = index; });
	};

	// Normal
	{
		wxBoxSizer    *row = new wxBoxSizer(wxHORIZONTAL);
		wxRadioButton *rb =
			new wxRadioButton(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_NORMAL")),
		                      wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
		rb->SetValue(true);
		row->Add(rb, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
		row->AddStretchSpacer();
		launchModeGroup->Add(row, 0, wxEXPAND | wxALL, 3);

		setupRadio(rb, 0);
	}

	// Map
	{
		wxBoxSizer    *row = new wxBoxSizer(wxHORIZONTAL);
		wxRadioButton *rb =
			new wxRadioButton(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_MAP")));
		row->Add(rb, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
		row->AddStretchSpacer();
		wxSpinCtrl *mapSpinner = new wxSpinCtrl(panel, wxID_ANY, "1", wxDefaultPosition, wxDefaultSize, 0, 1, 100000);
		mapSpinner->SetValidator(wxGenericValidator(&currEdit->selectedLaunchMap));
		row->Add(mapSpinner, 0, wxALIGN_CENTER_VERTICAL);
		launchModeGroup->Add(row, 0, wxEXPAND | wxALL, 3);

		setupRadio(rb, 1);
	}

	// Savegame
	{
		wxBoxSizer    *row = new wxBoxSizer(wxHORIZONTAL);
		wxRadioButton *rb =
			new wxRadioButton(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_SAVE")));
		row->Add(rb, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
		row->AddStretchSpacer();
		wxTextCtrl *loadSavePath = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0,
		                                          wxTextValidator(wxFILTER_NONE, &currEdit->selectedLaunchSave));
		wxButton   *loadSavePathButton =
			new wxButton(panel, wxID_ANY, "...", wxDefaultPosition,
		                 panel->FromDIP(wxSize(loadSavePath->GetBestSize().y * 1.2, loadSavePath->GetBestSize().y)));
		// bind it
		loadSavePathButton->Bind(wxEVT_BUTTON, [panel, loadSavePath](wxCommandEvent &) {
			OpenPathPicker(panel, loadSavePath, wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_SAVEDIAG")),
			               false, wxString::FromUTF8(GStrings.GetString("FILETYPE_ZDS")));
		});
		row->Add(loadSavePath, 0, wxALIGN_CENTER_VERTICAL);
		row->Add(loadSavePathButton, 0);
		launchModeGroup->Add(row, 0, wxEXPAND | wxALL, 3);

		setupRadio(rb, 2);
	}

	// Demo Playback
	{
		wxBoxSizer    *row = new wxBoxSizer(wxHORIZONTAL);
		wxRadioButton *rb =
			new wxRadioButton(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_PLAYDEM")));
		row->Add(rb, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
		row->AddStretchSpacer();
		wxTextCtrl *playDemPath = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0,
		                                         wxTextValidator(wxFILTER_NONE, &currEdit->selectedLaunchDemoPlayback));
		wxButton   *playDemPathButton =
			new wxButton(panel, wxID_ANY, "...", wxDefaultPosition,
		                 panel->FromDIP(wxSize(playDemPath->GetBestSize().y * 1.2, playDemPath->GetBestSize().y)));
		// bind it
		playDemPathButton->Bind(wxEVT_BUTTON, [panel, playDemPath](wxCommandEvent &) {
			OpenPathPicker(panel, playDemPath, wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_DEMODIAG")), false,
			               wxString::FromUTF8(GStrings.GetString("FILETYPE_LMP")));
		});
		row->Add(playDemPath, 0, wxALIGN_CENTER_VERTICAL);
		row->Add(playDemPathButton, 0);
		launchModeGroup->Add(row, 0, wxEXPAND | wxALL, 3);

		setupRadio(rb, 3);
	}

	// Demo Record
	{
		wxBoxSizer    *row = new wxBoxSizer(wxHORIZONTAL);
		wxRadioButton *rb =
			new wxRadioButton(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_RECORD")));
		row->Add(rb, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
		row->AddStretchSpacer();
		wxTextCtrl *recDemPath = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0,
		                                        wxTextValidator(wxFILTER_NONE, &currEdit->selectedLaunchDemoRecord));
		wxButton   *recDemPathButton =
			new wxButton(panel, wxID_ANY, "...", wxDefaultPosition,
		                 panel->FromDIP(wxSize(recDemPath->GetBestSize().y * 1.2, recDemPath->GetBestSize().y)));
		// bind it
		recDemPathButton->Bind(wxEVT_BUTTON, [panel, recDemPath](wxCommandEvent &) {
			OpenPathPicker(panel, recDemPath, wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_DEMODIAG")), false,
			               wxString::FromUTF8(GStrings.GetString("FILETYPE_LMP")));
		});
		row->Add(recDemPath, 0, wxALIGN_CENTER_VERTICAL);
		row->Add(recDemPathButton, 0);
		launchModeGroup->Add(row, 0, wxEXPAND | wxALL, 3);

		setupRadio(rb, 4);
	}

	leftCol->Add(launchModeGroup, 0, wxEXPAND | wxALL, 5);

	// Difficulty settings
	wxStaticBoxSizer *gameplayGroup =
		new wxStaticBoxSizer(wxVERTICAL, panel, wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_GAMEPLAY")));

	// Skill select
	wxBoxSizer *skillRow = new wxBoxSizer(wxHORIZONTAL);
	skillRow->Add(new wxStaticText(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_SKILL"))), 1,
	              wxALIGN_CENTER_VERTICAL);

	wxComboBox *skillCombo = new wxComboBox(
		panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_SKILL3")), wxDefaultPosition,
		wxDefaultSize, 0, NULL, 0, wxGenericValidator(&currEdit->difficultySkillRating));
	skillCombo->Append(wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_SKILL1")));
	skillCombo->Append(wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_SKILL2")));
	skillCombo->Append(wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_SKILL3")));
	skillCombo->Append(wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_SKILL4")));
	skillCombo->Append(wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_SKILL5")));
	skillCombo->SetEditable(false);

	skillRow->Add(skillCombo, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
	gameplayGroup->Add(skillRow, 0, wxEXPAND | wxALL, 5);

	gameplayGroup->AddSpacer(5);

	// Checkboxes for modifiers
	long chkFlags = wxLEFT | wxBOTTOM; // add space between checkboxes for better look
	gameplayGroup->Add(
		new wxCheckBox(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_FMONSTER")),
	                   wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator(&currEdit->difficultyFastMonsters)),
		0, chkFlags, 5);
	gameplayGroup->Add(
		new wxCheckBox(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_RMONSTER")),
	                   wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator(&currEdit->difficultyRespawnMonsters)),
		0, chkFlags, 5);
	gameplayGroup->Add(
		new wxCheckBox(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_NMONSTER")),
	                   wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator(&currEdit->difficultyNoMonsters)),
		0, chkFlags, 5);

	wxButton *advGameplayButton =
		new wxButton(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_GAMEPLAYMORE")));
	gameplayGroup->Add(advGameplayButton, 0, wxALIGN_LEFT | wxEXPAND); // botton for more options
	// bind it
	advGameplayButton->Bind(wxEVT_BUTTON, [currEdit, panel](wxCommandEvent &) { advGameplay(currEdit, panel); });
	leftCol->Add(gameplayGroup, 0, wxEXPAND | wxALL, 5);

	// miscellaneous
	wxStaticBoxSizer *miscGroup =
		new wxStaticBoxSizer(wxVERTICAL, panel, wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_MISC")));
	wxFlexGridSizer *miscGrid = new wxFlexGridSizer(0, 2, 5, 5);
	miscGrid->AddGrowableCol(1);

	miscGrid->Add(new wxStaticText(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_NAME"))), 0,
	              wxALIGN_CENTER_VERTICAL);
	miscGrid->Add(new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0,
	                             wxTextValidator(wxFILTER_NONE, &currEdit->playerName)),
	              0, wxEXPAND);
	miscGrid->Add(new wxStaticText(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_CLASS"))), 0,
	              wxALIGN_CENTER_VERTICAL);
	miscGrid->Add(new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0,
	                             wxTextValidator(wxFILTER_NONE, &currEdit->playerClass)),
	              0, wxEXPAND);
	miscGrid->Add(new wxStaticText(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_GENDER"))), 0,
	              wxALIGN_CENTER_VERTICAL);
	wxComboBox *genderCombo =
		new wxComboBox(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_GENDER0")),
	                   wxDefaultPosition, wxDefaultSize, 0, NULL, 0);
	genderCombo->Append(wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_GENDER0")),
	                    new wxStringClientData("Male"));
	genderCombo->Append(wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_GENDER1")),
	                    new wxStringClientData("Female"));
	genderCombo->Append(wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_GENDER2")),
	                    new wxStringClientData("Neutral"));
	genderCombo->Append(wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_GENDER3")),
	                    new wxStringClientData("Object"));

	LinkComboData(genderCombo, &currEdit->playerGender);
	genderCombo->SetEditable(false);
	miscGrid->Add(genderCombo, 0, wxEXPAND);

	miscGrid->Add(new wxStaticText(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_LANG"))), 0,
	              wxALIGN_CENTER_VERTICAL | wxTOP, panel->FromDIP(15));
	wxComboBox *langCombo = new wxComboBox(panel, wxID_ANY, "enu", wxDefaultPosition, wxDefaultSize, 0, NULL, 0);

	langCombo->Append(wxString::FromUTF8("enu - English (US)"), new wxStringClientData("enu"));
	langCombo->Append(wxString::FromUTF8("eng - English (UK)"), new wxStringClientData("eng"));
	langCombo->Append(wxString::FromUTF8("cs - Česky (Czech)"), new wxStringClientData("cs"));
	langCombo->Append(wxString::FromUTF8("da - Dansk (Danish)"), new wxStringClientData("da"));
	langCombo->Append(wxString::FromUTF8("de - Deutsch (German)"), new wxStringClientData("de"));
	langCombo->Append(wxString::FromUTF8("es - Español (España) (Castilian Spanish)"), new wxStringClientData("es"));
	langCombo->Append(wxString::FromUTF8("esm - Español (Latino) (Latin American Spanish)"),
	                  new wxStringClientData("esm"));
	langCombo->Append(wxString::FromUTF8("eo - Esperanto"), new wxStringClientData("eo"));
	langCombo->Append(wxString::FromUTF8("fi - Suomi (Finnish)"), new wxStringClientData("fi"));
	langCombo->Append(wxString::FromUTF8("fr - Français (French)"), new wxStringClientData("fr"));
	langCombo->Append(wxString::FromUTF8("hu - Magyar (Hungarian)"), new wxStringClientData("hu"));
	langCombo->Append(wxString::FromUTF8("it - Italiano (Italian)"), new wxStringClientData("it"));
	langCombo->Append(wxString::FromUTF8("jp - 日本語 (Japanese)"), new wxStringClientData("jp"));
	langCombo->Append(wxString::FromUTF8("ko - 한국어 (Korean)"), new wxStringClientData("ko"));
	langCombo->Append(wxString::FromUTF8("nl - Nederlands (Dutch)"), new wxStringClientData("nl"));
	langCombo->Append(wxString::FromUTF8("nb - Norsk Bokmål (Norwegian)"), new wxStringClientData("nb"));
	langCombo->Append(wxString::FromUTF8("pl - Polski (Polish)"), new wxStringClientData("pl"));
	langCombo->Append(wxString::FromUTF8("ptg - Português (European Portuguese)"), new wxStringClientData("ptg"));
	langCombo->Append(wxString::FromUTF8("pt - Português do Brasil (Brazilian Portuguese)"),
	                  new wxStringClientData("pt"));
	langCombo->Append(wxString::FromUTF8("ro - Română (Romanian)"), new wxStringClientData("ro"));
	langCombo->Append(wxString::FromUTF8("ru - Русский (Russian)"), new wxStringClientData("ru"));
	langCombo->Append(wxString::FromUTF8("sr - Српски (Serbian)"), new wxStringClientData("sr"));
	langCombo->Append(wxString::FromUTF8("tr - Türkçe (Turkish)"), new wxStringClientData("tr"));

	LinkComboData(langCombo, &currEdit->wadLanguage);

	langCombo->SetEditable(false);
	miscGrid->Add(langCombo, 0, wxEXPAND | wxALIGN_CENTER_VERTICAL | wxTOP, panel->FromDIP(15));

	miscGroup->Add(miscGrid, 1, wxEXPAND | wxALL, 5);
	leftCol->Add(miscGroup, 0, wxEXPAND | wxALL, 5);

	// right side
	wxBoxSizer *rightCol = new wxBoxSizer(wxVERTICAL);

	// Remote Multiplayer
	wxStaticBoxSizer *remoteGroup =
		new wxStaticBoxSizer(wxVERTICAL, panel, wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_REMOTE")));
	wxFlexGridSizer *remoteGrid = new wxFlexGridSizer(3, 2, 5, 5);
	remoteGrid->AddGrowableCol(1);

	remoteGrid->Add(new wxStaticText(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_REMADDR"))),
	                0, wxALIGN_CENTER_VERTICAL);
	remoteGrid->Add(new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0,
	                               wxTextValidator(wxFILTER_NONE, &currEdit->joinAddress)),
	                0, wxEXPAND);
	remoteGrid->Add(new wxStaticText(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_REMPORT"))),
	                0, wxALIGN_CENTER_VERTICAL);
	remoteGrid->Add(new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0,
	                               wxTextValidator(wxFILTER_NONE, &currEdit->joinPort)),
	                0, wxEXPAND);
	remoteGrid->Add(new wxStaticText(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_TEAMNO"))),
	                0, wxALIGN_CENTER_VERTICAL);
	remoteGrid->Add(new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0,
	                               wxTextValidator(wxFILTER_NONE, &currEdit->joinTeamNo)),
	                0, wxEXPAND);

	remoteGroup->Add(remoteGrid, 1, wxEXPAND | wxALL, 5);
	rightCol->Add(remoteGroup, 0, wxEXPAND | wxALL, 5);

	// Host Multiplayer
	wxStaticBoxSizer *hostGroup =
		new wxStaticBoxSizer(wxVERTICAL, panel, wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_HOSTMP")));
	wxFlexGridSizer *hostGrid = new wxFlexGridSizer(5, 2, 5, 5);
	hostGrid->AddGrowableCol(1);

	hostGrid->Add(new wxStaticText(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_HOPORT"))), 0,
	              wxALIGN_CENTER_VERTICAL);
	hostGrid->Add(new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0,
	                             wxTextValidator(wxFILTER_NONE, &currEdit->hostPort)),
	              0, wxEXPAND);

	hostGrid->Add(new wxStaticText(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_HOMAXP"))), 0,
	              wxALIGN_CENTER_VERTICAL);
	wxSpinCtrl *playerCounterSpin = new wxSpinCtrl(panel, wxID_ANY, "8", wxDefaultPosition, wxDefaultSize, 0, 1, 64);
	playerCounterSpin->SetValidator(wxGenericValidator(&currEdit->hostMaxPlayers));
	hostGrid->Add(playerCounterSpin, 0, wxEXPAND);

	hostGrid->Add(new wxStaticText(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_HOTICK"))), 0,
	              wxALIGN_CENTER_VERTICAL);

	wxComboBox *TickRateCombo =
		new wxComboBox(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_TICK25")),
	                   wxDefaultPosition, wxDefaultSize, 0, NULL, 0);

	TickRateCombo->Append(wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_TICK25")),
	                      new wxStringClientData("25Hz"));
	TickRateCombo->Append(wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_TICK175")),
	                      new wxStringClientData("17.5Hz"));
	TickRateCombo->Append(wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_TICK116")),
	                      new wxStringClientData("11.6Hz"));

	LinkComboData(TickRateCombo, &currEdit->hostTickRate);

	TickRateCombo->SetEditable(false);
	hostGrid->Add(TickRateCombo, 0, wxEXPAND);

	hostGrid->Add(new wxStaticText(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_HOGAME"))), 0,
	              wxALIGN_CENTER_VERTICAL);
	wxComboBox *gameModeCombo =
		new wxComboBox(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_COOP")),
	                   wxDefaultPosition, wxDefaultSize, 0, NULL, 0);
	gameModeCombo->Append(wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_COOP")),
	                      new wxStringClientData("Cooperative"));
	gameModeCombo->Append(wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_TDM")),
	                      new wxStringClientData("Team Deathmatch"));
	gameModeCombo->Append(wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_ATDM")),
	                      new wxStringClientData("Alt. Team Deathmatch"));
	gameModeCombo->Append(wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_DM")),
	                      new wxStringClientData("Deathmatch"));
	gameModeCombo->Append(wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_ADM")),
	                      new wxStringClientData("Alt. Deathmatch"));

	LinkComboData(gameModeCombo, &currEdit->hostGamemode);
	gameModeCombo->SetEditable(false);
	hostGrid->Add(gameModeCombo, 0, wxEXPAND);

	hostGrid->Add(new wxStaticText(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_HOMODE"))), 0,
	              wxALIGN_CENTER_VERTICAL);
	wxComboBox *netModeCombo =
		new wxComboBox(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_PACKETS")),
	                   wxDefaultPosition, wxDefaultSize, 0, NULL, 0);
	netModeCombo->Append(wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_PACKETS")),
	                     new wxStringClientData("Packet Server"));
	netModeCombo->Append(wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_PEER")),
	                     new wxStringClientData("Peer-to-Peer"));

	LinkComboData(netModeCombo, &currEdit->hostNetworkMode);
	netModeCombo->SetEditable(false);
	hostGrid->Add(netModeCombo, 0, wxEXPAND);

	hostGroup->Add(hostGrid, 1, wxEXPAND | wxALL, 5);
	rightCol->Add(hostGroup, 0, wxEXPAND | wxALL, 5);

	// Compatibility
	wxStaticBoxSizer *compatGroup =
		new wxStaticBoxSizer(wxVERTICAL, panel, wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_COMP")));
	wxFlexGridSizer *compatGrid = new wxFlexGridSizer(2, 2, 5, 5);
	compatGrid->AddGrowableCol(1);

	compatGrid->Add(new wxStaticText(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_COMPPRE"))),
	                0, wxALIGN_CENTER_VERTICAL);
	wxComboBox *complevelCombo =
		new wxComboBox(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_COMP0")),
	                   wxDefaultPosition, wxDefaultSize, 0, NULL, 0, wxGenericValidator(&currEdit->compatLevel));
	complevelCombo->Append(wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_COMP0")));
	complevelCombo->Append(wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_COMP1")));
	complevelCombo->Append(wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_COMP2")));
	complevelCombo->Append(wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_COMP3")));
	complevelCombo->Append(wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_COMP4")));
	complevelCombo->Append(wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_COMP5")));
	complevelCombo->Append(wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_COMP6")));
	complevelCombo->Append(wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_COMP7")));
	complevelCombo->Append(wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_COMP8")));
	complevelCombo->Append(wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_COMP9")));
	complevelCombo->SetEditable(false);
	compatGrid->Add(complevelCombo, 0, wxALIGN_TOP | wxALIGN_RIGHT | wxTOP, 5);

	wxBoxSizer *compStack = new wxBoxSizer(wxVERTICAL);
	compatGrid->Add(compStack, 1, wxEXPAND);

	compatGroup->Add(compatGrid, 1, wxEXPAND | wxALL, 5);
	wxButton *advComButton =
		new wxButton(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_LAUNCH_COMPCUST")));
	compatGroup->Add(advComButton, 0, wxALIGN_LEFT | wxEXPAND);
	// bind it
	advComButton->Bind(wxEVT_BUTTON, [currEdit, panel](wxCommandEvent &) { advCompat(currEdit, panel); });
	rightCol->Add(compatGroup, 0, wxEXPAND | wxALL, 5);

	mainSizer->Add(leftCol, 1, wxEXPAND | wxALL, 5);
	mainSizer->Add(rightCol, 1, wxEXPAND | wxALL, 5);

	panel->SetSizer(mainSizer);
}

void CreateGeneralTab(Profile *currEdit, wxPanel *panel)
{

	wxBoxSizer *pageSizer = new wxBoxSizer(wxVERTICAL);

	wxGridBagSizer *gbSizer = new wxGridBagSizer(10, 20);

	gbSizer->Add(
		new wxStaticText(panel, wxID_ANY,
	                     wxString::Format("%s:", wxString::FromUTF8(GStrings.GetString("LAUNCHER_PROFLIST_TITLE")))),
		wxGBPosition(0, 0), wxGBSpan(1, 1), wxALIGN_CENTER_VERTICAL);
	gbSizer->Add(new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0,
	                            wxTextValidator(wxFILTER_NONE, &currEdit->title)),
	             wxGBPosition(0, 1), wxGBSpan(1, 1), wxEXPAND);

	gbSizer->Add(
		new wxStaticText(panel, wxID_ANY,
	                     wxString::Format("%s:", wxString::FromUTF8(GStrings.GetString("LAUNCHER_PROFLIST_AUTHORS")))),
		wxGBPosition(1, 0), wxGBSpan(1, 1), wxALIGN_CENTER_VERTICAL);
	gbSizer->Add(new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0,
	                            wxTextValidator(wxFILTER_NONE, &currEdit->author)),
	             wxGBPosition(1, 1), wxGBSpan(1, 1), wxEXPAND);

	gbSizer->Add(new wxStaticText(
					 panel, wxID_ANY,
					 wxString::Format("%s:", wxString::FromUTF8(GStrings.GetString("LAUNCHER_PROFLIST_RELEASEDATE")))),
	             wxGBPosition(2, 0), wxGBSpan(1, 1), wxALIGN_CENTER_VERTICAL);
	gbSizer->Add(new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0,
	                            wxTextValidator(wxFILTER_NONE, &currEdit->releaseDate)),
	             wxGBPosition(2, 1), wxGBSpan(1, 1), wxEXPAND);

	wxFlexGridSizer *rightSizer = new wxFlexGridSizer(2, 10, 10);
	rightSizer->AddGrowableCol(1);
	rightSizer->Add(
		new wxStaticText(panel, wxID_ANY,
	                     wxString::Format("%s", wxString::FromUTF8(GStrings.GetString("PROFSET_GENERAL_TYPE")))),
		0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);

	wxComboBox *typeCombo = new wxComboBox(panel, wxID_ANY, currEdit->isIWAD == 1 ? "IWAD" : "PWAD", wxDefaultPosition,
	                                       wxDefaultSize, 0, NULL, 0, wxGenericValidator(&currEdit->isIWAD));
	typeCombo->Append("PWAD");
	typeCombo->Append("IWAD");
	typeCombo->SetEditable(false);
	rightSizer->Add(typeCombo, 0, wxEXPAND);

	// always show IWAD row
	rightSizer->Add(new wxStaticText(panel, wxID_ANY, "IWAD:"), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);

	wxBoxSizer *iwadBox = new wxBoxSizer(wxHORIZONTAL);
	wxTextCtrl *iwadTxt = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0,
	                                     wxTextValidator(wxFILTER_NONE, &currEdit->iwadFilePath));
	iwadBox->Add(iwadTxt, 1, wxEXPAND | wxRIGHT, 5);
	wxButton *iwadButton =
		new wxButton(panel, wxID_ANY, "...", wxDefaultPosition,
	                 panel->FromDIP(wxSize(iwadTxt->GetBestSize().y * 1.2, iwadTxt->GetBestSize().y)));
	iwadBox->Add(iwadButton, 0);
	// bind it
	iwadButton->Bind(wxEVT_BUTTON, [panel, iwadTxt](wxCommandEvent &) {
		OpenPathPicker(panel, iwadTxt, wxString::FromUTF8(GStrings.GetString("PROFSET_GENERAL_IWADDIAG")), false,
		               wxString::FromUTF8(GStrings.GetString("FILETYPE_WAD")));
	});
	rightSizer->Add(iwadBox, 0, wxEXPAND);

	// adds the PWAD row, which will be conditionally shown/hidden
	wxStaticText *pwadLabel = new wxStaticText(panel, wxID_ANY, "PWAD:");
	rightSizer->Add(pwadLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);

	wxBoxSizer *pwadBox = new wxBoxSizer(wxHORIZONTAL);
	wxTextCtrl *pwadTxt = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0,
	                                     wxTextValidator(wxFILTER_NONE, &currEdit->pwadFilePath));
	pwadBox->Add(pwadTxt, 1, wxEXPAND | wxRIGHT, 5);
	wxButton *pwadButton =
		new wxButton(panel, wxID_ANY, "...", wxDefaultPosition,
	                 panel->FromDIP(wxSize(pwadTxt->GetBestSize().y * 1.2, pwadTxt->GetBestSize().y)));
	pwadBox->Add(pwadButton, 0);
	// bind it
	pwadButton->Bind(wxEVT_BUTTON, [panel, pwadTxt](wxCommandEvent &) {
		OpenPathPicker(panel, pwadTxt, wxString::FromUTF8(GStrings.GetString("PROFSET_GENERAL_PWADDIAG")), false,
		               wxString::FromUTF8(GStrings.GetString("FILETYPE_WAD")));
	});
	rightSizer->Add(pwadBox, 0, wxEXPAND);

	bool isPwad = (typeCombo->GetValue() == "PWAD");
	pwadLabel->Show(isPwad);
	pwadBox->ShowItems(isPwad);
	gbSizer->Add(rightSizer, wxGBPosition(0, 3), wxGBSpan(3, 2), wxEXPAND | wxLEFT, 20);

	// logic to show/hide the PWAD row based on selection
	typeCombo->Bind(wxEVT_COMBOBOX, [=](wxCommandEvent &) {
		panel->Freeze();

		bool showPwad = (typeCombo->GetValue() == "PWAD");
		pwadLabel->Show(showPwad);
		pwadBox->ShowItems(showPwad);

		panel->Layout(); // Recalculate positions because update
		panel->Thaw();
	});

	gbSizer->AddGrowableCol(1);
	gbSizer->AddGrowableCol(4);

	pageSizer->Add(gbSizer, 0, wxEXPAND | wxALL, 20);

	// Description Area
	wxBoxSizer   *descSizer = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText *descLabel =
		new wxStaticText(panel, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_GENERAL_DSC")));
	wxTextCtrl *descText = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE,
	                                      wxTextValidator(wxFILTER_NONE, &currEdit->description));

	descSizer->Add(descLabel, 0, wxTOP | wxRIGHT, 5);
	descSizer->Add(descText, 1, wxEXPAND);
	pageSizer->Add(descSizer, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 20);

	panel->SetSizer(pageSizer);
}

void ProfileSettings::ProfileSettingsMenu(wxWindow *parent, const wxString &title, const std::string &profilePath)
{
	// the profile we are editing
	Profile *currEdit = new Profile();

	// populate the Profile from filepath
	currEdit->loadFromFile(profilePath);

	// we have read it, now fill in all fields

	// create the window here since above is not a constructor
	this->Create(parent, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_TITLE")), wxDefaultPosition,
	             this->FromDIP(wxSize(850, 650)));

	this->SetExtraStyle(GetExtraStyle() | wxWS_EX_VALIDATE_RECURSIVELY);

	wxBoxSizer *mainSizer   = new wxBoxSizer(wxVERTICAL); // Main vertical sizer so fit everything
	wxBoxSizer *buttonSizer = new wxBoxSizer(wxHORIZONTAL);

	wxNotebook *tabber = new wxNotebook(this, wxID_ANY); // Used for the tabs in the settings menu

	// Generate every tab panel
	wxScrolledWindow *generalPanel = new wxScrolledWindow(tabber);
	generalPanel->SetScrollRate(5, 5);
	CreateGeneralTab(currEdit, generalPanel);

	wxScrolledWindow *launchPanel = new wxScrolledWindow(tabber);
	launchPanel->SetScrollRate(5, 5);
	CreateLaunchTab(currEdit, launchPanel);

	wxScrolledWindow *filesPanel = new wxScrolledWindow(tabber);
	filesPanel->SetScrollRate(5, 5);
	CreateFilesTab(currEdit, filesPanel);

	wxScrolledWindow *outputPanel = new wxScrolledWindow(tabber);
	outputPanel->SetScrollRate(5, 5);
	CreateOutputTab(currEdit, outputPanel);

	wxScrolledWindow *advancedPanel = new wxScrolledWindow(tabber);
	advancedPanel->SetScrollRate(5, 5);
	CreateAdvancedTab(currEdit, advancedPanel);

	// add them to the tabber
	tabber->AddPage(generalPanel, wxString::FromUTF8(GStrings.GetString("PROFSET_TAB_GENERAL")));
	tabber->AddPage(launchPanel, wxString::FromUTF8(GStrings.GetString("PROFSET_TAB_LAUNCH")));
	tabber->AddPage(filesPanel, wxString::FromUTF8(GStrings.GetString("PROFSET_TAB_FILES")));
	tabber->AddPage(outputPanel, wxString::FromUTF8(GStrings.GetString("PROFSET_TAB_OUTPUT")));
	tabber->AddPage(advancedPanel, wxString::FromUTF8(GStrings.GetString("PROFSET_TAB_ADVANCED")));

	mainSizer->Add(tabber, 1, wxEXPAND | wxALL, 5);

	// Buttons at the bottom of the dialog
	wxButton *DeleteButton = new wxButton(this, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_DELPROF")));

	wxButton *SaveButton = new wxButton(this, wxID_ANY, wxString::FromUTF8(GStrings.GetString("PROFSET_SV_RT")));

	// bind an event to save button
	SaveButton->Bind(wxEVT_BUTTON, [currEdit, this, profilePath](wxCommandEvent &) {
		if (this->TransferDataFromWindow())
		{
			try
			{
				currEdit->saveToFile(profilePath);

				delete currEdit;
				this->EndModal(wxID_OK); // all fine? go back
			}
			catch (const std::exception &e)
			{
				wxMessageBox(e.what(), wxString::FromUTF8(GStrings.GetString("LAUNCHER_ERROR_SAVEPROF")), wxICON_ERROR);
			}
		}
		else
		{
			// validation fail
			wxMessageBox(wxString::FromUTF8(GStrings.GetString("LAUNCHER_ERROR_INVALIDIN")), "UZDoom", wxICON_ERROR);
		}
	});

	DeleteButton->Bind(wxEVT_BUTTON, [currEdit, this, profilePath](wxCommandEvent &) {
		// ask the user , could be an accidental misclick
		wxMessageDialog check(this, wxString::FromUTF8(GStrings.GetString("PROFSET_DELMSG")), "UZDoom",
		                      wxYES_NO | wxICON_WARNING | wxNO_DEFAULT);

		if (check.ShowModal() == wxID_YES)
		{
			std::filesystem::path fileP(profilePath);
			std::filesystem::path dirToWipe = fileP.parent_path();

			// Ensure we have a valid parent directory
			if (!dirToWipe.empty() && std::filesystem::exists(dirToWipe))
			{
				std::filesystem::remove_all(dirToWipe); // fully scrub it

				delete currEdit;
				this->EndModal(wxID_REMOVE); // pass this back to the main window to signal deletion
			}
			else
				wxMessageBox(wxString::FromUTF8(GStrings.GetString("LAUNCHER_ERROR_DELETE")), "UZDoom", wxICON_ERROR);
		}
	});

	// tidy it up into the sizer
	buttonSizer->Add(DeleteButton, 0, wxALIGN_CENTER_VERTICAL);
	buttonSizer->AddStretchSpacer();
	buttonSizer->Add(SaveButton, 0, wxALIGN_CENTER_VERTICAL);

	mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 10);

	this->SetSizer(mainSizer);
}
