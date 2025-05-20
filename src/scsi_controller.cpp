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

#include "scsi_controller.h"
#include "cdrom_media.h"

ScsiController::ScsiController(CdRomMedia* cdrom_media)
{
    m_cdrom_media = cdrom_media;
    m_phase = SCSI_PHASE_BUS_FREE;
    m_bus.db = 0;
    m_bus.signals = 0;
    m_command_buffer.clear();
    m_command_buffer.resize(16);
    m_read_current_lba = 0;
    m_read_sectors_remaining = 0;
    m_read_sector_offset = 0;
}

ScsiController::~ScsiController()
{
}

void ScsiController::Init()
{
    Reset();
}

void ScsiController::Reset()
{
    m_phase = SCSI_PHASE_BUS_FREE;
    m_bus.db = 0;
    m_bus.signals = 0;
    m_command_buffer.clear();
    m_read_current_lba = 0;
    m_read_sectors_remaining = 0;
    m_read_sector_offset = 0;
}

void ScsiController::Clock(u32 cycles)
{
    if (m_next_event != SCSI_EVENT_NONE)
    {
        m_next_event_cycles -= cycles;
        if (m_next_event_cycles <= 0)
        {
            switch (m_next_event)
            {
                case SCSI_EVENT_SET_COMMAND_PHASE:
                    Debug("SCSI Set command phase");
                    SetEvent(SCSI_EVENT_NONE, 0);
                    SetPhase(SCSI_PHASE_COMMAND);
                    break;
                case SCSI_EVENT_SET_REQ_SIGNAL:
                    Debug("SCSI Set REQ signal");
                    SetEvent(SCSI_EVENT_NONE, 0);
                    SetSignal(SCSI_SIGNAL_REQ);
                    break;
                default:
                    break;
            }
        }
    }
}

void ScsiController::SetPhase(ScsiPhase phase)
{
    Debug("SCSI Set phase %d", phase);

    if (m_phase == phase)
        return;

    ClearSignal(SCSI_SIGNAL_BSY | SCSI_SIGNAL_REQ | SCSI_SIGNAL_CD | SCSI_SIGNAL_MSG | SCSI_SIGNAL_IO);
    m_phase = phase;

    switch (m_phase)
    {
        case SCSI_PHASE_BUS_FREE:
            break;
        case SCSI_PHASE_COMMAND:
            SetSignal(SCSI_SIGNAL_BSY | SCSI_SIGNAL_CD | SCSI_SIGNAL_REQ);
            break;
        case SCSI_PHASE_DATA_IN:
            SetSignal(SCSI_SIGNAL_BSY | SCSI_SIGNAL_CD);
            break;
        case SCSI_PHASE_MESSAGE_IN:
            SetSignal(SCSI_SIGNAL_BSY | SCSI_SIGNAL_CD | SCSI_SIGNAL_IO | SCSI_SIGNAL_MSG | SCSI_SIGNAL_REQ);
            break;
        case SCSI_PHASE_STATUS:
            SetSignal(SCSI_SIGNAL_BSY | SCSI_SIGNAL_CD | SCSI_SIGNAL_IO | SCSI_SIGNAL_REQ);
            break;
        default:
            break;
    }
}

void ScsiController::SetEvent(ScsiEvent event, u32 cycles)
{
    m_next_event = event;
    m_next_event_cycles = cycles;
}

void ScsiController::StartSelection()
{
    Debug("SCSI Start selection");

    // If target ID is not 0, ignore
    if (m_bus.db & 0x01)
    {
        SetEvent(SCSI_EVENT_SET_COMMAND_PHASE, 75000);
    }
}

void ScsiController::BusChange()
{
    switch (m_phase)
    {
        case SCSI_PHASE_COMMAND:
            UpdateCommandPhase();
            break;
        case SCSI_PHASE_DATA_IN:
            UpdateDataInPhase();
            break;
        default:
            break;
    }
}

