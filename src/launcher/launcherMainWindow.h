/*
** launcherMainWindow.h
**
** Header for launcherMainWindow.cpp
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

#pragma once

#include "imgui.h"
#include "loader.h"
#include "profile.h"
#include "profileSettings.h"
#include <atomic>
#include <string>
#include <vector>

#include <cstdint>
#include <imgui.h>

class LauncherTheme
{
  public:
	enum class BaseTheme
	{
		Custom,
		ImGuiLight,
		ImGuiDark
	};

	BaseTheme type = BaseTheme::ImGuiDark;

	// Custom theme colors
	ImVec4 bgColor;
	ImVec4 textColor;
	ImVec4 inputsColor;
	ImVec4 interactColor;
	ImVec4 hoverColor;
	ImVec4 clickColor;
	ImVec4 borderColor;

	// Default constructor
	LauncherTheme() = default;

	// Construct standard themes (Marked explicit to prevent accidental implicit conversions)
	explicit LauncherTheme(BaseTheme baseType) : type(baseType)
	{
	}

	// Construct custom themes
	LauncherTheme(uint32_t bg, uint32_t text, uint32_t inputs, uint32_t interact, uint32_t hover, uint32_t click,
	              uint32_t border)
		: type(BaseTheme::Custom), bgColor(HexToImVec4(bg)), textColor(HexToImVec4(text)),
		  inputsColor(HexToImVec4(inputs)), interactColor(HexToImVec4(interact)), hoverColor(HexToImVec4(hover)),
		  clickColor(HexToImVec4(click)), borderColor(HexToImVec4(border))
	{
	}

	// Static helper: Doesn't rely on class state, so it should be static
	static ImVec4 HexToImVec4(uint32_t hex, float alpha = 1.0f)
	{
		float r = ((hex >> 16) & 0xFF) / 255.0f;
		float g = ((hex >> 8) & 0xFF) / 255.0f;
		float b = ((hex >> 0) & 0xFF) / 255.0f;
		return ImVec4(r, g, b, alpha);
	}

	// Applies the theme to the current ImGui context
	void ApplyTheme() const
	{
		ImGuiStyle &style = ImGui::GetStyle();

		// 1. Reset Structural Styling
		style.WindowBorderSize = (type == BaseTheme::Custom) ? 1.0f : 0.0f;
		style.FrameBorderSize  = (type == BaseTheme::Custom) ? 1.0f : 0.0f;

		// 2. Handle Standard Themes
		if (type == BaseTheme::ImGuiLight)
		{
			ImGui::StyleColorsLight();
			return;
		}

		if (type == BaseTheme::ImGuiDark)
		{
			ImGui::StyleColorsDark();
			return;
		}

		// (scrollbars, tabs, separators) that any custom theme doesn't explicitly set.
		ImGui::StyleColorsDark();

		ImVec4 *colors = style.Colors;

		// Apply background and text
		colors[ImGuiCol_WindowBg] = bgColor;
		colors[ImGuiCol_ChildBg]  = bgColor;
		colors[ImGuiCol_PopupBg]  = inputsColor;
		colors[ImGuiCol_Text]     = textColor;

		colors[ImGuiCol_TextDisabled] = ImVec4(textColor.x, textColor.y, textColor.z, 0.5f);

		// Apply borders
		colors[ImGuiCol_Border] = borderColor;

		// Frame Backgrounds (Inputs, Checkboxes, etc.)
		colors[ImGuiCol_FrameBg]        = inputsColor;
		colors[ImGuiCol_FrameBgHovered] = hoverColor;
		colors[ImGuiCol_FrameBgActive]  = clickColor;

		// Interactive Elements (Buttons)
		colors[ImGuiCol_Button]        = interactColor;
		colors[ImGuiCol_ButtonHovered] = hoverColor;
		colors[ImGuiCol_ButtonActive]  = clickColor;

		// Headers (Selectable rows in your Profile List)
		colors[ImGuiCol_Header]        = interactColor;
		colors[ImGuiCol_HeaderHovered] = hoverColor;
		colors[ImGuiCol_HeaderActive]  = clickColor;

		// Table specific styling
		colors[ImGuiCol_TableHeaderBg]     = inputsColor;
		colors[ImGuiCol_TableBorderLight]  = borderColor;
		colors[ImGuiCol_TableBorderStrong] = borderColor;

		// Title bar
		colors[ImGuiCol_TitleBg]          = interactColor;
		colors[ImGuiCol_TitleBgActive]    = hoverColor;
		colors[ImGuiCol_TitleBgCollapsed] = interactColor;
	}
};

class LauncherMainWindow
{
  private:
	ProfileSettings settingsModal;

  public:
	LauncherMainWindow();
	~LauncherMainWindow() = default;

	// main ImGui render loop
	void Draw();

	// Helper to reload from config file
	void RefreshList();
	void SaveConfig();

  private:
	std::vector<std::string> profilePaths;
	std::vector<Profile>     cachedProfiles; // Store actual data so we don't read JSON every frame

	int selectedProfileIdx = -1;

	// State trackers
	std::string langVar  = "default";
	std::string themeVar = "system";

	// Modal Triggers
	bool showSettingsModal = false;
	bool showAboutNotes    = false;
	bool showAboutCredits  = false;
	bool showImportPopup   = false;

	// keep track of the last import status
	importStatus lastImportStatus = IMPORT_CANCELLED;

	// Async process tracking
	std::atomic<bool> isAlreadyLaunched{false};

	// Set by background thread when game closes and comes back to signal the UI to refresh playtime and last played
	std::atomic<bool> needsRefresh{false};

	// UI Helpers
	void DrawPopUp();
	void DrawMenuBar();
	void DrawProfileList();
	void DrawButtons();
	void DrawDescriptionBox();

	// Action Helpers
	void LaunchGame(const std::string &mode);
	void MoveSelectedEntry(int offset);
	void UpdateLanguage();
	void ApplyTheme();
	void CloneSelectedProfile();
};
