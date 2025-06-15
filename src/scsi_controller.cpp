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

#include <assert.h> 
#include "scsi_controller.h"
#include "cdrom.h"
#include "cdrom_media.h"
#include "cdrom_audio.h"
#include "huc6280.h"

ScsiController::ScsiController(CdRomMedia* cdrom_media, CdRomAudio* cdrom_audio)
{
    m_cdrom_media = cdrom_media;
    m_cdrom_audio = cdrom_audio;
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
    m_data_bus_latch = 0;

    m_state.DB = &m_bus.db;
    m_state.SIGNALS = &m_bus.signals;
    m_state.PHASE = &m_phase;
    m_state.NEXT_EVENT = &m_next_event;
    m_state.NEXT_EVENT_CYCLES = &m_next_event_cycles;
    m_state.NEXT_LOAD_CYCLES = &m_next_load_cycles;
    m_state.LOAD_SECTOR = &m_load_sector;
    m_state.LOAD_SECTOR_COUNT = &m_load_sector_count;
    m_state.AUTO_ACK_CYCLES = &m_auto_ack_cycles;
    m_state.COMMAND_BUFFER = &m_command_buffer;
    m_state.DATA_BUFFER = &m_data_buffer;
    m_state.DATA_BUFFER_OFFSET = &m_data_buffer_offset;
}

ScsiController::~ScsiController()
{
}

void ScsiController::Init(HuC6280* huc6280, CdRom* cdrom)
{
    m_huc6280 = huc6280;
    m_cdrom = cdrom;
    Reset();
}

void ScsiController::Reset(bool keep_rst_signal)
{
    m_bus.db = 0;
    if (keep_rst_signal)
        m_bus.signals &= SCSI_SIGNAL_RST;
    else    
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
    m_previous_signals = m_bus.signals;
    m_data_bus_latch = 0;
    UpdateIRQs();
    m_cdrom_audio->StopAudio();
    m_cdrom_media->SetCurrentSector(0);
    Debug("SCSI Reset");
}

ScsiController::Scsi_State* ScsiController::GetState()
{
    return &m_state;
}

void ScsiController::StartSelection()
{
    Debug("SCSI Start selection");

    if (m_phase != SCSI_PHASE_DATA_IN)
        NextEvent(SCSI_EVENT_SET_COMMAND_PHASE, 75000);
    else
    {
        Debug("SCSI Start selection when already in data in phase");
        StartStatus(SCSI_STATUS_GOOD, 8);
        NextEvent(SCSI_EVENT_SET_COMMAND_PHASE, 900000);
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
            m_next_load_cycles = 0;
            m_data_buffer.clear();
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
        case SCSI_PHASE_BUSY:
            SetSignal(SCSI_SIGNAL_BSY);
            break;
        default:
            break;
    }
}

