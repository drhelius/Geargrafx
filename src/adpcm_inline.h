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

INLINE void Adpcm::Clock(u32 cycles)
{
    CheckReset();
    CheckLength();
    UpdateAudio(cycles);
    UpdateReadWriteEvents(cycles);
    UpdateDMA(cycles);
    CheckLength();
    CheckReset();
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
            m_status = 0;
            m_status |= (m_playing ? 0x08 : 0x00);
            m_status |= (m_end ? 0x01 : 0x00);
            m_status |= (m_read_cycles > 0 ? 0x80 : 0x00);
            m_status |= (m_write_cycles > 0 ? 0x04 : 0x00);
            return m_status;
        case 0x0D:
            return m_control;
        case 0x0E:
            return m_sample_rate;
        default:
            Debug("ADPCM Read Invalid address: %04X", address);
            return 0;
    }
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
            if (!m_scsi_controller->IsDataReady())
                value &= ~0x01;
            m_dma = value;
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
}

INLINE u32 Adpcm::CalculateCyclesPerSample(u8 sample_rate)
{
    float frequency = 32000.0f / (16.0f - (float)sample_rate);
    return (u32)(GG_MASTER_CLOCK_RATE / frequency);
}

INLINE u32 Adpcm::NextSlotCycles(bool read)
{
    u64 cycles = m_core->GetMasterClockCycles();
    u8 offset = cycles % 36;

    return read ? m_read_latency[offset] : m_write_latency[offset];
}

INLINE void Adpcm::UpdateReadWriteEvents(u32 cycles)
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
                if (m_lenght > 0)
                {
                    m_lenght--;
                    HalfReached(m_lenght < 0x8000);
                }
                else
                {
                    HalfReached(false);
                    EndReached(true);
                }
            }
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

            if (m_lenght == 0)
                EndReached(true);

            HalfReached(m_lenght < 0x8000);

            if (!IS_SET_BIT(m_control, 4))
            {
                m_lenght++;
                m_lenght &= 0x1FFFF;
            }
        }
    }
}

INLINE void Adpcm::UpdateDMA(u32 cycles)
{
    bool dma_active = (m_dma & 0x03) != 0;

    if (!dma_active)
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
                    m_dma &= ~0x01;
            }
            else
                m_dma_cycles = 1;
        }
    }
    else if(!m_scsi_controller->IsSignalSet(ScsiController::SCSI_SIGNAL_ACK) &&
            !m_scsi_controller->IsSignalSet(ScsiController::SCSI_SIGNAL_CD) &&
            m_scsi_controller->IsSignalSet(ScsiController::SCSI_SIGNAL_IO) &&
            m_scsi_controller->IsSignalSet(ScsiController::SCSI_SIGNAL_REQ))
    {
        m_dma_cycles = 36;
    }
}

INLINE void Adpcm::UpdateAudio(u32 cycles)
{
    if (!m_playing)
        return;

    m_sample_cycle_counter += cycles;
    if (m_sample_cycle_counter >= m_cycles_per_sample)
    {
        m_sample_cycle_counter -= m_cycles_per_sample;

    }
}

INLINE void Adpcm::WriteControl(u8 value)
{
    if (IS_SET_BIT(value, 1) && !IS_SET_BIT(m_control, 1))
    {
        m_write_address = m_address - (IS_SET_BIT(value, 0) ? 0 : 1);
    }

    if (IS_SET_BIT(value, 3) && !IS_SET_BIT(m_control, 3))
    {
        m_read_address = m_address - (IS_SET_BIT(value, 2) ? 0 : 1);
    }

    m_control = value;
}

INLINE void Adpcm::EndReached(bool end)
{
    if (m_end != end)
    {
        m_end = end;
        if (m_end)
            m_cdrom->SetIRQ(CDROM_IRQ_ADPCM_END);
        else
            m_cdrom->ClearIRQ(CDROM_IRQ_ADPCM_END);
    }
}

INLINE void Adpcm::HalfReached(bool half)
{
    if (m_half != half)
    {
        m_half = half;
        if (m_half)
            m_cdrom->SetIRQ(CDROM_IRQ_ADPCM_HALF);
        else
            m_cdrom->ClearIRQ(CDROM_IRQ_ADPCM_HALF);
    }
}

INLINE bool Adpcm::CheckReset()
{
    if (IS_SET_BIT(m_control, 7))
    {
        ResetAdpcm();
        return true;
    }
    else
        return false;
}

INLINE void Adpcm::CheckLength()
{
    if (IS_SET_BIT(m_control, 4))
    {
        m_lenght = m_address;
        EndReached(false);
    }
}

#endif /* ADPCM_INLINE_H */