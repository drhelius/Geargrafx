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
#include "renderer.h"
#include "emu.h"
#include "utils.h"
#include "single_instance.h"

#define APPLICATION_IMPORT
#include "application.h"

#if defined(GG_DEBUG)
#define WINDOW_TITLE GG_TITLE " " GG_VERSION " (DEBUG)"
#else
#define WINDOW_TITLE GG_TITLE " " GG_VERSION
#endif

static SDL_GLContext gl_context = NULL;
static bool running = true;
static bool paused_when_focus_lost = false;
static Uint64 frame_time_start = 0;
static Uint64 frame_time_end = 0;
static bool input_updated = false;
static Uint16 input_last_state[GG_MAX_GAMEPADS] = { };
static bool input_turbo_toggle_prev[GG_MAX_GAMEPADS][2] = { };
static bool input_gamepad_shortcut_prev[GG_MAX_GAMEPADS][config_HotkeyIndex_COUNT] = { };
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
static void sdl_load_gamepad_mappings(void);
static void sdl_events(void);
static void sdl_events_quit(const SDL_Event* event);
static void sdl_events_app(const SDL_Event* event);
static void sdl_events_shortcuts_gui(const SDL_Event* event);
static void sdl_events_emu(void);
static void sdl_add_gamepads(void);
static void sdl_remove_gamepad(SDL_JoystickID instance_id);
static Uint16 input_build_state(int controller);
static void input_apply_state(int controller, Uint16 before, Uint16 now);
static void input_check_gamepad_shortcuts(int controller);
static bool input_get_button(SDL_GameController* controller, int mapping);
static void handle_mouse_cursor(void);
static void handle_menu(void);
static void handle_single_instance(void);
static void run_emulator(void);
static void render(void);
static void frame_throttle(void);
static void save_window_size(void);
static void log_sdl_error(const char* action, const char* file, int line);
static bool check_hotkey(const SDL_Event* event, const config_Hotkey& hotkey, bool allow_repeat);

#define SDL_ERROR(action) log_sdl_error(action, __FILE__, __LINE__)

#if defined(__APPLE__)
#include <SDL_syswm.h>
static void* macos_fullscreen_observer = NULL;
static void* macos_nswindow = NULL;
extern "C" void* macos_install_fullscreen_observer(void* nswindow, void(*enter_cb)(), void(*exit_cb)());
extern "C" void macos_set_native_fullscreen(void* nswindow, bool enter);
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

    if (!emu_init(application_input_pump))
    {
        Error("Failed to initialize emulator");
        return 2;
    }

    if (!gui_init())
    {
        Error("Failed to initialize GUI");
        return 3;
    }

    if (!ImGui_ImplSDL2_InitForOpenGL(application_sdl_window, gl_context))
    {
        Error("Failed to initialize ImGui for SDL2");
        return 4;
    }

    if (!renderer_init())
    {
        Error("Failed to initialize renderer");
        return 5;
    }

    SDL_GL_SetSwapInterval(config_video.sync ? 1 : 0);

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
    renderer_destroy();
    ImGui_ImplSDL2_Shutdown();
    gui_destroy();
    sdl_destroy();
    single_instance_destroy();
}

