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

#ifndef ADPCM_INLINE_H
#define ADPCM_INLINE_H

#include "adpcm.h"
#include "geargrafx_core.h"
#include "cdrom.h"
#include "scsi_controller.h"
#include "trace_logger.h"

INLINE void Adpcm::Clock(u32 cycles)
{
    CheckReset();
    CheckLength();
    RunAdpcm(cycles);
    UpdateReadWriteEvents(cycles);
    UpdateDMA(cycles);
    CheckLength();
    CheckReset();
}

inline void Adpcm::SoftReset()
{
    m_read_cycles = 0;
    m_write_cycles = 0;
    m_read_address = 0;
    m_write_address = 0;
    m_address = 0;
    m_length = 0;
    m_nibble_toggle = false;
    m_sample = 2048;
    m_step_index = 0;
    m_play_pending = false;
    m_filter_state = 0.0f;
    m_dc_prev_x = 0.0f;
    m_dc_prev_y = 0.0f;
    m_gain_smooth = 1.0f;
    SetEndIRQ(false);
    SetHalfIRQ(false);
}

INLINE u8 Adpcm::Read(u16 address)
{
    switch (address)
    {
        case 0x0A:
            m_read_cycles = NextSlotCycles(true);
            return m_read_value;
        case 0x0B:
            return m_dma;
        case 0x0C:
            return GetStatusRegisterSnapshot();
        case 0x0D:
            return m_control;
        case 0x0E:
            return m_sample_rate;
        default:
            Debug("ADPCM Read Invalid address: %04X", address);
            return 0;
    }
}

INLINE u8 Adpcm::GetStatusRegisterSnapshot()
{
    u8 status = 0;
    status |= (m_playing ? 0x08 : 0x00);
    status |= (m_end_irq ? 0x01 : 0x00);
    status |= (m_read_cycles > 0 ? 0x80 : 0x00);
    status |= (m_write_cycles > 0 ? 0x04 : 0x00);
    return status;
}

INLINE void Adpcm::Write(u16 address, u8 value)
{
    switch (address)
    {
        case 0x08:
            m_address = (m_address & 0xFF00) | value;
            break;
        case 0x09:
            m_address = (m_address & 0x00FF) | (value << 8);
            break;
        case 0x0A:
            m_write_cycles = NextSlotCycles(false);
            m_write_value = value;
            break;
        case 0x0B:
            m_dma = m_scsi_controller->IsDataReady() ? value : (value & ~0x01);
            TraceAdpcmEvent(TRACE_ADPCM_DMA_STATE, 0x0B, m_dma);
            break;
        case 0x0D:
            WriteControl(value);
            break;
        case 0x0E:
            m_sample_rate = value;
            m_cycles_per_sample = CalculateCyclesPerSample(m_sample_rate & 0x0F);
            break;
        default:
            Debug("ADPCM Write Invalid address: %04X, value: %02X", address, value);
            break;
    }

    TraceAdpcmEvent(TRACE_ADPCM_REG_WRITE, address, value);
}

INLINE u32 Adpcm::CalculateCyclesPerSample(u8 sample_rate)
{
    double frequency = 32000.0 / (16.0 - (double)sample_rate);
    return (u32)((double)GG_MASTER_CLOCK_RATE / frequency);
}

INLINE u32 Adpcm::NextSlotCycles(bool read)
{
    u64 cycles = m_core->GetMasterClockCycles();
    u8 offset = cycles % 36;

    return read ? m_read_latency[offset] : m_write_latency[offset];
}

