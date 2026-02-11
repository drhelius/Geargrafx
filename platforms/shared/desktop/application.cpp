/*
 * Geargrafx - PC Engine / TurboGrafx Emulator
 * Copyright (C) 2024  Ignacio Sanchez

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/
 *
 */

#include <SDL.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "geargrafx.h"
#include "config.h"
#include "gui.h"
#include "gui_debug_disassembler.h"
#include "ogl_renderer.h"
#include "emu.h"
#include "display.h"
#include "utils.h"
#include "single_instance.h"
#include "gamepad.h"
#include "events.h"

#define APPLICATION_IMPORT
#include "application.h"

#if defined(GG_DEBUG)
#define WINDOW_TITLE GG_TITLE " " GG_VERSION " (DEBUG)"
#else
#define WINDOW_TITLE GG_TITLE " " GG_VERSION
#endif

static bool running = true;
static bool paused_when_focus_lost = false;
static Uint32 mouse_last_motion_time = 0;
static const Uint32 mouse_hide_timeout_ms = 1500;
#if !defined(__APPLE__)
static int borderless_saved_x = 0;
static int borderless_saved_y = 0;
static int borderless_saved_w = 0;
static int borderless_saved_h = 0;
static bool borderless_active = false;
#endif

bool g_mcp_stdio_mode = false;

static bool sdl_init(void);
static void sdl_destroy(void);
static void sdl_events(void);
static void sdl_events_quit(const SDL_Event* event);
static void sdl_events_app(const SDL_Event* event);
static void handle_mouse_cursor(void);
static void handle_menu(void);
static void handle_single_instance(void);
static void run_emulator(void);
static void save_window_size(void);
static void log_sdl_error(const char* action, const char* file, int line);

#define SDL_ERROR(action) log_sdl_error(action, __FILE__, __LINE__)

#if defined(__APPLE__)
#include <SDL_syswm.h>
static void* macos_fullscreen_observer = NULL;
static void* macos_nswindow = NULL;
extern "C" void* macos_install_fullscreen_observer(void* nswindow, void(*enter_cb)(), void(*exit_cb)());
extern "C" void macos_set_native_fullscreen(void* nswindow, bool enter);
extern "C" void macos_kill_autofill_helpers(const char* app_name);
#endif

int application_init(const char* rom_file, const char* symbol_file, bool force_fullscreen, bool force_windowed, int mcp_mode, int mcp_tcp_port)
{
    Log("\n%s", GG_TITLE_ASCII);
    Log("%s %s Desktop App", GG_TITLE, GG_VERSION);

    application_show_menu = true;

    if (force_fullscreen)
    {
        config_emulator.fullscreen = true;
    }
    else if (force_windowed)
    {
        config_emulator.fullscreen = false;
    }

    if (!sdl_init())
    {
        Error("Failed to initialize SDL");
        return 1;
    }

    if (!gamepad_init())
    {
        Error("Failed to initialize gamepad subsystem");
        return 2;
    }

    if (!emu_init(application_input_pump))
    {
        Error("Failed to initialize emulator");
        return 3;
    }

    if (!gui_init())
    {
        Error("Failed to initialize GUI");
        return 4;
    }

    if (!ImGui_ImplSDL2_InitForOpenGL(application_sdl_window, display_gl_context))
    {
        Error("Failed to initialize ImGui for SDL2");
        return 5;
    }

    if (!ogl_renderer_init())
    {
        Error("Failed to initialize renderer");
        return 6;
    }

    application_set_vsync(config_video.sync);

    if (config_emulator.fullscreen)
        application_trigger_fullscreen(true);

    if (IsValidPointer(rom_file) && (strlen(rom_file) > 0))
    {
        Log("Rom file argument: %s", rom_file);
        gui_load_rom(rom_file);
    }

    if (IsValidPointer(symbol_file) && (strlen(symbol_file) > 0))
    {
        Log("Symbol file argument: %s", symbol_file);
        gui_debug_reset_symbols();
        gui_debug_load_symbols_file(symbol_file);
    }

    if (mcp_mode >= 0)
    {
        Log("Auto-starting MCP server (mode: %s, port: %d)...", 
            mcp_mode == 0 ? "stdio" : "http", mcp_tcp_port);
        config_debug.debug = true;
        emu_set_overscan(0);
        emu_set_scanline_start_end(0, 241);
        emu_mcp_set_transport(mcp_mode, mcp_tcp_port);
        emu_mcp_start();
    }

    return 0;
}

