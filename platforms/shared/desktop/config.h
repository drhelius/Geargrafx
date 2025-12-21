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

#ifndef CONFIG_H
#define CONFIG_H

#include <SDL.h>
#include "geargrafx.h"
#define MINI_CASE_SENSITIVE
#include "ini.h"
#include "imgui.h"

#ifdef CONFIG_IMPORT
    #define EXTERN
#else
    #define EXTERN extern
#endif

static const int config_max_recent_roms = 10;

struct config_Emulator
{
    bool maximized = false;
    bool fullscreen = false;
    bool always_show_menu = false;
    bool paused = false;
    int save_slot = 0;
    bool start_paused = false;
    bool pause_when_inactive = true;
    bool ffwd = false;
    int ffwd_speed = 1;
    bool show_info = false;
    std::string recent_roms[config_max_recent_roms];
    int savefiles_dir_option = 0;
    std::string savefiles_path;
    int savestates_dir_option = 0;
    std::string savestates_path;
    int screenshots_dir_option = 0;
    std::string backup_ram_path;
    int backup_ram_dir_option = 0;
    std::string mb128_path;
    int mb128_dir_option = 0;
    int mb128_mode = 0;
    std::string syscard_bios_path;
    std::string gameexpress_bios_path;
    std::string screenshots_path;
    std::string last_open_path;
    int window_width = 770;
    int window_height = 600;
    bool status_messages = false;
    bool backup_ram = true;
    int console_type = 0;
    int cdrom_type = 0;
    bool preload_cdrom = false;
    int mcp_tcp_port = 7777;
};

struct config_Video
{
    int scale = 0;
    int scale_manual = 1;
    int ratio = 1;
    int overscan = 0;
    int scanline_mode = 0;
    int scanline_start = 11;
    int scanline_end = 234;
    bool composite_palette = false;
    bool fps = false;
    bool bilinear = false;
    bool sprite_limit = false;
    bool mix_frames = true;
    float mix_frames_intensity = 0.60f;
    bool scanlines = true;
    bool scanlines_filter = true;
    float scanlines_intensity = 0.10f;
    bool sync = true;
    float background_color[3] = {0.1f, 0.1f, 0.1f};
    float background_color_debugger[3] = {0.2f, 0.2f, 0.2f};
};

struct config_Audio
{
    bool enable = true;
    bool sync = true;
    bool huc6280a = true;
    float psg_volume = 1.0f;
    float cdrom_volume = 1.0f;
    float adpcm_volume = 1.0f;
};

struct config_Input
{
    bool turbo_tap = false;
    int controller_type[GG_MAX_GAMEPADS];
    int avenue_pad_3_button[GG_MAX_GAMEPADS];
    bool turbo_enabled[GG_MAX_GAMEPADS][2];
    int turbo_speed[GG_MAX_GAMEPADS][2];
};

struct config_Input_Keyboard
{
    SDL_Scancode key_left;
    SDL_Scancode key_right;
    SDL_Scancode key_up;
    SDL_Scancode key_down;
    SDL_Scancode key_select;
    SDL_Scancode key_run;
    SDL_Scancode key_I;
    SDL_Scancode key_II;
    SDL_Scancode key_III;
    SDL_Scancode key_IV;
    SDL_Scancode key_V;
    SDL_Scancode key_VI;
    SDL_Scancode key_toggle_turbo_I;
    SDL_Scancode key_toggle_turbo_II;
};

struct config_Input_Gamepad
{
    int gamepad_directional;
    bool gamepad_invert_x_axis;
    bool gamepad_invert_y_axis;
    int gamepad_select;
    int gamepad_run;
    int gamepad_I;
    int gamepad_II;
    int gamepad_III;
    int gamepad_IV;
    int gamepad_V;
    int gamepad_VI;
    int gamepad_x_axis;
    int gamepad_y_axis;
    int gamepad_toggle_turbo_I;
    int gamepad_toggle_turbo_II;
};

