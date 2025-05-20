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
    m_command_buffer.reserve(16);
    m_data_buffer.clear();
    m_data_buffer.reserve(8192);
    m_data_buffer_offset = 0;
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
    m_data_buffer.clear();
    m_data_buffer_offset = 0;
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
                    Debug("SCSI Event Set command phase");
                    SetEvent(SCSI_EVENT_NONE, 0);
                    SetPhase(SCSI_PHASE_COMMAND);
                    break;
                case SCSI_EVENT_SET_REQ_SIGNAL:
                    Debug("SCSI Event Set REQ signal");
                    SetEvent(SCSI_EVENT_NONE, 0);
                    SetSignal(SCSI_SIGNAL_REQ);
                    break;
                case SCSI_SET_GOOD_STATUS:
                    Debug("SCSI Event Set good status");
                    SetEvent(SCSI_EVENT_NONE, 0);
                    StartStatus(SCSI_STATUS_GOOD);
                    break;
                default:
                    break;
            }
        }
    }
}

u8 ScsiController::ReadData()
{
    Debug("SCSI Read data: %02X", m_bus.db);
    return m_bus.db;
}

void ScsiController::WriteData(u8 value)
{
    Debug("SCSI Write data: %02X", value);
    m_bus.db = value;
}

u8 ScsiController::GetStatus()
{
    return (m_bus.signals & 0xF8);
}

void ScsiController::SetPhase(ScsiPhase phase)
{
    Debug("----------------");
    Debug("SCSI Set phase %s", k_scsi_phase_names[phase]);
    Debug("----------------");

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

void ScsiController::BusChange()
{
    Debug("SCSI Bus change: %02X %02X", m_bus.signals, m_bus.db);
    switch (m_phase)
    {
        case SCSI_PHASE_COMMAND:
            UpdateCommandPhase();
            break;
        case SCSI_PHASE_DATA_IN:
            UpdateDataInPhase();
            break;
        case SCSI_PHASE_STATUS:
            UpdateStatusPhase();
            break;
        case SCSI_PHASE_MESSAGE_IN:
            UpdateMessageInPhase();
            break;
        default:
            break;
    }
}

void ScsiController::StartSelection()
{
    Debug("SCSI Start selection");

    // If target ID is not 0, ignore
    if (m_bus.db & 0x01)
    {
        // 1ms delay
        SetEvent(SCSI_EVENT_SET_COMMAND_PHASE, TimeToCycles(1000));
    }
}

void ScsiController::StartStatus(ScsiStatus status, u8 length)
{
    Debug("SCSI Start status %02X", status);
    m_data_buffer.assign(length, (u8)status);
    m_bus.db = (u8)status;
    SetPhase(SCSI_PHASE_STATUS);
}

void ScsiController::UpdateCommandPhase()
{
    if (IsSignalSet(SCSI_SIGNAL_REQ) && IsSignalSet(SCSI_SIGNAL_ACK))
    {
        ClearSignal(SCSI_SIGNAL_REQ);
        m_command_buffer.push_back(m_bus.db);
    }
    else if (!IsSignalSet(SCSI_SIGNAL_REQ) && !IsSignalSet(SCSI_SIGNAL_ACK) && m_command_buffer.size() > 0)
    {
        u8 opcode = m_command_buffer[0];
        u8 length = CommandLength((ScsiCommand)opcode);

        if (length == 0)
        {
            Debug("SCSI Unknown command %02X", opcode);
            StartStatus(SCSI_STATUS_GOOD);
            m_command_buffer.clear();
        }
        else if (length <= m_command_buffer.size())
        {
            Debug("SCSI Command complete %02X", opcode);
            for (size_t i = 0; i < length; i++)
                Debug("  Command byte %02X", m_command_buffer[i]);
            ExecuteCommand();
            m_command_buffer.clear();
        }
        else
        {
            Debug("SCSI Command not complete %02X", opcode);
            // 50us delay
            SetEvent(SCSI_EVENT_SET_REQ_SIGNAL, TimeToCycles(50));
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

void ScsiController::UpdateStatusPhase()
{
    if (IsSignalSet(SCSI_SIGNAL_REQ) && IsSignalSet(SCSI_SIGNAL_ACK))
    {
        ClearSignal(SCSI_SIGNAL_REQ);
    }
    else if (!IsSignalSet(SCSI_SIGNAL_REQ) && !IsSignalSet(SCSI_SIGNAL_ACK))
    {
        if (m_data_buffer_offset < m_data_buffer.size())
        {
            m_bus.db = m_data_buffer[m_data_buffer_offset];
            m_data_buffer_offset++;
            if (m_data_buffer_offset == m_data_buffer.size())
            {
                Debug("SCSI Status phase complete");
                SetPhase(SCSI_PHASE_MESSAGE_IN);
            }
            else
            {
                Debug("SCSI Status phase data %02X", m_bus.db);
                SetSignal(SCSI_SIGNAL_REQ);
            }
        }
    }
}

void ScsiController::UpdateMessageInPhase()
{
    if (IsSignalSet(SCSI_SIGNAL_REQ) && IsSignalSet(SCSI_SIGNAL_ACK))
    {
        ClearSignal(SCSI_SIGNAL_REQ);
    }
    else if (!IsSignalSet(SCSI_SIGNAL_REQ) && !IsSignalSet(SCSI_SIGNAL_ACK))
    {
        Debug("SCSI Message in phase complete");
        SetPhase(SCSI_PHASE_BUS_FREE);
    }
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
    Debug("******");
    Debug("SCSI CMD Test Unit Ready");
    Debug("******");

    // 7ms delay
    SetEvent(SCSI_SET_GOOD_STATUS, TimeToCycles(7000));
}

void ScsiController::CommandRequestSense()
{
    Debug("******");
    Debug("SCSI CMD Request Sense");
    Debug("******");
}

void ScsiController::CommandRead()
{
    Debug("******");
    Debug("SCSI CMD Read");
    Debug("******");

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
