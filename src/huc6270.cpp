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
#include "huc6270.h"

HuC6270::HuC6270()
{
    InitPointer(m_vram);
    m_address_register = 0;
    m_status_register = 0;
    m_read_buffer = 0;
    for (int i = 0; i < 20; i++)
    {
        m_register[i] = 0;
    }
    m_state.AR = &m_address_register;
    m_state.SR = &m_status_register;
    m_state.R = m_register;
}

HuC6270::~HuC6270()
{
    SafeDeleteArray(m_vram);
}

void HuC6270::Init()
{
    m_vram = new u16[0x8000];
    Reset();
}

void HuC6270::Reset()
{
    m_address_register = 0;
    m_status_register = 0;
    m_read_buffer = 0xFFFF;

    for (int i = 0; i < 20; i++)
    {
        m_register[i] = 0;
    }

    for (int i = 0; i < 0x8000; i++)
    {
        m_vram[i] = rand() & 0xFFFF;
    }
}

HuC6270::HuC6270_State* HuC6270::GetState()
{
    return &m_state;
}

u16* HuC6270::GetVRAM()
{
    return m_vram;
}