void application_destroy(void)
{
    remove_directory_and_contents(config_temp_path);
    save_window_size();
    emu_destroy();
    ogl_renderer_destroy();
    ImGui_ImplSDL2_Shutdown();
    gui_destroy();
    gamepad_destroy();
    sdl_destroy();
    single_instance_destroy();

#if defined(__APPLE__)
    macos_kill_autofill_helpers(GG_TITLE);
#endif
}

void application_mainloop(void)
{
    Log("Starting main loop...");

    while (running)
    {
        display_begin_frame();
        sdl_events();
        handle_mouse_cursor();
        handle_menu();
        handle_single_instance();
        run_emulator();
        display_render();
        display_frame_throttle();
    }
}

void application_trigger_quit(void)
{
    SDL_Event event;
    event.type = SDL_QUIT;
    SDL_PushEvent(&event);
}

#if defined(__APPLE__)

static void on_enter_fullscreen()
{
    config_emulator.fullscreen = true;
}

static void on_exit_fullscreen()
{
    config_emulator.fullscreen = false;
}

#endif

void application_trigger_fullscreen(bool fullscreen)
{
#if defined(__APPLE__)
    macos_set_native_fullscreen(macos_nswindow, fullscreen);
#else
    if (fullscreen)
    {
        // Exit borderless first if switching to an SDL fullscreen mode
        if (borderless_active && config_emulator.fullscreen_mode != 2)
        {
            SDL_SetWindowBordered(application_sdl_window, SDL_TRUE);
            SDL_ERROR("SDL_SetWindowBordered");

            SDL_SetWindowPosition(application_sdl_window, borderless_saved_x, borderless_saved_y);
            SDL_ERROR("SDL_SetWindowPosition");

            SDL_SetWindowSize(application_sdl_window, borderless_saved_w, borderless_saved_h);
            SDL_ERROR("SDL_SetWindowSize");

            borderless_active = false;
        }

        switch (config_emulator.fullscreen_mode)
        {
            case 0:  // Exclusive (SDL_WINDOW_FULLSCREEN)
            {
                int display = SDL_GetWindowDisplayIndex(application_sdl_window);
                SDL_ERROR("SDL_GetWindowDisplayIndex");

                SDL_DisplayMode mode;
                SDL_GetDesktopDisplayMode(display, &mode);
                SDL_ERROR("SDL_GetDesktopDisplayMode");

                SDL_SetWindowDisplayMode(application_sdl_window, &mode);
                SDL_ERROR("SDL_SetWindowDisplayMode");

                SDL_SetWindowFullscreen(application_sdl_window, SDL_WINDOW_FULLSCREEN);
                SDL_ERROR("SDL_SetWindowFullscreen");
                break;
            }
            case 1:  // Fake Exclusive (SDL_WINDOW_FULLSCREEN_DESKTOP)
                SDL_SetWindowFullscreen(application_sdl_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
                SDL_ERROR("SDL_SetWindowFullscreen");
                break;
            case 2:  // Borderless windowed
            {
                SDL_SetWindowFullscreen(application_sdl_window, 0);
                SDL_ERROR("SDL_SetWindowFullscreen");

                SDL_GetWindowPosition(application_sdl_window, &borderless_saved_x, &borderless_saved_y);
                SDL_GetWindowSize(application_sdl_window, &borderless_saved_w, &borderless_saved_h);

                SDL_SetWindowBordered(application_sdl_window, SDL_FALSE);
                SDL_ERROR("SDL_SetWindowBordered");

                int display = SDL_GetWindowDisplayIndex(application_sdl_window);
                SDL_ERROR("SDL_GetWindowDisplayIndex");

                SDL_Rect rect;
                SDL_GetDisplayBounds(display, &rect);
                SDL_ERROR("SDL_GetDisplayBounds");

                SDL_SetWindowPosition(application_sdl_window, rect.x, rect.y);
                SDL_ERROR("SDL_SetWindowPosition");

                SDL_SetWindowSize(application_sdl_window, rect.w, rect.h);
                SDL_ERROR("SDL_SetWindowSize");

                borderless_active = true;
                break;
            }
            default:
                Error("Invalid fullscreen mode: %d", config_emulator.fullscreen_mode);
                break;
        }
    }
    else
    {
        if (borderless_active)
        {
            SDL_SetWindowBordered(application_sdl_window, SDL_TRUE);
            SDL_ERROR("SDL_SetWindowBordered");

            SDL_SetWindowPosition(application_sdl_window, borderless_saved_x, borderless_saved_y);
            SDL_ERROR("SDL_SetWindowPosition");

            SDL_SetWindowSize(application_sdl_window, borderless_saved_w, borderless_saved_h);
            SDL_ERROR("SDL_SetWindowSize");

            borderless_active = false;
        }
        else
        {
            SDL_SetWindowFullscreen(application_sdl_window, 0);
            SDL_ERROR("SDL_SetWindowFullscreen");
        }
    }
#endif

    mouse_last_motion_time = SDL_GetTicks();
    display_update_frame_pacing();
}

void application_trigger_fit_to_content(int width, int height)
{
    SDL_SetWindowSize(application_sdl_window, width, height);
    SDL_ERROR("SDL_SetWindowSize");
}

void application_set_vsync(bool enabled)
{
    SDL_GL_SetSwapInterval(enabled ? 1 : 0);
    display_update_frame_pacing();
}

void application_update_title_with_rom(const char* rom)
{
    char final_title[256];
    snprintf(final_title, 256, "%s - %s", WINDOW_TITLE, rom);
    SDL_SetWindowTitle(application_sdl_window, final_title);
    SDL_ERROR("SDL_SetWindowTitle");
}

void application_input_pump(void)
{
    events_emu();
}

bool application_check_single_instance(const char* rom_file, const char* symbol_file)
{
    if (!config_debug.single_instance)
        return true;

    single_instance_init(GG_TITLE);

    if (!single_instance_try_lock())
    {
        if (rom_file != NULL || symbol_file != NULL)
            single_instance_send_message(rom_file, symbol_file);

        single_instance_destroy();
        return false;
    }

    return true;
}

static bool sdl_init(void)
{
    Debug("Initializing SDL...");

#if defined(_WIN32)
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "1");
    SDL_ERROR("SDL_SetHint SDL_HINT_WINDOWS_DPI_SCALING");
#endif

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        SDL_ERROR("SDL_Init");
        return false;
    }

    SDL_VERSION(&application_sdl_build_version);
    SDL_GetVersion(&application_sdl_link_version);

    Log("Using SDL %d.%d.%d (build)", application_sdl_build_version.major, application_sdl_build_version.minor, application_sdl_build_version.patch);
    Log("Using SDL %d.%d.%d (link) ", application_sdl_link_version.major, application_sdl_link_version.minor, application_sdl_link_version.patch);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    if (config_emulator.maximized)
        window_flags = (SDL_WindowFlags)(window_flags | SDL_WINDOW_MAXIMIZED);

    application_sdl_window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, config_emulator.window_width, config_emulator.window_height, window_flags);

    if (!application_sdl_window)
    {
        SDL_ERROR("SDL_CreateWindow");
        return false;
    }

    display_gl_context = SDL_GL_CreateContext(application_sdl_window);

    if (!display_gl_context)
    {
        SDL_ERROR("SDL_GL_CreateContext");
        return false;
    }

    SDL_GL_MakeCurrent(application_sdl_window, display_gl_context);
    SDL_ERROR("SDL_GL_MakeCurrent");

