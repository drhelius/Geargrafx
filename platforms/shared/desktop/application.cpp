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
#include <fstream>
#include <string>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "geargrafx.h"
#include "config.h"
#include "gui.h"
#include "gui_debug_disassembler.h"
#include "renderer.h"
#include "emu.h"
#include "utils.h"

#define APPLICATION_IMPORT
#include "application.h"

#if defined(GG_DEBUG)
#define WINDOW_TITLE GG_TITLE " " GG_VERSION " (DEBUG)"
#else
#define WINDOW_TITLE GG_TITLE " " GG_VERSION
#endif

static SDL_GLContext gl_context;
static bool running = true;
static bool paused_when_focus_lost = false;
static Uint64 frame_time_start;
static Uint64 frame_time_end;

static int sdl_init(void);
static void sdl_destroy(void);
static void sdl_load_gamepad_mappings(void);
static void sdl_events(void);
static void sdl_events_app(const SDL_Event* event);
static void sdl_events_emu(const SDL_Event* event);
static void sdl_shortcuts_gui(const SDL_Event* event);
static void sdl_add_gamepads(void);
static void sdl_remove_gamepad(SDL_JoystickID instance_id);
static void handle_mouse_cursor(void);
static void run_emulator(void);
static void render(void);
static void frame_throttle(void);
static void save_window_size(void);

int application_init(const char* rom_file, const char* symbol_file, bool force_fullscreen, bool force_windowed)
{
    Log("\n%s", GG_TITLE_ASCII);
    Log("%s %s Desktop App", GG_TITLE, GG_VERSION);

    config_init();
    config_read();

    if (force_fullscreen)
    {
        config_emulator.fullscreen = true;
        config_emulator.show_menu = false;
    }
    else if (force_windowed)
    {
        config_emulator.fullscreen = false;
        config_emulator.show_menu = true;
    }

    int ret = sdl_init();
    emu_init();

    gui_init();

    ImGui_ImplSDL2_InitForOpenGL(application_sdl_window, gl_context);

    renderer_init();

    SDL_GL_SetSwapInterval(config_video.sync ? 1 : 0);

    if (config_emulator.fullscreen)
        application_trigger_fullscreen(true);

    if (IsValidPointer(rom_file) && (strlen(rom_file) > 0))
    {
        Debug("Rom file argument: %s", rom_file);
        gui_load_rom(rom_file);
    }
    if (IsValidPointer(symbol_file) && (strlen(symbol_file) > 0))
    {
        Debug("Symbol file argument: %s", symbol_file);
        gui_debug_reset_symbols();
        gui_debug_load_symbols_file(symbol_file);
    }

    return ret;
}

void application_destroy(void)
{
    save_window_size();
    config_write();
    emu_destroy();
    config_destroy();
    renderer_destroy();
    ImGui_ImplSDL2_Shutdown();
    gui_destroy();
    sdl_destroy();
}

void application_mainloop(void)
{
    while (running)
    {
        frame_time_start = SDL_GetPerformanceCounter();
        sdl_events();
        handle_mouse_cursor();
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

void application_trigger_fullscreen(bool fullscreen)
{
    SDL_SetWindowFullscreen(application_sdl_window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    if (config_debug.debug)
        config_emulator.show_menu = true;
    else
        config_emulator.show_menu = !fullscreen;
}

void application_trigger_fit_to_content(int width, int height)
{
    SDL_SetWindowSize(application_sdl_window, width, height);
}

void application_update_title_with_rom(const char* rom)
{
    char final_title[256];
    snprintf(final_title, 256, "%s - %s", WINDOW_TITLE, rom);
    SDL_SetWindowTitle(application_sdl_window, final_title);
}

static int sdl_init(void)
{
    Debug("Initializing SDL...");

    for (int i = 0; i < GG_MAX_GAMEPADS; i++)
        InitPointer(application_gamepad[i]);

#if defined(_WIN32)
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "1");
#endif
    
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        Log("ERROR: %s\n", SDL_GetError());
        return 1;
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
    gl_context = SDL_GL_CreateContext(application_sdl_window);
    SDL_GL_MakeCurrent(application_sdl_window, gl_context);
    SDL_GL_SetSwapInterval(0);

    SDL_SetWindowMinimumSize(application_sdl_window, 500, 300);

    sdl_load_gamepad_mappings();
    sdl_add_gamepads();

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

    return 0;
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
    char exe_path[1024] = {0};
    get_executable_path(exe_path, sizeof(exe_path));

    if (exe_path[0] != '\0') {
        db_path = std::string(exe_path) + "/gamecontrollerdb.txt";
    } else {
        db_path = "gamecontrollerdb.txt";
    }

    std::ifstream file(db_path);
    if (!file.is_open()) {
        file.open("gamecontrollerdb.txt");
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
                Log("ERROR: Unable to load game controller mapping in line %d from gamecontrollerdb.txt", line_number);
                Log("SDL: %s", SDL_GetError());
            }
            line_number++;
        }
        file.close();
    }
    else
    {
        Log("ERROR: Game controller database not found (gamecontrollerdb.txt)!!");
        return;
    }
    Log("Added %d new game controller mappings from gamecontrollerdb.txt", added_mappings);
    Log("Updated %d game controller mappings from gamecontrollerdb.txt", updated_mappings);
    application_added_gamepad_mappings = added_mappings;
    application_updated_gamepad_mappings = updated_mappings;
}

