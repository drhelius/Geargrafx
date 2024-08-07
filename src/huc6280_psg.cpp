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

#include "huc6280_psg.h"

HuC6280PSG::HuC6280PSG()
{
    InitPointer(m_channels);
}

HuC6280PSG::~HuC6280PSG()
{
    SafeDeleteArray(m_channels);
}

void HuC6280PSG::Init()
{
    m_channels = new HuC6280PSG_Channel[6];
    Reset();
}

void HuC6280PSG::Reset()
{
    m_channel_select = 0;
    m_main_amplitude = 0;
    m_lfo_frequency = 0;
    m_lfo_control = 0;

    for (int i = 0; i < 6; i++)
    {
        m_channels[i].frequency = 0;
        m_channels[i].control = 0;
        m_channels[i].amplitude = 0;
        m_channels[i].wave = 0;
        m_channels[i].noise = 0;
        for (int j = 0; j < 32; j++)
        {
            m_channels[i].wave_data[j] = 0;
        }
    }
}