void ScsiController::UpdateScsi()
{
    if (!m_bus_changed)
        return;

    m_bus_changed = false;

    if(IsSignalSet(SCSI_SIGNAL_RST))
    {
        Reset(true);
        return;
    }

    do
    {
        m_bus_changed = false;
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
    while (m_bus_changed);

    UpdateIRQs();
}

void ScsiController::UpdateCommandPhase()
{
    if (IsSignalSet(SCSI_SIGNAL_REQ) && IsSignalSet(SCSI_SIGNAL_ACK))
    {
        ClearSignal(SCSI_SIGNAL_REQ);
        m_command_buffer.push_back(m_data_bus_latch);
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
        else if (m_command_buffer.size() >= length)
        {
            for (size_t i = 0; i < length; i++)
                Debug("  Command byte %02X", m_command_buffer[i]);
            ExecuteCommand();
            m_command_buffer.clear();
        }
        else
            NextEvent(SCSI_EVENT_SET_REQ_SIGNAL, 3000);
    }
}

void ScsiController::UpdateDataInPhase()
{
    if (IsSignalSet(SCSI_SIGNAL_REQ) && IsSignalSet(SCSI_SIGNAL_ACK))
        ClearSignal(SCSI_SIGNAL_REQ);
    else if (!IsSignalSet(SCSI_SIGNAL_REQ) && !IsSignalSet(SCSI_SIGNAL_ACK))
    {
        if (m_data_buffer.size() > 0)
        {
            assert(m_data_buffer_offset < m_data_buffer.size());
            m_bus.db = m_data_buffer[m_data_buffer_offset];
            m_data_buffer_offset++;
            if (m_data_buffer_offset == m_data_buffer.size())
            {
                Debug("SCSI Data in phase completed %02X, %d", m_bus.signals, m_data_buffer_offset);
                m_data_buffer.clear();
            }
            SetSignal(SCSI_SIGNAL_REQ);
        }
        else
        {
            if (m_load_sector_count == 0)
            {
                Debug("SCSI Data in phase completed %02X, %d. No more sectors.", m_bus.signals, m_data_buffer_offset);
                NextEvent(SCSI_EVENT_SET_GOOD_STATUS, 3000);
            }
        }
    }
}

void ScsiController::UpdateStatusPhase()
{
    if (IsSignalSet(SCSI_SIGNAL_REQ) && IsSignalSet(SCSI_SIGNAL_ACK))
        ClearSignal(SCSI_SIGNAL_REQ);
    else if (!IsSignalSet(SCSI_SIGNAL_REQ) && !IsSignalSet(SCSI_SIGNAL_ACK))
    {
        if (m_data_buffer.size() > 0)
        {
            assert(m_data_buffer_offset < m_data_buffer.size());
            m_bus.db = m_data_buffer[m_data_buffer_offset];
            m_data_buffer_offset++;
            if (m_data_buffer_offset == m_data_buffer.size())
            {
                Debug("SCSI Status phase completed");
                m_data_buffer.clear();
                SetPhase(SCSI_PHASE_MESSAGE_IN);
            }
            else
                SetSignal(SCSI_SIGNAL_REQ);
        }
    }
}

void ScsiController::UpdateMessageInPhase()
{
    if (IsSignalSet(SCSI_SIGNAL_REQ) && IsSignalSet(SCSI_SIGNAL_ACK))
        ClearSignal(SCSI_SIGNAL_REQ);
    else if (!IsSignalSet(SCSI_SIGNAL_REQ) && !IsSignalSet(SCSI_SIGNAL_ACK))
    {
        Debug("SCSI Message in phase completed");
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
            Debug("SCSI Trying to execute unknown command %02X", command);
            break;
    }
}

void ScsiController::CommandTestUnitReady()
{
    Debug("******");
    Debug("SCSI CMD Test Unit Ready");
    Debug("******");

    NextEvent(SCSI_EVENT_SET_GOOD_STATUS, 450000);
}

void ScsiController::CommandRequestSense()
{
    Debug("******");
    Debug("SCSI CMD Request Sense");
    Debug("******");

    NextEvent(SCSI_EVENT_SET_GOOD_STATUS, TimeToCycles(21000));
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

    u32 current_lba = m_cdrom_media->GetCurrentSector();
    u32 seek_time = m_cdrom_media->SeekTime(current_lba, lba);
    u32 seek_cycles = TimeToCycles(seek_time * 1000);
    u32 transfer_cycles = m_cdrom_media->SectorTransferCycles();

    m_next_load_cycles = seek_cycles + transfer_cycles;
    m_load_sector = lba;
    m_load_sector_count = count;

    Debug("SCSI CMD Read: current lba %d, target lba %d, count %d, seek cycles %d, transfer cycles %d", current_lba, lba, count, seek_cycles, transfer_cycles);

    SetPhase(SCSI_PHASE_DATA_IN);
    m_cdrom_audio->SetIdle();
}

void ScsiController::CommandAudioStartPosition()
{
    Debug("******");
    Debug("SCSI CMD Audio Start Position");
    Debug("******");

    u32 start_lba = AudioLBA();
    u8 mode = m_command_buffer[1];

    Debug("SCSI CMD Audio Start Position: start LBA %d, mode %02X", start_lba, mode);

    m_cdrom_audio->StartAudio(start_lba, mode == 0);

    SetPhase(SCSI_PHASE_BUSY);
}

void ScsiController::CommandAudioStopPosition()
{
    Debug("******");
    Debug("SCSI CMD Audio Stop Position");
    Debug("******");

    u32 stop_lba = AudioLBA();
    u8 mode = m_command_buffer[1];

    Debug("SCSI CMD Audio Stop Position: stop LBA %d, mode %02X", stop_lba, mode);

    switch (mode)
    {
        case 0:
            m_cdrom_audio->StopAudio();
            break;
        case 1:
            m_cdrom_audio->SetStopLBA(stop_lba, CdRomAudio::CD_AUDIO_STOP_EVENT_LOOP);
            break;
        case 2:
            m_cdrom_audio->SetStopLBA(stop_lba, CdRomAudio::CD_AUDIO_STOP_EVENT_IRQ);
            break;
        case 3:
            m_cdrom_audio->SetStopLBA(stop_lba, CdRomAudio::CD_AUDIO_STOP_EVENT_STOP);
            break;
        default:
            Debug("SCSI CMD Audio Stop Position: Unknown mode %02X", m_command_buffer[1]);
            break;
    }

    if((mode == 1) || (mode == 2))
        SetPhase(SCSI_PHASE_BUSY);
    else
        StartStatus(SCSI_STATUS_GOOD);
}

void ScsiController::CommandAudioPause()
{
    Debug("******");
    Debug("SCSI CMD Audio Pause");
    Debug("******");

    m_cdrom_audio->PauseAudio();

    StartStatus(SCSI_STATUS_GOOD);
}

void ScsiController::CommandReadSubcodeQ()
{
    Debug("******");
    Debug("SCSI CMD Read Subcode Q");
    Debug("******");

    CdRomAudio::CdAudioState audio_state = m_cdrom_audio->GetCurrentState();
    u32 current_lba = m_cdrom_media->GetCurrentSector();
    s32 current_track = m_cdrom_media->GetTrackFromLBA(current_lba);
    bool is_data_track = current_track >= 0 ? m_cdrom_media->GetTrackType(current_track) != CdRomMedia::AUDIO_TRACK : false;
    u8 adr_control = is_data_track ? 0x41 : 0x01;

    u32 lba_offset = current_track >= 0 ? m_cdrom_media->GetFirstSectorOfTrack(current_track) - current_lba : 0;

    GG_CdRomMSF relative;
    GG_CdRomMSF absolute;

    LbaToMsf(lba_offset, &relative);
    LbaToMsf(current_lba, &absolute);

    const int buffer_size = 10;
    u8 buffer[buffer_size] = { };

    buffer[0] = (u8)audio_state;
    buffer[1] = adr_control;
    buffer[2] = DecToBcd(current_track + 1);
    buffer[3] = 1;
    buffer[4] = DecToBcd(relative.minutes);
    buffer[5] = DecToBcd(relative.seconds);
    buffer[6] = DecToBcd(relative.frames);
    buffer[7] = DecToBcd(absolute.minutes);
    buffer[8] = DecToBcd(absolute.seconds);
    buffer[9] = DecToBcd(absolute.frames);

    m_data_buffer.assign(buffer, buffer + buffer_size);
    m_data_buffer_offset = 0;

    Debug("SCSI CMD Read Subcode Q: audio state %d, track %d, relative %02X:%02X:%02X, absolute %02X:%02X:%02X",
          audio_state, current_track + 1, relative.minutes, relative.seconds, relative.frames,
          absolute.minutes, absolute.seconds, absolute.frames);

    NextEvent(SCSI_EVENT_SET_DATA_IN_PHASE, 3000);
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
            u8 buffer[buffer_size] = { 0x01, 0x00, 0x00, 0x00 };
            buffer[1] = DecToBcd((u8)m_cdrom_media->GetTracks().size());
            Debug("Number of tracks: %d", m_cdrom_media->GetTracks().size());
            m_data_buffer.assign(buffer, buffer + buffer_size);
            m_data_buffer_offset = 0;
            NextEvent(SCSI_EVENT_SET_DATA_IN_PHASE, 9000);
            break;
        }
        case 0x01:
        {
            u8 buffer[buffer_size] = { 0x00, 0x00, 0x00, 0x00 };
            GG_CdRomMSF length = m_cdrom_media->GetCdRomLength();
            buffer[0] = DecToBcd(length.minutes);
            buffer[1] = DecToBcd(length.seconds);
            buffer[2] = DecToBcd(length.frames);
            m_data_buffer.assign(buffer, buffer + buffer_size);
            m_data_buffer_offset = 0;
            Debug("Disc length: %d %02X:%02X:%02X", MsfToLba(&length), buffer[0], buffer[1], buffer[2]);
            NextEvent(SCSI_EVENT_SET_DATA_IN_PHASE, 9000);
            break;
        }
        case 0x02:
        {
            u8 track = BcdToDec(m_command_buffer[2]);

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

            u8 buffer[buffer_size] = { 0x00, 0x00, 0x00, 0x00 };
            buffer[0] = DecToBcd(start_msf.minutes);
            buffer[1] = DecToBcd(start_msf.seconds);
            buffer[2] = DecToBcd(start_msf.frames);
            buffer[3] = type;
            Debug("Track %d start: %d %02X:%02X:%02X, type: %d", track, MsfToLba(&start_msf), buffer[0], buffer[1], buffer[2], type);
            m_data_buffer.assign(buffer, buffer + buffer_size);
            m_data_buffer_offset = 0;

            NextEvent(SCSI_EVENT_SET_DATA_IN_PHASE, 9000);
            break;
        }
        default:
            Debug("SCSI CMD Read TOC: Unknown mode %02X", mode);
            break;
    }
}