static void handle_mouse_cursor(void)
{
    bool hide_cursor = false;

    if (gui_main_window_hovered && !config_debug.debug)
        hide_cursor = true;

    if (!config_emulator.show_menu && !config_debug.debug)
        hide_cursor = true;

    if (hide_cursor)
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);
    else
        ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
}

static void sdl_events(void)
{
    SDL_Event event;
        
    while (SDL_PollEvent(&event))
    {
        sdl_events_app(&event);

        if (running)
        {
            ImGui_ImplSDL2_ProcessEvent(&event);

            if (!gui_in_use)
            {
                sdl_events_emu(&event);
                sdl_shortcuts_gui(&event);
            }
        }
    }
}

static void sdl_events_app(const SDL_Event* event)
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

    switch (event->type)
    {
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

static void sdl_events_emu(const SDL_Event* event)
{
    switch(event->type)
    {
         case (SDL_DROPFILE):
        {
            char* dropped_filedir = event->drop.file;
            gui_load_rom(dropped_filedir);
            SDL_free(dropped_filedir);
            SDL_SetWindowInputFocus(application_sdl_window);
        }
        break;

        case SDL_WINDOWEVENT:
        {
            switch (event->window.event)
            {
                case SDL_WINDOWEVENT_FOCUS_GAINED:
                {
                    if (!paused_when_focus_lost)
                        emu_resume();
                }
                break;

                case SDL_WINDOWEVENT_FOCUS_LOST:
                {
                    paused_when_focus_lost = emu_is_paused();
                    emu_pause();
                }
                break;
            }
        }
        break;

        case SDL_CONTROLLERBUTTONDOWN:
        {
            for (int i = 0; i < GG_MAX_GAMEPADS; i++)
            {
                if (!IsValidPointer(application_gamepad[i]))
                    continue;

                GG_Controllers controller = (GG_Controllers)i;
                SDL_JoystickID id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(application_gamepad[i]));

                if (event->cbutton.which != id)
                    continue;
                
                if (event->cbutton.button == config_input_gamepad[i].gamepad_I)
                    emu_key_pressed(controller, GG_KEY_I);
                else if (event->cbutton.button == config_input_gamepad[i].gamepad_II)
                    emu_key_pressed(controller, GG_KEY_II);
                else if (event->cbutton.button == config_input_gamepad[i].gamepad_III)
                    emu_key_pressed(controller, GG_KEY_III);
                else if (event->cbutton.button == config_input_gamepad[i].gamepad_IV)
                    emu_key_pressed(controller, GG_KEY_IV);
                else if (event->cbutton.button == config_input_gamepad[i].gamepad_V)
                    emu_key_pressed(controller, GG_KEY_V);
                else if (event->cbutton.button == config_input_gamepad[i].gamepad_VI)
                    emu_key_pressed(controller, GG_KEY_VI);
                else if (event->cbutton.button == config_input_gamepad[i].gamepad_run)
                    emu_key_pressed(controller, GG_KEY_RUN);
                else if (event->cbutton.button == config_input_gamepad[i].gamepad_select)
                    emu_key_pressed(controller, GG_KEY_SELECT);

                if (config_input_gamepad[i].gamepad_directional == 1)
                    continue;
                
                if (event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP)
                    emu_key_pressed(controller, GG_KEY_UP);
                else if (event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN)
                    emu_key_pressed(controller, GG_KEY_DOWN);
                else if (event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT)
                    emu_key_pressed(controller, GG_KEY_LEFT);
                else if (event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
                    emu_key_pressed(controller, GG_KEY_RIGHT);
            }
        }
        break;

        case SDL_CONTROLLERBUTTONUP:
        {
            for (int i = 0; i < GG_MAX_GAMEPADS; i++)
            {
                if (!IsValidPointer(application_gamepad[i]))
                    continue;

                GG_Controllers controller = (GG_Controllers)i;
                SDL_JoystickID id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(application_gamepad[i]));

                if (event->cbutton.which != id)
                    continue;
                
                if (event->cbutton.button == config_input_gamepad[i].gamepad_I)
                    emu_key_released(controller, GG_KEY_I);
                else if (event->cbutton.button == config_input_gamepad[i].gamepad_II)
                    emu_key_released(controller, GG_KEY_II);
                else if (event->cbutton.button == config_input_gamepad[i].gamepad_III)
                    emu_key_released(controller, GG_KEY_III);
                else if (event->cbutton.button == config_input_gamepad[i].gamepad_IV)
                    emu_key_released(controller, GG_KEY_IV);
                else if (event->cbutton.button == config_input_gamepad[i].gamepad_V)
                    emu_key_released(controller, GG_KEY_V);
                else if (event->cbutton.button == config_input_gamepad[i].gamepad_VI)
                    emu_key_released(controller, GG_KEY_VI);
                else if (event->cbutton.button == config_input_gamepad[i].gamepad_run)
                    emu_key_released(controller, GG_KEY_RUN);
                else if (event->cbutton.button == config_input_gamepad[i].gamepad_select)
                    emu_key_released(controller, GG_KEY_SELECT);

                if (config_input_gamepad[i].gamepad_directional == 1)
                    continue;
                
                if (event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP)
                    emu_key_released(controller, GG_KEY_UP);
                else if (event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN)
                    emu_key_released(controller, GG_KEY_DOWN);
                else if (event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT)
                    emu_key_released(controller, GG_KEY_LEFT);
                else if (event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
                    emu_key_released(controller, GG_KEY_RIGHT);
            }
        }
        break;

        case SDL_CONTROLLERAXISMOTION:
        {
            for (int i = 0; i < GG_MAX_GAMEPADS; i++)
            {
                if (!IsValidPointer(application_gamepad[i]))
                    continue;

                GG_Controllers controller = (GG_Controllers)i;
                SDL_JoystickID id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(application_gamepad[i]));

                if (config_input_gamepad[i].gamepad_directional == 0)
                    continue;

                if (event->caxis.which != id)
                    continue;

                const int STICK_DEAD_ZONE = 8000;
                    
                if(event->caxis.axis == config_input_gamepad[i].gamepad_x_axis)
                {
                    int x_motion = event->caxis.value * (config_input_gamepad[i].gamepad_invert_x_axis ? -1 : 1);

                    if (x_motion < -STICK_DEAD_ZONE)
                        emu_key_pressed(controller, GG_KEY_LEFT);
                    else if (x_motion > STICK_DEAD_ZONE)
                        emu_key_pressed(controller, GG_KEY_RIGHT);
                    else
                    {
                        emu_key_released(controller, GG_KEY_LEFT);
                        emu_key_released(controller, GG_KEY_RIGHT);
                    }
                }
                else if(event->caxis.axis == config_input_gamepad[i].gamepad_y_axis)
                {
                    int y_motion = event->caxis.value * (config_input_gamepad[i].gamepad_invert_y_axis ? -1 : 1);

                    if (y_motion < -STICK_DEAD_ZONE)
                        emu_key_pressed(controller, GG_KEY_UP);
                    else if (y_motion > STICK_DEAD_ZONE)
                        emu_key_pressed(controller, GG_KEY_DOWN);
                    else
                    {
                        emu_key_released(controller, GG_KEY_UP);
                        emu_key_released(controller, GG_KEY_DOWN);
                    }
                }
            }
        }
        break;

        case SDL_KEYDOWN:
        {
            if (event->key.repeat != 0)
                break;

            if (event->key.keysym.mod & KMOD_CTRL)
                break;
            if (event->key.keysym.mod & KMOD_SHIFT)
                break;

            int key = event->key.keysym.scancode;

            if (key == SDL_SCANCODE_ESCAPE)
            {
                application_trigger_fullscreen(false);
                break;
            }

            for (int i = 0; i < GG_MAX_GAMEPADS; i++)
            {
                GG_Controllers controller = (GG_Controllers)i;

                if (key == config_input_keyboard[i].key_left)
                    emu_key_pressed(controller, GG_KEY_LEFT);
                else if (key == config_input_keyboard[i].key_right)
                    emu_key_pressed(controller, GG_KEY_RIGHT);
                else if (key == config_input_keyboard[i].key_up)
                    emu_key_pressed(controller, GG_KEY_UP);
                else if (key == config_input_keyboard[i].key_down)
                    emu_key_pressed(controller, GG_KEY_DOWN);
                else if (key == config_input_keyboard[i].key_I)
                    emu_key_pressed(controller, GG_KEY_I);
                else if (key == config_input_keyboard[i].key_II)
                    emu_key_pressed(controller, GG_KEY_II);
                else if (key == config_input_keyboard[i].key_III)
                    emu_key_pressed(controller, GG_KEY_III);
                else if (key == config_input_keyboard[i].key_IV)
                    emu_key_pressed(controller, GG_KEY_IV);
                else if (key == config_input_keyboard[i].key_V)
                    emu_key_pressed(controller, GG_KEY_V);
                else if (key == config_input_keyboard[i].key_VI)
                    emu_key_pressed(controller, GG_KEY_VI);
                else if (key == config_input_keyboard[i].key_run)
                    emu_key_pressed(controller, GG_KEY_RUN);
                else if (key == config_input_keyboard[i].key_select)
                    emu_key_pressed(controller, GG_KEY_SELECT);
            }
        }
        break;

        case SDL_KEYUP:
        {
            int key = event->key.keysym.scancode;

            for (int i = 0; i < GG_MAX_GAMEPADS; i++)
            {
                GG_Controllers controller = (GG_Controllers)i;

                if (key == config_input_keyboard[i].key_left)
                    emu_key_released(controller, GG_KEY_LEFT);
                else if (key == config_input_keyboard[i].key_right)
                    emu_key_released(controller, GG_KEY_RIGHT);
                else if (key == config_input_keyboard[i].key_up)
                    emu_key_released(controller, GG_KEY_UP);
                else if (key == config_input_keyboard[i].key_down)
                    emu_key_released(controller, GG_KEY_DOWN);
                else if (key == config_input_keyboard[i].key_I)
                    emu_key_released(controller, GG_KEY_I);
                else if (key == config_input_keyboard[i].key_II)
                    emu_key_released(controller, GG_KEY_II);
                else if (key == config_input_keyboard[i].key_III)
                    emu_key_released(controller, GG_KEY_III);
                else if (key == config_input_keyboard[i].key_IV)
                    emu_key_released(controller, GG_KEY_IV);
                else if (key == config_input_keyboard[i].key_V)
                    emu_key_released(controller, GG_KEY_V);
                else if (key == config_input_keyboard[i].key_VI)
                    emu_key_released(controller, GG_KEY_VI);
                else if (key == config_input_keyboard[i].key_run)
                    emu_key_released(controller, GG_KEY_RUN);
                else if (key == config_input_keyboard[i].key_select)
                    emu_key_released(controller, GG_KEY_SELECT);
            }
        }
        break;
    }
}

