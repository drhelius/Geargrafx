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
#include <iomanip>
#include "geargrafx.h"

#define MINI_CASE_SENSITIVE
#include "ini.h"

#define CONFIG_IMPORT
#include "config.h"

static bool check_portable(void);
static int read_int(const char* group, const char* key, int default_value);
static void write_int(const char* group, const char* key, int integer);
static float read_float(const char* group, const char* key, float default_value);
static void write_float(const char* group, const char* key, float value);
static bool read_bool(const char* group, const char* key, bool default_value);
static void write_bool(const char* group, const char* key, bool boolean);
static std::string read_string(const char* group, const char* key);
static void write_string(const char* group, const char* key, std::string value);
static config_Hotkey read_hotkey(const char* group, const char* key, config_Hotkey default_value);
static void write_hotkey(const char* group, const char* key, config_Hotkey hotkey);
static config_Hotkey make_hotkey(SDL_Scancode key, SDL_Keymod mod);

void config_init(void)
{
    if (check_portable())
        config_root_path = SDL_GetBasePath();
    else
        config_root_path = SDL_GetPrefPath("Geardome", GG_TITLE);

    strncpy_fit(config_temp_path, config_root_path, sizeof(config_temp_path));
    strncat_fit(config_temp_path, "tmp/", sizeof(config_temp_path));
    create_directory_if_not_exists(config_temp_path);

    strncpy_fit(config_emu_file_path, config_root_path, sizeof(config_emu_file_path));
    strncat_fit(config_emu_file_path, "config.ini", sizeof(config_emu_file_path));

    strncpy_fit(config_imgui_file_path, config_root_path, sizeof(config_imgui_file_path));
    strncat_fit(config_imgui_file_path, "imgui.ini", sizeof(config_imgui_file_path));

    config_input_keyboard[0].key_left = SDL_SCANCODE_LEFT;
    config_input_keyboard[0].key_right = SDL_SCANCODE_RIGHT;
    config_input_keyboard[0].key_up = SDL_SCANCODE_UP;
    config_input_keyboard[0].key_down = SDL_SCANCODE_DOWN;
    config_input_keyboard[0].key_select = SDL_SCANCODE_A;
    config_input_keyboard[0].key_run = SDL_SCANCODE_S;
    config_input_keyboard[0].key_I = SDL_SCANCODE_X;
    config_input_keyboard[0].key_II = SDL_SCANCODE_Z;
    config_input_keyboard[0].key_III = SDL_SCANCODE_C;
    config_input_keyboard[0].key_IV = SDL_SCANCODE_V;
    config_input_keyboard[0].key_V = SDL_SCANCODE_B;
    config_input_keyboard[0].key_VI = SDL_SCANCODE_N;
    config_input_keyboard[0].key_toggle_turbo_I = SDL_SCANCODE_W;
    config_input_keyboard[0].key_toggle_turbo_II = SDL_SCANCODE_Q;

    config_input_keyboard[1].key_left = SDL_SCANCODE_J;
    config_input_keyboard[1].key_right = SDL_SCANCODE_L;
    config_input_keyboard[1].key_up = SDL_SCANCODE_I;
    config_input_keyboard[1].key_down = SDL_SCANCODE_K;
    config_input_keyboard[1].key_select = SDL_SCANCODE_G;
    config_input_keyboard[1].key_run = SDL_SCANCODE_H;
    config_input_keyboard[1].key_I = SDL_SCANCODE_Y;
    config_input_keyboard[1].key_II = SDL_SCANCODE_T;
    config_input_keyboard[1].key_III = SDL_SCANCODE_5;
    config_input_keyboard[1].key_IV = SDL_SCANCODE_6;
    config_input_keyboard[1].key_V = SDL_SCANCODE_7;
    config_input_keyboard[1].key_VI = SDL_SCANCODE_8;
    config_input_keyboard[1].key_toggle_turbo_I = SDL_SCANCODE_P;
    config_input_keyboard[1].key_toggle_turbo_II = SDL_SCANCODE_O;

    for (int i = 2; i < GG_MAX_GAMEPADS; i++)
    {
        config_input_keyboard[i].key_left = SDL_SCANCODE_UNKNOWN;
        config_input_keyboard[i].key_right = SDL_SCANCODE_UNKNOWN;
        config_input_keyboard[i].key_up = SDL_SCANCODE_UNKNOWN;
        config_input_keyboard[i].key_down = SDL_SCANCODE_UNKNOWN;
        config_input_keyboard[i].key_select = SDL_SCANCODE_UNKNOWN;
        config_input_keyboard[i].key_run = SDL_SCANCODE_UNKNOWN;
        config_input_keyboard[i].key_I = SDL_SCANCODE_UNKNOWN;
        config_input_keyboard[i].key_II = SDL_SCANCODE_UNKNOWN;
        config_input_keyboard[i].key_III = SDL_SCANCODE_UNKNOWN;
        config_input_keyboard[i].key_IV = SDL_SCANCODE_UNKNOWN;
        config_input_keyboard[i].key_V = SDL_SCANCODE_UNKNOWN;
        config_input_keyboard[i].key_VI = SDL_SCANCODE_UNKNOWN;
        config_input_keyboard[i].key_toggle_turbo_I = SDL_SCANCODE_UNKNOWN;
        config_input_keyboard[i].key_toggle_turbo_II = SDL_SCANCODE_UNKNOWN;
    }

    for (int i = 0; i < GG_MAX_GAMEPADS; i++)
    {
        config_input.controller_type[i] = 0;
        config_input.avenue_pad_3_button[i] = 0;
        for (int j = 0; j < 2; j++)
        {
            config_input.turbo_enabled[i][j] = false;
            config_input.turbo_speed[i][j] = 4;
        }

        config_input_gamepad[i].gamepad_directional = 0;
        config_input_gamepad[i].gamepad_invert_x_axis = false;
        config_input_gamepad[i].gamepad_invert_y_axis = false;
        config_input_gamepad[i].gamepad_select = SDL_CONTROLLER_BUTTON_BACK;
        config_input_gamepad[i].gamepad_run = SDL_CONTROLLER_BUTTON_START;
        config_input_gamepad[i].gamepad_I = SDL_CONTROLLER_BUTTON_A;
        config_input_gamepad[i].gamepad_II = SDL_CONTROLLER_BUTTON_B;
        config_input_gamepad[i].gamepad_III = SDL_CONTROLLER_BUTTON_Y;
        config_input_gamepad[i].gamepad_IV = SDL_CONTROLLER_BUTTON_X;
        config_input_gamepad[i].gamepad_V = SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
        config_input_gamepad[i].gamepad_VI = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
        config_input_gamepad[i].gamepad_x_axis = SDL_CONTROLLER_AXIS_LEFTX;
        config_input_gamepad[i].gamepad_y_axis = SDL_CONTROLLER_AXIS_LEFTY;
        config_input_gamepad[i].gamepad_toggle_turbo_I = SDL_CONTROLLER_BUTTON_RIGHTSTICK;
        config_input_gamepad[i].gamepad_toggle_turbo_II = SDL_CONTROLLER_BUTTON_LEFTSTICK;

        for (int j = 0; j < config_HotkeyIndex_COUNT; j++)
        {
            config_input_gamepad_shortcuts[i].gamepad_shortcuts[j] = SDL_CONTROLLER_BUTTON_INVALID;
        }
    }

    config_hotkeys[config_HotkeyIndex_OpenROM] = make_hotkey(SDL_SCANCODE_O, KMOD_CTRL);
    config_hotkeys[config_HotkeyIndex_Quit] = make_hotkey(SDL_SCANCODE_Q, KMOD_CTRL);
    config_hotkeys[config_HotkeyIndex_Reset] = make_hotkey(SDL_SCANCODE_R, KMOD_CTRL);
    config_hotkeys[config_HotkeyIndex_Pause] = make_hotkey(SDL_SCANCODE_P, KMOD_CTRL);
    config_hotkeys[config_HotkeyIndex_FFWD] = make_hotkey(SDL_SCANCODE_F, KMOD_CTRL);
    config_hotkeys[config_HotkeyIndex_SaveState] = make_hotkey(SDL_SCANCODE_S, KMOD_CTRL);
    config_hotkeys[config_HotkeyIndex_LoadState] = make_hotkey(SDL_SCANCODE_L, KMOD_CTRL);
    config_hotkeys[config_HotkeyIndex_Screenshot] = make_hotkey(SDL_SCANCODE_X, KMOD_CTRL);
    config_hotkeys[config_HotkeyIndex_Fullscreen] = make_hotkey(SDL_SCANCODE_F12, KMOD_NONE);
    config_hotkeys[config_HotkeyIndex_ShowMainMenu] = make_hotkey(SDL_SCANCODE_M, KMOD_CTRL);
    config_hotkeys[config_HotkeyIndex_DebugStepInto] = make_hotkey(SDL_SCANCODE_F11, KMOD_NONE);
    config_hotkeys[config_HotkeyIndex_DebugStepOver] = make_hotkey(SDL_SCANCODE_F10, KMOD_NONE);
    config_hotkeys[config_HotkeyIndex_DebugStepOut] = make_hotkey(SDL_SCANCODE_F11, KMOD_SHIFT);
    config_hotkeys[config_HotkeyIndex_DebugStepFrame] = make_hotkey(SDL_SCANCODE_F6, KMOD_NONE);
    config_hotkeys[config_HotkeyIndex_DebugContinue] = make_hotkey(SDL_SCANCODE_F5, KMOD_NONE);
    config_hotkeys[config_HotkeyIndex_DebugBreak] = make_hotkey(SDL_SCANCODE_F7, KMOD_NONE);
    config_hotkeys[config_HotkeyIndex_DebugRunToCursor] = make_hotkey(SDL_SCANCODE_F8, KMOD_NONE);
    config_hotkeys[config_HotkeyIndex_DebugBreakpoint] = make_hotkey(SDL_SCANCODE_F9, KMOD_NONE);
    config_hotkeys[config_HotkeyIndex_DebugGoBack] = make_hotkey(SDL_SCANCODE_BACKSPACE, KMOD_CTRL);
    config_hotkeys[config_HotkeyIndex_SelectSlot1] = make_hotkey(SDL_SCANCODE_1, KMOD_CTRL);
    config_hotkeys[config_HotkeyIndex_SelectSlot2] = make_hotkey(SDL_SCANCODE_2, KMOD_CTRL);
    config_hotkeys[config_HotkeyIndex_SelectSlot3] = make_hotkey(SDL_SCANCODE_3, KMOD_CTRL);
    config_hotkeys[config_HotkeyIndex_SelectSlot4] = make_hotkey(SDL_SCANCODE_4, KMOD_CTRL);
    config_hotkeys[config_HotkeyIndex_SelectSlot5] = make_hotkey(SDL_SCANCODE_5, KMOD_CTRL);

    config_ini_file = new mINI::INIFile(config_emu_file_path);
}

