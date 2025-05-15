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

#ifndef CDROM_COMMON_H
#define CDROM_COMMON_H

#include "common.h"

struct GG_CdRomPosition
{
    u32 minutes;
    u32 seconds;
    u32 frames;
};

inline void FromLBA(u32 lba, GG_CdRomPosition* pos)
{
    pos->minutes = lba / 75 / 60;
    pos->seconds = lba / 75 % 60;
    pos->frames = lba % 75;
}

inline u32 ToLBA(GG_CdRomPosition* pos)
{
    return (pos->minutes * 60 + pos->seconds) * 75 + pos->frames;
}

#endif /* CDROM_COMMON_H */