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

struct GG_CdRomMSF
{
    u8 minutes;
    u8 seconds;
    u8 frames;
};

INLINE void LbaToMsf(u32 lba, GG_CdRomMSF* msf)
{
    msf->minutes = (u8)(lba / 75 / 60);
    msf->seconds = (u8)(lba / 75 % 60);
    msf->frames = (u8)(lba % 75);
}

INLINE u32 MsfToLba(GG_CdRomMSF* msf)
{
    return (msf->minutes * 60 + msf->seconds) * 75 + msf->frames;
}

INLINE u8 DecToBcd(u8 val)
{
    return ((val / 10) << 4) | (val % 10);
}

INLINE u8 BcdToDec(u8 bcd)
{
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

INLINE u32 TimeToCycles(u32 us)
{
    // Convert microseconds to PCE master clock cycles (21.47727 MHz) using integer math
    assert(us <= 199000000); // 199 seconds
    return (u32)(((u64)us * 21477273ULL) / 1000000ULL);
}

#endif /* CDROM_COMMON_H */