void application_mainloop(void)
{
    Log("Starting main loop...");

    while (running)
    {
        frame_time_start = SDL_GetPerformanceCounter();
        sdl_events();
        handle_mouse_cursor();
        handle_menu();
        handle_single_instance();
        run_emulator();
        render();
        frame_time_end = SDL_GetPerformanceCounter();
        frame_throttle();
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
}

void application_trigger_fit_to_content(int width, int height)
{
    SDL_SetWindowSize(application_sdl_window, width, height);
    SDL_ERROR("SDL_SetWindowSize");
}

void application_update_title_with_rom(const char* rom)
{
    char final_title[256];
    snprintf(final_title, 256, "%s - %s", WINDOW_TITLE, rom);
    SDL_SetWindowTitle(application_sdl_window, final_title);
    SDL_ERROR("SDL_SetWindowTitle");
}

void application_assign_gamepad(int slot, int device_index)
{
    if (slot < 0 || slot >= GG_MAX_GAMEPADS)
        return;

    if (device_index < 0)
    {
        if (IsValidPointer(application_gamepad[slot]))
        {
            SDL_GameControllerClose(application_gamepad[slot]);
            application_gamepad[slot] = NULL;
            Debug("Player %d controller set to None", slot + 1);
        }
        return;
    }

    if (device_index >= SDL_NumJoysticks())
    {
        Log("Warning: device_index %d out of range", device_index);
        return;
    }

    if (!SDL_IsGameController(device_index))
    {
        Log("Warning: device_index %d is not a game controller", device_index);
        return;
    }

    SDL_JoystickID wanted_id = SDL_JoystickGetDeviceInstanceID(device_index);

    if (IsValidPointer(application_gamepad[slot]))
    {
        SDL_JoystickID current_id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(application_gamepad[slot]));
        if (current_id == wanted_id)
            return;
    }

    int other = -1;
    for (int i = 0; i < GG_MAX_GAMEPADS; i++)
    {
        if (i == slot)
            continue;

        if (IsValidPointer(application_gamepad[i]))
        {
            SDL_JoystickID id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(application_gamepad[i]));
            if (id == wanted_id)
            {
                other = i;
                break;
            }
        }
    }

    if (other != -1)
    {
        if (IsValidPointer(application_gamepad[slot]))
        {
            SDL_GameControllerClose(application_gamepad[slot]);
            application_gamepad[slot] = NULL;
        }

        application_gamepad[slot] = application_gamepad[other];
        application_gamepad[other] = NULL;

        Debug("Moved controller from Player %d to Player %d", other + 1, slot + 1);
        return;
    }

    if (IsValidPointer(application_gamepad[slot]))
    {
        SDL_GameControllerClose(application_gamepad[slot]);
        application_gamepad[slot] = NULL;
    }

    SDL_GameController* controller = SDL_GameControllerOpen(device_index);
    if (!IsValidPointer(controller))
    {
        Log("SDL_GameControllerOpen failed for device_index %d", device_index);
        SDL_ERROR("SDL_GameControllerOpen");
        return;
    }

    application_gamepad[slot] = controller;
    Debug("Game controller %d assigned to Player %d", device_index, slot + 1);
}

void application_input_pump(void)
{
    sdl_events_emu();
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

    for (int i = 0; i < GG_MAX_GAMEPADS; i++)
        InitPointer(application_gamepad[i]);

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

    gl_context = SDL_GL_CreateContext(application_sdl_window);

    if (!gl_context)
    {
        SDL_ERROR("SDL_GL_CreateContext");
        return false;
    }

    SDL_GL_MakeCurrent(application_sdl_window, gl_context);
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

    sdl_load_gamepad_mappings();
    sdl_add_gamepads();

    return true;
}

static void sdl_destroy(void)
{
    for (int i = 0; i < GG_MAX_GAMEPADS; i++)
    {
        if (IsValidPointer(application_gamepad[i]))
        {
            SDL_GameControllerClose(application_gamepad[i]);
        }
    }

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(application_sdl_window);
    SDL_Quit();
}

