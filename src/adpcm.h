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

#ifndef ADPCM_H
#define ADPCM_H

#include <iostream>
#include <fstream>
#include "common.h"

class GeargrafxCore;

class Adpcm
{

public:
    Adpcm();
    ~Adpcm();
    void Init();
    void Reset();
    void Clock(u32 cycles);
    u8 Read(u16 address);
    void Write(u16 address, u8 value);
    int EndFrame(s16* sample_buffer);
    void SetCore(GeargrafxCore* core);
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);

private:
    void Sync();
    void ComputeDeltaLUT();
    void ComputeLatencyLUTs();
    u8 ComputeLatency(int offset, bool read);
    u32 CalculateCyclesPerSample(u8 sample_rate);
    u32 NextSlotCycles(bool read);
    void UpdateReadWriteEvents(u32 cycles);

private:
    GeargrafxCore* m_core;
    s16 m_step_delta[49 * 8] = {};
    u8 m_adpcm_ram[0x10000] = {};
    u8 m_read_latency[36] = {};
    u8 m_write_latency[36] = {};
    u8 m_read_value;
    u8 m_write_value;
    u32 m_read_cycles;
    u32 m_write_cycles;
    u16 m_read_address;
    u16 m_write_address;
    u16 m_address;
    u32 m_samples_left;
    u8 m_sample_rate;
    u32 m_cycles_per_sample;
    u8 m_control;
    u8 m_dma;
};

static const s16 k_adpcm_index_shift[8] = { -1, -1, -1, -1, 2, 4, 6, 8 };

#include "adpcm_inline.h"

#endif /* ADPCM_H */