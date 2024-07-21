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

#include "huc6260.h"

HuC6260::HuC6260()
{
    m_control_register = 0;
    m_color_table_address = 0;
    m_speed = HuC6260_SPEED_5_36_MHZ;
    InitPointer(m_color_table);
    state.CR = &m_control_register;
    state.CTA = &m_color_table_address;
}

HuC6260::~HuC6260()
{
    SafeDeleteArray(m_color_table);
}

void HuC6260::Init()
{
    m_color_table = new u16[512];
    Reset();
}

void HuC6260::Reset()
{
    m_control_register = 0;
    m_color_table_address = 0;
    m_speed = HuC6260_SPEED_5_36_MHZ;
    for (int i = 0; i < 512; i++)
    {
        m_color_table[i] = 0;
    }
}

HuC6260::HuC6260_State* HuC6260::GetState()
{
    return &state;
}

HuC6260::HuC6260_Speed HuC6260::GetSpeed()
{
    return m_speed;
}

u16* HuC6260::GetColorTable()
{
    return m_color_table;
}