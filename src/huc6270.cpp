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

HuC6270::HuC6270(HuC6280* HuC6280)
{
    m_huc6280 = HuC6280;
    InitPointer(m_vram);
    InitPointer(m_sat);
    m_address_register = 0;
    m_status_register = 0;
    m_read_buffer = 0;
    m_hpos = 0;
    m_vpos = 0;
    for (int i = 0; i < 20; i++)
    {
        m_register[i] = 0;
    }
    m_state.AR = &m_address_register;
    m_state.SR = &m_status_register;
    m_state.R = m_register;
    m_state.READ_BUFFER = &m_read_buffer;
    m_state.HPOS = &m_hpos;
    m_state.VPOS = &m_vpos;
}

HuC6270::~HuC6270()
{
    SafeDeleteArray(m_vram);
    SafeDeleteArray(m_sat);
}

void HuC6270::Init()
{
    m_vram = new u16[HUC6270_VRAM_SIZE];
    m_sat = new u16[HUC6270_SAT_SIZE];
    Reset();
}

void HuC6270::Reset()
{
    m_address_register = 0;
    m_status_register = 0;
    m_read_buffer = 0xFFFF;
    m_hpos = 0;
    m_vpos = 0;

    for (int i = 0; i < 20; i++)
    {
        m_register[i] = 0;
    }

    for (int i = 0; i < HUC6270_VRAM_SIZE; i++)
    {
        m_vram[i] = rand() & 0xFFFF;
    }

    for (int i = 0; i < HUC6270_SAT_SIZE; i++)
    {
        m_sat[i] = rand() & 0xFFFF;
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

u16* HuC6270::GetSAT()
{
    return m_sat;
}