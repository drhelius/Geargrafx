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

#ifndef RENDERER_H
#define RENDERER_H

#include <stdint.h>

#ifdef RENDERER_IMPORT
    #define EXTERN
#else
    #define EXTERN extern
#endif

#define FRAME_BUFFER_SCALE 4
#define SYSTEM_TEXTURE_WIDTH 2048
#define SYSTEM_TEXTURE_HEIGHT 512
#define FRAME_BUFFER_WIDTH (SYSTEM_TEXTURE_WIDTH * 1)
#define FRAME_BUFFER_HEIGHT (SYSTEM_TEXTURE_HEIGHT * FRAME_BUFFER_SCALE)

EXTERN uint32_t renderer_emu_texture;
EXTERN uint32_t renderer_emu_debug_huc6270_background[2];
EXTERN uint32_t renderer_emu_debug_huc6270_sprites[2][64];
EXTERN uint32_t renderer_emu_savestates;
EXTERN const char* renderer_opengl_version;

EXTERN bool renderer_init(void);
EXTERN void renderer_destroy(void);
EXTERN void renderer_begin_render(void);
EXTERN void renderer_render(void);
EXTERN void renderer_end_render(void);

#undef RENDERER_IMPORT
#undef EXTERN
#endif /* RENDERER_H */