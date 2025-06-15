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

#ifndef EMU_H
#define	EMU_H

#include "geargrafx.h"

#ifdef EMU_IMPORT
    #define EXTERN
#else
    #define EXTERN extern
#endif

enum Debug_Command
{
    Debug_Command_Continue,
    Debug_Command_Step,
    Debug_Command_StepFrame,
    Debug_Command_None
};

enum Directory_Location
{
    Directory_Location_Default = 0,
    Directory_Location_ROM = 1,
    Directory_Location_Custom = 2
};

EXTERN u8* emu_frame_buffer;
EXTERN GG_SaveState_Header emu_savestates[5];
EXTERN GG_SaveState_Screenshot emu_savestates_screenshots[5];
EXTERN u8* emu_debug_sprite_buffers[2][64];
EXTERN u8* emu_debug_background_buffer[2];
EXTERN int emu_debug_background_buffer_width[2];
EXTERN int emu_debug_background_buffer_height[2];
EXTERN int emu_debug_sprite_widths[2][64];
EXTERN int emu_debug_sprite_heights[2][64];
EXTERN Debug_Command emu_debug_command;
EXTERN bool emu_debug_pc_changed;

EXTERN bool emu_audio_sync;
EXTERN bool emu_debug_disable_breakpoints;
EXTERN bool emu_debug_irq_breakpoints;

EXTERN void emu_init(void);
EXTERN void emu_destroy(void);
EXTERN void emu_update(void);
EXTERN void emu_load_rom(const char* file_path);
EXTERN void emu_key_pressed(GG_Controllers controller, GG_Keys key);
EXTERN void emu_key_released(GG_Controllers controller, GG_Keys key);
EXTERN void emu_pause(void);
EXTERN void emu_resume(void);
EXTERN bool emu_is_paused(void);
EXTERN bool emu_is_debug_idle(void);
EXTERN bool emu_is_empty(void);
EXTERN void emu_reset(void);
EXTERN void emu_audio_mute(bool mute);
EXTERN void emu_audio_mute_psg(bool mute);
EXTERN void emu_audio_mute_adpcm(bool mute);
EXTERN void emu_audio_mute_cdrom(bool mute);
EXTERN void emu_audio_reset(void);
EXTERN bool emu_is_audio_enabled(void);
EXTERN bool emu_is_audio_open(void);
EXTERN void emu_save_ram(const char* file_path);
EXTERN void emu_load_ram(const char* file_path);
EXTERN void emu_save_state_slot(int index);
EXTERN void emu_load_state_slot(int index);
EXTERN void emu_save_state_file(const char* file_path);
EXTERN void emu_load_state_file(const char* file_path);
EXTERN void update_savestates_data(void);
EXTERN void emu_get_runtime(GG_Runtime_Info& runtime);
EXTERN void emu_get_info(char* info, int buffer_size);
EXTERN GeargrafxCore* emu_get_core(void);
EXTERN void emu_debug_step_over(void);
EXTERN void emu_debug_step_into(void);
EXTERN void emu_debug_step_out(void);
EXTERN void emu_debug_step_frame(void);
EXTERN void emu_debug_break(void);
EXTERN void emu_debug_continue(void);
EXTERN void emu_debug_set_callback(GeargrafxCore::GG_Debug_Callback callback);
EXTERN void emu_set_composite_palette(bool enabled);
EXTERN void emu_video_no_sprite_limit(bool enabled);
EXTERN void emu_set_overscan(int overscan);
EXTERN void emu_set_scanline_start_end(int start, int end);
EXTERN void emu_set_memory_reset_values(int mpr, int wram, int card_ram);
EXTERN void emu_set_huc6260_color_table_reset_value(int value);
EXTERN void emu_set_huc6280_registers_reset_value(int value);
EXTERN void emu_set_pce_japanese(bool enabled);
EXTERN void emu_set_force_sgx(bool enabled);
EXTERN void emu_set_backup_ram(bool enabled);
EXTERN void emu_set_turbo_tap(bool enabled);
EXTERN void emu_set_pad_type(GG_Controllers controller, GG_Controller_Type type);
EXTERN void emu_set_avenue_pad_3_button(GG_Controllers controller, GG_Keys button);
EXTERN void emu_save_screenshot(const char* file_path);
EXTERN void emu_load_syscard_bios(const char* file_path);
EXTERN void emu_load_gameexpress_bios(const char* file_path);

#undef EMU_IMPORT
#undef EXTERN
#endif	/* EMU_H */