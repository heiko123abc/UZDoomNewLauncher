/*
** errorwindow.cpp
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

#include "errorwindow.h"
#include "gstrings.h"
#include "printf.h"

#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <miniz.h>
#include <sstream>

#include "starter.h"

// Defined in UZDoom headers
#define TEXTCOLOR_ESCAPE '\x1c'

std::string ParseAndCleanLog(const std::string &log, const std::string &errorText)
{
	std::string processedLog;
	processedLog.reserve(log.size());

	for (size_t i = 0; i < log.size(); ++i)
	{
		unsigned char chr = log[i];
		if (chr == TEXTCOLOR_ESCAPE)
		{
			i++; // Skip the 'A', 'B', etc. defining the color
			continue;
		}

		// Replace control range with Box Drawings Double Horizontal
		if (chr >= 0x1D && chr <= 0x1F)
		{
			processedLog += "\xE2\x95\x90";
		}
		else
		{
			processedLog += chr;
		}
	}

	return processedLog + "\n\nExecution could not continue.\n" + errorText + "\n";
}

void SaveReportToDisk(const std::vector<uint8_t> &minidump, const std::string &cleanClipboardText)
{
	// Instead of a file dialog, we simply save directly to the execution directory (thats the easierst way)
	std::time_t        t       = std::time(nullptr);
	std::tm            tm_info = *std::localtime(&t);
	std::ostringstream oss;
	oss << "UZDoomCrashReport_" << std::put_time(&tm_info, "%Y-%m-%d_%H-%M-%S") << ".zip";

	std::string filename = oss.str();

	mz_zip_archive zip = {};
	if (mz_zip_writer_init_heap(&zip, 0, 16 * 1024 * 1024))
	{
		// Add Minidump
		mz_zip_writer_add_mem(&zip, "minidump.dmp", minidump.data(), minidump.size(), MZ_DEFAULT_COMPRESSION);

		// Add Log text
		mz_zip_writer_add_mem(&zip, "log.txt", cleanClipboardText.data(), cleanClipboardText.size(),
		                      MZ_DEFAULT_COMPRESSION);
	}

	// Finalize Zip
	void  *buffer     = nullptr;
	size_t buffersize = 0;
	mz_zip_writer_finalize_heap_archive(&zip, &buffer, &buffersize);
	mz_zip_writer_end(&zip);

	// Write to disk
	std::ofstream file(filename, std::ios::binary);
	if (file.is_open())
	{
		file.write(static_cast<const char *>(buffer), buffersize);
		file.close();
	}

	// Free the buffer allocated by miniz
	mz_free(buffer);
}

bool ErrorWindow::ExecModal(const std::string &text, const std::string &log, std::vector<uint8_t> minidump)
{
	std::string                title   = GStrings.GetString("CRASHREPORT_TITLE");
	Starter::ImGuiContextState context = Starter::SetupContext(title.c_str(), 1200, 700, SDL_INIT_VIDEO);

	if (!context.window)
		return false;

	ImGuiIO    &io                 = ImGui::GetIO();
	std::string cleanClipboardText = ParseAndCleanLog(log, text);

	// Run the abstracted loop
	Starter::RunImGuiLoop(context, [&](bool &done) {
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImVec2(800, 600));

		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

		ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));       // Dark red
		ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.8f, 0.1f, 0.1f, 1.0f)); // Bright red when active

		ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;

		ImGui::Begin("Fatal Error Panel", nullptr, flags);

		// Log View (Takes up most of the space)
		ImGui::BeginChild("LogView", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 20), true,
		                  ImGuiWindowFlags_HorizontalScrollbar);

		ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]); // Teletype/monospace ideally
		ImGui::TextUnformatted(cleanClipboardText.c_str());
		ImGui::PopFont();

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// Error Header (Red)
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.66f, 0.66f, 1.0f));
		ImGui::PopStyleColor();

		// Body (Yellow)
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.66f, 1.0f));
		ImGui::TextWrapped("%s", text.c_str());
		ImGui::PopStyleColor();

		// Auto-scroll to bottom if log is long
		if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
			ImGui::SetScrollHereY(1.0f);

		ImGui::EndChild();

		ImGui::Spacing();

		// Buttons at the bottom
		float windowWidth = ImGui::GetWindowWidth();

		if (ImGui::Button(GStrings.GetString("CRASHREPORT_COPYCLIP"), ImVec2(200, 0)))
		{
			ImGui::SetClipboardText(cleanClipboardText.c_str());
		}

		// Centering logic for middle button
		ImGui::SameLine((windowWidth / 2.0f) - 100.0f);

		ImGui::BeginDisabled(minidump.empty());
		std::string saveBtnTxt =
			minidump.empty() ? GStrings.GetString("CRASHREPORT_NOSAVE") : GStrings.GetString("CRASHREPORT_SAVE");
		if (ImGui::Button(saveBtnTxt.c_str(), ImVec2(200, 0)))
		{
			SaveReportToDisk(minidump, cleanClipboardText);
		}
		ImGui::EndDisabled();

		// Right alignment logic for last button
		ImGui::SameLine(windowWidth - 200.0f - ImGui::GetStyle().WindowPadding.x);
		if (ImGui::Button(GStrings.GetString("CRASHREPORT_QUIT"), ImVec2(200, 0)))
		{
			done = true; // Setting this exits the loop gracefully
		}

		ImGui::End();
	});

	Starter::TeardownContext(context);

	return false;
}