#if defined(__APPLE__)
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (SDL_GetWindowWMInfo(application_sdl_window, &info))
    {
        void* nswindow = info.info.cocoa.window;
        macos_nswindow = nswindow;
        macos_fullscreen_observer = macos_install_fullscreen_observer(nswindow, on_enter_fullscreen, on_exit_fullscreen);
    }
#endif

    SDL_GL_SetSwapInterval(0);
    SDL_ERROR("SDL_GL_SetSwapInterval");

    SDL_SetWindowMinimumSize(application_sdl_window, 500, 300);

    int w, h;
    int display_w, display_h;
    SDL_GetWindowSize(application_sdl_window, &w, &h);
    SDL_GL_GetDrawableSize(application_sdl_window, &display_w, &display_h);
    
    if (w > 0 && h > 0)
    {
        float scale_w = (float)display_w / w;
        float scale_h = (float)display_h / h;

        application_display_scale = (scale_w > scale_h) ? scale_w : scale_h;
    }

    SDL_EventState(SDL_DROPFILE, SDL_ENABLE);
    SDL_ERROR("SDL_EventState SDL_DROPFILE");

    return true;
}

static void sdl_destroy(void)
{
    SDL_GL_DeleteContext(display_gl_context);
    SDL_DestroyWindow(application_sdl_window);
    SDL_Quit();
}

