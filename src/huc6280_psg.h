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

class HuC6280PSG
{
public:
    struct HuC6280PSG_Channel
    {
        u16 frequency;
        u8 control;
        u8 amplitude;
        u8 wave;
        u8 noise;
        u8 wave_data[32];
    };

    struct HuC6280PSG_State
    {
        HuC6280PSG_Channel* CHANNELS;
        u8* CHANNEL_SELECT;
        u8* MAIN_AMPLITUDE;
        u8* LFO_FREQUENCY;
        u8* LFO_CONTROL;
    };

public:
    HuC6280PSG();
    ~HuC6280PSG();
    void Init();
    void Reset();
    void Clock();
    void Write(u32 address, u8 value);

private:
    HuC6280PSG_Channel* m_channels;
    u8 m_channel_select;
    u8 m_main_amplitude;
    u8 m_lfo_frequency;
    u8 m_lfo_control;
};

#include "huc6280_psg_inline.h"

#endif /* HUC6280_PSG_H */