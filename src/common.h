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

#ifndef COMMON_H
#define	COMMON_H

#include <time.h>
#include "defines.h"
#include "types.h"
#include "log.h"
#include "bit_ops.h"

inline int AsHex(const char c)
{
   return c >= 'A' ? c - 'A' + 0xA : c - '0';
}

inline unsigned int Pow2Ceil(u16 n)
{
    --n;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    ++n;
    return n;
}

inline void GetDateTimeString(time_t timestamp, char* buffer, size_t size)
{
    struct tm* timeinfo = localtime(&timestamp);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", timeinfo);
}

inline void GetCurrentDateTimeString(char* buffer, size_t size)
{
    time_t timestamp = time(NULL);
    GetDateTimeString(timestamp, buffer, size);
}

#endif /* COMMON_H */