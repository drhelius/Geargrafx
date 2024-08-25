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

#include <cmath>
#include <algorithm>
#include "huc6280_psg.h"

HuC6280PSG::HuC6280PSG()
{
    InitPointer(m_channels);
    InitPointer(m_ch);
}

HuC6280PSG::~HuC6280PSG()
{
    SafeDeleteArray(m_buffer);
    SafeDeleteArray(m_channels);
}

void HuC6280PSG::Init()
{
    m_buffer = new s16[GG_AUDIO_BUFFER_SIZE];
    m_channels = new HuC6280PSG_Channel[6];

    m_state.CHANNEL_SELECT = &m_channel_select;
    m_state.MAIN_AMPLITUDE = &m_main_amplitude;
    m_state.LFO_FREQUENCY = &m_lfo_frequency;
    m_state.LFO_CONTROL = &m_lfo_control;
    m_state.BUFFER_INDEX = &m_buffer_index;
    m_state.BUFFER = m_buffer;
    m_state.CHANNELS = m_channels;

    ComputeVolumeLUT();
    Reset();
}

void HuC6280PSG::Reset()
{
    m_elapsed_cycles = 0;
    m_buffer_index = 0;
    m_cycles_per_sample = GG_AUDIO_CYCLES_PER_SAMPLE;
    m_sample_cycle_counter = 0;

    m_channel_select = 0;
    m_main_amplitude = 0;
    m_lfo_frequency = 0;
    m_lfo_control = 0;

    m_left_sample = 0;
    m_right_sample = 0;

    m_ch = &m_channels[0];

    for (int i = 0; i < 6; i++)
    {
        m_channels[i].frequency = 0;
        m_channels[i].control = 0;
        m_channels[i].amplitude = 0;
        m_channels[i].wave = 0;
        m_channels[i].wave_index = 0;
        m_channels[i].noise_control = 0;
        m_channels[i].noise_frequency = 0;
        m_channels[i].noise_seed = 0;
        m_channels[i].noise_counter = 0;
        m_channels[i].counter = 0;
        m_channels[i].dda = 0;

        for (int j = 0; j < 32; j++)
        {
            m_channels[i].wave_data[j] = 0;
        }
    }
}

int HuC6280PSG::EndFrame(s16* sample_buffer)
{
    Sync();

    int samples = 0;

    if (IsValidPointer(sample_buffer))
    {
        samples = m_buffer_index;

        for (int s = 0; s < samples; s++)
            sample_buffer[s] = m_buffer[s];
    }

    m_buffer_index = 0;

    return samples;
}

HuC6280PSG::HuC6280PSG_State* HuC6280PSG::GetState()
{
    return &m_state;
}

void HuC6280PSG::Sync()
{
    

    for (int i = 0; i < m_elapsed_cycles; i++)
    {
        u8 main_left_vol = (m_main_amplitude >> 4) & 0x0F;
        u8 main_right_vol = m_main_amplitude & 0x0F;

        m_left_sample = 0;
        m_right_sample = 0;

        for (int i = 0; i < 6; i++)
        {
            HuC6280PSG_Channel* ch = &m_channels[i];

            // Channel off
            if (!(ch->control & 0x80))
                continue;

            u8 left_vol = (ch->amplitude >> 4) & 0x0F;
            u8 right_vol = ch->amplitude & 0x0F;
            u8 channel_vol = (ch->control >> 1) & 0x0F;

            int temp_left_vol = std::min(0x0F, (0x0F - main_left_vol) + (0x0F - left_vol) + (0x0F - channel_vol));
            int temp_right_vol = std::min(0x0F, (0x0F - main_right_vol) + (0x0F - right_vol) + (0x0F - channel_vol));

            int final_left_vol = m_volume_lut[(temp_left_vol << 1) | (~ch->control & 0x01)];
            int final_right_vol = m_volume_lut[(temp_right_vol << 1) | (~ch->control & 0x01)];

            // Noise
            if ((i >=4) && (ch->noise_control & 0x80))
            {

            }
            // DDA
            else if (ch->control & 0x40)
            {

            }
            // Waveform
            else
            {
                // LFO
                if ((i < 2) && (m_lfo_control & 0x03))
                {
                    if (i == 1)
                        continue;
                }
                // No LFO
                else
                {
                    u32 feq = ch->frequency ? ch->frequency : 0x1000;

                    //for (int j = 0; j < cycles; j++)
                    {
                        int data = ch->wave_data[ch->wave_index];
                        ch->counter--;

                        if (ch->counter <= 0)
                        {
                            ch->counter = feq;
                            ch->wave_index = (ch->wave_index + 1) & 0x1F;
                        }

                        m_left_sample += (s16)((data - 16) * final_left_vol);
                        m_right_sample += (s16)((data - 16) * final_right_vol);
                    }
                }
            }
        }
        
        m_sample_cycle_counter++;

        if (m_sample_cycle_counter >= m_cycles_per_sample)
        {
            m_sample_cycle_counter -= m_cycles_per_sample;

            m_buffer[m_buffer_index] = m_left_sample;
            m_buffer[m_buffer_index + 1] = m_right_sample;
            m_buffer_index += 2;

            if (m_buffer_index >= GG_AUDIO_BUFFER_SIZE)
            {
                Log("ERROR: PSG buffer overflow");
                m_buffer_index = 0;
            }
        }
    }

    m_elapsed_cycles = 0;
}

void HuC6280PSG::UpdateChannels(int cycles)
{

}

void HuC6280PSG::ComputeVolumeLUT()
{
    double amplitude = 65535.0 / 6.0 / 32.0;
    double step = 48.0 / 32.0;
    
    for (int i = 0; i < 30; i++)
    {
        m_volume_lut[i] = (u16)amplitude;
        amplitude /= pow(10.0, step / 20.0);
    }

    m_volume_lut[30] = 0;
    m_volume_lut[31] = 0;
}