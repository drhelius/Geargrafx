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

CdRom::CdRom(CdRomAudio* cdrom_audio, ScsiController* scsi_controller, Audio* audio, GeargrafxCore* core)
{
    m_core = core;
    m_cdrom_audio = cdrom_audio;
    m_scsi_controller = scsi_controller;
    m_audio = audio;
    InitPointer(m_adpcm);
    InitPointer(m_memory);
    m_reset = 0;
    m_bram_enabled = false;
    m_active_irqs = 0;
    m_enabled_irqs = 0;
    m_cdaudio_sample_toggle = false;
    m_cdaudio_sample = 0;
    m_cdaudio_sample_last_clock = 0;
    m_state.RESET = &m_reset;
    m_state.BRAM_ENABLED = &m_bram_enabled;
    m_state.ACTIVE_IRQS = &m_active_irqs;
    m_state.ENABLED_IRQS = &m_enabled_irqs;
}

CdRom::~CdRom()
{
}

void CdRom::Init(HuC6280* huc6280, Memory* memory, Adpcm* adpcm)
{
    m_huc6280 = huc6280;
    m_memory = memory;
    m_adpcm = adpcm;
    Reset();
}

void CdRom::Reset()
{
    m_reset = 0;
    m_bram_enabled = true;
    m_active_irqs = 0;
    m_enabled_irqs = 0;
    m_cdaudio_sample_toggle = false;
    m_cdaudio_sample = 0;
    m_cdaudio_sample_last_clock = 0;
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
            //Debug("CDROM Read %02X SCSI get data: %02X", reg, ret);
            return ret;
        }
        case 0x02:
            // IRQs
            //Debug("CDROM Read IRQs %02X", reg);
            return (m_enabled_irqs & 0x7F) | (m_scsi_controller->IsSignalSet(ScsiController::SCSI_SIGNAL_ACK) ? 0x80 : 0x00);
        case 0x03:
        {
            // BRAM Lock
            //Debug("CDROM Read BRAM Lock %02X", reg);
            m_bram_enabled = false;
            m_memory->UpdateBackupRam(m_bram_enabled);
            return m_active_irqs | 0x10 | (m_cdaudio_sample_toggle ? 0x00 : 0x02);
        }
        case 0x04:
            // Reset
            //Debug("CDROM Read Reset %02X", reg);
            return m_reset;
        case 0x05:
            // Audio Sample LSB
            //Debug("CDROM Read Audio Sample LSB %02X", reg);
            return (u8)(m_cdaudio_sample & 0xFF);
        case 0x06:
            // Audio Sample MSB
            //Debug("CDROM Read Audio Sample MSB %02X", reg);
            return (u8)((m_cdaudio_sample >> 8) & 0xFF);
        case 0x07:
            // Is BRAM Locked?
            //Debug("CDROM Read Is BRAM Locked? %02X", reg);
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
            return m_cdrom_audio->ReadFader();
        case 0xC0:
        case 0xC1:
        case 0xC2:
        case 0xC3:
            // CDROM Signature
            //Debug("CDROM Read Signature %02X", reg);
            if (m_core->GetMedia()->GetCDROMType() != GG_CDROM_STANDARD)
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
            //Debug("CDROM Write SCSI control %02X, value: %02X", reg, value);
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
            //Debug("CDROM Write Reset %02X, value: %02X", reg, value);
            m_reset = value & 0x0F;
            if ((value & 0x02) != 0)
            {
                m_scsi_controller->SetSignal(ScsiController::SCSI_SIGNAL_RST);
                m_active_irqs &= 0x8F;
                AssertIRQ2();
            }
            else
                m_scsi_controller->ClearSignal(ScsiController::SCSI_SIGNAL_RST);
            break;
        case 0x05:
            // Audio Sample
            LatchCdAudioSample();
            break;
        case 0x07:
            // Is BRAM control
            //Debug("CDROM Write BRAM control %02X, value: %02X", reg, value);
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
            m_cdrom_audio->WriteFader(value);
            break;
        default:
            Debug("CDROM Write Invalid register %04X, value: %02X", reg, value);
            break;
    }
}

void CdRom::SaveState(std::ostream& stream)
{
    using namespace std;

    stream.write(reinterpret_cast<const char*> (&m_reset), sizeof(m_reset));
    stream.write(reinterpret_cast<const char*> (&m_bram_enabled), sizeof(m_bram_enabled));
    stream.write(reinterpret_cast<const char*> (&m_active_irqs), sizeof(m_active_irqs));
    stream.write(reinterpret_cast<const char*> (&m_enabled_irqs), sizeof(m_enabled_irqs));
    stream.write(reinterpret_cast<const char*> (&m_cdaudio_sample_toggle), sizeof(m_cdaudio_sample_toggle));
    stream.write(reinterpret_cast<const char*> (&m_cdaudio_sample), sizeof(m_cdaudio_sample));
    stream.write(reinterpret_cast<const char*> (&m_cdaudio_sample_last_clock), sizeof(m_cdaudio_sample_last_clock));
}

void CdRom::LoadState(std::istream& stream)
{
    using namespace std;

    stream.read(reinterpret_cast<char*> (&m_reset), sizeof(m_reset));
    stream.read(reinterpret_cast<char*> (&m_bram_enabled), sizeof(m_bram_enabled));
    stream.read(reinterpret_cast<char*> (&m_active_irqs), sizeof(m_active_irqs));
    stream.read(reinterpret_cast<char*> (&m_enabled_irqs), sizeof(m_enabled_irqs));
    stream.read(reinterpret_cast<char*> (&m_cdaudio_sample_toggle), sizeof(m_cdaudio_sample_toggle));
    stream.read(reinterpret_cast<char*> (&m_cdaudio_sample), sizeof(m_cdaudio_sample));
    stream.read(reinterpret_cast<char*> (&m_cdaudio_sample_last_clock), sizeof(m_cdaudio_sample_last_clock));
}