static void sdl_load_gamepad_mappings(void)
{
    std::string db_path;
    char exe_path[1024] = { };
    get_executable_path(exe_path, sizeof(exe_path));

    if (exe_path[0] != '\0')
    {
        db_path = std::string(exe_path) + "/gamecontrollerdb.txt";
    }
    else
    {
        db_path = "gamecontrollerdb.txt";
    }

    std::ifstream file;
    open_ifstream_utf8(file, db_path.c_str(), std::ios::in);
    if (!file.is_open())
    {
        open_ifstream_utf8(file, "gamecontrollerdb.txt", std::ios::in);
    }

    int added_mappings = 0;
    int updated_mappings = 0;
    int line_number = 0;
    char platform_field[64] = { };
    snprintf(platform_field, 64, "platform:%s", SDL_GetPlatform());
    if (file.is_open())
    {
        Debug("Loading gamecontrollerdb.txt file");
        std::string line;
        while (std::getline(file, line))
        {
            size_t comment = line.find_first_of('#');
            if (comment != std::string::npos)
                line = line.substr(0, comment);
            line = line.erase(0, line.find_first_not_of(" \t\r\n"));
            line = line.erase(line.find_last_not_of(" \t\r\n") + 1);
            while (line[0] == ' ')
                line = line.substr(1);
            if (line.empty())
                continue;
            if ((line.find("platform:") != std::string::npos) && (line.find(platform_field) == std::string::npos))
                continue;
            int result = SDL_GameControllerAddMapping(line.c_str());
            if (result == 1)
                added_mappings++;
            else if (result == 0)
                updated_mappings++;
            else if (result == -1)
            {
                Error("Unable to load game controller mapping in line %d from gamecontrollerdb.txt", line_number);
                SDL_ERROR("SDL_GameControllerAddMapping");
            }
            line_number++;
        }
        file.close();
    }
    else
    {
        Error("Game controller database not found (gamecontrollerdb.txt)!!");
        return;
    }
    Log("Added %d new game controller mappings from gamecontrollerdb.txt", added_mappings);
    Log("Updated %d game controller mappings from gamecontrollerdb.txt", updated_mappings);
    application_added_gamepad_mappings = added_mappings;
    application_updated_gamepad_mappings = updated_mappings;
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
                sdl_events_shortcuts_gui(&event);
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
                    if (config_emulator.pause_when_inactive && !paused_when_focus_lost)
                        emu_resume();
                    break;
                }
                case SDL_WINDOWEVENT_FOCUS_LOST:
                {
                    if (config_emulator.pause_when_inactive)
                    {
                        paused_when_focus_lost = emu_is_paused();
                        emu_pause();
                    }
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
            sdl_add_gamepads();
            break;
        }
        case SDL_CONTROLLERDEVICEREMOVED:
        {
            sdl_remove_gamepad(event->cdevice.which);
            break;
        }
    }
}

static bool check_hotkey(const SDL_Event* event, const config_Hotkey& hotkey, bool allow_repeat)
{
    if (event->type != SDL_KEYDOWN)
        return false;

    if (!allow_repeat && event->key.repeat != 0)
        return false;

    if (event->key.keysym.scancode != hotkey.key)
        return false;

    SDL_Keymod mods = (SDL_Keymod)event->key.keysym.mod;
    SDL_Keymod expected = hotkey.mod;

    SDL_Keymod mods_normalized = (SDL_Keymod)0;
    if (mods & (KMOD_LCTRL | KMOD_RCTRL)) mods_normalized = (SDL_Keymod)(mods_normalized | KMOD_CTRL);
    if (mods & (KMOD_LSHIFT | KMOD_RSHIFT)) mods_normalized = (SDL_Keymod)(mods_normalized | KMOD_SHIFT);
    if (mods & (KMOD_LALT | KMOD_RALT)) mods_normalized = (SDL_Keymod)(mods_normalized | KMOD_ALT);
    if (mods & (KMOD_LGUI | KMOD_RGUI)) mods_normalized = (SDL_Keymod)(mods_normalized | KMOD_GUI);

    SDL_Keymod expected_normalized = (SDL_Keymod)0;
    if (expected & (KMOD_LCTRL | KMOD_RCTRL | KMOD_CTRL)) expected_normalized = (SDL_Keymod)(expected_normalized | KMOD_CTRL);
    if (expected & (KMOD_LSHIFT | KMOD_RSHIFT | KMOD_SHIFT)) expected_normalized = (SDL_Keymod)(expected_normalized | KMOD_SHIFT);
    if (expected & (KMOD_LALT | KMOD_RALT | KMOD_ALT)) expected_normalized = (SDL_Keymod)(expected_normalized | KMOD_ALT);
    if (expected & (KMOD_LGUI | KMOD_RGUI | KMOD_GUI)) expected_normalized = (SDL_Keymod)(expected_normalized | KMOD_GUI);

    return mods_normalized == expected_normalized;
}

