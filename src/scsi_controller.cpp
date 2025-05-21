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
    m_bus.db = 0;
    m_bus.signals = 0;
    m_phase = SCSI_PHASE_BUS_FREE;
    m_next_event = SCSI_EVENT_NONE;
    m_next_event_cycles = 0;
    m_next_load_cycles = 0;
    m_load_sector = 0;
    m_load_sector_count = 0;
    m_auto_ack_cycles = 0;
    m_command_buffer.clear();
    m_command_buffer.reserve(16);
    m_data_buffer.clear();
    m_data_buffer.reserve(2048);
    m_data_buffer_offset = 0;
    m_bus_changed = false;
    m_previous_signals = 0;
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
    m_bus.db = 0;
    m_bus.signals = 0;
    m_phase = SCSI_PHASE_BUS_FREE;
    m_next_event = SCSI_EVENT_NONE;
    m_next_event_cycles = 0;
    m_next_load_cycles = 0;
    m_load_sector = 0;
    m_load_sector_count = 0;
    m_auto_ack_cycles = 0;
    m_command_buffer.clear();
    m_data_buffer.clear();
    m_data_buffer_offset = 0;
    m_bus_changed = false;
    m_previous_signals = 0;
}

void ScsiController::Clock(u32 cycles)
{
    if (m_auto_ack_cycles > 0)
    {
        m_auto_ack_cycles -= cycles;
        if (m_auto_ack_cycles <= 0)
        {
            m_auto_ack_cycles = 0;
            ClearSignal(SCSI_SIGNAL_ACK);
        }
    }

    if (m_next_event != SCSI_EVENT_NONE)
    {
        m_next_event_cycles -= cycles;
        if (m_next_event_cycles <= 0)
        {
            RunEvent();
        }
    }

    if (m_bus_changed)
    {
        m_bus_changed = false;
        UpdateScsi();
    }

    if (m_next_load_cycles > 0)
    {
        m_next_load_cycles -= cycles;
        if (m_next_load_cycles <= 0)
        {
            m_next_load_cycles = 0;
            LoadSector();
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
            SetSignal(SCSI_SIGNAL_BSY | SCSI_SIGNAL_IO);
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

void ScsiController::NextEvent(ScsiEvent event, u32 cycles)
{
    m_next_event = event;
    m_next_event_cycles = cycles;
}

void ScsiController::RunEvent()
{
    switch (m_next_event)
    {
        case SCSI_EVENT_SET_COMMAND_PHASE:
            Debug("SCSI Event Set command phase");
            NextEvent(SCSI_EVENT_NONE, 0);
            SetPhase(SCSI_PHASE_COMMAND);
            break;
        case SCSI_EVENT_SET_REQ_SIGNAL:
            Debug("SCSI Event Set REQ signal");
            NextEvent(SCSI_EVENT_NONE, 0);
            SetSignal(SCSI_SIGNAL_REQ);
            break;
        case SCSI_EVENT_SET_GOOD_STATUS:
            Debug("SCSI Event Set good status");
            NextEvent(SCSI_EVENT_NONE, 0);
            StartStatus(SCSI_STATUS_GOOD);
            break;
        case SCSI_EVENT_SET_DATA_IN_PHASE:
            Debug("SCSI Event Set data in phase");
            NextEvent(SCSI_EVENT_NONE, 0);
            SetPhase(SCSI_PHASE_DATA_IN);
            break;
        default:
            break;
    }
}

void ScsiController::UpdateScsi()
{
    //Debug(">>>> SCSI UPDATE: %02X %02X <<<<----------------------", m_bus.signals, m_bus.db);

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
        // 3ms delay
        NextEvent(SCSI_EVENT_SET_COMMAND_PHASE, TimeToCycles(3000));
    }
}

void ScsiController::StartStatus(ScsiStatus status, u8 length)
{
    Debug("SCSI Start status %02X", status);
    m_data_buffer.assign(length, (u8)status);
    m_data_buffer_offset = 0;
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
            // 150us delay
            NextEvent(SCSI_EVENT_SET_REQ_SIGNAL, TimeToCycles(150));
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
        if (m_data_buffer_offset < m_data_buffer.size())
        {
            m_bus.db = m_data_buffer[m_data_buffer_offset];
            m_data_buffer_offset++;
            //Debug("SCSI Data in phase data %02X", m_bus.db);
            SetSignal(SCSI_SIGNAL_REQ);
        }
        else
        {
            if (m_load_sector_count == 0)
            {
                // 150us delay
                NextEvent(SCSI_EVENT_SET_GOOD_STATUS, TimeToCycles(150));
            }
        }
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
            CommandAudioStartPosition();
            break;
        case SCSI_CMD_AUDIO_STOP_POSITION:
            CommandAudioStopPosition();
            break;
        case SCSI_CMD_AUDIO_PAUSE:
            CommandAudioPause();
            break;
        case SCSI_CMD_READ_SUBCODE_Q:
            CommandReadSubcodeQ();
            break;
        case SCSI_CMD_READ_TOC:
            CommandReadTOC();
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

    // 21ms delay
    NextEvent(SCSI_EVENT_SET_GOOD_STATUS, TimeToCycles(21000));
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
        StartStatus(SCSI_STATUS_GOOD);
        return;
    }

    u32 current_lba = 0;//m_cdrom_media->GetCurrentSector();
    u32 seek_time = m_cdrom_media->SeekTime(current_lba, lba);
    u32 seek_cycles = TimeToCycles(seek_time * 1000);
    u32 transfer_time = m_cdrom_media->SectorTransferTime();
    u32 transfer_cycles = TimeToCycles(transfer_time * 1000);

    m_next_load_cycles = seek_cycles + transfer_cycles;
    m_load_sector = lba;
    m_load_sector_count = count;

    SetPhase(SCSI_PHASE_DATA_IN);
}

void ScsiController::CommandAudioStartPosition()
{
    Debug("******");
    Debug("SCSI CMD Audio Start Position");
    Debug("******");
    Debug("NOT IMPLEMENTED");
}

void ScsiController::CommandAudioStopPosition()
{
    Debug("******");
    Debug("SCSI CMD Audio Stop Position");
    Debug("******");
    Debug("NOT IMPLEMENTED");
}

void ScsiController::CommandAudioPause()
{
    Debug("******");
    Debug("SCSI CMD Audio Pause");
    Debug("******");
    Debug("NOT IMPLEMENTED");
}
void ScsiController::CommandReadSubcodeQ()
{
    Debug("******");
    Debug("SCSI CMD Read Subcode Q");
    Debug("******");
    Debug("NOT IMPLEMENTED");
}

void ScsiController::CommandReadTOC()
{
    Debug("******");
    Debug("SCSI CMD Read TOC");
    Debug("******");

    const int buffer_size = 4;
    u8 mode = m_command_buffer[1];

    switch (mode)
    {
        case 0x00:
        {
            Debug("Mode: Number of tracks");
            u8 buffer[buffer_size] = { 0x01, 0x00, 0x00, 0x00 };
            buffer[1] = DecToBcd((u8)m_cdrom_media->GetTracks().size());
            Debug("Number of tracks: %d", m_cdrom_media->GetTracks().size());
            m_data_buffer.assign(buffer, buffer + buffer_size);
            m_data_buffer_offset = 0;
            // 420us delay
            NextEvent(SCSI_EVENT_SET_DATA_IN_PHASE, TimeToCycles(420));
            break;
        }
        case 0x01:
        {
            Debug("Mode: Disc length");
            u8 buffer[buffer_size] = { 0x00, 0x00, 0x00, 0x00 };
            u32 length = m_cdrom_media->GetCdRomLengthLba() + 150;
            Debug("Disc length: %d", length);
            GG_CdRomMSF length_msf = { 0, 0, 0 };
            LbaToMsf(length, &length_msf);
            buffer[0] = DecToBcd(length_msf.minutes);
            buffer[1] = DecToBcd(length_msf.seconds);
            buffer[2] = DecToBcd(length_msf.frames);
            m_data_buffer.assign(buffer, buffer + buffer_size);
            m_data_buffer_offset = 0;
            // 420us delay
            NextEvent(SCSI_EVENT_SET_DATA_IN_PHASE, TimeToCycles(420));
            break;
        }
        case 0x02:
        {
            u8 track = BcdToDec(m_command_buffer[2]);
            Debug("Mode: Track %d start", track);

            if (track == 0)
                track = 1;

            u8 type = 0x04;

            if (m_cdrom_media->GetTracks()[track - 1].type == CdRomMedia::AUDIO_TRACK)
                type = 0x00;

            GG_CdRomMSF start_msf = { 0, 0, 0 };
            if (track > m_cdrom_media->GetTracks().size())
            {
                start_msf = m_cdrom_media->GetCdRomLength();
                type = 0x00;
            }
            else
            {
                u32 start_lba = m_cdrom_media->GetTracks()[track - 1].start_lba + 150;
                LbaToMsf(start_lba, &start_msf);
            }

            Debug("Track %d start: %d", track, MsfToLba(&start_msf));
            u8 buffer[buffer_size] = { 0x00, 0x00, 0x00, 0x00 };
            buffer[0] = DecToBcd(start_msf.minutes);
            buffer[1] = DecToBcd(start_msf.seconds);
            buffer[2] = DecToBcd(start_msf.frames);
            buffer[3] = type;
            m_data_buffer.assign(buffer, buffer + buffer_size);
            m_data_buffer_offset = 0;
            // 420us delay
            NextEvent(SCSI_EVENT_SET_DATA_IN_PHASE, TimeToCycles(420));
            break;
        }
        default:
            Debug("SCSI CMD Read TOC: Unknown mode %02X", mode);
            break;
    }
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

void ScsiController::LoadSector()
{
    m_data_buffer.resize(2048);
    m_cdrom_media->ReadSector(m_load_sector, m_data_buffer.data());

    Debug("SCSI Load sector %d", m_load_sector);

    m_data_buffer_offset = 0;
    m_load_sector++;
    m_load_sector &= 0x1FFFFF;
    m_load_sector_count--;

    if (m_load_sector_count == 0)
        m_next_load_cycles = 0;
    else
        m_next_load_cycles = TimeToCycles(m_cdrom_media->SectorTransferTime() * 1000);

    //SetSignal(SCSI_SIGNAL_REQ);
    m_bus_changed = true;
}
