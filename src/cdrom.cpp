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

CdRom::CdRom(ScsiController* scsi_controller)
{
    m_scsi_controller = scsi_controller;
}

CdRom::~CdRom()
{
}

void CdRom::Init()
{
    Reset();
}

void CdRom::Reset()
{
}

void CdRom::Clock(u32 cycles)
{
    m_scsi_controller->Clock(cycles);
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
            // SCSI get data
            //Debug("CDROM Read SCSI get data %02X", reg);
            return m_scsi_controller->ReadData();
        case 0x02:
            // IRQs
            //Debug("CDROM Read IRQs %02X", reg);
            return 0x00 | (m_scsi_controller->IsSignalSet(ScsiController::SCSI_SIGNAL_ACK) ? 0x80 : 0x00);
        case 0x03:
            // BRAM Lock
            Debug("CDROM Read BRAM Lock %02X", reg);
            return 0x00;
        case 0x04:
            // Reset
            Debug("CDROM Read Reset %02X", reg);
            return 0x00;
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
            return 0x00;
        case 0x08:
            // SCSI get data
            Debug("CDROM Read SCSI get data %02X", reg);
            return m_scsi_controller->ReadData();
        case 0x09:
        case 0x0A:
        case 0x0B:
        case 0x0C:
        case 0x0D:
        case 0x0E:
            // ADPCM Read
            Debug("CDROM Read ADPCM %02X", reg);
            return 0x00;
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
            m_scsi_controller->BusChange();
            break;
        case 0x02:
        {
            // ACK
            Debug("CDROM Write ACK %02X, value: %02X", reg, value);
            bool ack = (value & 0x80) != 0;
            if (ack)
                m_scsi_controller->SetSignal(ScsiController::SCSI_SIGNAL_ACK);
            else
                m_scsi_controller->ClearSignal(ScsiController::SCSI_SIGNAL_ACK);
            m_scsi_controller->BusChange();
            break;
        }
        case 0x04:
            // Reset
            Debug("CDROM Write Reset %02X, value: %02X", reg, value);
            break;
        case 0x05:
            // Audio Sample
            Debug("CDROM Write Audio Sample %02X, value: %02X", reg, value);
            break;
        case 0x07:
            // Is BRAM control
            Debug("CDROM Write BRAM control %02X, value: %02X", reg, value);
            break;
        case 0x08:
        case 0x09:
        case 0x0A:
        case 0x0B:
        case 0x0C:
        case 0x0D:
        case 0x0E:
            // ADPCM Write
            Debug("CDROM Write ADPCM %02X, value: %02X", reg, value);
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

CdRom::CdRom_State* CdRom::GetState()
{
    return &m_state;
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
