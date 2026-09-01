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

#ifndef CONFIG_DATA_H
#define CONFIG_DATA_H

#include <SDL3/SDL.h>
#include <string>
#include "geargrafx.h"

static const int config_version = 8;
static const int config_minimum_version = 2;
static const int config_max_recent_roms = 15;
static const int config_memory_editor_count = 16;

enum config_ShaderMode
{
    config_ShaderMode_PixelPerfect = 0,
    config_ShaderMode_External = 1
};

enum config_Theme
{
    config_Theme_Light = 0,
    config_Theme_Dark = 1,
    config_Theme_Count = 2
};

enum config_VideoSync
{
    config_VideoSync_Disabled = 0,
    config_VideoSync_Fixed = 1,
    config_VideoSync_VRR = 2
};

struct config_Emulator
{
    bool maximized;
    bool fullscreen;
    int fullscreen_mode;
    bool always_show_menu;
    int theme;
    bool paused;
    int save_slot;
    bool start_paused;
    bool pause_when_inactive;
    bool softpatching;
    bool ffwd;
    int ffwd_speed;
    int runahead;
    bool show_info;
    std::string recent_roms[config_max_recent_roms];
    int savefiles_dir_option;
    std::string savefiles_path;
    int savestates_dir_option;
    std::string savestates_path;
    int screenshots_dir_option;
    std::string backup_ram_path;
    int backup_ram_dir_option;
    std::string mb128_path;
    int mb128_dir_option;
    int mb128_mode;
    std::string syscard_bios_path;
    std::string gameexpress_bios_path;
    std::string screenshots_path;
    std::string last_open_path;
    int window_width;
    int window_height;
    bool status_messages;
    bool allow_screensaver;
    bool backup_ram;
    int console_type;
    int cdrom_type;
    bool preload_cdrom;
    int mcp_tcp_port;
    std::string mcp_http_address;
    int turbolink_session;
    int turbolink_stall_us;
    bool capture_mouse;
    int mouse_sensitivity;
};

struct config_Video
{
    int scale;
    int scale_manual;
    int ratio;
    int overscan;
    int scanline_mode;
    int scanline_start;
    int scanline_end;
    int palette;
    bool fps;
    bool sprite_limit;
    bool safe_vdc_defaults;
    int sync_mode;
    float background_color[config_Theme_Count][3];
    float background_color_debugger[config_Theme_Count][3];
    bool lowpass_filter;
    float lowpass_intensity;
    float lowpass_cutoff_mhz;
    bool lowpass_speed[3];
    int shader_mode;
    std::string shader_preset_path;
};

struct config_Audio
{
    bool enable;
    bool sync;
    bool huc6280a;
    float master_volume;
    float psg_volume;
    float cdrom_volume;
    float adpcm_volume;
    int buffer_count;
};

struct config_Rewind
{
    bool enabled;
    int buffer_seconds;
    int frames_per_snapshot;
    float speed;
};

struct config_Input
{
    bool turbo_tap;
    bool allow_up_down;
    int controller_type[GG_MAX_GAMEPADS];
    int avenue_pad_3_button[GG_MAX_GAMEPADS];
    bool turbo_enabled[GG_MAX_GAMEPADS][2];
    int turbo_speed[GG_MAX_GAMEPADS][2];
};

enum config_InputProfile
{
    config_InputProfile_2Button = 0,
    config_InputProfile_3Button,
    config_InputProfile_6Button,
    config_InputProfile_COUNT
};

static const int config_InputBindingCount = 2;

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
    config_HotkeyIndex_ReloadROM,
    config_HotkeyIndex_Quit,
    config_HotkeyIndex_Reset,
    config_HotkeyIndex_Pause,
    config_HotkeyIndex_FFWD,
    config_HotkeyIndex_Rewind,
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
    config_HotkeyIndex_CaptureMouse,
    config_HotkeyIndex_Mute,
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
    bool debug;
    bool show_screen;
    bool show_disassembler;
    bool show_processor;
    bool show_call_stack;
    bool show_breakpoints;
    bool show_symbols;
    bool show_memory;
    bool show_huc6202_info;
    bool show_huc6260_info;
    bool show_huc6260_palettes;
    bool show_huc6270_1_registers;
    bool show_huc6270_1_background;
    bool show_huc6270_1_sprites;
    bool show_huc6270_1_tiles;
    bool show_huc6270_1_info;
    bool show_huc6270_2_registers;
    bool show_huc6270_2_background;
    bool show_huc6270_2_sprites;
    bool show_huc6270_2_tiles;
    bool show_huc6270_2_info;
    bool show_psg;
    bool show_cdrom;
    bool show_cdrom_toc;
    bool show_cdrom_audio;
    bool show_adpcm;
    bool show_arcade_card;
    bool show_trace_logger;
    bool show_turbolink;
    bool show_turbolink_transport;
    bool show_rewind;
    bool trace_counter;
    bool trace_cycles;
    bool trace_bank;
    bool trace_registers;
    bool trace_flags;
    bool trace_bytes;
    bool trace_cpu_enabled;
    bool trace_cpu;
    bool trace_cpu_irq;
    bool trace_vdc;
    bool trace_input;
    bool trace_timer;
    bool trace_cdrom;
    bool trace_psg;
    bool trace_adpcm;
    bool trace_vce;
    bool trace_scsi;
    bool trace_system;
    int trace_vdc_events;
    int trace_input_events;
    int trace_timer_events;
    int trace_cdrom_events;
    int trace_psg_events;
    int trace_adpcm_events;
    int trace_vce_events;
    int trace_scsi_events;
    int trace_system_events;
    int trace_output;
    int trace_capacity;
    int trace_disk_dir_option;
    int trace_disk_size;
    std::string trace_disk_path;
    bool dis_show_mem;
    bool dis_show_symbols;
    bool dis_show_segment;
    bool dis_show_bank;
    bool dis_show_auto_symbols;
    bool dis_dim_auto_symbols;
    bool dis_replace_symbols;
    bool dis_replace_labels;
    int dis_syntax;
    int dis_look_ahead_count;
    bool pause_on_brk;
    int pause_on_brk_value;
    bool pause_on_brk_trigger_irq;
    int font_size;
    int scale;
    bool multi_viewport;
    bool single_instance;
    bool auto_debug_settings;
    int mem_editor_bytes_per_row[config_memory_editor_count];
    int mem_editor_preview_data_type[config_memory_editor_count];
    int mem_editor_preview_endianess[config_memory_editor_count];
    bool mem_editor_uppercase_hex[config_memory_editor_count];
    bool mem_editor_gray_out_zeros[config_memory_editor_count];
    int reset_ram;
    int reset_card_ram;
    int reset_registers;
    int reset_color_table;
    int reset_mpr;
    int reset_arcade_card;
};

EXTERN config_Emulator config_emulator;
EXTERN config_Video config_video;
EXTERN config_Audio config_audio;
EXTERN config_Rewind config_rewind;
EXTERN config_Input config_input;
EXTERN config_Input_Keyboard config_input_keyboard[GG_MAX_GAMEPADS][config_InputProfile_COUNT][config_InputBindingCount];
EXTERN config_Input_Gamepad config_input_gamepad[GG_MAX_GAMEPADS][config_InputProfile_COUNT][config_InputBindingCount];
EXTERN config_Input_Gamepad_Shortcuts config_input_gamepad_shortcuts[GG_MAX_GAMEPADS];
EXTERN config_Hotkey config_hotkeys[config_HotkeyIndex_COUNT];
EXTERN config_Debug config_debug;

#endif /* CONFIG_DATA_H */
