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

#ifndef GUI_FILEDIALOGS_H
#define GUI_FILEDIALOGS_H

#ifdef GUI_FILEDIALOGS_IMPORT
    #define EXTERN
#else
    #define EXTERN extern
#endif

EXTERN void gui_file_dialog_open_rom(void);
EXTERN void gui_file_dialog_load_ram(void);
EXTERN void gui_file_dialog_save_ram(void);
EXTERN void gui_file_dialog_load_state(void);
EXTERN void gui_file_dialog_save_state(void);
EXTERN void gui_file_dialog_choose_savestate_path(void);
EXTERN void gui_file_dialog_choose_screenshot_path(void);
EXTERN void gui_file_dialog_choose_backup_ram_path(void);
EXTERN void gui_file_dialog_load_bios(bool syscard);
EXTERN void gui_file_dialog_load_symbols(void);
EXTERN void gui_file_dialog_save_screenshot(void);
EXTERN void gui_file_dialog_save_memory_dump(bool binary);
EXTERN void gui_file_dialog_save_disassembler(bool full);
EXTERN void gui_file_dialog_save_log(void);

#undef GUI_FILEDIALOGS_IMPORT
#undef EXTERN
#endif /* GUI_FILEDIALOGS_H */