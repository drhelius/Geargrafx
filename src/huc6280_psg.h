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

#include <iostream>
#include <fstream>
#include "common.h"

#define GG_MASTER_CLOCK_RATE 21477273
#define GG_AUDIO_SAMPLE_RATE 44100
#define GG_AUDIO_BUFFER_SIZE 2048
#define GG_AUDIO_BUFFER_COUNT 3
#define GG_AUDIO_CLOCK_RATE (GG_MASTER_CLOCK_RATE / 6)
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
        u32 noise_seed;
        s32 noise_counter;
        s32 counter;
        s16 dda;
        s16 output[GG_AUDIO_BUFFER_SIZE];
        s16 left_sample;
        s16 right_sample;
        bool mute;
    };

    struct HuC6280PSG_State
    {
        HuC6280PSG_Channel* CHANNELS;
        u8* CHANNEL_SELECT;
        u8* MAIN_AMPLITUDE;
        u8* LFO_FREQUENCY;
        u8* LFO_CONTROL;
        s32* BUFFER_INDEX;
        s32* FRAME_SAMPLES;
    };

public:
    HuC6280PSG();
    ~HuC6280PSG();
    void Init();
    void Reset();
    void Clock();
    void Write(u16 address, u8 value);
    int EndFrame(s16* sample_buffer);
    HuC6280PSG_State* GetState();
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);

private:
    void Sync();
    void ComputeVolumeLUT();

private:
    HuC6280PSG_State m_state;
    HuC6280PSG_Channel* m_channels;
    HuC6280PSG_Channel* m_ch;
    u8 m_channel_select;
    u8 m_main_amplitude;
    u8 m_lfo_frequency;
    u8 m_lfo_control;
    s32 m_elapsed_cycles;
    s32 m_sample_cycle_counter;
    s32 m_frame_samples;
    s32 m_buffer_index;
    u16 m_volume_lut[32];
};

#include "huc6280_psg_inline.h"

#endif /* HUC6280_PSG_H */