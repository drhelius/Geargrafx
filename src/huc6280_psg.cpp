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

#include <math.h>
#include <algorithm>
#include "huc6280_psg.h"

HuC6280PSG::HuC6280PSG()
{
    InitPointer(m_channels);
    InitPointer(m_ch);
}

HuC6280PSG::~HuC6280PSG()
{
    SafeDeleteArray(m_channels);
}

void HuC6280PSG::Init()
{
    m_channels = new HuC6280PSG_Channel[6];

    m_state.CHANNELS = m_channels;
    m_state.CHANNEL_SELECT = &m_channel_select;
    m_state.MAIN_AMPLITUDE = &m_main_amplitude;
    m_state.LFO_FREQUENCY = &m_lfo_frequency;
    m_state.LFO_CONTROL = &m_lfo_control;
    m_state.BUFFER_INDEX = &m_buffer_index;
    m_state.FRAME_SAMPLES = &m_frame_samples;

    for (int i = 0; i < 6; i++)
    {
        m_channels[i].mute = false;
    }

    ComputeVolumeLUT();
    Reset();
}

void HuC6280PSG::Reset()
{
    m_elapsed_cycles = 0;
    m_buffer_index = 0;
    m_sample_cycle_counter = 0;
    m_frame_samples = 0;

    m_channel_select = 0;
    m_main_amplitude = 0;
    m_lfo_frequency = 0;
    m_lfo_control = 0;

    m_ch = &m_channels[0];

    for (int i = 0; i < 6; i++)
    {
        m_channels[i].frequency = 0;
        m_channels[i].control = 0;
        m_channels[i].amplitude = 0;
        m_channels[i].wave = 0;
        m_channels[i].wave_index = 0;
        m_channels[i].noise_control = 0;
        m_channels[i].noise_seed = 1;
        m_channels[i].noise_counter = 0;
        m_channels[i].counter = 0;
        m_channels[i].dda = 0;
        m_channels[i].left_sample = 0;
        m_channels[i].right_sample = 0;

        for (int j = 0; j < 32; j++)
        {
            m_channels[i].wave_data[j] = 0;
        }

        for (int j = 0; j < GG_AUDIO_BUFFER_SIZE; j++)
        {
            m_channels[i].output[j] = 0;
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
        m_frame_samples = m_buffer_index;

        for (int s = 0; s < samples; s++)
        {
            s16 final_sample = 0;
            for (int i = 0; i < 6; i++)
                final_sample += m_channels[i].output[s];
            sample_buffer[s] = final_sample;
        }
    }

    m_buffer_index = 0;

    return samples;
}

void HuC6280PSG::Write(u16 address, u8 value)
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
            if (IS_SET_BIT(m_ch->control, 7) != IS_SET_BIT(value, 7))
            {
                m_ch->counter = m_ch->frequency;
            }

            // DDA on, channel off
            if (IS_SET_BIT(m_ch->control, 6) && !IS_SET_BIT(value, 7))
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
            if (IS_SET_BIT(m_ch->control, 6))
            {
                m_ch->dda = value & 0x1F;
            }
            // DDA off, Channel off
            else if(!IS_SET_BIT(m_ch->control, 7))
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

void HuC6280PSG::Sync()
{
    for (int cycles = 0; cycles < m_elapsed_cycles; cycles++)
    {
        u8 main_left_vol = (m_main_amplitude >> 4) & 0x0F;
        u8 main_right_vol = m_main_amplitude & 0x0F;

        for (int i = 0; i < 6; i++)
        {
            HuC6280PSG_Channel* ch = &m_channels[i];
            ch->left_sample = 0;
            ch->right_sample = 0;
            s8 noise_data = 0;

            if (i >= 4)
            {
                noise_data = IS_SET_BIT(ch->noise_seed, 0) ? 0x1F : 0;
                ch->noise_counter--;

                if (ch->noise_counter <= 0)
                {
                    u32 freq = (ch->noise_control & 0x1F) ^ 0x1F;
                    ch->noise_counter = freq << 6;
                    u32 seed = ch->noise_seed;
                    ch->noise_seed = (seed >> 1) | ((IS_SET_BIT(seed, 0) ^ IS_SET_BIT(seed, 1) ^ IS_SET_BIT(seed, 11) ^ IS_SET_BIT(seed, 12) ^ IS_SET_BIT(seed, 17)) << 17);
                }
            }

            if (!(ch->control & 0x80))
                continue;

            u8 left_vol = (ch->amplitude >> 4) & 0x0F;
            u8 right_vol = ch->amplitude & 0x0F;
            u8 channel_vol = (ch->control >> 1) & 0x0F;

            u8 temp_left_vol = MIN(0x0F, (0x0F - main_left_vol) + (0x0F - left_vol) + (0x0F - channel_vol));
            u8 temp_right_vol = MIN(0x0F, (0x0F - main_right_vol) + (0x0F - right_vol) + (0x0F - channel_vol));

            u16 final_left_vol = m_volume_lut[(temp_left_vol << 1) | (~ch->control & 0x01)];
            u16 final_right_vol = m_volume_lut[(temp_right_vol << 1) | (~ch->control & 0x01)];

            s8 data = 0;

            // Noise
            if ((i >=4) && (ch->noise_control & 0x80))
            {
                data = noise_data;
            }
            // DDA
            else if (ch->control & 0x40)
            {
                data = ch->dda;
            }
            // Waveform
            else
            {
                // LFO
                if ((i < 2) && (m_lfo_control & 0x03))
                {
                    if (i == 1)
                        continue;

                    HuC6280PSG_Channel* src = &m_channels[1];
                    HuC6280PSG_Channel* dest = &m_channels[0];

                    u16 lfo_freq = src->frequency ? src->frequency : 0x1000;
                    s32 freq = dest->frequency ? dest->frequency : 0x1000;

                    if (m_lfo_control & 0x80)
                    {
                        src->counter = lfo_freq * m_lfo_frequency;
                        src->wave_index = 0;
                    }
                    else
                    {
                        s16 lfo_data = src->wave_data[src->wave_index];
                        src->counter--;

                        if (src->counter <= 0)
                        {
                            src->counter = lfo_freq * m_lfo_frequency;
                            src->wave_index = (src->wave_index + 1) & 0x1f;
                        }

                        freq += ((lfo_data - 16) << (((m_lfo_control & 3) - 1) << 1));
                    }

                    data = dest->wave_data[dest->wave_index];
                    dest->counter--;

                    if (dest->counter <= 0)
                    {
                        dest->counter = freq;
                        dest->wave_index = (dest->wave_index + 1) & 0x1f;
                    }
                }
                // No LFO
                else
                {
                    u16 freq = ch->frequency ? ch->frequency : 0x1000;

                    if (freq > 7)
                        data = ch->wave_data[ch->wave_index];
                    ch->counter--;

                    if (ch->counter <= 0)
                    {
                        ch->counter = freq;
                        ch->wave_index = (ch->wave_index + 1) & 0x1F;
                    }
                }
            }

            if (!ch->mute)
            {
                ch->left_sample = (s16)((data - 16) * final_left_vol);
                ch->right_sample = (s16)((data - 16) * final_right_vol);
            }
        }

        m_sample_cycle_counter++;

        if (m_sample_cycle_counter >= GG_AUDIO_CYCLES_PER_SAMPLE)
        {
            m_sample_cycle_counter -= GG_AUDIO_CYCLES_PER_SAMPLE;

            for (int i = 0; i < 6; i++)
            {
                m_channels[i].output[m_buffer_index + 0] = m_channels[i].left_sample;
                m_channels[i].output[m_buffer_index + 1] = m_channels[i].right_sample;
            }

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

void HuC6280PSG::SaveState(std::ostream& stream)
{
    stream.write(reinterpret_cast<const char*> (&m_channel_select), sizeof(m_channel_select));
    stream.write(reinterpret_cast<const char*> (&m_main_amplitude), sizeof(m_main_amplitude));
    stream.write(reinterpret_cast<const char*> (&m_lfo_frequency), sizeof(m_lfo_frequency));
    stream.write(reinterpret_cast<const char*> (&m_lfo_control), sizeof(m_lfo_control));
    stream.write(reinterpret_cast<const char*> (&m_elapsed_cycles), sizeof(m_elapsed_cycles));
    stream.write(reinterpret_cast<const char*> (&m_sample_cycle_counter), sizeof(m_sample_cycle_counter));
    stream.write(reinterpret_cast<const char*> (&m_frame_samples), sizeof(m_frame_samples));
    stream.write(reinterpret_cast<const char*> (&m_buffer_index), sizeof(m_buffer_index));

    for (int i = 0; i < 6; i++)
    {
        stream.write(reinterpret_cast<const char*> (&m_channels[i].frequency), sizeof(m_channels[i].frequency));
        stream.write(reinterpret_cast<const char*> (&m_channels[i].control), sizeof(m_channels[i].control));
        stream.write(reinterpret_cast<const char*> (&m_channels[i].amplitude), sizeof(m_channels[i].amplitude));
        stream.write(reinterpret_cast<const char*> (&m_channels[i].wave), sizeof(m_channels[i].wave));
        stream.write(reinterpret_cast<const char*> (&m_channels[i].wave_index), sizeof(m_channels[i].wave_index));
        stream.write(reinterpret_cast<const char*> (m_channels[i].wave_data), sizeof(m_channels[i].wave_data));
        stream.write(reinterpret_cast<const char*> (&m_channels[i].noise_control), sizeof(m_channels[i].noise_control));
        stream.write(reinterpret_cast<const char*> (&m_channels[i].noise_seed), sizeof(m_channels[i].noise_seed));
        stream.write(reinterpret_cast<const char*> (&m_channels[i].noise_counter), sizeof(m_channels[i].noise_counter));
        stream.write(reinterpret_cast<const char*> (&m_channels[i].counter), sizeof(m_channels[i].counter));
        stream.write(reinterpret_cast<const char*> (&m_channels[i].dda), sizeof(m_channels[i].dda));
        stream.write(reinterpret_cast<const char*> (m_channels[i].output), sizeof(m_channels[i].output));
        stream.write(reinterpret_cast<const char*> (&m_channels[i].left_sample), sizeof(m_channels[i].left_sample));
        stream.write(reinterpret_cast<const char*> (&m_channels[i].right_sample), sizeof(m_channels[i].right_sample));
    }
}

void HuC6280PSG::LoadState(std::istream& stream)
{
    stream.read(reinterpret_cast<char*> (&m_channel_select), sizeof(m_channel_select));
    stream.read(reinterpret_cast<char*> (&m_main_amplitude), sizeof(m_main_amplitude));
    stream.read(reinterpret_cast<char*> (&m_lfo_frequency), sizeof(m_lfo_frequency));
    stream.read(reinterpret_cast<char*> (&m_lfo_control), sizeof(m_lfo_control));
    stream.read(reinterpret_cast<char*> (&m_elapsed_cycles), sizeof(m_elapsed_cycles));
    stream.read(reinterpret_cast<char*> (&m_sample_cycle_counter), sizeof(m_sample_cycle_counter));
    stream.read(reinterpret_cast<char*> (&m_frame_samples), sizeof(m_frame_samples));
    stream.read(reinterpret_cast<char*> (&m_buffer_index), sizeof(m_buffer_index));

    for (int i = 0; i < 6; i++)
    {
        stream.read(reinterpret_cast<char*> (&m_channels[i].frequency), sizeof(m_channels[i].frequency));
        stream.read(reinterpret_cast<char*> (&m_channels[i].control), sizeof(m_channels[i].control));
        stream.read(reinterpret_cast<char*> (&m_channels[i].amplitude), sizeof(m_channels[i].amplitude));
        stream.read(reinterpret_cast<char*> (&m_channels[i].wave), sizeof(m_channels[i].wave));
        stream.read(reinterpret_cast<char*> (&m_channels[i].wave_index), sizeof(m_channels[i].wave_index));
        stream.read(reinterpret_cast<char*> (m_channels[i].wave_data), sizeof(m_channels[i].wave_data));
        stream.read(reinterpret_cast<char*> (&m_channels[i].noise_control), sizeof(m_channels[i].noise_control));
        stream.read(reinterpret_cast<char*> (&m_channels[i].noise_seed), sizeof(m_channels[i].noise_seed));
        stream.read(reinterpret_cast<char*> (&m_channels[i].noise_counter), sizeof(m_channels[i].noise_counter));
        stream.read(reinterpret_cast<char*> (&m_channels[i].counter), sizeof(m_channels[i].counter));
        stream.read(reinterpret_cast<char*> (&m_channels[i].dda), sizeof(m_channels[i].dda));
        stream.read(reinterpret_cast<char*> (m_channels[i].output), sizeof(m_channels[i].output));
        stream.read(reinterpret_cast<char*> (&m_channels[i].left_sample), sizeof(m_channels[i].left_sample));
        stream.read(reinterpret_cast<char*> (&m_channels[i].right_sample), sizeof(m_channels[i].right_sample));
    }
}