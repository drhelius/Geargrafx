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

#define GUI_DEBUG_IMPORT
#include "gui_debug.h"

#include "geargrafx.h"
#include "imgui.h"
#include "gui_debug_disassembler.h"
#include "gui_debug_huc6280.h"
#include "gui_debug_huc6270.h"
#include "gui_debug_huc6260.h"
#include "gui_debug_huc6202.h"
#include "gui_debug_memory.h"
#include "gui_debug_psg.h"
#include "gui_debug_cdrom.h"
#include "gui_debug_cdrom_audio.h"
#include "gui_debug_adpcm.h"
#include "gui_debug_trace_logger.h"
#include "emu.h"
#include "config.h"


void gui_debug_init(void)
{
    gui_debug_disassembler_init();
    gui_debug_psg_init();
    gui_debug_cdrom_audio_init();
    gui_debug_adpcm_init();
    gui_debug_memory_reset();
}

void gui_debug_destroy(void)
{
    gui_debug_disassembler_destroy();
    gui_debug_psg_destroy();
    gui_debug_cdrom_audio_destroy();
    gui_debug_adpcm_destroy();
}

void gui_debug_reset(void)
{
    gui_debug_disassembler_reset();
    gui_debug_memory_reset();
    gui_debug_reset_breakpoints();
    gui_debug_reset_symbols();
}

void gui_debug_callback(void)
{
    gui_debug_trace_logger_update();
}

void gui_debug_windows(void)
{
    if (config_debug.debug)
    {
        if (config_debug.show_processor)
            gui_debug_window_huc6280();
        if (config_debug.show_memory)
            gui_debug_window_memory();
        if (config_debug.show_disassembler)
            gui_debug_window_disassembler();
        if (config_debug.show_call_stack)
            gui_debug_window_call_stack();
        if (config_debug.show_breakpoints)
            gui_debug_window_breakpoints();
        if (config_debug.show_symbols)
            gui_debug_window_symbols();
        if (config_debug.show_huc6260_info)
            gui_debug_window_huc6260_info();
        if (config_debug.show_huc6260_palettes)
            gui_debug_window_huc6260_palettes();
        if (config_debug.show_huc6270_1_registers)
            gui_debug_window_huc6270_registers(1);
        if (config_debug.show_huc6270_1_background)
            gui_debug_window_huc6270_background(1);
        if (config_debug.show_huc6270_1_sprites)
            gui_debug_window_huc6270_sprites(1);
        if (config_debug.show_huc6270_1_info)
            gui_debug_window_huc6270_info(1);
        if (emu_get_core()->GetMedia()->IsSGX())
        {
            if (config_debug.show_huc6202_info)
                gui_debug_window_huc6202_info();
            if (config_debug.show_huc6270_2_registers)
                gui_debug_window_huc6270_registers(2);
            if (config_debug.show_huc6270_2_background)
                gui_debug_window_huc6270_background(2);
            if (config_debug.show_huc6270_2_sprites)
                gui_debug_window_huc6270_sprites(2);
            if (config_debug.show_huc6270_2_info)
                gui_debug_window_huc6270_info(2);
        }
        if (config_debug.show_psg)
            gui_debug_window_psg();
        if (config_debug.show_cdrom && emu_get_core()->GetMedia()->IsCDROM())
            gui_debug_window_cdrom();
        if (config_debug.show_cdrom_audio && emu_get_core()->GetMedia()->IsCDROM())
            gui_debug_window_cdrom_audio();
        if (config_debug.show_adpcm && emu_get_core()->GetMedia()->IsCDROM())
            gui_debug_window_adpcm();
        if (config_debug.show_arcade_card && emu_get_core()->GetMedia()->IsArcadeCard())
            gui_debug_window_arcade_card();
        if (config_debug.show_trace_logger)
            gui_debug_window_trace_logger();

        gui_debug_memory_watches_window();
        gui_debug_memory_search_window();
    }
}
