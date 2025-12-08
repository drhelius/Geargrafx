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

#include "vgm_recorder.h"
#include "log.h"
#include <cstring>

VgmRecorder::VgmRecorder()
{
    m_recording = false;
    m_pending_wait = 0;
    m_total_samples = 0;
    m_clock_rate = 0;
    m_huc6280_used = false;
}

VgmRecorder::~VgmRecorder()
{
    if (m_recording)
    {
        Stop();
    }
}

void VgmRecorder::Start(const char* file_path, int clock_rate)
{
    if (m_recording)
        return;

    m_file_path = file_path;
    m_clock_rate = clock_rate;
    m_recording = true;
    m_pending_wait = 0;
    m_total_samples = 0;
    m_huc6280_used = false;
    m_command_buffer.clear();
    
    Log("VGM: Start recording, clock_rate=%d (0x%08X)", clock_rate, clock_rate);
}

void VgmRecorder::Stop()
{
    if (!m_recording)
        return;

    FlushPendingWait();

    // Write end of sound data command
    WriteCommand(0x66);

    std::ofstream file(m_file_path.c_str(), std::ios::binary);
    if (file.is_open())
    {
        u8 header[256];
        memset(header, 0, 256);

        // File identification "Vgm " (0x56 0x67 0x6d 0x20)
        header[0x00] = 0x56;
        header[0x01] = 0x67;
        header[0x02] = 0x6d;
        header[0x03] = 0x20;

        // EOF offset (file length - 4)
        u32 eof_offset = (256 + (u32)m_command_buffer.size()) - 4;
        header[0x04] = (eof_offset >> 0) & 0xFF;
        header[0x05] = (eof_offset >> 8) & 0xFF;
        header[0x06] = (eof_offset >> 16) & 0xFF;
        header[0x07] = (eof_offset >> 24) & 0xFF;

        // Version number (1.61 = 0x00000161)
        header[0x08] = 0x61;
        header[0x09] = 0x01;
        header[0x0A] = 0x00;
        header[0x0B] = 0x00;

        // SN76489 clock (not used, set to 0)
        header[0x0C] = 0x00;
        header[0x0D] = 0x00;
        header[0x0E] = 0x00;
        header[0x0F] = 0x00;

        // GD3 offset (0 = no GD3 tag)
        header[0x14] = 0x00;
        header[0x15] = 0x00;
        header[0x16] = 0x00;
        header[0x17] = 0x00;

        // Total # samples
        header[0x18] = (m_total_samples >> 0) & 0xFF;
        header[0x19] = (m_total_samples >> 8) & 0xFF;
        header[0x1A] = (m_total_samples >> 16) & 0xFF;
        header[0x1B] = (m_total_samples >> 24) & 0xFF;

        // Loop offset (0 = no loop)
        header[0x1C] = 0x00;
        header[0x1D] = 0x00;
        header[0x1E] = 0x00;
        header[0x1F] = 0x00;

        // Loop # samples (0 = no loop)
        header[0x20] = 0x00;
        header[0x21] = 0x00;
        header[0x22] = 0x00;
        header[0x23] = 0x00;

        // Rate (60Hz for NTSC)
        u32 rate = 60;
        header[0x24] = (rate >> 0) & 0xFF;
        header[0x25] = (rate >> 8) & 0xFF;
        header[0x26] = (rate >> 16) & 0xFF;
        header[0x27] = (rate >> 24) & 0xFF;

        // VGM data offset (relative from 0x34)
        // Data starts at 0x100 (256 bytes), so offset from 0x34 is 0x100 - 0x34 = 0xCC
        header[0x34] = 0xCC;
        header[0x35] = 0x00;
        header[0x36] = 0x00;
        header[0x37] = 0x00;

        // HuC6280 clock (offset 0xA4)
        u32 huc6280_clock = m_clock_rate;
        header[0xA4] = (huc6280_clock >> 0) & 0xFF;
        header[0xA5] = (huc6280_clock >> 8) & 0xFF;
        header[0xA6] = (huc6280_clock >> 16) & 0xFF;
        header[0xA7] = (huc6280_clock >> 24) & 0xFF;
        
        Log("VGM: Stop recording, clock_rate=%d (0x%08X), total_samples=%d", m_clock_rate, m_clock_rate, m_total_samples);
        Log("VGM: Header bytes at 0xA4: %02X %02X %02X %02X", header[0xA4], header[0xA5], header[0xA6], header[0xA7]);

        // Write header
        file.write(reinterpret_cast<const char*>(header), 256);

        // Write command buffer
        file.write(reinterpret_cast<const char*>(&m_command_buffer[0]), m_command_buffer.size());

        file.close();
    }

    m_recording = false;
    m_command_buffer.clear();
}

void VgmRecorder::WriteHuC6280(u16 address, u8 data)
{
    if (!m_recording)
        return;

    FlushPendingWait();

    m_huc6280_used = true;

    // 0xB9 aa dd - HuC6280, write value dd to register aa
    // Register 00 equals HuC6280 address 0x0800
    // Valid range: 0x0800-0x0809 (HuC6280 PSG registers)
    if (address >= 0x0800 && address <= 0x0809)
    {
        u8 reg = address - 0x0800;
        WriteCommand(0xB9, reg, data);
    }
    else
    {
        Debug("VGM: Skipping invalid address 0x%04X", address);
    }
}

void VgmRecorder::UpdateTiming(int elapsed_samples)
{
    if (!m_recording)
        return;

    m_pending_wait += elapsed_samples;
    m_total_samples += elapsed_samples;
}

void VgmRecorder::WriteCommand(u8 command)
{
    m_command_buffer.push_back(command);
}

void VgmRecorder::WriteCommand(u8 command, u8 data)
{
    m_command_buffer.push_back(command);
    m_command_buffer.push_back(data);
}

void VgmRecorder::WriteCommand(u8 command, u8 data1, u8 data2)
{
    m_command_buffer.push_back(command);
    m_command_buffer.push_back(data1);
    m_command_buffer.push_back(data2);
}

void VgmRecorder::WriteWait(int samples)
{
    if (samples <= 0)
        return;

    while (samples > 0)
    {
        if (samples == 735)
        {
            // 0x62 - wait 735 samples (60th of a second)
            WriteCommand(0x62);
            samples -= 735;
        }
        else if (samples == 882)
        {
            // 0x63 - wait 882 samples (50th of a second)
            WriteCommand(0x63);
            samples -= 882;
        }
        else if (samples <= 16)
        {
            // 0x7n - wait n+1 samples, n can range from 0 to 15
            WriteCommand(0x70 + (samples - 1));
            samples = 0;
        }
        else if (samples <= 65535)
        {
            // 0x61 nn nn - Wait n samples
            WriteCommand(0x61);
            m_command_buffer.push_back(samples & 0xFF);
            m_command_buffer.push_back((samples >> 8) & 0xFF);
            samples = 0;
        }
        else
        {
            // Write maximum wait and continue
            WriteCommand(0x61);
            m_command_buffer.push_back(0xFF);
            m_command_buffer.push_back(0xFF);
            samples -= 65535;
        }
    }
}

void VgmRecorder::FlushPendingWait()
{
    if (m_pending_wait > 0)
    {
        WriteWait(m_pending_wait);
        m_pending_wait = 0;
    }
}