inline void Adpcm::UpdateReadWriteEvents(u32 cycles)
{
    if (m_read_cycles > 0)
    {
        m_read_cycles -= cycles;
        if (m_read_cycles <= 0)
        {
            m_read_cycles = 0;
            m_read_value = m_adpcm_ram[m_read_address];
            m_read_address++;

            if (!IS_SET_BIT(m_control, 4))
            {
                if (m_length > 0)
                {
                    m_length--;
                    SetHalfIRQ(m_length < 0x8000);
                }
                else
                {
                    SetHalfIRQ(false);
                    SetEndIRQ(true);
                }
            }

            TraceAdpcmEvent(TRACE_ADPCM_READ_COMPLETE, 0, m_read_value);
        }
    }

    if (m_write_cycles > 0)
    {
        m_write_cycles -= cycles;
        if (m_write_cycles <= 0)
        {
            m_write_cycles = 0;
            m_adpcm_ram[m_write_address] = m_write_value;
            m_write_address++;

            SetHalfIRQ(m_length < 0x8000);
            if (m_length == 0)
                SetEndIRQ(m_length == 0);

            if (!IS_SET_BIT(m_control, 4))
            {
                m_length++;
                m_length &= 0x1FFFF;
            }

            TraceAdpcmEvent(TRACE_ADPCM_WRITE_COMPLETE, 0, m_write_value);
        }
    }
}

inline void Adpcm::UpdateDMA(u32 cycles)
{
    if ((m_dma & 0x03) == 0)
        return;

    if (m_dma_cycles > 0)
    {
        m_dma_cycles -= cycles;
        if (m_dma_cycles <= 0)
        {
            m_dma_cycles = 0;
            if (m_write_cycles == 0)
            {
                m_write_cycles = NextSlotCycles(false);
                m_write_value = m_scsi_controller->ReadData();
                m_scsi_controller->AutoAck();
                if (!m_scsi_controller->IsDataReady())
                {
                    m_dma &= ~0x01;
                    TraceAdpcmEvent(TRACE_ADPCM_DMA_STATE, 0x0B, m_dma);
                }
            }
            else
                m_dma_cycles = 1;
        }
        return;
    }

    if (!m_scsi_controller->IsSignalSet(ScsiController::SCSI_SIGNAL_ACK) &&
        !m_scsi_controller->IsSignalSet(ScsiController::SCSI_SIGNAL_CD) &&
        m_scsi_controller->IsSignalSet(ScsiController::SCSI_SIGNAL_IO) &&
        m_scsi_controller->IsSignalSet(ScsiController::SCSI_SIGNAL_REQ))
    {
        m_dma_cycles = 36;
    }
}

inline void Adpcm::RunAdpcm(u32 cycles)
{
    if (IS_SET_BIT(m_control, 7))
    {
        bool playing = IS_SET_BIT(m_control, 5);
        TraceAdpcmEvent(TRACE_ADPCM_PLAY_START, 0x0D, m_control, m_read_address);
        TraceAdpcmEvent(TRACE_ADPCM_PLAY_STOP, 0x0D, m_control, m_read_address);
        m_playing = playing;
        m_play_pending = false;
        return;
    }

    if (!m_playing && !m_play_pending)
        return;

    if (!IS_SET_BIT(m_control, 5) || (IS_SET_BIT(m_control, 6) && (m_length == 0)))
    {
        TraceAdpcmEvent(TRACE_ADPCM_PLAY_STOP, 0x0D, m_control, m_read_address);
        m_play_pending = false;
        m_playing = false;
        return;
    }

    m_adpcm_cycle_counter += cycles;
    if (m_adpcm_cycle_counter >= m_cycles_per_sample)
    {
        m_adpcm_cycle_counter -= m_cycles_per_sample;

        if (m_play_pending)
        {
            TraceAdpcmEvent(TRACE_ADPCM_PLAY_START, 0x0D, m_control, m_read_address);
            m_play_pending = false;
            m_playing = true;
            m_sample = 2048;
            m_step_index = 0;
        }

        u8 ram_byte = m_adpcm_ram[m_read_address];
        u8 nibble = 0;
        m_nibble_toggle = !m_nibble_toggle;

        if (m_nibble_toggle)
            nibble = (ram_byte >> 4) & 0x0F;
        else
        {
            nibble = ram_byte & 0x0F;
            m_read_address++;
            m_length = (m_length - 1) & 0x1FFFF;

            SetHalfIRQ(m_length <= 0x8000);
            if (m_length == 0)
                SetEndIRQ(m_length == 0);
        }

        s8 sign = (nibble & 0x08) ? -1 : 1;
        u8 value = nibble & 0x07;
        s16 delta = m_step_delta[(m_step_index << 3) + value] * sign;

        m_sample = (m_sample + delta) & 0x0FFF;

        m_step_index = CLAMP(m_step_index + k_adpcm_index_shift[value], 0, 48);
    }
}

