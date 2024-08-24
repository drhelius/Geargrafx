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

#ifndef HUC6280_PSG_INLINE_H
#define HUC6280_PSG_INLINE_H

#include "huc6280_psg.h"

inline void HuC6280PSG::Clock()
{
    m_elapsed_cycles++;
}

inline void HuC6280PSG::Write(u32 address, u8 value)
{
    int reg = address & 0x0F;

    // Channel select
    if (reg == 0)
    {
        m_channel_select = value & 0x07;
        m_ch = &m_channels[m_channel_select];
        return;
    }

    Sync();

    switch (reg)
    {
    // Main amplitude
    case 1:
        m_main_amplitude = value;
        break;
    // Channel frequency (low)
    case 2:
        if (m_channel_select < 6)
        {
            m_ch->frequency = (m_ch->frequency & 0x0F00) | value;
        }
        break;
    // Channel frequency (high)
    case 3:
        if (m_channel_select < 6)
        {
            m_ch->frequency = (m_ch->frequency & 0x00FF) | ((value & 0x0F) << 8);
        }
        break;
    // Channel control
    case 4:
        if (m_channel_select < 6)
        {
            // DDA on to off
            if ((m_ch->control & 0x40) && (!(value & 0x40))) 
            {
                m_ch->wave_index = 0;
            }
            // Channel off to on
            if ((!(m_ch->control & 0x80)) && (value & 0x80)) 
            {
                m_ch->counter = m_ch->frequency;
            }
            m_ch->control = value;
        }
        break;
    // Channel amplitude
    case 5:
        if (m_channel_select < 6)
        {
            m_ch->amplitude = value;
        }
        break;
    // Channel waveform data
    case 6:
        if (m_channel_select < 6)
        {
            int data = value & 0x1F;
            m_ch->wave = data;

            // DDA off
            if((m_ch->control & 0x40) == 0)
            {
                m_ch->wave_data[m_ch->wave_index] = data;
            }

            // Channel off, DDA off
            if((m_ch->control & 0xC0) == 0)
                m_ch->wave_index = ((m_ch->wave_index + 1) & 0x1F);
        }
        break;
    // Channel noise (only channels 4 and 5)
    case 7:
        if ((m_channel_select > 3) && (m_channel_select < 6))
        {
            m_ch->noise_control = value;
        }
        break;
    // LFO frequency
    case 8:
        m_lfo_frequency = value;
        break;
    // LFO control
    case 9:
        m_lfo_control = value;
        break;
    }
}

#endif /* HUC6280_PSG_INLINE_H */