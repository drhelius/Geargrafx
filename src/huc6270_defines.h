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

#ifndef HUC6270_DEFINES_H
#define HUC6270_DEFINES_H

#define GG_MAX_RESOLUTION_WIDTH 256
#define GG_MAX_RESOLUTION_HEIGHT 192

#define GG_MAX_BACKGROUND_WIDTH 1024
#define GG_MAX_BACKGROUND_HEIGHT 512


#define HUC6270_COLLISION 0x01
#define HUC6270_OVERFLOW 0x02
#define HUC6270_SCANLINE 0x04
#define HUC6270_VBLANK_CR 0x08
#define HUC6270_VBLANK_SR 0x20
#define HUC6270_BUSY 0x40

#define HUC6270_VRAM_SIZE 0x8000
#define HUC6270_SAT_SIZE 0x100

#define HUC6270_SPRITES 64

#define HUC6270_LINES 263
#define HUC6270_LINES_TOP_BLANKING 14
#define HUC6270_LINES_ACTIVE 242
#define HUC6270_LINES_BOTTOM_BLANKING 4
#define HUC6270_LINES_SYNC 3

#endif /* HUC6270_DEFINES_H */