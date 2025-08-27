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

#ifndef GUI_ACTIONS_H
#define GUI_ACTIONS_H

#ifdef GUI_ACTIONS_IMPORT
    #define EXTERN
#else
    #define EXTERN extern
#endif

EXTERN void gui_action_reset(void);
EXTERN void gui_action_pause(void);
EXTERN void gui_action_ffwd(void);
EXTERN void gui_action_save_screenshot(const char* path);
EXTERN void gui_action_save_sprite(const char* path, int vdc, int index);
EXTERN void gui_action_save_background(const char* path, int vdc);

#undef GUI_ACTIONS_IMPORT
#undef EXTERN
#endif /* GUI_ACTIONS_H */