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

#ifndef GUI_DEBUG_MEMORY_H
#define	GUI_DEBUG_MEMORY_H

#ifdef GUI_DEBUG_MEMORY_IMPORT
    #define EXTERN
#else
    #define EXTERN extern
#endif

enum Memory_Editor_Tabs
{
    MEMORY_EDITOR_RAM = 0,
    MEMORY_EDITOR_ZERO_PAGE,
    MEMORY_EDITOR_ROM,
    MEMORY_EDITOR_VRAM,
    MEMORY_EDITOR_SAT,
    MEMORY_EDITOR_PALETTES,
    MEMORY_EDITOR_MAX
};

EXTERN void gui_debug_window_memory(void);
EXTERN void gui_debug_window_watches(void);
EXTERN void gui_debug_copy_memory(void);
EXTERN void gui_debug_paste_memory(void);
EXTERN void gui_debug_select_all(void);
EXTERN void gui_debug_memory_goto(int editor, int address);
EXTERN void gui_debug_save_memory_dump(const char* file_path);

#undef GUI_DEBUG_MEMORY_IMPORT
#undef EXTERN
#endif /* GUI_DEBUG_MEMORY_H */