void ScsiController::UpdateCommandPhase()
{
    if (IsSignalSet(SCSI_SIGNAL_REQ) && IsSignalSet(SCSI_SIGNAL_ACK))
    {
        Debug("SCSI UpdateCommandPhase REQ and ACK set");
        ClearSignal(SCSI_SIGNAL_REQ);
        m_command_buffer.push_back(m_bus.db);
    }
    else if (!IsSignalSet(SCSI_SIGNAL_REQ) && !IsSignalSet(SCSI_SIGNAL_ACK) && m_command_buffer.size() > 0)
    {
        Debug("SCSI UpdateCommandPhase REQ and ACK not set");
        u8 opcode = m_command_buffer[0];
        u8 length = CommandLength((ScsiCommand)opcode);

        if (length == 0)
        {
            Debug("SCSI Unknown command %02X", opcode);
            SetPhase(SCSI_PHASE_STATUS);
            m_bus.db = 0x00; // Good
        }
        else if (length <= m_command_buffer.size())
        {
            Debug("SCSI Command complete %02X", opcode);
            for (size_t i = 0; i < length; i++)
                Debug("  Command byte %02X", m_command_buffer[i]);
            ExecuteCommand();
        }
        else
        {
            Debug("SCSI Command not complete %02X", opcode);
            SetEvent(SCSI_EVENT_SET_REQ_SIGNAL, 3000);
        }
    }
}

void ScsiController::UpdateDataInPhase()
{
    if (IsSignalSet(SCSI_SIGNAL_REQ) && IsSignalSet(SCSI_SIGNAL_ACK))
    {
        ClearSignal(SCSI_SIGNAL_REQ);
    }
    else if (!IsSignalSet(SCSI_SIGNAL_REQ) && !IsSignalSet(SCSI_SIGNAL_ACK))
    {

    }
}

u8 ScsiController::ReadData()
{
    Debug("SCSI Read data");
    return m_bus.db;
}

void ScsiController::WriteData(u8 value)
{
    Debug("SCSI Write data %02X", value);
    m_bus.db = value;
}

u8 ScsiController::GetStatus()
{
    return (m_bus.signals & 0xF8);
}

void ScsiController::ExecuteCommand()
{
    ScsiCommand command = (ScsiCommand)m_command_buffer[0];

    switch(command)
    {
        case SCSI_CMD_TEST_UNIT_READY:
            CommandTestUnitReady();
            break;
        case SCSI_CMD_REQUEST_SENSE:
            CommandRequestSense();
            break;
        case SCSI_CMD_READ:
            CommandRead();
            break;
        case SCSI_CMD_AUDIO_START_POSITION:
            //CommandAudioStartPosition();
            break;
        case SCSI_CMD_AUDIO_STOP_POSITION:
            //CommandAudioStopPosition();
            break;
        case SCSI_CMD_AUDIO_PAUSE:
            //CommandAudioPause();
            break;
        case SCSI_CMD_READ_SUBCODE_Q:
            //CommandReadSubcodeQ();
            break;
        case SCSI_CMD_READ_TOC:
            //CommandReadTOC();
            break;
        default:
            Debug("SCSI Unknown command %02X", command);
            break;
    }
}

void ScsiController::CommandTestUnitReady()
{
    Debug("SCSI CMD Test Unit Ready");
}

void ScsiController::CommandRequestSense()
{
    Debug("SCSI CMD Request Sense");
}

void ScsiController::CommandRead()
{
    Debug("SCSI CMD Read");


    u32 lba = ((m_command_buffer[1] & 0x1F) << 16) | (m_command_buffer[2] << 8) | m_command_buffer[3];
    u16 count = m_command_buffer[4];

    if (count == 0)
    {
        Debug("SCSI CMD Read: count is 0");
        SetPhase(SCSI_PHASE_STATUS);
        m_bus.db = 0x00; // Good
    }

    //m_cdrom_media->ReadSector(m_read_current_lba, m_data_buffer)

    SetPhase(SCSI_PHASE_DATA_IN);
}

u8 ScsiController::CommandLength(ScsiCommand command)
{
    switch (command)
    {
        case SCSI_CMD_TEST_UNIT_READY:
        case SCSI_CMD_READ:
        case SCSI_CMD_REQUEST_SENSE:
            return 6;
        case SCSI_CMD_AUDIO_START_POSITION:
        case SCSI_CMD_AUDIO_STOP_POSITION:
        case SCSI_CMD_AUDIO_PAUSE:
        case SCSI_CMD_READ_SUBCODE_Q:
        case SCSI_CMD_READ_TOC:
            return 10;
        default:
            return 0;
    }
}