static void sdl_events_shortcuts_gui(const SDL_Event* event)
{
    if (event->type != SDL_KEYDOWN)
        return;

    // Check special case hotkeys first
    if (check_hotkey(event, config_hotkeys[config_HotkeyIndex_Quit], false))
    {
        application_trigger_quit();
        return;
    }

    if (check_hotkey(event, config_hotkeys[config_HotkeyIndex_Fullscreen], false))
    {
        config_emulator.fullscreen = !config_emulator.fullscreen;
        application_trigger_fullscreen(config_emulator.fullscreen);
        return;
    }

    // Check slot selection hotkeys
    for (int i = 0; i < 5; i++)
    {
        if (check_hotkey(event, config_hotkeys[config_HotkeyIndex_SelectSlot1 + i], false))
        {
            config_emulator.save_slot = i;
            return;
        }
    }

    // Check all hotkeys mapped to gui shortcuts
    for (int i = 0; i < GUI_HOTKEY_MAP_COUNT; i++)
    {
        if (gui_hotkey_map[i].shortcut >= 0 && check_hotkey(event, config_hotkeys[gui_hotkey_map[i].config_index], gui_hotkey_map[i].allow_repeat))
        {
            gui_shortcut((gui_ShortCutEvent)gui_hotkey_map[i].shortcut);
            return;
        }
    }

    // Fixed hotkeys for debug copy/paste/select operations
    int key = event->key.keysym.scancode;
    SDL_Keymod mods = (SDL_Keymod)event->key.keysym.mod;

    if (event->key.repeat == 0 && key == SDL_SCANCODE_A && (mods & KMOD_CTRL))
    {
        gui_shortcut(gui_ShortcutDebugSelectAll);
        return;
    }

    if (event->key.repeat == 0 && key == SDL_SCANCODE_C && (mods & KMOD_CTRL))
    {
        gui_shortcut(gui_ShortcutDebugCopy);
        return;
    }

    if (event->key.repeat == 0 && key == SDL_SCANCODE_V && (mods & KMOD_CTRL))
    {
        gui_shortcut(gui_ShortcutDebugPaste);
        return;
    }

    // ESC to exit fullscreen
    if (event->key.repeat == 0 && key == SDL_SCANCODE_ESCAPE)
    {
        if (config_emulator.fullscreen && !config_emulator.always_show_menu)
        {
            config_emulator.fullscreen = false;
            application_trigger_fullscreen(false);
        }
    }
}

static void sdl_events_emu(void)
{
    if (input_updated || gui_in_use)
        return;
    input_updated = true;

    SDL_PumpEvents();

    int max_controller = config_input.turbo_tap ? GG_MAX_GAMEPADS : 1;

    for (int controller = 0; controller < max_controller; controller++)
    {
        Uint16 now = input_build_state(controller);
        Uint16 before = input_last_state[controller];

        if (now != before)
            input_apply_state(controller, before, now);

        input_last_state[controller] = now;

        input_check_gamepad_shortcuts(controller);
    }
}