void config_destroy(void)
{
    SafeDelete(config_ini_file)
    SDL_free(config_root_path);
}

void config_read(void)
{
    if (!config_ini_file->read(config_ini_data))
    {
        Log("Unable to load settings from %s", config_emu_file_path);
        return;
    }

    Log("Loading settings from %s", config_emu_file_path);

#if defined(GG_DISABLE_DISASSEMBLER)
        config_debug.debug = false;
#else
        config_debug.debug = read_bool("Debug", "Debug", false);
#endif
    config_debug.show_disassembler = read_bool("Debug", "Disassembler", true);
    config_debug.show_screen = read_bool("Debug", "Screen", true);
    config_debug.show_memory = read_bool("Debug", "Memory", false);
    config_debug.show_processor = read_bool("Debug", "Processor", true);
    config_debug.show_call_stack = read_bool("Debug", "CallStack", false);
    config_debug.show_huc6202_info = read_bool("Debug", "HuC6202Info", false);
    config_debug.show_huc6260_info = read_bool("Debug", "HuC6260Info", false);
    config_debug.show_huc6260_palettes = read_bool("Debug", "HuC6260Palettes", false);
    config_debug.show_huc6270_1_registers = read_bool("Debug", "HuC6270Registers1", false);
    config_debug.show_huc6270_1_background = read_bool("Debug", "HuC6270Background1", false);
    config_debug.show_huc6270_1_sprites = read_bool("Debug", "HuC6270Sprites1", false);
    config_debug.show_huc6270_1_info = read_bool("Debug", "HuC6270Info1", false);
    config_debug.show_huc6270_2_registers = read_bool("Debug", "HuC6270Registers2", false);
    config_debug.show_huc6270_2_background = read_bool("Debug", "HuC6270Background2", false);
    config_debug.show_huc6270_2_sprites = read_bool("Debug", "HuC6270Sprites2", false);
    config_debug.show_huc6270_2_info = read_bool("Debug", "HuC6270Info2", false);
    config_debug.show_psg = read_bool("Debug", "PSG", false);
    config_debug.show_cdrom = read_bool("Debug", "CDROM", false);
    config_debug.show_cdrom_audio = read_bool("Debug", "CDROMAudio", false);
    config_debug.show_adpcm = read_bool("Debug", "ADPCM", false);
    config_debug.show_arcade_card = read_bool("Debug", "ArcadeCard", false);
    config_debug.show_trace_logger = read_bool("Debug", "TraceLogger", false);
    config_debug.trace_counter = read_bool("Debug", "TraceCounter", true);
    config_debug.trace_bank = read_bool("Debug", "TraceBank", true);
    config_debug.trace_registers = read_bool("Debug", "TraceRegisters", true);
    config_debug.trace_flags = read_bool("Debug", "TraceFlags", true);
    config_debug.trace_bytes = read_bool("Debug", "TraceBytes", true);
    config_debug.dis_show_mem = read_bool("Debug", "DisMem", true);
    config_debug.dis_show_symbols = read_bool("Debug", "DisSymbols", true);
    config_debug.dis_show_segment = read_bool("Debug", "DisSegment", true);
    config_debug.dis_show_bank = read_bool("Debug", "DisBank", true);
    config_debug.dis_show_auto_symbols = read_bool("Debug", "DisAutoSymbols", true);
    config_debug.dis_replace_symbols = read_bool("Debug", "DisReplaceSymbols", true);
    config_debug.dis_replace_labels = read_bool("Debug", "DisReplaceLabels", true);
    config_debug.dis_look_ahead_count = read_int("Debug", "DisLookAheadCount", 20);
    config_debug.font_size = read_int("Debug", "FontSize", 0);
    config_debug.scale = read_int("Debug", "Scale", 1);
    config_debug.multi_viewport = read_bool("Debug", "MultiViewport", false);
    config_debug.reset_ram = read_int("Debug", "InitRam", 1);
    config_debug.reset_card_ram = read_int("Debug", "InitCardRam", 1);
    config_debug.reset_registers = read_int("Debug", "InitRegisters", 0);
    config_debug.reset_color_table = read_int("Debug", "InitColorTable", 0);
    config_debug.reset_mpr = read_int("Debug", "InitMPR", 0);
    config_debug.reset_arcade_card = read_int("Debug", "InitArcadeCard", 1);

    config_emulator.maximized = read_bool("Emulator", "Maximized", false);
    config_emulator.fullscreen = read_bool("Emulator", "FullScreen", false);
    config_emulator.always_show_menu = read_bool("Emulator", "AlwaysShowMenu", false);
    config_emulator.ffwd_speed = read_int("Emulator", "FFWD", 1);
    config_emulator.save_slot = read_int("Emulator", "SaveSlot", 0);
    config_emulator.start_paused = read_bool("Emulator", "StartPaused", false);
    config_emulator.pause_when_inactive = read_bool("Emulator", "PauseWhenInactive", true);
    config_emulator.savefiles_dir_option = read_int("Emulator", "SaveFilesDirOption", 0);
    config_emulator.savefiles_path = read_string("Emulator", "SaveFilesPath");
    config_emulator.savestates_dir_option = read_int("Emulator", "SaveStatesDirOption", 0);
    config_emulator.savestates_path = read_string("Emulator", "SaveStatesPath");
    config_emulator.screenshots_dir_option = read_int("Emulator", "ScreenshotDirOption", 0);
    config_emulator.screenshots_path = read_string("Emulator", "ScreenshotPath");
    config_emulator.backup_ram_dir_option = read_int("Emulator", "BackupRAMDirOption", 0);
    config_emulator.backup_ram_path = read_string("Emulator", "BackupRAMPath");
    config_emulator.mb128_dir_option = read_int("Emulator", "MB128DirOption", 0);
    config_emulator.mb128_path = read_string("Emulator", "MB128Path");
    config_emulator.mb128_mode = read_int("Emulator", "MB128Mode", 0);
    config_emulator.last_open_path = read_string("Emulator", "LastOpenPath");
    config_emulator.syscard_bios_path = read_string("Emulator", "SysCardBiosPath");
    config_emulator.gameexpress_bios_path = read_string("Emulator", "GameExpressBiosPath");
    config_emulator.window_width = read_int("Emulator", "WindowWidth", 770);
    config_emulator.window_height = read_int("Emulator", "WindowHeight", 600);
    config_emulator.status_messages = read_bool("Emulator", "StatusMessages", false);
    config_emulator.backup_ram = read_bool("Emulator", "BackupRAM", true);
    config_emulator.console_type = read_int("Emulator", "ConsoleType", 0);
    config_emulator.cdrom_type = read_int("Emulator", "CDROMType", 0);
    config_emulator.preload_cdrom = read_bool("Emulator", "PreloadCDROM", false);
    config_emulator.mcp_tcp_port = read_int("Emulator", "MCPTCPPort", 7777);

    if (config_emulator.savefiles_path.empty())
    {
        config_emulator.savefiles_path = config_root_path;
    }
    if (config_emulator.savestates_path.empty())
    {
        config_emulator.savestates_path = config_root_path;
    }
    if (config_emulator.screenshots_path.empty())
    {
        config_emulator.screenshots_path = config_root_path;
    }

    if (config_emulator.backup_ram_path.empty())
    {
        config_emulator.backup_ram_path = config_root_path;
    }

    if (config_emulator.mb128_path.empty())
    {
        config_emulator.mb128_path = config_root_path;
    }

    for (int i = 0; i < config_max_recent_roms; i++)
    {
        std::string item = "RecentROM" + std::to_string(i);
        config_emulator.recent_roms[i] = read_string("Emulator", item.c_str());
    }

    config_video.scale = read_int("Video", "Scale", 0);
    if (config_video.scale > 3)
        config_video.scale -= 2;
    config_video.scale_manual = read_int("Video", "ScaleManual", 1);
    config_video.ratio = read_int("Video", "AspectRatio", 1);
    config_video.overscan = read_int("Video", "Overscan", 0);
    config_video.scanline_mode = read_int("Video", "ScanlineMode", 0);
    config_video.scanline_start = read_int("Video", "ScanlineStart", 11);
    config_video.scanline_end = read_int("Video", "ScanlineEnd", 234);
    config_video.composite_palette = read_bool("Video", "CompositePalette", false);
    config_video.fps = read_bool("Video", "FPS", false);
    config_video.bilinear = read_bool("Video", "Bilinear", false);
    config_video.sprite_limit = read_bool("Video", "SpriteLimit", false);
    config_video.mix_frames = read_bool("Video", "MixFrames", true);
    config_video.mix_frames_intensity = read_float("Video", "MixFramesIntensity", 0.60f);
    config_video.scanlines = read_bool("Video", "Scanlines", true);
    config_video.scanlines_filter = read_bool("Video", "ScanlinesFilter", true);
    config_video.scanlines_intensity = read_float("Video", "ScanlinesIntensity", 0.10f);
    config_video.sync = read_bool("Video", "Sync", true);
    config_video.background_color[0] = read_float("Video", "BackgroundColorR", 0.1f);
    config_video.background_color[1] = read_float("Video", "BackgroundColorG", 0.1f);
    config_video.background_color[2] = read_float("Video", "BackgroundColorB", 0.1f);
    config_video.background_color_debugger[0] = read_float("Video", "BackgroundColorDebuggerR", 0.2f);
    config_video.background_color_debugger[1] = read_float("Video", "BackgroundColorDebuggerG", 0.2f);
    config_video.background_color_debugger[2] = read_float("Video", "BackgroundColorDebuggerB", 0.2f);

    config_audio.enable = read_bool("Audio", "Enable", true);
    config_audio.sync = read_bool("Audio", "Sync", true);
    config_audio.huc6280a = read_bool("Audio", "HuC6280A", true);
    config_audio.psg_volume = read_float("Audio", "PSGVolume", 1.0f);
    config_audio.cdrom_volume = read_float("Audio", "CDROMVolume", 1.0f);
    config_audio.adpcm_volume = read_float("Audio", "ADPCMVolume", 1.0f);

    config_input.turbo_tap = read_bool("Input", "TurboTap", false);

    for (int i = 0; i < GG_MAX_GAMEPADS; i++)
    {
        char input_group[32];
        snprintf(input_group, sizeof(input_group), "Input%d", i + 1);
        config_input.controller_type[i] = read_int(input_group, "ControllerType", 0);
        config_input.avenue_pad_3_button[i] = read_int(input_group, "AvenuePad3Button", 0);
        for (int j = 0; j < 2; j++)
        {
            char turbo_group[32];
            snprintf(turbo_group, sizeof(turbo_group), "TurboEnabled%d", j + 1);
            config_input.turbo_enabled[i][j] = read_bool(input_group, turbo_group, false);
            snprintf(turbo_group, sizeof(turbo_group), "TurboSpeed%d", j + 1);
            config_input.turbo_speed[i][j] = read_int(input_group, turbo_group, 4);
        }
    }

    config_input_keyboard[0].key_left = (SDL_Scancode)read_int("InputKeyboard1", "KeyLeft", SDL_SCANCODE_LEFT);
    config_input_keyboard[0].key_right = (SDL_Scancode)read_int("InputKeyboard1", "KeyRight", SDL_SCANCODE_RIGHT);
    config_input_keyboard[0].key_up = (SDL_Scancode)read_int("InputKeyboard1", "KeyUp", SDL_SCANCODE_UP);
    config_input_keyboard[0].key_down = (SDL_Scancode)read_int("InputKeyboard1", "KeyDown", SDL_SCANCODE_DOWN);
    config_input_keyboard[0].key_select = (SDL_Scancode)read_int("InputKeyboard1", "KeySelect", SDL_SCANCODE_A);
    config_input_keyboard[0].key_run = (SDL_Scancode)read_int("InputKeyboard1", "KeyRun", SDL_SCANCODE_S);
    config_input_keyboard[0].key_I = (SDL_Scancode)read_int("InputKeyboard1", "KeyI", SDL_SCANCODE_X);
    config_input_keyboard[0].key_II = (SDL_Scancode)read_int("InputKeyboard1", "KeyII", SDL_SCANCODE_Z);
    config_input_keyboard[0].key_III = (SDL_Scancode)read_int("InputKeyboard1", "KeyIII", SDL_SCANCODE_C);
    config_input_keyboard[0].key_IV = (SDL_Scancode)read_int("InputKeyboard1", "KeyIV", SDL_SCANCODE_V);
    config_input_keyboard[0].key_V = (SDL_Scancode)read_int("InputKeyboard1", "KeyV", SDL_SCANCODE_B);
    config_input_keyboard[0].key_VI = (SDL_Scancode)read_int("InputKeyboard1", "KeyVI", SDL_SCANCODE_N);
    config_input_keyboard[0].key_toggle_turbo_I = (SDL_Scancode)read_int("InputKeyboard1", "KeyToogleTurboI", SDL_SCANCODE_W);
    config_input_keyboard[0].key_toggle_turbo_II = (SDL_Scancode)read_int("InputKeyboard1", "KeyToogleTurboII", SDL_SCANCODE_Q);

    config_input_keyboard[1].key_left = (SDL_Scancode)read_int("InputKeyboard2", "KeyLeft", SDL_SCANCODE_J);
    config_input_keyboard[1].key_right = (SDL_Scancode)read_int("InputKeyboard2", "KeyRight", SDL_SCANCODE_L);
    config_input_keyboard[1].key_up = (SDL_Scancode)read_int("InputKeyboard2", "KeyUp", SDL_SCANCODE_I);
    config_input_keyboard[1].key_down = (SDL_Scancode)read_int("InputKeyboard2", "KeyDown", SDL_SCANCODE_K);
    config_input_keyboard[1].key_select = (SDL_Scancode)read_int("InputKeyboard2", "KeySelect", SDL_SCANCODE_G);
    config_input_keyboard[1].key_run = (SDL_Scancode)read_int("InputKeyboard2", "KeyRun", SDL_SCANCODE_H);
    config_input_keyboard[1].key_I = (SDL_Scancode)read_int("InputKeyboard2", "KeyI", SDL_SCANCODE_Y);
    config_input_keyboard[1].key_II = (SDL_Scancode)read_int("InputKeyboard2", "KeyII", SDL_SCANCODE_T);
    config_input_keyboard[1].key_III = (SDL_Scancode)read_int("InputKeyboard2", "KeyIII", SDL_SCANCODE_5);
    config_input_keyboard[1].key_IV = (SDL_Scancode)read_int("InputKeyboard2", "KeyIV", SDL_SCANCODE_6);
    config_input_keyboard[1].key_V = (SDL_Scancode)read_int("InputKeyboard2", "KeyV", SDL_SCANCODE_7);
    config_input_keyboard[1].key_VI = (SDL_Scancode)read_int("InputKeyboard2", "KeyVI", SDL_SCANCODE_8);
    config_input_keyboard[1].key_toggle_turbo_I = (SDL_Scancode)read_int("InputKeyboard2", "KeyToogleTurboI", SDL_SCANCODE_P);
    config_input_keyboard[1].key_toggle_turbo_II = (SDL_Scancode)read_int("InputKeyboard2", "KeyToogleTurboII", SDL_SCANCODE_O);

    for (int i = 2; i < GG_MAX_GAMEPADS; i++)
    {
        char input_group[32];
        snprintf(input_group, sizeof(input_group), "InputKeyboard%d", i + 1);
        config_input_keyboard[i].key_left = (SDL_Scancode)read_int(input_group, "KeyLeft", SDL_SCANCODE_UNKNOWN);
        config_input_keyboard[i].key_right = (SDL_Scancode)read_int(input_group, "KeyRight", SDL_SCANCODE_UNKNOWN);
        config_input_keyboard[i].key_up = (SDL_Scancode)read_int(input_group, "KeyUp", SDL_SCANCODE_UNKNOWN);
        config_input_keyboard[i].key_down = (SDL_Scancode)read_int(input_group, "KeyDown", SDL_SCANCODE_UNKNOWN);
        config_input_keyboard[i].key_select = (SDL_Scancode)read_int(input_group, "KeySelect", SDL_SCANCODE_UNKNOWN);
        config_input_keyboard[i].key_run = (SDL_Scancode)read_int(input_group, "KeyRun", SDL_SCANCODE_UNKNOWN);
        config_input_keyboard[i].key_I = (SDL_Scancode)read_int(input_group, "KeyI", SDL_SCANCODE_UNKNOWN);
        config_input_keyboard[i].key_II = (SDL_Scancode)read_int(input_group, "KeyII", SDL_SCANCODE_UNKNOWN);
        config_input_keyboard[i].key_III = (SDL_Scancode)read_int(input_group, "KeyIII", SDL_SCANCODE_UNKNOWN);
        config_input_keyboard[i].key_IV = (SDL_Scancode)read_int(input_group, "KeyIV", SDL_SCANCODE_UNKNOWN);
        config_input_keyboard[i].key_V = (SDL_Scancode)read_int(input_group, "KeyV", SDL_SCANCODE_UNKNOWN);
        config_input_keyboard[i].key_VI = (SDL_Scancode)read_int(input_group, "KeyVI", SDL_SCANCODE_UNKNOWN);
        config_input_keyboard[i].key_toggle_turbo_I = (SDL_Scancode)read_int(input_group, "KeyToogleTurboI", SDL_SCANCODE_UNKNOWN);
        config_input_keyboard[i].key_toggle_turbo_II = (SDL_Scancode)read_int(input_group, "KeyToogleTurboII", SDL_SCANCODE_UNKNOWN);
    }

    for (int i = 0; i < GG_MAX_GAMEPADS; i++)
    {
        char input_group[32];
        snprintf(input_group, sizeof(input_group), "InputGamepad%d", i + 1);
        config_input_gamepad[i].gamepad_directional = read_int(input_group, "GamepadDirectional", 0);
        config_input_gamepad[i].gamepad_invert_x_axis = read_bool(input_group, "GamepadInvertX", false);
        config_input_gamepad[i].gamepad_invert_y_axis = read_bool(input_group, "GamepadInvertY", false);
        config_input_gamepad[i].gamepad_select = read_int(input_group, "GamepadSelect", SDL_CONTROLLER_BUTTON_BACK);
        config_input_gamepad[i].gamepad_run = read_int(input_group, "GamepadRun", SDL_CONTROLLER_BUTTON_START);
        config_input_gamepad[i].gamepad_x_axis = read_int(input_group, "GamepadX", SDL_CONTROLLER_AXIS_LEFTX);
        config_input_gamepad[i].gamepad_y_axis = read_int(input_group, "GamepadY", SDL_CONTROLLER_AXIS_LEFTY);
        config_input_gamepad[i].gamepad_I = read_int(input_group, "GamepadI", SDL_CONTROLLER_BUTTON_A);
        config_input_gamepad[i].gamepad_II = read_int(input_group, "GamepadII", SDL_CONTROLLER_BUTTON_B);
        config_input_gamepad[i].gamepad_III = read_int(input_group, "GamepadIII", SDL_CONTROLLER_BUTTON_Y);
        config_input_gamepad[i].gamepad_IV = read_int(input_group, "GamepadIV", SDL_CONTROLLER_BUTTON_X);
        config_input_gamepad[i].gamepad_V = read_int(input_group, "GamepadV", SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
        config_input_gamepad[i].gamepad_VI = read_int(input_group, "GamepadVI", SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
        config_input_gamepad[i].gamepad_toggle_turbo_I = read_int(input_group, "GamepadToogleTurboI", SDL_CONTROLLER_BUTTON_LEFTSTICK);
        config_input_gamepad[i].gamepad_toggle_turbo_II = read_int(input_group, "GamepadToogleTurboII", SDL_CONTROLLER_BUTTON_RIGHTSTICK);
    }

    for (int i = 0; i < GG_MAX_GAMEPADS; i++)
    {
        char input_group[32];
        snprintf(input_group, sizeof(input_group), "InputGamepadShortcuts%d", i + 1);
        for (int j = 0; j < config_HotkeyIndex_COUNT; j++)
        {
            char key_name[32];
            snprintf(key_name, sizeof(key_name), "Shortcut%d", j);
            config_input_gamepad_shortcuts[i].gamepad_shortcuts[j] = read_int(input_group, key_name, SDL_CONTROLLER_BUTTON_INVALID);
        }
    }

    // Read hotkeys
    config_hotkeys[config_HotkeyIndex_OpenROM] = read_hotkey("Hotkeys", "OpenROM", make_hotkey(SDL_SCANCODE_O, KMOD_CTRL));
    config_hotkeys[config_HotkeyIndex_Quit] = read_hotkey("Hotkeys", "Quit", make_hotkey(SDL_SCANCODE_Q, KMOD_CTRL));
    config_hotkeys[config_HotkeyIndex_Reset] = read_hotkey("Hotkeys", "Reset", make_hotkey(SDL_SCANCODE_R, KMOD_CTRL));
    config_hotkeys[config_HotkeyIndex_Pause] = read_hotkey("Hotkeys", "Pause", make_hotkey(SDL_SCANCODE_P, KMOD_CTRL));
    config_hotkeys[config_HotkeyIndex_FFWD] = read_hotkey("Hotkeys", "FFWD", make_hotkey(SDL_SCANCODE_F, KMOD_CTRL));
    config_hotkeys[config_HotkeyIndex_SaveState] = read_hotkey("Hotkeys", "SaveState", make_hotkey(SDL_SCANCODE_S, KMOD_CTRL));
    config_hotkeys[config_HotkeyIndex_LoadState] = read_hotkey("Hotkeys", "LoadState", make_hotkey(SDL_SCANCODE_L, KMOD_CTRL));
    config_hotkeys[config_HotkeyIndex_Screenshot] = read_hotkey("Hotkeys", "Screenshot", make_hotkey(SDL_SCANCODE_X, KMOD_CTRL));
    config_hotkeys[config_HotkeyIndex_Fullscreen] = read_hotkey("Hotkeys", "Fullscreen", make_hotkey(SDL_SCANCODE_F12, KMOD_NONE));
    config_hotkeys[config_HotkeyIndex_ShowMainMenu] = read_hotkey("Hotkeys", "ShowMainMenu", make_hotkey(SDL_SCANCODE_M, KMOD_CTRL));
    config_hotkeys[config_HotkeyIndex_DebugStepInto] = read_hotkey("Hotkeys", "DebugStepInto", make_hotkey(SDL_SCANCODE_F11, KMOD_NONE));
    config_hotkeys[config_HotkeyIndex_DebugStepOver] = read_hotkey("Hotkeys", "DebugStepOver", make_hotkey(SDL_SCANCODE_F10, KMOD_NONE));
    config_hotkeys[config_HotkeyIndex_DebugStepOut] = read_hotkey("Hotkeys", "DebugStepOut", make_hotkey(SDL_SCANCODE_F11, KMOD_SHIFT));
    config_hotkeys[config_HotkeyIndex_DebugStepFrame] = read_hotkey("Hotkeys", "DebugStepFrame", make_hotkey(SDL_SCANCODE_F6, KMOD_NONE));
    config_hotkeys[config_HotkeyIndex_DebugContinue] = read_hotkey("Hotkeys", "DebugContinue", make_hotkey(SDL_SCANCODE_F5, KMOD_NONE));
    config_hotkeys[config_HotkeyIndex_DebugBreak] = read_hotkey("Hotkeys", "DebugBreak", make_hotkey(SDL_SCANCODE_F7, KMOD_NONE));
    config_hotkeys[config_HotkeyIndex_DebugRunToCursor] = read_hotkey("Hotkeys", "DebugRunToCursor", make_hotkey(SDL_SCANCODE_F8, KMOD_NONE));
    config_hotkeys[config_HotkeyIndex_DebugBreakpoint] = read_hotkey("Hotkeys", "DebugBreakpoint", make_hotkey(SDL_SCANCODE_F9, KMOD_NONE));
    config_hotkeys[config_HotkeyIndex_DebugGoBack] = read_hotkey("Hotkeys", "DebugGoBack", make_hotkey(SDL_SCANCODE_BACKSPACE, KMOD_CTRL));
    config_hotkeys[config_HotkeyIndex_SelectSlot1] = read_hotkey("Hotkeys", "SelectSlot1", make_hotkey(SDL_SCANCODE_1, KMOD_CTRL));
    config_hotkeys[config_HotkeyIndex_SelectSlot2] = read_hotkey("Hotkeys", "SelectSlot2", make_hotkey(SDL_SCANCODE_2, KMOD_CTRL));
    config_hotkeys[config_HotkeyIndex_SelectSlot3] = read_hotkey("Hotkeys", "SelectSlot3", make_hotkey(SDL_SCANCODE_3, KMOD_CTRL));
    config_hotkeys[config_HotkeyIndex_SelectSlot4] = read_hotkey("Hotkeys", "SelectSlot4", make_hotkey(SDL_SCANCODE_4, KMOD_CTRL));
    config_hotkeys[config_HotkeyIndex_SelectSlot5] = read_hotkey("Hotkeys", "SelectSlot5", make_hotkey(SDL_SCANCODE_5, KMOD_CTRL));

    Debug("Settings loaded");
}

void config_write(void)
{
    Log("Saving settings to %s", config_emu_file_path);

    if (config_emulator.ffwd)
        config_audio.sync = true;

    write_bool("Debug", "Debug", config_debug.debug);
    write_bool("Debug", "Disassembler", config_debug.show_disassembler);
    write_bool("Debug", "Screen", config_debug.show_screen);
    write_bool("Debug", "Memory", config_debug.show_memory);
    write_bool("Debug", "Processor", config_debug.show_processor);
    write_bool("Debug", "CallStack", config_debug.show_call_stack);
    write_bool("Debug", "HuC6202Info", config_debug.show_huc6202_info);
    write_bool("Debug", "HuC6260Info", config_debug.show_huc6260_info);
    write_bool("Debug", "HuC6260Palettes", config_debug.show_huc6260_palettes);
    write_bool("Debug", "HuC6270Registers1", config_debug.show_huc6270_1_registers);
    write_bool("Debug", "HuC6270Background1", config_debug.show_huc6270_1_background);
    write_bool("Debug", "HuC6270Sprites1", config_debug.show_huc6270_1_sprites);
    write_bool("Debug", "HuC6270Info1", config_debug.show_huc6270_1_info);
    write_bool("Debug", "HuC6270Registers2", config_debug.show_huc6270_2_registers);
    write_bool("Debug", "HuC6270Background2", config_debug.show_huc6270_2_background);
    write_bool("Debug", "HuC6270Sprites2", config_debug.show_huc6270_2_sprites);
    write_bool("Debug", "HuC6270Info2", config_debug.show_huc6270_2_info);
    write_bool("Debug", "PSG", config_debug.show_psg);
    write_bool("Debug", "CDROM", config_debug.show_cdrom);
    write_bool("Debug", "CDROMAudio", config_debug.show_cdrom_audio);
    write_bool("Debug", "ADPCM", config_debug.show_adpcm);
    write_bool("Debug", "ArcadeCard", config_debug.show_arcade_card);
    write_bool("Debug", "TraceLogger", config_debug.show_trace_logger);
    write_bool("Debug", "TraceCounter", config_debug.trace_counter);
    write_bool("Debug", "TraceBank", config_debug.trace_bank);
    write_bool("Debug", "TraceRegisters", config_debug.trace_registers);
    write_bool("Debug", "TraceFlags", config_debug.trace_flags);
    write_bool("Debug", "TraceBytes", config_debug.trace_bytes);
    write_bool("Debug", "DisMem", config_debug.dis_show_mem);
    write_bool("Debug", "DisSymbols", config_debug.dis_show_symbols);
    write_bool("Debug", "DisSegment", config_debug.dis_show_segment);
    write_bool("Debug", "DisBank", config_debug.dis_show_bank);
    write_bool("Debug", "DisAutoSymbols", config_debug.dis_show_auto_symbols);
    write_bool("Debug", "DisReplaceSymbols", config_debug.dis_replace_symbols);
    write_bool("Debug", "DisReplaceLabels", config_debug.dis_replace_labels);
    write_int("Debug", "DisLookAheadCount", config_debug.dis_look_ahead_count);
    write_int("Debug", "FontSize", config_debug.font_size);
    write_int("Debug", "Scale", config_debug.scale);
    write_bool("Debug", "MultiViewport", config_debug.multi_viewport);
    write_int("Debug", "InitRam", config_debug.reset_ram);
    write_int("Debug", "InitCardRam", config_debug.reset_card_ram);
    write_int("Debug", "InitRegisters", config_debug.reset_registers);
    write_int("Debug", "InitColorTable", config_debug.reset_color_table);
    write_int("Debug", "InitMPR", config_debug.reset_mpr);
    write_int("Debug", "InitArcadeCard", config_debug.reset_arcade_card);

    write_bool("Emulator", "Maximized", config_emulator.maximized);
    write_bool("Emulator", "FullScreen", config_emulator.fullscreen);
    write_bool("Emulator", "AlwaysShowMenu", config_emulator.always_show_menu);
    write_int("Emulator", "FFWD", config_emulator.ffwd_speed);
    write_int("Emulator", "SaveSlot", config_emulator.save_slot);
    write_bool("Emulator", "StartPaused", config_emulator.start_paused);
    write_bool("Emulator", "PauseWhenInactive", config_emulator.pause_when_inactive);
    write_int("Emulator", "SaveFilesDirOption", config_emulator.savefiles_dir_option);
    write_string("Emulator", "SaveFilesPath", config_emulator.savefiles_path);
    write_int("Emulator", "SaveStatesDirOption", config_emulator.savestates_dir_option);
    write_string("Emulator", "SaveStatesPath", config_emulator.savestates_path);
    write_int("Emulator", "ScreenshotDirOption", config_emulator.screenshots_dir_option);
    write_string("Emulator", "ScreenshotPath", config_emulator.screenshots_path);
    write_int("Emulator", "BackupRAMDirOption", config_emulator.backup_ram_dir_option);
    write_string("Emulator", "BackupRAMPath", config_emulator.backup_ram_path);
    write_int("Emulator", "MB128DirOption", config_emulator.mb128_dir_option);
    write_string("Emulator", "MB128Path", config_emulator.mb128_path);
    write_int("Emulator", "MB128Mode", config_emulator.mb128_mode);
    write_string("Emulator", "LastOpenPath", config_emulator.last_open_path);
    write_string("Emulator", "SysCardBiosPath", config_emulator.syscard_bios_path);
    write_string("Emulator", "GameExpressBiosPath", config_emulator.gameexpress_bios_path);
    write_int("Emulator", "WindowWidth", config_emulator.window_width);
    write_int("Emulator", "WindowHeight", config_emulator.window_height);
    write_bool("Emulator", "StatusMessages", config_emulator.status_messages);
    write_bool("Emulator", "BackupRAM", config_emulator.backup_ram);
    write_int("Emulator", "ConsoleType", config_emulator.console_type);
    write_int("Emulator", "CDROMType", config_emulator.cdrom_type);
    write_bool("Emulator", "PreloadCDROM", config_emulator.preload_cdrom);
    write_int("Emulator", "MCPTCPPort", config_emulator.mcp_tcp_port);

    for (int i = 0; i < config_max_recent_roms; i++)
    {
        std::string item = "RecentROM" + std::to_string(i);
        write_string("Emulator", item.c_str(), config_emulator.recent_roms[i]);
    }

    write_int("Video", "Scale", config_video.scale);
    write_int("Video", "ScaleManual", config_video.scale_manual);
    write_int("Video", "AspectRatio", config_video.ratio);
    write_int("Video", "Overscan", config_video.overscan);
    write_int("Video", "ScanlineMode", config_video.scanline_mode);
    write_int("Video", "ScanlineStart", config_video.scanline_start);
    write_int("Video", "ScanlineEnd", config_video.scanline_end);
    write_bool("Video", "CompositePalette", config_video.composite_palette);
    write_bool("Video", "FPS", config_video.fps);
    write_bool("Video", "Bilinear", config_video.bilinear);
    write_bool("Video", "SpriteLimit", config_video.sprite_limit);
    write_bool("Video", "MixFrames", config_video.mix_frames);
    write_float("Video", "MixFramesIntensity", config_video.mix_frames_intensity);
    write_bool("Video", "Scanlines", config_video.scanlines);
    write_bool("Video", "ScanlinesFilter", config_video.scanlines_filter);
    write_float("Video", "ScanlinesIntensity", config_video.scanlines_intensity);
    write_bool("Video", "Sync", config_video.sync);
    write_float("Video", "BackgroundColorR", config_video.background_color[0]);
    write_float("Video", "BackgroundColorG", config_video.background_color[1]);
    write_float("Video", "BackgroundColorB", config_video.background_color[2]);
    write_float("Video", "BackgroundColorDebuggerR", config_video.background_color_debugger[0]);
    write_float("Video", "BackgroundColorDebuggerG", config_video.background_color_debugger[1]);
    write_float("Video", "BackgroundColorDebuggerB", config_video.background_color_debugger[2]);

    write_bool("Audio", "Enable", config_audio.enable);
    write_bool("Audio", "Sync", config_audio.sync);
    write_bool("Audio", "HuC6280A", config_audio.huc6280a);
    write_float("Audio", "PSGVolume", config_audio.psg_volume);
    write_float("Audio", "CDROMVolume", config_audio.cdrom_volume);
    write_float("Audio", "ADPCMVolume", config_audio.adpcm_volume);

    write_bool("Input", "TurboTap", config_input.turbo_tap);

    for (int i = 0; i < GG_MAX_GAMEPADS; i++)
    {
        char input_group[32];
        snprintf(input_group, sizeof(input_group), "Input%d", i + 1);
        write_int(input_group, "ControllerType", config_input.controller_type[i]);
        write_int(input_group, "AvenuePad3Button", config_input.avenue_pad_3_button[i]);
        for (int j = 0; j < 2; j++)
        {
            char turbo_group[32];
            snprintf(turbo_group, sizeof(turbo_group), "TurboEnabled%d", j + 1);
            write_bool(input_group, turbo_group, config_input.turbo_enabled[i][j]);
            snprintf(turbo_group, sizeof(turbo_group), "TurboSpeed%d", j + 1);
            write_int(input_group, turbo_group, config_input.turbo_speed[i][j]);
        }
    }

    write_int("InputKeyboard1", "KeyLeft", config_input_keyboard[0].key_left);
    write_int("InputKeyboard1", "KeyRight", config_input_keyboard[0].key_right);
    write_int("InputKeyboard1", "KeyUp", config_input_keyboard[0].key_up);
    write_int("InputKeyboard1", "KeyDown", config_input_keyboard[0].key_down);
    write_int("InputKeyboard1", "KeySelect", config_input_keyboard[0].key_select);
    write_int("InputKeyboard1", "KeyRun", config_input_keyboard[0].key_run);
    write_int("InputKeyboard1", "KeyI", config_input_keyboard[0].key_I);
    write_int("InputKeyboard1", "KeyII", config_input_keyboard[0].key_II);
    write_int("InputKeyboard1", "KeyIII", config_input_keyboard[0].key_III);
    write_int("InputKeyboard1", "KeyIV", config_input_keyboard[0].key_IV);
    write_int("InputKeyboard1", "KeyV", config_input_keyboard[0].key_V);
    write_int("InputKeyboard1", "KeyVI", config_input_keyboard[0].key_VI);

    write_int("InputKeyboard2", "KeyLeft", config_input_keyboard[1].key_left);
    write_int("InputKeyboard2", "KeyRight", config_input_keyboard[1].key_right);
    write_int("InputKeyboard2", "KeyUp", config_input_keyboard[1].key_up);
    write_int("InputKeyboard2", "KeyDown", config_input_keyboard[1].key_down);
    write_int("InputKeyboard2", "KeySelect", config_input_keyboard[1].key_select);
    write_int("InputKeyboard2", "KeyRun", config_input_keyboard[1].key_run);
    write_int("InputKeyboard2", "KeyI", config_input_keyboard[1].key_I);
    write_int("InputKeyboard2", "KeyII", config_input_keyboard[1].key_II);
    write_int("InputKeyboard2", "KeyIII", config_input_keyboard[1].key_III);
    write_int("InputKeyboard2", "KeyIV", config_input_keyboard[1].key_IV);
    write_int("InputKeyboard2", "KeyV", config_input_keyboard[1].key_V);
    write_int("InputKeyboard2", "KeyVI", config_input_keyboard[1].key_VI);

    for (int i = 2; i < GG_MAX_GAMEPADS; i++)
    {
        char input_group[32];
        snprintf(input_group, sizeof(input_group), "InputKeyboard%d", i + 1);
        write_int(input_group, "KeyLeft", config_input_keyboard[i].key_left);
        write_int(input_group, "KeyRight", config_input_keyboard[i].key_right);
        write_int(input_group, "KeyUp", config_input_keyboard[i].key_up);
        write_int(input_group, "KeyDown", config_input_keyboard[i].key_down);
        write_int(input_group, "KeySelect", config_input_keyboard[i].key_select);
        write_int(input_group, "KeyRun", config_input_keyboard[i].key_run);
        write_int(input_group, "KeyI", config_input_keyboard[i].key_I);
        write_int(input_group, "KeyII", config_input_keyboard[i].key_II);
        write_int(input_group, "KeyIII", config_input_keyboard[i].key_III);
        write_int(input_group, "KeyIV", config_input_keyboard[i].key_IV);
        write_int(input_group, "KeyV", config_input_keyboard[i].key_V);
        write_int(input_group, "KeyVI", config_input_keyboard[i].key_VI);
    }

    for (int i = 0; i < GG_MAX_GAMEPADS; i++)
    {
        char input_group[32];
        snprintf(input_group, sizeof(input_group), "InputGamepad%d", i + 1);
        write_int(input_group, "GamepadDirectional", config_input_gamepad[i].gamepad_directional);
        write_bool(input_group, "GamepadInvertX", config_input_gamepad[i].gamepad_invert_x_axis);
        write_bool(input_group, "GamepadInvertY", config_input_gamepad[i].gamepad_invert_y_axis);
        write_int(input_group, "GamepadSelect", config_input_gamepad[i].gamepad_select);
        write_int(input_group, "GamepadRun", config_input_gamepad[i].gamepad_run);
        write_int(input_group, "GamepadX", config_input_gamepad[i].gamepad_x_axis);
        write_int(input_group, "GamepadY", config_input_gamepad[i].gamepad_y_axis);
        write_int(input_group, "GamepadI", config_input_gamepad[i].gamepad_I);
        write_int(input_group, "GamepadII", config_input_gamepad[i].gamepad_II);
        write_int(input_group, "GamepadIII", config_input_gamepad[i].gamepad_III);
        write_int(input_group, "GamepadIV", config_input_gamepad[i].gamepad_IV);
        write_int(input_group, "GamepadV", config_input_gamepad[i].gamepad_V);
        write_int(input_group, "GamepadVI", config_input_gamepad[i].gamepad_VI);
    }

    for (int i = 0; i < GG_MAX_GAMEPADS; i++)
    {
        char input_group[32];
        snprintf(input_group, sizeof(input_group), "InputGamepadShortcuts%d", i + 1);
        for (int j = 0; j < config_HotkeyIndex_COUNT; j++)
        {
            char key_name[32];
            snprintf(key_name, sizeof(key_name), "Shortcut%d", j);
            write_int(input_group, key_name, config_input_gamepad_shortcuts[i].gamepad_shortcuts[j]);
        }
    }

    // Write hotkeys
    write_hotkey("Hotkeys", "OpenROM", config_hotkeys[config_HotkeyIndex_OpenROM]);
    write_hotkey("Hotkeys", "Quit", config_hotkeys[config_HotkeyIndex_Quit]);
    write_hotkey("Hotkeys", "Reset", config_hotkeys[config_HotkeyIndex_Reset]);
    write_hotkey("Hotkeys", "Pause", config_hotkeys[config_HotkeyIndex_Pause]);
    write_hotkey("Hotkeys", "FFWD", config_hotkeys[config_HotkeyIndex_FFWD]);
    write_hotkey("Hotkeys", "SaveState", config_hotkeys[config_HotkeyIndex_SaveState]);
    write_hotkey("Hotkeys", "LoadState", config_hotkeys[config_HotkeyIndex_LoadState]);
    write_hotkey("Hotkeys", "Screenshot", config_hotkeys[config_HotkeyIndex_Screenshot]);
    write_hotkey("Hotkeys", "Fullscreen", config_hotkeys[config_HotkeyIndex_Fullscreen]);
    write_hotkey("Hotkeys", "ShowMainMenu", config_hotkeys[config_HotkeyIndex_ShowMainMenu]);
    write_hotkey("Hotkeys", "DebugStepInto", config_hotkeys[config_HotkeyIndex_DebugStepInto]);
    write_hotkey("Hotkeys", "DebugStepOver", config_hotkeys[config_HotkeyIndex_DebugStepOver]);
    write_hotkey("Hotkeys", "DebugStepOut", config_hotkeys[config_HotkeyIndex_DebugStepOut]);
    write_hotkey("Hotkeys", "DebugStepFrame", config_hotkeys[config_HotkeyIndex_DebugStepFrame]);
    write_hotkey("Hotkeys", "DebugContinue", config_hotkeys[config_HotkeyIndex_DebugContinue]);
    write_hotkey("Hotkeys", "DebugBreak", config_hotkeys[config_HotkeyIndex_DebugBreak]);
    write_hotkey("Hotkeys", "DebugRunToCursor", config_hotkeys[config_HotkeyIndex_DebugRunToCursor]);
    write_hotkey("Hotkeys", "DebugBreakpoint", config_hotkeys[config_HotkeyIndex_DebugBreakpoint]);
    write_hotkey("Hotkeys", "DebugGoBack", config_hotkeys[config_HotkeyIndex_DebugGoBack]);
    write_hotkey("Hotkeys", "SelectSlot1", config_hotkeys[config_HotkeyIndex_SelectSlot1]);
    write_hotkey("Hotkeys", "SelectSlot2", config_hotkeys[config_HotkeyIndex_SelectSlot2]);
    write_hotkey("Hotkeys", "SelectSlot3", config_hotkeys[config_HotkeyIndex_SelectSlot3]);
    write_hotkey("Hotkeys", "SelectSlot4", config_hotkeys[config_HotkeyIndex_SelectSlot4]);
    write_hotkey("Hotkeys", "SelectSlot5", config_hotkeys[config_HotkeyIndex_SelectSlot5]);

    if (config_ini_file->write(config_ini_data, true))
    {
        Debug("Settings saved");
    }
}

static bool check_portable(void)
{
    char* base_path;
    char portable_file_path[260];
    
    base_path = SDL_GetBasePath();
    
    strcpy(portable_file_path, base_path);
    strcat(portable_file_path, "portable.ini");

    FILE* file = fopen_utf8(portable_file_path, "r");
    
    if (IsValidPointer(file))
    {
        fclose(file);
        return true;
    }

    return false;
}

static int read_int(const char* group, const char* key, int default_value)
{
    int ret = 0;

    std::string value = config_ini_data[group][key];

    if(value.empty())
        ret = default_value;
    else
        ret = std::stoi(value);

    Debug("Load integer setting: [%s][%s]=%d", group, key, ret);
    return ret;
}

static void write_int(const char* group, const char* key, int integer)
{
    std::string value = std::to_string(integer);
    config_ini_data[group][key] = value;
    Debug("Save integer setting: [%s][%s]=%s", group, key, value.c_str());
}

static float read_float(const char* group, const char* key, float default_value)
{
    float ret = 0.0f;

    std::string value = config_ini_data[group][key];

    if(value.empty())
        ret = default_value;
    else
        ret = strtof(value.c_str(), NULL);

    Debug("Load float setting: [%s][%s]=%.2f", group, key, ret);
    return ret;
}

static void write_float(const char* group, const char* key, float value)
{
    std::ostringstream oss;
    oss.imbue(std::locale::classic());
    oss << std::fixed << std::setprecision(2) << value;
    std::string value_str = oss.str();
    config_ini_data[group][key] = oss.str();
    Debug("Save float setting: [%s][%s]=%s", group, key, value_str.c_str());
}

static bool read_bool(const char* group, const char* key, bool default_value)
{
    bool ret;

    std::string value = config_ini_data[group][key];

    if(value.empty())
        ret = default_value;
    else
        std::istringstream(value) >> std::boolalpha >> ret;

    Debug("Load bool setting: [%s][%s]=%s", group, key, ret ? "true" : "false");
    return ret;
}

static void write_bool(const char* group, const char* key, bool boolean)
{
    std::stringstream converter;
    converter << std::boolalpha << boolean;
    std::string value;
    value = converter.str();
    config_ini_data[group][key] = value;
    Debug("Save bool setting: [%s][%s]=%s", group, key, value.c_str());
}

static std::string read_string(const char* group, const char* key)
{
    std::string ret = config_ini_data[group][key];
    Debug("Load string setting: [%s][%s]=%s", group, key, ret.c_str());
    return ret;
}

static void write_string(const char* group, const char* key, std::string value)
{
    config_ini_data[group][key] = value;
    Debug("Save string setting: [%s][%s]=%s", group, key, value.c_str());
}

static config_Hotkey read_hotkey(const char* group, const char* key, config_Hotkey default_value)
{
    config_Hotkey ret = default_value;

    std::string scancode_key = std::string(key) + "Scancode";
    std::string mod_key = std::string(key) + "Mod";

    ret.key = (SDL_Scancode)read_int(group, scancode_key.c_str(), default_value.key);
    ret.mod = (SDL_Keymod)read_int(group, mod_key.c_str(), default_value.mod);

    config_update_hotkey_string(&ret);

    return ret;
}

static void write_hotkey(const char* group, const char* key, config_Hotkey hotkey)
{
    std::string scancode_key = std::string(key) + "Scancode";
    std::string mod_key = std::string(key) + "Mod";

    write_int(group, scancode_key.c_str(), hotkey.key);
    write_int(group, mod_key.c_str(), hotkey.mod);
}

static config_Hotkey make_hotkey(SDL_Scancode key, SDL_Keymod mod)
{
    config_Hotkey hotkey;
    hotkey.key = key;
    hotkey.mod = mod;
    config_update_hotkey_string(&hotkey);
    return hotkey;
}

void config_update_hotkey_string(config_Hotkey* hotkey)
{
    if (hotkey->key == SDL_SCANCODE_UNKNOWN)
    {
        strcpy(hotkey->str, "");
        return;
    }

    std::string result = "";

    if (hotkey->mod & (KMOD_CTRL | KMOD_LCTRL | KMOD_RCTRL))
        result += "Ctrl+";
    if (hotkey->mod & (KMOD_SHIFT | KMOD_LSHIFT | KMOD_RSHIFT))
        result += "Shift+";
    if (hotkey->mod & (KMOD_ALT | KMOD_LALT | KMOD_RALT))
        result += "Alt+";
    if (hotkey->mod & (KMOD_GUI | KMOD_LGUI | KMOD_RGUI))
        result += "Cmd+";

    const char* key_name = SDL_GetScancodeName(hotkey->key);
    if (key_name && strlen(key_name) > 0)
        result += key_name;
    else
        result += "Unknown";

    strncpy(hotkey->str, result.c_str(), sizeof(hotkey->str) - 1);
    hotkey->str[sizeof(hotkey->str) - 1] = '\0';
}
