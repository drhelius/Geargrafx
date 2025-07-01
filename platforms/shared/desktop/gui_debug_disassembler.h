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

#ifndef GUI_DEBUG_DISASSEMBLER_H
#define GUI_DEBUG_DISASSEMBLER_H

#include "geargrafx.h"

#ifdef GUI_DEBUG_DISASSEMBLER_IMPORT
    #define EXTERN
#else
    #define EXTERN extern
#endif

EXTERN void gui_debug_disassembler_init(void);
EXTERN void gui_debug_disassembler_destroy(void);
EXTERN void gui_debug_disassembler_reset(void);
EXTERN void gui_debug_reset_symbols(void);
EXTERN void gui_debug_reset_breakpoints(void);
EXTERN void gui_debug_load_symbols_file(const char* file_path);
EXTERN void gui_debug_toggle_breakpoint(void);
EXTERN void gui_debug_add_bookmark(void);
EXTERN void gui_debug_add_symbol(void);
EXTERN void gui_debug_runtocursor(void);
EXTERN void gui_debug_runto_address(u16 address);
EXTERN void gui_debug_go_back(void);
EXTERN void gui_debug_window_disassembler(void);
EXTERN void gui_debug_save_disassembler(const char* file_path, bool full);
EXTERN void gui_debug_window_call_stack(void);

#undef GUI_DEBUG_DISASSEMBLER_IMPORT
#undef EXTERN
#endif /* GUI_DEBUG_DISASSEMBLER_H */