static void sdl_add_gamepads(void)
{
    for (int i = 0; i < GG_MAX_GAMEPADS; i++)
    {
        if (IsValidPointer(application_gamepad[i]))
        {
            SDL_Joystick* js = SDL_GameControllerGetJoystick(application_gamepad[i]);

            if (!IsValidPointer(js) || SDL_JoystickGetAttached(js) == SDL_FALSE)
            {
                SDL_GameControllerClose(application_gamepad[i]);
                application_gamepad[i] = NULL;
                Debug("Game controller %d closed when adding a new gamepad", i);
            }
        }
    }

    bool player_connected[GG_MAX_GAMEPADS] = { };

    for (int i = 0; i < GG_MAX_GAMEPADS; i++)
    {
        player_connected[i] = IsValidPointer(application_gamepad[i]);
    }

    bool all_slots_filled = true;
    for (int i = 0; i < GG_MAX_GAMEPADS; i++)
    {
        if (!player_connected[i])
        {
            all_slots_filled = false;
            break;
        }
    }
    if (all_slots_filled)
        return;

    for (int i = 0; i < SDL_NumJoysticks(); i++)
    {
        if (!SDL_IsGameController(i))
            continue;

        SDL_JoystickID joystick_id = SDL_JoystickGetDeviceInstanceID(i);

        bool already_assigned = false;
        for (int p = 0; p < GG_MAX_GAMEPADS; p++)
        {
            if (player_connected[p])
            {
                SDL_JoystickID player_id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(application_gamepad[p]));
                if (player_id == joystick_id)
                {
                    already_assigned = true;
                    break;
                }
            }
        }

        if (already_assigned)
            continue;

        SDL_GameController* controller = SDL_GameControllerOpen(i);
        if (!IsValidPointer(controller))
        {
            Log("Warning: Unable to open game controller %d!\n", i);
            SDL_ERROR("SDL_GameControllerOpen");
            continue;
        }

        bool assigned = false;
        for (int p = 0; p < GG_MAX_GAMEPADS; p++)
        {
            if (!player_connected[p])
            {
                application_gamepad[p] = controller;
                player_connected[p] = true;
                Debug("Game controller %d assigned to Player %d", i, p+1);
                assigned = true;
                break;
            }
        }

        if (!assigned)
        {
            SDL_GameControllerClose(controller);
            Debug("Game controller %d detected but all player slots are full", i);
        }

        all_slots_filled = true;
        for (int i = 0; i < GG_MAX_GAMEPADS; i++)
        {
            if (!player_connected[i])
            {
                all_slots_filled = false;
                break;
            }
        }
        if (all_slots_filled)
            break;
    }
}

static void sdl_remove_gamepad(SDL_JoystickID instance_id)
{
    for (int i = 0; i < GG_MAX_GAMEPADS; i++)
    {
        if (application_gamepad[i] != NULL)
        {
            SDL_JoystickID current_id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(application_gamepad[i]));
            if (current_id == instance_id)
            {
                SDL_GameControllerClose(application_gamepad[i]);
                application_gamepad[i] = NULL;
                Debug("Game controller %d disconnected from slot %d", instance_id, i);
                break;
            }
        }
    }
}