void ScsiController::LoadSector()
{
    if (m_data_buffer.empty())
    {
        m_data_buffer.resize(2048);
        m_data_buffer_offset = 0;
        m_cdrom_media->ReadSector(m_load_sector, m_data_buffer.data());

        Debug("SCSI Load sector %d", m_load_sector);

        m_load_sector++;
        m_load_sector &= 0x1FFFFF;
        m_load_sector_count--;

        if (m_load_sector_count == 0)
            m_next_load_cycles = 0;
        else
            m_next_load_cycles = m_cdrom_media->SectorTransferCycles();

        Debug("SCSI Sectors left: %d, next:%d, cycles: %d", m_load_sector_count, m_load_sector, m_next_load_cycles);

        m_bus_changed = true;
    }
    else
    {
        Debug("**** SCSI Load sector: buffer not empty *******************");
        Debug("**** Data buffer size: %d, offset: %d", m_data_buffer.size(), m_data_buffer_offset);

        m_next_load_cycles = TimeToCycles(290000);
    }
}

u32 ScsiController::AudioLBA()
{
    u8 mode = m_command_buffer[9] & 0xC0;

    switch (mode)
    {
        case 0x00:
            return m_command_buffer[3] << 16 | m_command_buffer[4] << 8 | m_command_buffer[5];
        case 0x40:
        {
            GG_CdRomMSF position;
            position.minutes = BcdToDec(m_command_buffer[2]);
            position.seconds = BcdToDec(m_command_buffer[3]);
            position.frames = BcdToDec(m_command_buffer[4]);
            u32 lba = MsfToLba(&position);
            return lba - 150;
        }
        case 0x80:
        {
            int track = BcdToDec(m_command_buffer[2]) - 1;
            if (track >= 0)
                return m_cdrom_media->GetFirstSectorOfTrack(track);
            else
                return 0;
        }
        default:
        {
            Debug("SCSI CMD Audio LBA: Unknown mode %02X", mode);
            assert(false);
            return 0;
        }
    }
}