enum config_HotkeyIndex
{
    config_HotkeyIndex_OpenROM = 0,
    config_HotkeyIndex_Quit,
    config_HotkeyIndex_Reset,
    config_HotkeyIndex_Pause,
    config_HotkeyIndex_FFWD,
    config_HotkeyIndex_SaveState,
    config_HotkeyIndex_LoadState,
    config_HotkeyIndex_Screenshot,
    config_HotkeyIndex_Fullscreen,
    config_HotkeyIndex_ShowMainMenu,
    config_HotkeyIndex_DebugStepInto,
    config_HotkeyIndex_DebugStepOver,
    config_HotkeyIndex_DebugStepOut,
    config_HotkeyIndex_DebugStepFrame,
    config_HotkeyIndex_DebugContinue,
    config_HotkeyIndex_DebugBreak,
    config_HotkeyIndex_DebugRunToCursor,
    config_HotkeyIndex_DebugBreakpoint,
    config_HotkeyIndex_DebugGoBack,
    config_HotkeyIndex_SelectSlot1,
    config_HotkeyIndex_SelectSlot2,
    config_HotkeyIndex_SelectSlot3,
    config_HotkeyIndex_SelectSlot4,
    config_HotkeyIndex_SelectSlot5,
    config_HotkeyIndex_COUNT
};

struct config_Input_Gamepad_Shortcuts
{
    int gamepad_shortcuts[config_HotkeyIndex_COUNT];
};

struct config_Hotkey
{
    SDL_Scancode key;
    SDL_Keymod mod;
    char str[64];
};

struct config_Debug
{
    bool debug = false;
    bool show_screen = true;
    bool show_disassembler = true;
    bool show_processor = true;
    bool show_call_stack = false;
    bool show_memory = false;
    bool show_huc6202_info = false;
    bool show_huc6260_info = false;
    bool show_huc6260_palettes = false;
    bool show_huc6270_1_registers = false;
    bool show_huc6270_1_background = false;
    bool show_huc6270_1_sprites = false;
    bool show_huc6270_1_info = false;
    bool show_huc6270_2_registers = false;
    bool show_huc6270_2_background = false;
    bool show_huc6270_2_sprites = false;
    bool show_huc6270_2_info = false;
    bool show_psg = false;
    bool show_cdrom = false;
    bool show_cdrom_audio = false;
    bool show_adpcm = false;
    bool show_arcade_card = false;
    bool show_trace_logger = false;
    bool trace_counter = true;
    bool trace_bank = true;
    bool trace_registers = true;
    bool trace_flags = true;
    bool trace_bytes = true;
    bool dis_show_mem = true;
    bool dis_show_symbols = true;
    bool dis_show_segment = true;
    bool dis_show_bank = true;
    bool dis_show_auto_symbols = true;
    bool dis_replace_symbols = true;
    bool dis_replace_labels = true;
    int font_size = 0;
    bool multi_viewport = false;
    int reset_ram = 1;
    int reset_card_ram = 1;
    int reset_registers = 0;
    int reset_color_table = 0;
    int reset_mpr = 0;
    int reset_arcade_card = 1;
};

EXTERN mINI::INIFile* config_ini_file;
EXTERN mINI::INIStructure config_ini_data;
EXTERN char* config_root_path;
EXTERN char config_temp_path[512];
EXTERN char config_emu_file_path[512];
EXTERN char config_imgui_file_path[512];
EXTERN config_Emulator config_emulator;
EXTERN config_Video config_video;
EXTERN config_Audio config_audio;
EXTERN config_Input config_input;
EXTERN config_Input_Keyboard config_input_keyboard[GG_MAX_GAMEPADS];
EXTERN config_Input_Gamepad config_input_gamepad[GG_MAX_GAMEPADS];
EXTERN config_Input_Gamepad_Shortcuts config_input_gamepad_shortcuts[GG_MAX_GAMEPADS];
EXTERN config_Hotkey config_hotkeys[config_HotkeyIndex_COUNT];
EXTERN config_Debug config_debug;

EXTERN void config_init(void);
EXTERN void config_destroy(void);
EXTERN void config_read(void);
EXTERN void config_write(void);
EXTERN void config_update_hotkey_string(config_Hotkey* hotkey);

#undef CONFIG_IMPORT
#undef EXTERN
#endif /* CONFIG_H */