static Uint16 input_build_state(int controller)
{
    SDL_Keymod mods = SDL_GetModState();
    if (mods & (KMOD_CTRL | KMOD_SHIFT | KMOD_ALT | KMOD_GUI))
        return 0;

    const Uint8* keyboard_state = SDL_GetKeyboardState(NULL);
    Uint16 ret = 0;

    if (keyboard_state[config_input_keyboard[controller].key_left])
        ret |= GG_KEY_LEFT;
    if (keyboard_state[config_input_keyboard[controller].key_right])
        ret |= GG_KEY_RIGHT;
    if (keyboard_state[config_input_keyboard[controller].key_up])
        ret |= GG_KEY_UP;
    if (keyboard_state[config_input_keyboard[controller].key_down])
        ret |= GG_KEY_DOWN;
    if (keyboard_state[config_input_keyboard[controller].key_I])
        ret |= GG_KEY_I;
    if (keyboard_state[config_input_keyboard[controller].key_II])
        ret |= GG_KEY_II;
    if (keyboard_state[config_input_keyboard[controller].key_III])
        ret |= GG_KEY_III;
    if (keyboard_state[config_input_keyboard[controller].key_IV])
        ret |= GG_KEY_IV;
    if (keyboard_state[config_input_keyboard[controller].key_V])
        ret |= GG_KEY_V;
    if (keyboard_state[config_input_keyboard[controller].key_VI])
        ret |= GG_KEY_VI;
    if (keyboard_state[config_input_keyboard[controller].key_run])
        ret |= GG_KEY_RUN;
    if (keyboard_state[config_input_keyboard[controller].key_select])
        ret |= GG_KEY_SELECT;

    bool kb_turbo_I  = keyboard_state[config_input_keyboard[controller].key_toggle_turbo_I] != 0;
    bool kb_turbo_II = keyboard_state[config_input_keyboard[controller].key_toggle_turbo_II] != 0;
    bool gp_turbo_I = false;
    bool gp_turbo_II = false;

    SDL_GameController* sdl_controller = application_gamepad[controller];

    if (IsValidPointer(sdl_controller))
    {
        if (input_get_button(sdl_controller, config_input_gamepad[controller].gamepad_I))
            ret |= GG_KEY_I;
        if (input_get_button(sdl_controller, config_input_gamepad[controller].gamepad_II))
            ret |= GG_KEY_II;
        if (input_get_button(sdl_controller, config_input_gamepad[controller].gamepad_III))
            ret |= GG_KEY_III;
        if (input_get_button(sdl_controller, config_input_gamepad[controller].gamepad_IV))
            ret |= GG_KEY_IV;
        if (input_get_button(sdl_controller, config_input_gamepad[controller].gamepad_V))
            ret |= GG_KEY_V;
        if (input_get_button(sdl_controller, config_input_gamepad[controller].gamepad_VI))
            ret |= GG_KEY_VI;
        if (input_get_button(sdl_controller, config_input_gamepad[controller].gamepad_run))
            ret |= GG_KEY_RUN;
        if (input_get_button(sdl_controller, config_input_gamepad[controller].gamepad_select))
            ret |= GG_KEY_SELECT;

        gp_turbo_I  = input_get_button(sdl_controller, config_input_gamepad[controller].gamepad_toggle_turbo_I);
        gp_turbo_II = input_get_button(sdl_controller, config_input_gamepad[controller].gamepad_toggle_turbo_II);

        // Use D-Pad
        if (config_input_gamepad[controller].gamepad_directional == 0)
        {
            if (SDL_GameControllerGetButton(sdl_controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT))
                ret |= GG_KEY_LEFT;
            if (SDL_GameControllerGetButton(sdl_controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT))
                ret |= GG_KEY_RIGHT;
            if (SDL_GameControllerGetButton(sdl_controller, SDL_CONTROLLER_BUTTON_DPAD_UP))
                ret |= GG_KEY_UP;
            if (SDL_GameControllerGetButton(sdl_controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN))
                ret |= GG_KEY_DOWN;
        }
        // Use analog sticks
        else
        {
            const Sint16 STICK_DEAD_ZONE = 8000;
            const Sint16 rawx = SDL_GameControllerGetAxis(sdl_controller, (SDL_GameControllerAxis)config_input_gamepad[controller].gamepad_x_axis);
            const Sint16 rawy = SDL_GameControllerGetAxis(sdl_controller, (SDL_GameControllerAxis)config_input_gamepad[controller].gamepad_y_axis);

            const Sint16 x = config_input_gamepad[controller].gamepad_invert_x_axis ? -rawx : rawx;
            const Sint16 y = config_input_gamepad[controller].gamepad_invert_y_axis ? -rawy : rawy;

            if (x < -STICK_DEAD_ZONE)
                ret |= GG_KEY_LEFT;
            else if (x > STICK_DEAD_ZONE)
                ret |= GG_KEY_RIGHT;

            if (y < -STICK_DEAD_ZONE)
                ret |= GG_KEY_UP;
            else if (y > STICK_DEAD_ZONE)
                ret |= GG_KEY_DOWN;
        }
    }

    bool pressed_turbo_I  = kb_turbo_I || gp_turbo_I;
    bool pressed_turbo_II = kb_turbo_II || gp_turbo_II;

    if (pressed_turbo_I && !input_turbo_toggle_prev[controller][0])
    {
        config_input.turbo_enabled[controller][0] = !config_input.turbo_enabled[controller][0];
        emu_set_turbo((GG_Controllers)controller, GG_KEY_I, config_input.turbo_enabled[controller][0]);
    }
    if (pressed_turbo_II && !input_turbo_toggle_prev[controller][1])
    {
        config_input.turbo_enabled[controller][1] = !config_input.turbo_enabled[controller][1];
        emu_set_turbo((GG_Controllers)controller, GG_KEY_II, config_input.turbo_enabled[controller][1]);
    }

    input_turbo_toggle_prev[controller][0] = pressed_turbo_I;
    input_turbo_toggle_prev[controller][1] = pressed_turbo_II;

    return ret;
}

