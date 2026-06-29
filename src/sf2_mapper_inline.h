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

#ifndef SF2_MAPPER_INLINE_H
#define SF2_MAPPER_INLINE_H

#include "sf2_mapper.h"
#include "media.h"

INLINE int SF2Mapper::ComputeBankAddress(int bank)
{
    bank &= 0x0F;

    int rom_size = m_media->GetROMSize();
    int blocks = (rom_size > 0x80000) ? ((rom_size - 0x80000) / 0x80000) : 1;

    if (blocks < 1)
        blocks = 1;

    return (bank % blocks) * 0x80000;
}

#endif /* SF2_MAPPER_INLINE_H */