static void handle_mouse_cursor(void)
{
    if (!config_debug.debug && gui_main_window_hovered)
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);
    else if (!config_debug.debug && config_emulator.fullscreen && !config_emulator.always_show_menu)
    {
        Uint32 now = SDL_GetTicks();

        if ((now - mouse_last_motion_time) < mouse_hide_timeout_ms)
            ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
        else
            ImGui::SetMouseCursor(ImGuiMouseCursor_None);
    }
    else
        ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
}

static void handle_menu(void)
{
    if (config_emulator.always_show_menu)
        application_show_menu = true;
    else if (config_debug.debug)
        application_show_menu = true;
    else if (config_emulator.fullscreen)
        application_show_menu = config_emulator.always_show_menu;
    else
        application_show_menu = true;
}

static void handle_single_instance(void)
{
    if (!config_debug.single_instance || !single_instance_is_primary())
        return;

    single_instance_poll();

    static char s_pending_rom_path[4096];
    static char s_pending_symbol_path[4096];
    if (single_instance_get_pending_load(s_pending_rom_path, sizeof(s_pending_rom_path), s_pending_symbol_path, sizeof(s_pending_symbol_path)))
    {
        if (s_pending_rom_path[0] != '\0')
            gui_load_rom(s_pending_rom_path);
        if (s_pending_symbol_path[0] != '\0')
        {
            gui_debug_reset_symbols();
            gui_debug_load_symbols_file(s_pending_symbol_path);
        }
    }
}

static void sdl_events(void)
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        sdl_events_quit(&event);

        if (running)
        {
            sdl_events_app(&event);

            ImGui_ImplSDL2_ProcessEvent(&event);

            if (!gui_in_use)
                events_shortcuts(&event);
        }
    }
}

static void sdl_events_quit(const SDL_Event* event)
{
    if (event->type == SDL_QUIT)
    {
        running = false;
        return;
    }

    if (event->type == SDL_WINDOWEVENT && event->window.event == SDL_WINDOWEVENT_CLOSE && event->window.windowID == SDL_GetWindowID(application_sdl_window))
    {
        running = false;
        return;
    }
}

static void sdl_events_app(const SDL_Event* event)
{
    switch (event->type)
    {
        case (SDL_DROPFILE):
        {
            char* dropped_filedir = event->drop.file;
            gui_load_rom(dropped_filedir);
            SDL_free(dropped_filedir);
            SDL_SetWindowInputFocus(application_sdl_window);
            break;
        }
        case SDL_WINDOWEVENT:
        {
            switch (event->window.event)
            {
                case SDL_WINDOWEVENT_FOCUS_GAINED:
                {
                    application_set_vsync(config_video.sync);
                    if (config_emulator.pause_when_inactive && !paused_when_focus_lost)
                        emu_resume();
                    break;
                }
                case SDL_WINDOWEVENT_FOCUS_LOST:
                {
                    application_set_vsync(false);
                    if (config_emulator.pause_when_inactive)
                    {
                        paused_when_focus_lost = emu_is_paused();
                        emu_pause();
                    }
                    break;
                }
                case SDL_WINDOWEVENT_DISPLAY_CHANGED:
                {
                    if (config_video.sync)
                        display_recreate_gl_context();
                    break;
                }
            }
            break;
        }
        case (SDL_MOUSEMOTION):
        {
            mouse_last_motion_time = SDL_GetTicks();
            break;
        }
        case SDL_CONTROLLERDEVICEADDED:
        {
            gamepad_add();
            break;
        }
        case SDL_CONTROLLERDEVICEREMOVED:
        {
            gamepad_remove(event->cdevice.which);
            break;
        }
    }
}

static void run_emulator(void)
{
    if (!display_should_run_emu_frame())
        return;

    config_emulator.paused = emu_is_paused();
    emu_audio_sync = config_audio.sync;
    emu_update();

    if (!events_input_updated())
        events_emu();
    events_reset_input();
}

static void save_window_size(void)
{
    if (!config_emulator.fullscreen)
    {
        int width, height;
        SDL_GetWindowSize(application_sdl_window, &width, &height);
        config_emulator.window_width = width;
        config_emulator.window_height = height;
        config_emulator.maximized = (SDL_GetWindowFlags(application_sdl_window) & SDL_WINDOW_MAXIMIZED);
    }
}

static void log_sdl_error(const char* action, const char* file, int line)
{
    const char* error = SDL_GetError();
    if (error && error[0] != '\0')
    {
        Log("SDL Error: %s (%s:%d) - %s", action, file, line, error);
        SDL_ClearError();
    }
}