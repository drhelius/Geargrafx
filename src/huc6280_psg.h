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

#ifndef HUC6280_PSG_H
#define HUC6280_PSG_H

#include "common.h"

#define GG_AUDIO_SAMPLE_RATE 44100
#define GG_AUDIO_BUFFER_SIZE 2048
#define GG_AUDIO_BUFFER_COUNT 3
#define GG_AUDIO_CLOCK_RATE (GG_CLOCK_RATE / 6)
#define GG_AUDIO_CYCLES_PER_SAMPLE (GG_AUDIO_CLOCK_RATE / GG_AUDIO_SAMPLE_RATE)

class HuC6280PSG
{
public:
    struct HuC6280PSG_Channel
    {
        u16 frequency;
        u8 control;
        u8 amplitude;
        u8 wave;
        u8 wave_index;
        u8 wave_data[32];
        u8 noise_control;
        u32 noise_frequency;
        u32 noise_seed;
        int noise_counter;
        int counter;
        s16 output;
    };

    struct HuC6280PSG_State
    {
        HuC6280PSG_Channel* CHANNELS;
        u8* CHANNEL_SELECT;
        u8* MAIN_AMPLITUDE;
        u8* LFO_FREQUENCY;
        u8* LFO_CONTROL;
        int* BUFFER_INDEX;
        s16* BUFFER;
    };

public:
    HuC6280PSG();
    ~HuC6280PSG();
    void Init();
    void Reset();
    void Clock();
    void Write(u32 address, u8 value);
    int EndFrame(s16* sample_buffer);
    HuC6280PSG_State* GetState();

private:
    void Sync();
    void UpdateChannels(int cycles);
    void ComputeVolumeLUT();

private:
    HuC6280PSG_State m_state;
    HuC6280PSG_Channel* m_channels;
    HuC6280PSG_Channel* m_ch;
    u8 m_channel_select;
    u8 m_main_amplitude;
    u8 m_lfo_frequency;
    u8 m_lfo_control;
    int m_elapsed_cycles;
    int m_sample_cycle_counter;
    int m_cycles_per_sample;
    int m_buffer_index;
    s16* m_buffer;
    u16 m_volume_lut[32];
    s16 m_left_sample;
    s16 m_right_sample;
};

#include "huc6280_psg_inline.h"

#endif /* HUC6280_PSG_H */