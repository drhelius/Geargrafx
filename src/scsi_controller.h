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

#ifndef SCSI_CONTROLLER_H
#define SCSI_CONTROLLER_H

#include <vector>
#include "common.h"

class CdRom;
class CdRomMedia;
class CdRomAudio;
class HuC6280;

class ScsiController
{
public:

    enum ScsiSignal
    {
        SCSI_SIGNAL_BSY = 0x80,
        SCSI_SIGNAL_SEL = 0x01,
        SCSI_SIGNAL_CD = 0x10,
        SCSI_SIGNAL_IO = 0x08,
        SCSI_SIGNAL_MSG = 0x20,
        SCSI_SIGNAL_REQ = 0x40,
        SCSI_SIGNAL_ACK = 0x02,
        SCSI_SIGNAL_ATN = 0x04,
        SCSI_SIGNAL_RST = 0x100
    };

    struct ScsiBus
    {
        u8 db;
        u16 signals;
    };

    enum ScsiPhase
    {
        SCSI_PHASE_BUS_FREE,
        SCSI_PHASE_SELECTION,
        SCSI_PHASE_MESSAGE_OUT,
        SCSI_PHASE_COMMAND,
        SCSI_PHASE_DATA_IN,
        SCSI_PHASE_DATA_OUT,
        SCSI_PHASE_MESSAGE_IN,
        SCSI_PHASE_STATUS,
        SCSI_PHASE_BUSY
    };

    enum ScsiCommand
    {
        SCSI_CMD_TEST_UNIT_READY = 0x00,
        SCSI_CMD_REQUEST_SENSE = 0x03,
        SCSI_CMD_READ = 0x08,
        SCSI_CMD_AUDIO_START_POSITION = 0xD8,
        SCSI_CMD_AUDIO_STOP_POSITION = 0xD9,
        SCSI_CMD_AUDIO_PAUSE = 0xDA,
        SCSI_CMD_READ_SUBCODE_Q = 0xDD,
        SCSI_CMD_READ_TOC = 0xDE
    };

    enum ScsiEvent
    {
        SCSI_EVENT_NONE,
        SCSI_EVENT_SET_COMMAND_PHASE,
        SCSI_EVENT_SET_REQ_SIGNAL,
        SCSI_EVENT_SET_GOOD_STATUS,
        SCSI_EVENT_SET_DATA_IN_PHASE
    };

    enum ScsiStatus
    {
        SCSI_STATUS_GOOD = 0x00,
        SCSI_STATUS_CHECK_CONDITION = 0x02,
        SCSI_STATUS_CONDITION_MET = 0x04,
        SCSI_STATUS_BUSY = 0x08,
        SCSI_STATUS_INTERMEDIATE = 0x10,
        SCSI_STATUS_INTERMEDIATE_CONDITION_MET = 0x14,
        SCSI_STATUS_RESERVATION_CONFLICT = 0x18,
        SCSI_STATUS_COMMAND_TERMINATED = 0x22,
        SCSI_STATUS_QUEUE_FULL = 0x28
    };

    struct Scsi_State
    {
        u8* DB;
        u16* SIGNALS;
        ScsiPhase* PHASE;
        ScsiEvent* NEXT_EVENT;
        s32* NEXT_EVENT_CYCLES;
        s32* NEXT_LOAD_CYCLES;
        u32* LOAD_SECTOR;
        u32* LOAD_SECTOR_COUNT;
        s32* AUTO_ACK_CYCLES;
        std::vector<u8>* COMMAND_BUFFER;
        std::vector<u8>* DATA_BUFFER;
        u32* DATA_BUFFER_OFFSET;
    };

public:
    ScsiController(CdRomMedia* cdrom_media, CdRomAudio* cdrom_audio);
    ~ScsiController();
    void Init(HuC6280* huc6280, CdRom* cdrom);
    void Reset(bool keep_rst_signal = false);
    void Clock(u32 cycles);
    u8 ReadData();
    void WriteData(u8 value);
    u8 GetStatus();
    void SetSignal(u16 signals);
    void ClearSignal(u16 signals);
    bool IsSignalSet(ScsiSignal signal);
    void AutoAck();
    void StartSelection();
    void StartStatus(ScsiStatus status, u8 length = 1);
    bool IsDataReady();
    Scsi_State* GetState();

private:
    void SetPhase(ScsiPhase phase);
    void NextEvent(ScsiEvent event, u32 cycles);
    void RunEvent();
    void UpdateScsi();
    void UpdateEvents(u32 cycles);
    void UpdateSectorLoading(u32 cycles);
    void UpdateAutoAck(u32 cycles);
    void UpdateIRQs();
    void UpdateCommandPhase();
    void UpdateDataInPhase();
    void UpdateStatusPhase();
    void UpdateMessageInPhase();
    void ExecuteCommand();
    void CommandTestUnitReady();
    void CommandRequestSense();
    void CommandRead();
    void CommandAudioStartPosition();
    void CommandAudioStopPosition();
    void CommandAudioPause();
    void CommandReadSubcodeQ();
    void CommandReadTOC();
    u8 CommandLength(ScsiCommand command);
    void LoadSector();
    u32 AudioLBA();

private:
    Scsi_State m_state;
    HuC6280* m_huc6280;
    CdRom* m_cdrom;
    CdRomMedia* m_cdrom_media;
    CdRomAudio* m_cdrom_audio;
    ScsiBus m_bus;
    ScsiPhase m_phase;
    ScsiEvent m_next_event;
    s32 m_next_event_cycles;
    s32 m_next_load_cycles;
    u32 m_load_sector;
    u32 m_load_sector_count;
    s32 m_auto_ack_cycles;
    std::vector<u8> m_command_buffer;
    std::vector<u8> m_data_buffer;
    u32 m_data_buffer_offset;
    bool m_bus_changed;
    u16 m_previous_signals;
    u8 m_data_bus_latch;
};

static const char* k_scsi_phase_names[] = {
    "BUS FREE",
    "SELECTION",
    "MESSAGE OUT",
    "COMMAND",
    "DATA IN",
    "DATA OUT",
    "MESSAGE IN",
    "STATUS",
    "BUSY"
};

static const char* k_scsi_event_names[] = {
    "NONE",
    "SET COMMAND PHASE",
    "SET REQ SIGNAL",
    "SET GOOD STATUS",
    "SET DATA IN PHASE"
};

#include "scsi_controller_inline.h"

#endif /* SCSI_CONTROLLER_H */