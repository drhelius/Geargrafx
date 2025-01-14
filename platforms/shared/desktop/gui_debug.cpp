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

#include "../../../src/geargrafx.h"
#include "imgui/imgui.h"
#include "gui_debug_disassembler.h"
#include "gui_debug_huc6280.h"
#include "gui_debug_huc6270.h"
#include "gui_debug_huc6260.h"
#include "gui_debug_memory.h"
#include "gui_debug_psg.h"
#include "gui_debug_trace_logger.h"
#include "config.h"

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
        if (config_debug.show_huc6260_info)
            gui_debug_window_huc6260_info();
        if (config_debug.show_huc6260_palettes)
            gui_debug_window_huc6260_palettes();
        if (config_debug.show_huc6270_registers)
            gui_debug_window_huc6270_registers();
        if (config_debug.show_huc6270_background)
            gui_debug_window_huc6270_background();
        if (config_debug.show_huc6270_sprites)
            gui_debug_window_huc6270_sprites();
        if (config_debug.show_huc6270_info)
            gui_debug_window_huc6270_info();
        if (config_debug.show_psg)
            gui_debug_window_psg();
        if (config_debug.show_trace_logger)
            gui_debug_window_trace_logger();
    }
}