static void sdl_shortcuts_gui(const SDL_Event* event)
{
    if (event->type == SDL_KEYDOWN)
    {
        int key = event->key.keysym.scancode;

        switch (key)
        {
            case SDL_SCANCODE_Q:
                application_trigger_quit();
                break;
            case SDL_SCANCODE_A:
                if (event->key.keysym.mod & KMOD_CTRL)
                    gui_shortcut(gui_ShortcutDebugSelectAll);
                break;
            case SDL_SCANCODE_C:
                if (event->key.keysym.mod & KMOD_CTRL)
                    gui_shortcut(gui_ShortcutDebugCopy);
                break;
            case SDL_SCANCODE_V:
                if (event->key.keysym.mod & KMOD_CTRL)
                    gui_shortcut(gui_ShortcutDebugPaste);
                break;
            case SDL_SCANCODE_O:
                if (event->key.keysym.mod & KMOD_CTRL)
                    gui_shortcut(gui_ShortcutOpenROM);
                break;
            case SDL_SCANCODE_R:
                if (event->key.keysym.mod & KMOD_CTRL)
                    gui_shortcut(gui_ShortcutReset);
                break;
            case SDL_SCANCODE_P:
                if (event->key.keysym.mod & KMOD_CTRL)
                    gui_shortcut(gui_ShortcutPause);
                break;
            case SDL_SCANCODE_F:
                if (event->key.keysym.mod & KMOD_CTRL)
                    gui_shortcut(gui_ShortcutFFWD);
                break;
            case SDL_SCANCODE_L:
                if (event->key.keysym.mod & KMOD_CTRL)
                    gui_shortcut(gui_ShortcutLoadState);
                break;
            case SDL_SCANCODE_S:
                if (event->key.keysym.mod & KMOD_CTRL)
                    gui_shortcut(gui_ShortcutSaveState);
                break;
            case SDL_SCANCODE_X:
                if (event->key.keysym.mod & KMOD_CTRL)
                    gui_shortcut(gui_ShortcutScreenshot);
                break;
            case SDL_SCANCODE_M:
                if (event->key.keysym.mod & KMOD_CTRL)
                    gui_shortcut(gui_ShortcutShowMainMenu);
                break;
            case SDL_SCANCODE_F5:
                gui_shortcut(gui_ShortcutDebugContinue);
                break;
            case SDL_SCANCODE_F6:
                gui_shortcut(gui_ShortcutDebugStepFrame);
                break;
            case SDL_SCANCODE_F7:
                gui_shortcut(gui_ShortcutDebugBreak);
                break;
            case SDL_SCANCODE_F8:
                gui_shortcut(gui_ShortcutDebugRuntocursor);
                break;
            case SDL_SCANCODE_F9:
                gui_shortcut(gui_ShortcutDebugBreakpoint);
                break;
            case SDL_SCANCODE_F10:
                gui_shortcut(gui_ShortcutDebugStepOver);
                break;
            case SDL_SCANCODE_F11:
                if (event->key.keysym.mod & KMOD_SHIFT)
                    gui_shortcut(gui_ShortcutDebugStepOut);
                else
                    gui_shortcut(gui_ShortcutDebugStepInto);
                break;
            case SDL_SCANCODE_F12:
                config_emulator.fullscreen = !config_emulator.fullscreen;
                application_trigger_fullscreen(config_emulator.fullscreen);
                break;
            case SDL_SCANCODE_BACKSPACE:
                if (event->key.keysym.mod & KMOD_CTRL)
                    gui_shortcut(gui_ShortcutDebugGoBack);
                break;
        }
    }
}

static void sdl_add_gamepads(void)
{
    bool player_connected[GG_MAX_GAMEPADS] = {false};

    for (int i = 0; i < GG_MAX_GAMEPADS; i++)
    {
        player_connected[i] = IsValidPointer(application_gamepad[i]);
        config_input_gamepad[i].detected = player_connected[i];
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
            Log("Warning: Unable to open game controller %d! SDL Error: %s\n", i, SDL_GetError());
            continue;
        }

        bool assigned = false;
        for (int p = 0; p < GG_MAX_GAMEPADS; p++)
        {
            if (!player_connected[p])
            {
                application_gamepad[p] = controller;
                player_connected[p] = true;
                config_input_gamepad[p].detected = true;
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
                config_input_gamepad[i].detected = false;
                Debug("Game controller %d disconnected from slot %d", instance_id, i);
                break;
            }
        }
    }
}

static void run_emulator(void)
{
    config_emulator.paused = emu_is_paused();
    emu_audio_sync = config_audio.sync;
    emu_update();
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