static void input_apply_state(int controller, Uint16 before, Uint16 now)
{
    Uint16 pressed  = now & (Uint16)(~before);
    Uint16 released = before & (Uint16)(~now);

    if ((pressed | released) == 0)
        return;

    static const Uint16 keys[12] = {
        GG_KEY_LEFT, GG_KEY_RIGHT, GG_KEY_UP, GG_KEY_DOWN,
        GG_KEY_I, GG_KEY_II, GG_KEY_III, GG_KEY_IV,
        GG_KEY_V, GG_KEY_VI, GG_KEY_RUN, GG_KEY_SELECT
    };

    for (unsigned i = 0; i < 12; i++)
    {
        Uint16 key = keys[i];
        if (pressed & key)  emu_key_pressed((GG_Controllers)controller, (GG_Keys)key);
        if (released & key) emu_key_released((GG_Controllers)controller, (GG_Keys)key);
    }
}

static void input_check_gamepad_shortcuts(int controller)
{
    SDL_GameController* sdl_controller = application_gamepad[controller];
    if (!IsValidPointer(sdl_controller))
        return;

    for (int i = 0; i < config_HotkeyIndex_COUNT; i++)
    {
        int button_mapping = config_input_gamepad_shortcuts[controller].gamepad_shortcuts[i];
        if (button_mapping == SDL_CONTROLLER_BUTTON_INVALID)
            continue;

        bool button_pressed = input_get_button(sdl_controller, button_mapping);

        if (button_pressed && !input_gamepad_shortcut_prev[controller][i])
        {
            if (i >= config_HotkeyIndex_SelectSlot1 && i <= config_HotkeyIndex_SelectSlot5)
            {
                config_emulator.save_slot = i - config_HotkeyIndex_SelectSlot1;
            }
            else
            {
                for (int j = 0; j < GUI_HOTKEY_MAP_COUNT; j++)
                {
                    if (gui_hotkey_map[j].config_index == i)
                    {
                        gui_shortcut((gui_ShortCutEvent)gui_hotkey_map[j].shortcut);
                        break;
                    }
                }
            }
        }

        input_gamepad_shortcut_prev[controller][i] = button_pressed;
    }
}

static bool input_get_button(SDL_GameController* controller, int mapping)
{
    if (!IsValidPointer(controller))
        return false;

    if (mapping >= 0 && mapping < SDL_CONTROLLER_BUTTON_MAX)
        return SDL_GameControllerGetButton(controller, (SDL_GameControllerButton)mapping) != 0;

    if (mapping >= GAMEPAD_VBTN_AXIS_BASE)
    {
        int axis = mapping - GAMEPAD_VBTN_AXIS_BASE;
        Sint16 value = SDL_GameControllerGetAxis(controller, (SDL_GameControllerAxis)axis);
        return value > GAMEPAD_VBTN_AXIS_THRESHOLD;
    }

    return false;
}

static void run_emulator(void)
{
    config_emulator.paused = emu_is_paused();
    emu_audio_sync = config_audio.sync;
    emu_update();

    if (!input_updated)
        sdl_events_emu();
    input_updated = false;
}

static void render(void)
{
    renderer_begin_render();
    ImGui_ImplSDL2_NewFrame();
    gui_render();
    renderer_render();
    renderer_end_render();

    SDL_GL_SwapWindow(application_sdl_window);
}

static void frame_throttle(void)
{
    if (emu_is_empty() || emu_is_paused() || emu_is_debug_idle() || !emu_is_audio_open() || config_emulator.ffwd)
    {
        Uint64 count_per_sec = SDL_GetPerformanceFrequency();
        float elapsed = (float)(frame_time_end - frame_time_start) / (float)count_per_sec;
        elapsed *= 1000.0f;

        float min = 16.666f;

        if (config_emulator.ffwd)
        {
            switch (config_emulator.ffwd_speed)
            {
                case 0:
                    min = 16.666f / 1.5f;
                    break;
                case 1: 
                    min = 16.666f / 2.0f;
                    break;
                case 2:
                    min = 16.666f / 2.5f;
                    break;
                case 3:
                    min = 16.666f / 3.0f;
                    break;
                default:
                    min = 0.0f;
            }
        }

        if (elapsed < min)
            SDL_Delay((Uint32)(min - elapsed));
    }
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
