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
class CdRom;
class ScsiController;

class Adpcm
{
public:
    struct Adpcm_State
    {
        u8* CONTROL;
        u8* DMA;
        u8* STATUS;
        bool* END;
        bool* PLAYING;
        u8* SAMPLE_RATE;
        u16* READ_ADDRESS;
        u16* WRITE_ADDRESS;
    };

public:
    Adpcm();
    ~Adpcm();
    void Init(GeargrafxCore* core, CdRom* cdrom, ScsiController* scsi_controller);
    void Reset();
    void ResetAdpcm();
    void Clock(u32 cycles);
    u8 Read(u16 address);
    void Write(u16 address, u8 value);
    int EndFrame(s16* sample_buffer);
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);

private:
    void ComputeDeltaLUT();
    void ComputeLatencyLUTs();
    u8 ComputeLatency(int offset, bool read);
    u32 CalculateCyclesPerSample(u8 sample_rate);
    u32 NextSlotCycles(bool read);
    void UpdateReadWriteEvents(u32 cycles);
    void UpdateDMA(u32 cycles);
    void UpdateAudio(u32 cycles);
    void RunAdpcm(u32 cycles);
    void WriteControl(u8 value);
    void EndReached(bool end);
    void HalfReached(bool half);
    bool CheckReset();
    void CheckLength();

private:
    GeargrafxCore* m_core;
    CdRom* m_cdrom;
    ScsiController* m_scsi_controller;
    s16 m_step_delta[49 * 8] = {};
    u8 m_adpcm_ram[0x10000] = {};
    u8 m_read_latency[36] = {};
    u8 m_write_latency[36] = {};
    u8 m_read_value;
    u8 m_write_value;
    s32 m_read_cycles;
    s32 m_write_cycles;
    u16 m_read_address;
    u16 m_write_address;
    u16 m_address;
    u32 m_samples_left;
    u8 m_sample_rate;
    s32 m_cycles_per_sample;
    u8 m_control;
    u8 m_dma;
    s32 m_dma_cycles;
    u8 m_status;
    bool m_end;
    bool m_half;
    bool m_playing;
    bool m_play_pending;
    bool m_high_nibble;
    u32 m_length;
    s16 m_sample;
    u8 m_magnitude;
    s32 m_sample_cycle_counter_adpcm;
    s32 m_sample_cycle_counter;
    s32 m_buffer_index;
    s16 m_buffer[GG_AUDIO_BUFFER_SIZE] = {};
};

static const s16 k_adpcm_index_shift[8] = { -1, -1, -1, -1, 2, 4, 6, 8 };

#include "adpcm_inline.h"

#endif /* ADPCM_H */