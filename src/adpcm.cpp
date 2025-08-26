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
#include "geargrafx_core.h"
#include "cdrom.h"

Adpcm::Adpcm()
{
    InitPointer(m_core);
    InitPointer(m_scsi_controller);
    Reset();

    m_state.CONTROL = &m_control;
    m_state.DMA = &m_dma;
    m_state.END_IRQ = &m_end_irq;
    m_state.HALF_IRQ = &m_half_irq;
    m_state.PLAYING = &m_playing;
    m_state.SAMPLE_RATE = &m_sample_rate;
    m_state.ADDRESS = &m_address;
    m_state.READ_ADDRESS = &m_read_address;
    m_state.WRITE_ADDRESS = &m_write_address;
    m_state.LENGTH = &m_length;
    m_state.FRAME_SAMPLES = &m_frame_samples;
    m_state.BUFFER = m_buffer;
}

Adpcm::~Adpcm()
{

}

void Adpcm::Init(GeargrafxCore* core, CdRom* cdrom, ScsiController* scsi_controller)
{
    m_core = core;
    m_cdrom = cdrom;
    m_scsi_controller = scsi_controller;
    ComputeDeltaLUT();
    ComputeLatencyLUTs();
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
    m_end_irq = false;
    m_half_irq = false;
    m_playing = false;
    m_play_pending = false;
    m_nibble_toggle = false;
    m_length = 0;
    m_sample = 2048;
    m_step_index = 0;
    m_adpcm_cycle_counter = 0;
    m_audio_cycle_counter = 0;
    m_buffer_index = 0;
    m_frame_samples = 0;
    m_filter_state = 0.0f;
    m_dc_prev_x = 0.0f;
    m_dc_prev_y = 0.0f;
    m_gain_smooth = 1.0f;
    memset(m_adpcm_ram, 0, sizeof(m_adpcm_ram));
}

int Adpcm::EndFrame(s16* sample_buffer)
{
    int samples = 0;

    if (IsValidPointer(sample_buffer))
    {
        samples = m_buffer_index;
        m_frame_samples = m_buffer_index;
        memcpy(sample_buffer, m_buffer, samples * sizeof(s16));
    }

    m_buffer_index = 0;

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

void Adpcm::SaveState(std::ostream& stream)
{
    using namespace std;

    stream.write(reinterpret_cast<const char*> (m_adpcm_ram), sizeof(m_adpcm_ram));
    stream.write(reinterpret_cast<const char*> (&m_read_value), sizeof(m_read_value));
    stream.write(reinterpret_cast<const char*> (&m_write_value), sizeof(m_write_value));
    stream.write(reinterpret_cast<const char*> (&m_read_cycles), sizeof(m_read_cycles));
    stream.write(reinterpret_cast<const char*> (&m_write_cycles), sizeof(m_write_cycles));
    stream.write(reinterpret_cast<const char*> (&m_read_address), sizeof(m_read_address));
    stream.write(reinterpret_cast<const char*> (&m_write_address), sizeof(m_write_address));
    stream.write(reinterpret_cast<const char*> (&m_address), sizeof(m_address));
    stream.write(reinterpret_cast<const char*> (&m_samples_left), sizeof(m_samples_left));
    stream.write(reinterpret_cast<const char*> (&m_sample_rate), sizeof(m_sample_rate));
    stream.write(reinterpret_cast<const char*> (&m_cycles_per_sample), sizeof(m_cycles_per_sample));
    stream.write(reinterpret_cast<const char*> (&m_control), sizeof(m_control));
    stream.write(reinterpret_cast<const char*> (&m_dma), sizeof(m_dma));
    stream.write(reinterpret_cast<const char*> (&m_dma_cycles), sizeof(m_dma_cycles));
    stream.write(reinterpret_cast<const char*> (&m_end_irq), sizeof(m_end_irq));
    stream.write(reinterpret_cast<const char*> (&m_half_irq), sizeof(m_half_irq));
    stream.write(reinterpret_cast<const char*> (&m_playing), sizeof(m_playing));
    stream.write(reinterpret_cast<const char*> (&m_play_pending), sizeof(m_play_pending));
    stream.write(reinterpret_cast<const char*> (&m_nibble_toggle), sizeof(m_nibble_toggle));
    stream.write(reinterpret_cast<const char*> (&m_length), sizeof(m_length));
    stream.write(reinterpret_cast<const char*> (&m_sample), sizeof(m_sample));
    stream.write(reinterpret_cast<const char*> (&m_step_index), sizeof(m_step_index));
    stream.write(reinterpret_cast<const char*> (&m_adpcm_cycle_counter), sizeof(m_adpcm_cycle_counter));
    stream.write(reinterpret_cast<const char*> (&m_audio_cycle_counter), sizeof(m_audio_cycle_counter));
    stream.write(reinterpret_cast<const char*> (&m_filter_state), sizeof(m_filter_state));
}

void Adpcm::LoadState(std::istream& stream)
{
    using namespace std;

    stream.read(reinterpret_cast<char*> (m_adpcm_ram), sizeof(m_adpcm_ram));
    stream.read(reinterpret_cast<char*> (&m_read_value), sizeof(m_read_value));
    stream.read(reinterpret_cast<char*> (&m_write_value), sizeof(m_write_value));
    stream.read(reinterpret_cast<char*> (&m_read_cycles), sizeof(m_read_cycles));
    stream.read(reinterpret_cast<char*> (&m_write_cycles), sizeof(m_write_cycles));
    stream.read(reinterpret_cast<char*> (&m_read_address), sizeof(m_read_address));
    stream.read(reinterpret_cast<char*> (&m_write_address), sizeof(m_write_address));
    stream.read(reinterpret_cast<char*> (&m_address), sizeof(m_address));
    stream.read(reinterpret_cast<char*> (&m_samples_left), sizeof(m_samples_left));
    stream.read(reinterpret_cast<char*> (&m_sample_rate), sizeof(m_sample_rate));
    stream.read(reinterpret_cast<char*> (&m_cycles_per_sample), sizeof(m_cycles_per_sample));
    stream.read(reinterpret_cast<char*> (&m_control), sizeof(m_control));
    stream.read(reinterpret_cast<char*> (&m_dma), sizeof(m_dma));
    stream.read(reinterpret_cast<char*> (&m_dma_cycles), sizeof(m_dma_cycles));
    stream.read(reinterpret_cast<char*> (&m_end_irq), sizeof(m_end_irq));
    stream.read(reinterpret_cast<char*> (&m_half_irq), sizeof(m_half_irq));
    stream.read(reinterpret_cast<char*> (&m_playing), sizeof(m_playing));
    stream.read(reinterpret_cast<char*> (&m_play_pending), sizeof(m_play_pending));
    stream.read(reinterpret_cast<char*> (&m_nibble_toggle), sizeof(m_nibble_toggle));
    stream.read(reinterpret_cast<char*> (&m_length), sizeof(m_length));
    stream.read(reinterpret_cast<char*> (&m_sample), sizeof(m_sample));
    stream.read(reinterpret_cast<char*> (&m_step_index), sizeof(m_step_index));
    stream.read(reinterpret_cast<char*> (&m_adpcm_cycle_counter), sizeof(m_adpcm_cycle_counter));
    stream.read(reinterpret_cast<char*> (&m_audio_cycle_counter), sizeof(m_audio_cycle_counter));
    stream.read(reinterpret_cast<char*> (&m_filter_state), sizeof(m_filter_state));
}