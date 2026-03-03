/*
** starter.cpp
**
** The launcher/netui/errorwindow is STARTED here. Create required files and folder on first launch.
** Includes SDL2 + OpenGL3 backend initialization.
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

#include "starter.h"
#include "i_interface.h"
#include "launcherMainWindow.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json      = nlohmann::json;
bool execResult = false;

// Global instance of the main UI state
static LauncherMainWindow *MainWindow = nullptr;

// Encapsulated SDL State for the Launcher Window
static Starter::ImGuiContextState LauncherContext;

// This is called from outside to kickstart the launcher ui and logics
bool ImGuiKickStarter(const FStartupSelectionInfo &info)
{
	if (!Starter::Init())
	{
		std::cerr << "Failed to initialize the launcher." << std::endl;
		Starter::Shutdown();
		return false;
	}

	Starter::RunLoop();
	Starter::Shutdown();

	return execResult;
}

// use this to establish the SDL and ImGui context for the launcher, error and netstart windows
Starter::ImGuiContextState Starter::SetupContext(const char *title, int width, int height, Uint32 sdl_init_flags)
{
	ImGuiContextState state;

#ifdef _WIN32
	::SetProcessDPIAware();
#endif
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
	{
		printf("Error: %s\n", SDL_GetError());
		return state;
	}

#if defined(IMGUI_IMPL_OPENGL_ES2)
	const char *glsl_version = "#version 100";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
	const char *glsl_version = "#version 300 es";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
	const char *glsl_version = "#version 150";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
	const char *glsl_version = "#version 130";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

#ifdef SDL_HINT_IME_SHOW_UI
	SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	state.scale = ImGui_ImplSDL2_GetContentScaleForDisplay(0);
	if (state.scale <= 0.0f)
		state.scale = 1.0f;

	SDL_WindowFlags window_flags =
		(SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	state.window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, (int)(width * state.scale),
	                                (int)(height * state.scale), window_flags);
	if (!state.window)
	{
		std::cerr << "Error: SDL_CreateWindow(): " << SDL_GetError() << std::endl;
		return state;
	}

	state.gl_context = SDL_GL_CreateContext(state.window);
	if (!state.gl_context)
	{
		std::cerr << "Error: SDL_GL_CreateContext(): " << SDL_GetError() << std::endl;
		return state;
	}

	SDL_GL_MakeCurrent(state.window, state.gl_context);
	SDL_GL_SetSwapInterval(1); // Enable vsync

	// Setup Dear ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

	// Setup scaling options automatically
	ImGuiStyle &style = ImGui::GetStyle();
	style.ScaleAllSizes(state.scale);
	style.FontScaleDpi = state.scale;

	// Setup UTF-8 font using noto
	ImFontConfig config;
	float        fontSize = 16.0f;
	config.PixelSnapH     = true;

	io.Fonts->AddFontFromFileTTF("../../wadsrc/static/ui/noto/noto-sans.ttf", fontSize, &config);
	config.MergeMode = true;

	// Add additional fonts for CJK characters, merging them with the default font
	io.Fonts->AddFontFromFileTTF("../../wadsrc/static/ui/noto/noto-sans-jp.ttf", fontSize, &config);
	io.Fonts->AddFontFromFileTTF("../../wadsrc/static/ui/noto/noto-sans-kr.ttf", fontSize, &config);

	ImGui::StyleColorsDark();

	ImGui_ImplSDL2_InitForOpenGL(state.window, state.gl_context);
	ImGui_ImplOpenGL3_Init(glsl_version);

	return state;
}

void Starter::TeardownContext(ImGuiContextState &state)
{
	if (state.gl_context)
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();
		SDL_GL_DeleteContext(state.gl_context);
		state.gl_context = nullptr;
	}

	if (state.window)
	{
		SDL_DestroyWindow(state.window);
		state.window = nullptr;
	}

	SDL_Quit();
}

// This function is called on application startup and sets up
bool Starter::Init()
{
	LauncherContext =
		SetupContext("UZDoom Launcher", 1280, 800, SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER);
	if (!LauncherContext.window)
		return false;

	// 3. DEFINE THE PATHS AND FILES WE NEED
	exePath     = "./"; // we just need the folder where the executable is located
	ROOT_DIR    = exePath + "launcher/";
	PROFILE_DIR = ROOT_DIR + "profiles/";
	CONFIG_FILE = ROOT_DIR + "config.json";

	// hang on, lets see if folder for launchers profiles exists
	// if not, create them
	std::filesystem::create_directories(PROFILE_DIR);

	// and now the config file for launcher
	if (!std::filesystem::exists(CONFIG_FILE))
	{
		// Create default JSON config
		std::ofstream configFile(CONFIG_FILE);
		if (configFile.is_open())
		{
			json j;
			j["lang"]     = "default"; // Fallback to default if macro isn't defined
			j["theme"]    = "system";
			j["profiles"] = json::array(); // Ready array for later profiles

			configFile << j.dump(4); // indent for readability
			configFile.close();
		}
	}

	// Initialize Main Window class
	MainWindow = new LauncherMainWindow();

	return true;
}

// the core of dearImgui: the main loop where we poll events and render the UI
void Starter::RunLoop()
{
	ImGuiIO &io          = ImGui::GetIO();
	bool     done        = false;
	ImVec4   clear_color = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);

	while (!done)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSDL2_ProcessEvent(&event);
			if (event.type == SDL_QUIT)
				done = true;
			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
			    event.window.windowID == SDL_GetWindowID(LauncherContext.window))
				done = true;
		}

		if (SDL_GetWindowFlags(LauncherContext.window) & SDL_WINDOW_MINIMIZED)
		{
			SDL_Delay(10);
			continue;
		}

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		// --- DRAW UZDOOM LAUNCHER UI ---
		if (MainWindow)
		{
			MainWindow->Draw();
		}

		// Rendering
		ImGui::Render();
		glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w,
		             clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(LauncherContext.window);
	}
}

void Starter::Shutdown()
{
	// Clean up mian UI class
	if (MainWindow)
	{
		delete MainWindow;
		MainWindow = nullptr;
	}

	TeardownContext(LauncherContext);
}
