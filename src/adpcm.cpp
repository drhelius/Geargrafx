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
#include <assert.h>
#include <algorithm>
#include "adpcm.h"

Adpcm::Adpcm()
{
    InitPointer(m_core);
    InitPointer(m_scsi_controller);
    m_read_value = 0;
    m_write_value = 0;
    m_read_cycles = 0;
    m_write_cycles = 0;
    m_read_address = 0;
    m_write_address = 0;
    m_address = 0;
    m_samples_left = 0;
    m_sample_rate = 0;
    m_cycles_per_sample = 0;
    m_control = 0;
    m_dma = 0;
    m_dma_cycles = 0;
    m_status = 0;
    m_end = false;
    m_playing = false;
}

Adpcm::~Adpcm()
{

}

void Adpcm::Init()
{
    ComputeDeltaLUT();
    Reset();
}

void Adpcm::Reset()
{
    m_read_value = 0;
    m_write_value = 0;
    m_read_cycles = 0;
    m_write_cycles = 0;
    m_read_address = 0;
    m_write_address = 0;
    m_address = 0;
    m_samples_left = 0;
    m_sample_rate = 0xF;
    m_cycles_per_sample = CalculateCyclesPerSample(m_sample_rate);
    m_control = 0;
    m_dma = 0;
    m_dma_cycles = 0;
    m_status = 0;
    m_end = false;
    m_playing = false;
    memset(m_adpcm_ram, 0, sizeof(m_adpcm_ram));
}

void Adpcm::Sync()
{
   
}

int Adpcm::EndFrame(s16* sample_buffer)
{
    Sync();

    int samples = 0;

    return samples;
}

void Adpcm::ComputeDeltaLUT()
{
    for (int step = 0; step < 49; step++)
    {
        int step_value = (int)floor(16.0f * pow(11.0f / 10.0f, (float)step));
        
        for (int nibble = 0; nibble < 8; nibble++)
        {
            m_step_delta[(step << 3) + nibble] = (step_value / 8) +
                (IS_SET_BIT(nibble, 0) ? (step_value / 4) : 0) +
                (IS_SET_BIT(nibble, 1) ? (step_value / 2) : 0) +
                (IS_SET_BIT(nibble, 2) ? (step_value / 1) : 0);
        }
    }
}

void Adpcm::ComputeLatencyLUTs()
{
    for(int i = 0; i < 36; ++i)
    {
        m_read_latency[i] = ComputeLatency(i, true);
        m_write_latency[i] = ComputeLatency(i, false);
    }
}

u8 Adpcm::ComputeLatency(int offset, bool read)
{
    for(int d = 1; d <= 36; d++)
    {
        int slot = ((offset + d) / 9) & 0x03;  // 0=refresh, 1=write, 2=write, 3=read

        if(read)
        {
            if(slot == 3)
                return d;
        }
        else
        {
            if(slot == 1 || slot == 2)
                return d;
        }
    }

    return 36;
}

void Adpcm::SetCore(GeargrafxCore* core)
{
    m_core = core;
}

void Adpcm::SetScsiController(ScsiController* scsi_controller)
{
    m_scsi_controller = scsi_controller;
}

void Adpcm::SaveState(std::ostream& stream)
{
    
}

void Adpcm::LoadState(std::istream& stream)
{
    
}