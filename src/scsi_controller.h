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

class CdRomMedia;

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
        SCSI_SET_GOOD_STATUS,
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

public:
    ScsiController(CdRomMedia* cdrom_media);
    ~ScsiController();
    void Init();
    void Reset();
    void Clock(u32 cycles);
    u8 ReadData();
    void WriteData(u8 value);
    u8 GetStatus();
    void SetSignal(u16 signals);
    void ClearSignal(u16 signals);
    bool IsSignalSet(ScsiSignal signal);
    void StartSelection();
    void StartStatus(ScsiStatus status, u8 length = 1);
    void BusChange();

private:
    void SetPhase(ScsiPhase phase);
    void SetEvent(ScsiEvent event, u32 cycles);
    void UpdateCommandPhase();
    void UpdateDataInPhase();
    void UpdateStatusPhase();
    void UpdateMessageInPhase();
    void ExecuteCommand();
    void CommandTestUnitReady();
    void CommandRequestSense();
    void CommandRead();
    u8 CommandLength(ScsiCommand command);
    u32 TimeToCycles(u32 us);

private:
    CdRomMedia* m_cdrom_media;
    ScsiBus m_bus;
    ScsiPhase m_phase;
    ScsiEvent m_next_event;
    s32 m_next_event_cycles;
    std::vector<u8> m_command_buffer;
    std::vector<u8> m_data_buffer;
    u32 m_data_buffer_offset = 0;
    u32 m_read_current_lba = 0;
    u16 m_read_sectors_remaining = 0;
    u32 m_read_sector_offset = 0;
};

static const char* k_scsi_phase_names[] = {
    "BUS FREE",
    "SELECTION",
    "MESSAGE OUT",
    "COMMAND",
    "DATA IN",
    "DATA OUT",
    "MESSAGE IN",
    "STATUS"
};

INLINE void ScsiController::SetSignal(u16 signals)
{
    m_bus.signals |= signals;
}

INLINE void ScsiController::ClearSignal(u16 signals)
{
    m_bus.signals &= ~signals;
}

INLINE bool ScsiController::IsSignalSet(ScsiSignal signal)
{
    return (m_bus.signals & signal) != 0;
}

INLINE u32 ScsiController::TimeToCycles(u32 us)
{
    // Convert microseconds to PCE master clock cycles (21.47727 MHz) using integer math
    return (us * 21) + ((us * 47727) / 1000000);
}

#endif /* SCSI_CONTROLLER_H */