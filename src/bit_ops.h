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

#ifndef BIT_OPS_H
#define	BIT_OPS_H

#include "types.h"

#define BIT(n) (1U << (n))
#define BIT_MASK(n) (BIT(n) - 1)
#define SET_BIT(value, bit) ((value) | (1U << (bit)))
#define UNSET_BIT(value, bit) ((value) & (~(1U << (bit))))
#define IS_SET_BIT(value, bit) (((value) & (1U << (bit))) != 0)
#define FLIP_BIT(value, bit) ((value) ^ (1U << (bit)))

static const u8 k_bitops_reverse_lut[16] = {
    0x0, 0x8, 0x4, 0xC, 0x2, 0xA, 0x6, 0xE,
    0x1, 0x9, 0x5, 0xD, 0x3, 0xB, 0x7, 0xF
};

INLINE u8 ReverseBits(const u8 value)
{
    return (k_bitops_reverse_lut[value & 0xF] << 4) | k_bitops_reverse_lut[value >> 4];
}

#endif /* BIT_OPS_H */