INLINE void Adpcm::WriteControl(u8 value)
{
    if (IS_SET_BIT(value, 1) && !IS_SET_BIT(m_control, 1))
        m_write_address = m_address - (IS_SET_BIT(value, 0) ? 0 : 1);

    if (IS_SET_BIT(value, 3) && !IS_SET_BIT(m_control, 3))
        m_read_address = m_address - (IS_SET_BIT(value, 2) ? 0 : 1);

    if (IS_SET_BIT(value, 5) && !m_playing)
    {
        m_play_pending = true;
        TraceAdpcmEvent(TRACE_ADPCM_PLAY_REQUEST, 0x0D, value, m_read_address);
    }

    m_control = value;
}

INLINE void Adpcm::SetEndIRQ(bool asserted)
{
    if (asserted)
        m_cdrom->SetIRQ(CDROM_IRQ_ADPCM_END);
    else
        m_cdrom->ClearIRQ(CDROM_IRQ_ADPCM_END);
    TraceAdpcmEvent(TRACE_ADPCM_END_IRQ, 0, asserted, m_read_address);
    m_end_irq = asserted;
}

INLINE void Adpcm::SetHalfIRQ(bool asserted)
{
    if (asserted)
        m_cdrom->SetIRQ(CDROM_IRQ_ADPCM_HALF);
    else
        m_cdrom->ClearIRQ(CDROM_IRQ_ADPCM_HALF);
    TraceAdpcmEvent(TRACE_ADPCM_HALF_IRQ, 0, asserted, m_read_address);
    m_half_irq = asserted;
}

INLINE void Adpcm::TraceAdpcmEvent(u8 event, u16 reg, u8 value, u16 address)
{
    if (IsValidPointer(m_trace_logger) && m_trace_logger->IsEventEnabled(TRACE_ADPCM, event))
        LogAdpcmEvent(event, reg, value, address);
}

INLINE bool Adpcm::CheckReset()
{
    if (IS_SET_BIT(m_control, 7))
    {
        SoftReset();
        return true;
    }
    else
        return false;
}

INLINE void Adpcm::CheckLength()
{
    if (IS_SET_BIT(m_control, 4))
    {
        m_length = m_address;
        SetEndIRQ(false);
    }
}

INLINE void Adpcm::Sample()
{
    float x = (float)((int)m_sample - 2048) * 10.0f;

    const float R = 0.997f;
    float y = x - m_dc_prev_x + R * m_dc_prev_y;
    m_dc_prev_x = x;
    m_dc_prev_y = y;

    const float alpha_lpf = 0.4f;
    m_filter_state += alpha_lpf * (y - m_filter_state);

    float target_gain = m_cdrom->IsFaderEnabled(true) ? (float)m_cdrom->GetFaderValue() : 1.0f;
    const float gain_smooth = 0.003f;
    m_gain_smooth += (target_gain - m_gain_smooth) * gain_smooth;

    int out32 = (int)(m_filter_state * m_gain_smooth);
    s16 final_sample = (s16)CLAMP(out32, -32768, 32767);

    m_buffer[m_buffer_index + 0] = final_sample;
    m_buffer[m_buffer_index + 1] = final_sample;

    m_buffer_index += 2;

    if (m_buffer_index >= GG_AUDIO_BUFFER_SIZE)
    {
        Error("ADPCM buffer overflow");
        m_buffer_index = 0;
    }
}

INLINE u8* Adpcm::GetRAM()
{
    return m_adpcm_ram;
}

INLINE Adpcm::Adpcm_State* Adpcm::GetState()
{
    return &m_state;
}

#endif /* ADPCM_INLINE_H */
