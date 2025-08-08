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

#ifndef APPLICATION_H
#define APPLICATION_H

#include <SDL.h>
#include "geargrafx.h"

#ifdef APPLICATION_IMPORT
    #define EXTERN
#else
    #define EXTERN extern
#endif

EXTERN SDL_Window* application_sdl_window;
EXTERN SDL_GameController* application_gamepad[GG_MAX_GAMEPADS];
EXTERN int application_added_gamepad_mappings;
EXTERN int application_updated_gamepad_mappings;
EXTERN float application_display_scale;
EXTERN SDL_version application_sdl_build_version;
EXTERN SDL_version application_sdl_link_version;
EXTERN bool application_show_menu;

EXTERN int application_init(const char* rom_file, const char* symbol_file, bool force_fullscreen, bool force_windowed);
EXTERN void application_destroy(void);
EXTERN void application_mainloop(void);
EXTERN void application_trigger_quit(void);
EXTERN void application_trigger_fullscreen(bool fullscreen);
EXTERN void application_trigger_fit_to_content(int width, int height);
EXTERN void application_update_title_with_rom(const char* rom);
EXTERN void application_assign_gamepad(int slot, int device_index);

#undef APPLICATION_IMPORT
#undef EXTERN
#endif /* APPLICATION_H */
