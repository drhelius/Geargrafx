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

#ifndef GUI_H
#define GUI_H

#include <SDL.h>
#include "geargrafx.h"
#include "imgui/imgui.h"

#ifdef GUI_IMPORT
    #define EXTERN
#else
    #define EXTERN extern
#endif

enum gui_ShortCutEvent
{
    gui_ShortcutOpenROM = 0,
    gui_ShortcutReset,
    gui_ShortcutPause,
    gui_ShortcutFFWD,
    gui_ShortcutSaveState,
    gui_ShortcutLoadState,
    gui_ShortcutScreenshot,
    gui_ShortcutDebugStepOver,
    gui_ShortcutDebugStepInto,
    gui_ShortcutDebugStepOut,
    gui_ShortcutDebugStepFrame,
    gui_ShortcutDebugBreak,
    gui_ShortcutDebugContinue,
    gui_ShortcutDebugRuntocursor,
    gui_ShortcutDebugGoBack,
    gui_ShortcutDebugBreakpoint,
    gui_ShortcutDebugCopy,
    gui_ShortcutDebugPaste,
    gui_ShortcutDebugSelectAll,
    gui_ShortcutShowMainMenu
};

EXTERN bool gui_in_use;
EXTERN bool gui_main_window_hovered;
EXTERN bool gui_main_menu_hovered;
EXTERN ImFont* gui_default_font;
EXTERN ImFont* gui_default_fonts[4];
EXTERN ImFont* gui_roboto_font;
EXTERN ImFont* gui_material_icons_font;
EXTERN int gui_main_window_width;
EXTERN int gui_main_window_height;
EXTERN int gui_main_menu_height;
EXTERN SDL_Scancode* gui_configured_key;
EXTERN int* gui_configured_button;
EXTERN bool gui_dialog_in_use;
EXTERN bool gui_shortcut_open_rom;
EXTERN bool gui_audio_mute_cdrom;
EXTERN bool gui_audio_mute_psg;
EXTERN bool gui_audio_mute_adpcm;

EXTERN bool gui_init(void);
EXTERN void gui_destroy(void);
EXTERN void gui_render(void);
EXTERN void gui_shortcut(gui_ShortCutEvent event);
EXTERN void gui_load_rom(const char* path);
EXTERN void gui_load_bios(const char* path, bool syscard);
EXTERN void gui_set_status_message(const char* message, u32 milliseconds);
EXTERN void gui_set_error_message(const char* message);

#undef GUI_IMPORT
#undef EXTERN
#endif /* GUI_H */