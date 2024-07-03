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

#ifndef HUC6280_TIMING_H
#define HUC6280_TIMING_H

#include "common.h"

const u8 k_opcode_tstates[256] =
{
/*          0  1  2  3   4  5  6  7  8  9  A  B  C  D  E  F */
/* 0x00 */  8, 7, 3, 8,  6, 4, 6, 7, 3, 2, 2, 2, 7, 5, 7, 6,
/* 0x10 */  2, 7, 7, 8,  6, 4, 6, 7, 2, 5, 2, 2, 7, 5, 7, 6,
/* 0x20 */  7, 7, 3, 8,  4, 4, 6, 7, 4, 2, 2, 2, 5, 5, 7, 6,
/* 0x30 */  2, 7, 7, 2,  4, 4, 6, 7, 2, 5, 2, 2, 5, 5, 7, 6,
/* 0x40 */  7, 7, 3, 4,  8, 4, 6, 7, 3, 2, 2, 2, 4, 5, 7, 6,
/* 0x50 */  2, 7, 7, 5,  3, 4, 6, 7, 2, 5, 3, 2, 2, 5, 7, 6,
/* 0x60 */  7, 7, 2, 2,  4, 4, 6, 7, 4, 2, 2, 2, 7, 5, 7, 6,
/* 0x70 */  2, 7, 7, 17, 4, 4, 6, 7, 2, 5, 4, 2, 7, 5, 7, 6,
/* 0x80 */  2, 7, 2, 7,  4, 4, 4, 7, 2, 2, 2, 2, 5, 5, 5, 6,
/* 0x90 */  2, 7, 7, 8,  4, 4, 4, 7, 2, 5, 2, 2, 5, 5, 5, 6,
/* 0xA0 */  2, 7, 2, 7,  4, 4, 4, 7, 2, 2, 2, 2, 5, 5, 5, 6,
/* 0xB0 */  2, 7, 7, 8,  4, 4, 4, 7, 2, 5, 2, 2, 5, 5, 5, 6,
/* 0xC0 */  2, 7, 2, 17, 4, 4, 6, 7, 2, 2, 2, 2, 5, 5, 7, 6,
/* 0xD0 */  2, 7, 7, 17, 3, 4, 6, 7, 2, 5, 3, 2, 2, 5, 7, 6,
/* 0xE0 */  2, 7, 2, 17, 4, 4, 6, 7, 2, 2, 2, 2, 5, 5, 7, 6,
/* 0xF0 */  2, 7, 7, 17, 2, 4, 6, 7, 2, 5, 4, 2, 2, 5, 7, 6
};

const u8 k_opcode_sizes[256] =
{
/*          0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
/* 0x00 */  2, 2, 1, 0, 2, 2, 2, 2, 1, 2, 1, 1, 3, 3, 3, 3,
/* 0x10 */  2, 2, 2, 0, 2, 2, 2, 2, 1, 3, 1, 1, 3, 3, 3, 3,
/* 0x20 */  3, 2, 1, 0, 2, 2, 2, 2, 1, 2, 1, 1, 3, 3, 3, 3,
/* 0x30 */  2, 2, 2, 1, 2, 2, 2, 2, 1, 3, 1, 1, 3, 3, 3, 3,
/* 0x40 */  1, 2, 1, 2, 2, 2, 2, 2, 1, 2, 1, 1, 3, 3, 3, 3,
/* 0x50 */  2, 2, 2, 2, 1, 2, 2, 2, 1, 3, 1, 1, 1, 3, 3, 3,
/* 0x60 */  1, 2, 1, 1, 2, 2, 2, 2, 1, 2, 1, 1, 3, 3, 3, 3,
/* 0x70 */  2, 2, 2, 7, 2, 2, 2, 2, 1, 3, 1, 1, 3, 3, 3, 3,
/* 0x80 */  2, 2, 1, 3, 2, 2, 2, 2, 1, 2, 1, 1, 3, 3, 3, 3,
/* 0x90 */  2, 2, 2, 4, 2, 2, 2, 2, 1, 3, 1, 1, 3, 3, 3, 3,
/* 0xA0 */  2, 2, 2, 3, 2, 2, 2, 2, 1, 2, 1, 1, 3, 3, 3, 3,
/* 0xB0 */  2, 2, 2, 4, 2, 2, 2, 2, 1, 3, 1, 1, 3, 3, 3, 3,
/* 0xC0 */  2, 2, 1, 7, 2, 2, 2, 2, 1, 2, 1, 1, 3, 3, 3, 3,
/* 0xD0 */  2, 2, 2, 7, 1, 2, 2, 2, 1, 3, 1, 1, 1, 3, 3, 3,
/* 0xE0 */  2, 2, 1, 7, 2, 2, 2, 2, 1, 2, 1, 1, 3, 3, 3, 3,
/* 0xF0 */  2, 2, 2, 7, 1, 2, 2, 2, 1, 3, 1, 1, 1, 3, 3, 3
};

#endif /* HUC6280_TIMING_H */