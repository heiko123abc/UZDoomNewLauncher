/*
** starter.cpp
**
** The launcher is STARTED here. Create required files and folder on first launch.
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

// ImGui & SDL Backends
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#include <SDL.h>
#include <stdio.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif
#ifdef _WIN32
#include <windows.h> // SetProcessDPIAware()
#endif

using json      = nlohmann::json;
bool execResult = false;

// Global instance of the main UI state
static LauncherMainWindow *MainWindow = nullptr;

// SDL State
static SDL_Window   *Window      = nullptr;
static SDL_GLContext GLContext   = nullptr;
static const char   *GlslVersion = "";

// This is called from outside to kickstart the launcher ui and logics
bool ImGuiKickStarter()
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

// This function is called on application startup and sets up
bool Starter::Init()
{
	// 1. SETUP SDL & OPENGL
#ifdef _WIN32
	::SetProcessDPIAware();
#endif
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
	{
		std::cerr << "Error: " << SDL_GetError() << std::endl;
		return false;
	}

	// Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
	// GL ES 2.0 + GLSL 100 (WebGL 1.0)
	const char *glsl_version = "#version 100";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
	// GL ES 3.0 + GLSL 300 es (WebGL 2.0)
	const char *glsl_version = "#version 300 es";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
	// GL 3.2 Core + GLSL 150
	const char *glsl_version = "#version 150";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
	// GL 3.0 + GLSL 130
	const char *glsl_version = "#version 130";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

	// From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
	SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

	// Create window with graphics context
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	float main_scale = ImGui_ImplSDL2_GetContentScaleForDisplay(0);

	SDL_WindowFlags window_flags =
		(SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	SDL_Window *window =
		SDL_CreateWindow("UZDoom Launcher", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
	                     (int)(1280 * main_scale), (int)(800 * main_scale), window_flags);
	if (window == nullptr)
	{
		printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
		return 1;
	}

	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	if (gl_context == nullptr)
	{
		printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
		return 1;
	}

	SDL_GL_MakeCurrent(window, gl_context);
	SDL_GL_SetSwapInterval(1); // Enable vsync

	// 2. SETUP DEAR IMGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	(void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

	// setup scaling options
	ImGuiStyle &style = ImGui::GetStyle();
	style.ScaleAllSizes(main_scale); // Bake a fixed style scale. (until we have a solution for dynamic style scaling,
	                                 // changing this requires resetting Style + calling this again)
	style.FontScaleDpi = main_scale; // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true
	                                 // automatically overrides this for every window depending on the current monitor)

	// Setup Platform/Renderer backends
	ImGui_ImplSDL2_InitForOpenGL(Window, GLContext);
	ImGui_ImplOpenGL3_Init(GlslVersion);

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
			    event.window.windowID == SDL_GetWindowID(Window))
				done = true;
		}

		if (SDL_GetWindowFlags(Window) & SDL_WINDOW_MINIMIZED)
		{
			SDL_Delay(10);
			continue;
		}

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		// --- DRAW UZDOOM LAUNCHER UI --- (THAT IS WHAT THIS starter.cpp FILE IS INITIALIZING)
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
		SDL_GL_SwapWindow(Window);
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

	// Shutdown ImGui
	if (GLContext)
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();
	}

	// Shutdown SDL
	SDL_GL_DeleteContext(GLContext);
	SDL_DestroyWindow(Window);
	SDL_Quit();
}
