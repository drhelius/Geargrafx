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
    InitPointer(m_data_buffer);
    m_phase = SCSI_PHASE_BUS_FREE;
    m_bus.db = 0;
    for (int i = 0; i < SCSI_SIGNAL_COUNT; i++)
        m_bus.signals[i] = false;
    m_command_index = 0;
    m_command_length = 0;
    m_data_length = 0;
    m_data_index = 0;
    m_read_current_lba = 0;
    m_read_sectors_remaining = 0;
    m_read_sector_offset = 0;
}

ScsiController::~ScsiController()
{
    SafeDelete(m_data_buffer);
}

void ScsiController::Init()
{
    m_data_buffer = new u8[SCSI_DATA_BUFFER_SIZE];
    Reset();
}

void ScsiController::Reset()
{
    m_phase = SCSI_PHASE_BUS_FREE;
    m_bus.db = 0;
    for (int i = 0; i < SCSI_SIGNAL_COUNT; i++)
        m_bus.signals[i] = false;
    m_command_index = 0;
    m_command_length = 0;
    m_data_length = 0;
    m_data_index = 0;
    m_read_current_lba = 0;
    m_read_sectors_remaining = 0;
    m_read_sector_offset = 0;

    memset(m_command_buffer, 0, SCSI_COMMAND_BUFFER_SIZE);
    memset(m_data_buffer, 0, SCSI_DATA_BUFFER_SIZE);
}

void ScsiController::Clock(u32 cycles)
{

}

u8 ScsiController::ReadData()
{
    Debug("SCSI Read data");

    if (m_phase == SCSI_PHASE_DATA_IN)
    {
        u8 value = m_data_buffer[m_data_index++];
        if (m_data_index >= m_data_length)
        {
            // Sector leído completo: avanza al siguiente si quedan más
            m_read_sector_offset = 0;
            m_read_current_lba++;
            m_read_sectors_remaining--;
            if (m_read_sectors_remaining > 0)
            {
                // Leer siguiente sector
                if (m_cdrom_media->ReadSector(m_read_current_lba, m_data_buffer))
                {
                    m_data_length = 2048; // O 2352 si tocase audio
                    m_data_index = 0;
                }
                else
                {
                    m_phase = SCSI_PHASE_STATUS;
                    
                    return 0xFF;
                }
            }
            else
            {
                m_phase = SCSI_PHASE_STATUS;
            }
        }
        return value;
    }
    else if (m_phase == SCSI_PHASE_STATUS)
    {
        m_phase = SCSI_PHASE_BUS_FREE;
        return 0;
    }
    return 0xFF;
}

void ScsiController::WriteData(u8 value)
{
    Debug("SCSI Write data %02X", value);

    // Fase de comando SCSI: acumula comando
    if (m_phase == SCSI_PHASE_BUS_FREE)
    {
        m_command_buffer[0] = value;
        // Detecta longitud del comando SCSI según opcode
        m_command_length = CommandLength((ScsiCommand)value);
        if (m_command_length == 0)
        {
            // Unknown command
            Debug("SCSI Unknown command %02X", value);

        }
        else
        {
            m_command_index = 1;
            m_phase = SCSI_PHASE_COMMAND;
        }
    }
    else if (m_phase == SCSI_PHASE_COMMAND)
    {
        m_command_buffer[m_command_index++] = value;
        if (m_command_index >= m_command_length)
        {
            ExecuteCommand();
        }
    }
    // Si estuviera en PHASE_DATAIN/PHASE_STATUS podrías ignorar/escalar error
}

u8 ScsiController::GetStatus()
{
    //Debug("SCSI Get status %02X", m_status);
    return (
        (m_bus.signals[SCSI_SIGNAL_IO] ?  0x08 : 0) |
        (m_bus.signals[SCSI_SIGNAL_CD] ?  0x10 : 0) |
        (m_bus.signals[SCSI_SIGNAL_MSG] ? 0x20 : 0) |
        (m_bus.signals[SCSI_SIGNAL_REQ] ? 0x40 : 0) |
        (m_bus.signals[SCSI_SIGNAL_BSY] ? 0x80 : 0)
    );
}

void ScsiController::ExecuteCommand()
{
    u8 opcode = m_command_buffer[0];
    if (opcode == SCSI_CMD_TEST_UNIT_READY)
        CommandTestUnitReady();
    else if (opcode == SCSI_CMD_REQUEST_SENSE)
        CommandRequestSense();
    else if (opcode == SCSI_CMD_READ)
        CommandRead();
    // else if ((opcode & 0xF0) == 0xD0)
    //     CommandVendor(opcode);
    // else /* comando desconocido */
    // m_status = 0x00;

    // Después de ejecutar, pones el phase adecuado
}

void ScsiController::CommandTestUnitReady()
{
    Debug("SCSI CMD Test Unit Ready");
    // Preparado siempre que haya CD montado
    //m_status = m_cdrom_media->IsReady() ? 0x00 : 0x02;
    m_phase = SCSI_PHASE_STATUS;
}

void ScsiController::CommandRequestSense()
{
    Debug("SCSI CMD Request Sense");
    // Devuelve dummy sense data, puedes ajustar según necesidades
    memset(m_data_buffer, 0, 4);
    m_data_length = 4;
    m_data_index = 0;
    m_phase = SCSI_PHASE_DATA_IN;
}

void ScsiController::CommandRead()
{
    Debug("SCSI CMD Read");
    // Extrae LBA y count
    u8* buffer = m_command_buffer;
    u32 lba = (buffer[2] << 24) | (buffer[3] << 16) | (buffer[4] << 8) | buffer[5];
    u16 count = (buffer[7] << 8) | buffer[8];

    m_read_current_lba = lba;
    m_read_sectors_remaining = count;
    m_read_sector_offset = 0;

    // Leer el primer sector en el buffer
    if (m_read_sectors_remaining > 0 && m_cdrom_media->ReadSector(m_read_current_lba, m_data_buffer))
    {
        m_data_length = 2048; // Si es datos, sector 2048. Si quieres soportar audio, cámbialo a 2352.
        m_data_index = 0;
        m_phase = SCSI_PHASE_DATA_IN;
    }
    else
    {
        m_data_length = 0;
        m_phase = SCSI_PHASE_STATUS;
        //m_status = 0x02; // Error
    }
}

void ScsiController::CommandVendor(u8 opcode)
{
    Debug("SCSI CMD Vendor %02X", opcode);
    // Implementar según los comandos $D8, $D9, $DA, $DD, $DE
    //m_status = 0x00; // dummy ok
    m_phase = SCSI_PHASE_STATUS;
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
