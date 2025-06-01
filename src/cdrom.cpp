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

#include "cdrom.h"
#include "scsi_controller.h"
#include "huc6280.h"
#include "memory.h"
#include "audio.h"

CdRom::CdRom(ScsiController* scsi_controller, Audio* audio)
{
    m_scsi_controller = scsi_controller;
    m_audio = audio;
    InitPointer(m_adpcm);
    InitPointer(m_memory);
    m_reset = 0;
    m_bram_enabled = false;
    m_active_irqs = 0;
    m_enabled_irqs = 0;
    m_state.RESET = &m_reset;
    m_state.BRAM_ENABLED = &m_bram_enabled;
    m_state.ACTIVE_IRQS = &m_active_irqs;
    m_state.ENABLED_IRQS = &m_enabled_irqs;
}

CdRom::~CdRom()
{
}

void CdRom::Init(HuC6280* huc6280, Memory* memory)
{
    m_huc6280 = huc6280;
    m_memory = memory;
    m_adpcm = m_audio->GetAdpcm();
    Reset();
}

void CdRom::Reset()
{
    m_reset = 0;
    m_bram_enabled = true;
    m_active_irqs = 0;
    m_enabled_irqs = 0;
    m_memory->UpdateBackupRam(m_bram_enabled);
}

u8 CdRom::ReadRegister(u16 address)
{
    u16 reg = address & 0x3FF;
    switch (reg)
    {
        case 0x00:
            // SCSI get status
            //Debug("CDROM Read SCSI get status %02X", reg);
            return m_scsi_controller->GetStatus();
        case 0x01:
        {
            // SCSI get data
            u8 ret = m_scsi_controller->ReadData();
            Debug("CDROM Read %02X SCSI get data: %02X", reg, ret);
            return ret;
        }
        case 0x02:
            // IRQs
            //Debug("CDROM Read IRQs %02X", reg);
            return (m_enabled_irqs & 0x7F) | (m_scsi_controller->IsSignalSet(ScsiController::SCSI_SIGNAL_ACK) ? 0x80 : 0x00);
        case 0x03:
        {
            // BRAM Lock
            Debug("CDROM Read BRAM Lock %02X", reg);
            m_bram_enabled = false;
            m_memory->UpdateBackupRam(m_bram_enabled);
            u8 ret = m_active_irqs | 0x10;
            return ret;
        }
        case 0x04:
            // Reset
            Debug("CDROM Read Reset %02X", reg);
            return m_reset;
        case 0x05:
            // Audio Sample LSB
            Debug("CDROM Read Audio Sample LSB %02X", reg);
            return 0x00;
        case 0x06:
            // Audio Sample MSB
            Debug("CDROM Read Audio Sample MSB %02X", reg);
            return 0x00;
        case 0x07:
            // Is BRAM Locked?
            Debug("CDROM Read Is BRAM Locked? %02X", reg);
            return m_bram_enabled ? 0x80 : 0x00;
        case 0x08:
        {
            // SCSI get data
            //Debug("+++ CDROM Read SCSI get data %02X", reg);
            u8 ret = m_scsi_controller->ReadData();
            //Debug("CDROM Read %02X SCSI get data: %02X", reg, ret);
            m_scsi_controller->AutoAck();
            return ret;
        }
        case 0x09:
        case 0x0A:
        case 0x0B:
        case 0x0C:
        case 0x0D:
        case 0x0E:
            // ADPCM Read
            //Debug("CDROM Read ADPCM %02X", reg);
            return m_adpcm->Read(reg);
        case 0x0F:
            // Audio Fader
            Debug("CDROM Read Audio Fader %02X", reg);
            return 0x00;
        case 0xC0:
        case 0xC1:
        case 0xC2:
        case 0xC3:
            // CDROM Signature
            Debug("CDROM Read Signature %02X", reg);
            if (true)
                return k_super_cdrom_signature[reg & 0x03];
            else
                return 0xFF;
        default:
            Debug("CDROM Read Invalid register %04X", reg);
            return 0xFF;
    }
}

void CdRom::WriteRegister(u16 address, u8 value)
{
    u16 reg = address & 0x3FF;
    switch (reg)
    {
        case 0x00:
            // SCSI control
            Debug("CDROM Write SCSI control %02X, value: %02X", reg, value);
            m_scsi_controller->StartSelection();
            break;
        case 0x01:
            // SCSI command
            //Debug("CDROM Write SCSI command %02X, value: %02X", reg, value);
            m_scsi_controller->WriteData(value);
            break;
        case 0x02:
        {
            // ACK
            //Debug("CDROM Write ACK %02X, value: %02X", reg, value);
            if ((value & 0x80) != 0)
                m_scsi_controller->SetSignal(ScsiController::SCSI_SIGNAL_ACK);
            else
                m_scsi_controller->ClearSignal(ScsiController::SCSI_SIGNAL_ACK);

            m_enabled_irqs = value & 0x7F;
            AssertIRQ2();
            break;
        }
        case 0x04:
            // Reset
            Debug("CDROM Write Reset %02X, value: %02X", reg, value);
            m_reset = value & 0x0F;
            if ((value & 0x02) != 0)
            {
                m_scsi_controller->SetSignal(ScsiController::SCSI_SIGNAL_RST);
                ClearIRQ(CDROM_IRQ_DATA_IN | CDROM_IRQ_STATUS_AND_MSG_IN);
            }
            else
                m_scsi_controller->ClearSignal(ScsiController::SCSI_SIGNAL_RST);
            break;
        case 0x05:
            // Audio Sample
            Debug("CDROM Write Audio Sample %02X, value: %02X", reg, value);
            break;
        case 0x07:
            // Is BRAM control
            Debug("CDROM Write BRAM control %02X, value: %02X", reg, value);
            m_bram_enabled = (value & 0x80) != 0;
            m_memory->UpdateBackupRam(m_bram_enabled);
            break;
        case 0x08:
        case 0x09:
        case 0x0A:
        case 0x0B:
        case 0x0C:
        case 0x0D:
        case 0x0E:
            // ADPCM Write
            //Debug("CDROM Write ADPCM %02X, value: %02X", reg, value);
            m_adpcm->Write(reg, value);
            break;
        case 0x0F:
            // Audio Fader
            Debug("CDROM Write Audio Fader %02X, value: %02X", reg, value);
            break;
        default:
            Debug("CDROM Write Invalid register %04X, value: %02X", reg, value);
            break;
    }
}

void CdRom::SaveState(std::ostream& stream)
{
    UNUSED(stream);
    using namespace std;
}

void CdRom::LoadState(std::istream& stream)
{
    UNUSED(stream);
    using namespace std;
}
