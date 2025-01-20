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

inline void HuC6280PSG::Write(u16 address, u8 value)
{
    Sync();

    switch (address & 0x0F)
    {
    // Channel select
    case 0:
        m_channel_select = value & 0x07;
        m_ch = &m_channels[m_channel_select];
        break;
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
            // Channel enable/disable
            if (IsSetBit(m_ch->control, 7) != IsSetBit(value, 7))
            {
                m_ch->counter = m_ch->frequency;
            }

            // DDA on, channel off
            if (IsSetBit(m_ch->control, 6) && !IsSetBit(value, 7))
            {
                m_ch->wave_index = 0;
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
            m_ch->wave = value & 0x1F;

            // DDA on
            if (IsSetBit(m_ch->control, 6))
            {
                m_ch->dda = value & 0x1F;
            }
            // DDA off, Channel off
            else if(!IsSetBit(m_ch->control, 7))
            {
                m_ch->wave_data[m_ch->wave_index] = value & 0x1F;
                m_ch->wave_index = ((m_ch->wave_index + 1) & 0x1F);
            }
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

        if (value & 0x80)
        {
            u16 lfo_freq = m_channels[1].frequency ? m_channels[1].frequency : 0x1000;
            m_channels[1].counter = lfo_freq * m_lfo_frequency;
            m_channels[1].wave_index = 0;
        }
        break;
    // LFO control
    case 9:
        m_lfo_control = value;
        break;
    }
}

#endif /* HUC6280_PSG_INLINE_H */