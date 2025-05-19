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

#include "common.h"

#define SCSI_COMMAND_BUFFER_SIZE 12
#define SCSI_DATA_BUFFER_SIZE 2352

class CdRomMedia;

class ScsiController
{
public:
    enum ScsiSignal
    {
        SCSI_SIGNAL_BSY,
        SCSI_SIGNAL_SEL,
        SCSI_SIGNAL_CD,
        SCSI_SIGNAL_IO,
        SCSI_SIGNAL_MSG,
        SCSI_SIGNAL_REQ,
        SCSI_SIGNAL_ACK,
        SCSI_SIGNAL_ATN,
        SCSI_SIGNAL_RST,
        SCSI_SIGNAL_COUNT
    };

    struct ScsiBus
    {
        u8 db;
        bool signals[SCSI_SIGNAL_COUNT];
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

public:
    ScsiController(CdRomMedia* cdrom_media);
    ~ScsiController();
    void Init();
    void Reset();
    void Clock(u32 cycles);
    u8 ReadData();
    void WriteData(u8 value);
    u8 GetStatus();
    void SetSignal(ScsiSignal signal);
    void ClearSignal(ScsiSignal signal);
    bool IsSignalSet(ScsiSignal signal);

private:
    // Interno: ejecuta el comando cuando buffer está lleno
    void ExecuteCommand();

    // Helpers para los comandos clásicos (TEST UNIT READY, REQUEST SENSE, READ, etc.)
    void CommandTestUnitReady();
    void CommandRequestSense();
    void CommandRead();
    void CommandVendor(u8 opcode);
    u8 CommandLength(ScsiCommand command);

private:
    CdRomMedia* m_cdrom_media;
    ScsiBus m_bus;
    ScsiPhase m_phase;

    u8 m_command_buffer[SCSI_COMMAND_BUFFER_SIZE];
    // Buffer de comandos recibidos
    u8 m_command_index;
    u8 m_command_length;

    // Buffer de datos que va devolviendo el SCSI (lectura de sectores, info, etc)
    u8* m_data_buffer;
    u32 m_data_length;
    u32 m_data_index;

    u32 m_read_current_lba = 0;
    u16 m_read_sectors_remaining = 0;
    u32 m_read_sector_offset = 0;
};

INLINE void ScsiController::SetSignal(ScsiSignal signal)
{
    m_bus.signals[signal] = true;
}

INLINE void ScsiController::ClearSignal(ScsiSignal signal)
{
    m_bus.signals[signal] = false;
}

INLINE bool ScsiController::IsSignalSet(ScsiSignal signal)
{
    return m_bus.signals[signal];
}

#endif /* SCSI_CONTROLLER_H */