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

#include <stdlib.h>
#include "memory.h"

Memory::Memory(Cartridge* cartridge)
{
    m_cartridge = cartridge;
    InitPointer(m_wram);
}

Memory::~Memory()
{
    SafeDeleteArray(m_wram);
}

void Memory::Init()
{
    m_wram = new u8[0x2000];
    Reset();
}

void Memory::Reset()
{
    m_mpr[7] = 0x00;

    for (int i = 0; i < 7; i++)
    {
        m_mpr[i] = rand() & 0xFF;
    }

    for (int i = 0; i < 0x2000; i++)
    {
        m_wram[i] = rand() & 0xFF;
    }
}


