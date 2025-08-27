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

#ifndef GUI_HES_H
#define GUI_HES_H

#ifdef GUI_HES_IMPORT
    #define EXTERN
#else
    #define EXTERN extern
#endif

EXTERN void gui_hes_init(void);
EXTERN void gui_hes_destroy(void);
EXTERN void gui_draw_hes_visualization(void);

#undef GUI_HES_IMPORT
#undef EXTERN
#endif /* GUI_HES_H */