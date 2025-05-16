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
#define	CONFIG_H

#include <SDL.h>
#include "geargrafx.h"
#define MINI_CASE_SENSITIVE
#include "mINI/ini.h"
#include "imgui/imgui.h"

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
    bool show_menu = true;
    bool paused = false;
    int save_slot = 0;
    bool start_paused = false;
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
    std::string syscard_bios_path;
    std::string gameexpress_bios_path;
    std::string screenshots_path;
    std::string last_open_path;
    int window_width = 770;
    int window_height = 600;
    bool status_messages = false;
    bool backup_ram = true;
    bool pce_jap = false;
    bool force_sgx = false;
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
};

struct config_Audio
{
    bool enable = true;
    bool sync = true;
};

struct config_Input
{
    bool turbo_tap = false;
    int controller_type[GG_MAX_GAMEPADS];
    int avenue_pad_3_button[GG_MAX_GAMEPADS];
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
};

struct config_Input_Gamepad
{
    bool detected;
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
};

EXTERN mINI::INIFile* config_ini_file;
EXTERN mINI::INIStructure config_ini_data;
EXTERN char* config_root_path;
EXTERN char config_emu_file_path[260];
EXTERN char config_imgui_file_path[260];
EXTERN config_Emulator config_emulator;
EXTERN config_Video config_video;
EXTERN config_Audio config_audio;
EXTERN config_Input config_input;
EXTERN config_Input_Keyboard config_input_keyboard[GG_MAX_GAMEPADS];
EXTERN config_Input_Gamepad config_input_gamepad[GG_MAX_GAMEPADS];
EXTERN config_Debug config_debug;

EXTERN void config_init(void);
EXTERN void config_destroy(void);
EXTERN void config_read(void);
EXTERN void config_write(void);

#undef CONFIG_IMPORT
#undef EXTERN
#endif	/* CONFIG_H */
