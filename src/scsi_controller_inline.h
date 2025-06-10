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

#ifndef SCSI_CONTROLLER_INLINE_H
#define SCSI_CONTROLLER_INLINE_H

#include "scsi_controller.h"
#include "cdrom.h"
#include "cdrom_common.h"
#include "huc6280.h"

INLINE void ScsiController::Clock(u32 cycles)
{
    UpdateEvents(cycles);
    UpdateSectorLoading(cycles);
    UpdateScsi();
    UpdateAutoAck(cycles);
}

INLINE void ScsiController::UpdateEvents(u32 cycles)
{
    if (m_next_event != SCSI_EVENT_NONE)
    {
        m_next_event_cycles -= cycles;
        if (m_next_event_cycles <= 0)
        {
            RunEvent();
        }
    }
}

INLINE void ScsiController::UpdateSectorLoading(u32 cycles)
{
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

INLINE void ScsiController::UpdateAutoAck(u32 cycles)
{
    if (m_auto_ack_cycles > 0)
    {
        m_auto_ack_cycles -= cycles;
        if (m_auto_ack_cycles <= 0)
        {
            m_auto_ack_cycles = 0;
            ClearSignal(SCSI_SIGNAL_ACK);
            //Debug("SCSI Auto ACK (clear) %02X", m_bus.signals);
        }
    }
}

INLINE void ScsiController::UpdateIRQs()
{
    if(IsSignalSet(SCSI_SIGNAL_BSY) && IsSignalSet(SCSI_SIGNAL_IO) && IsSignalSet(SCSI_SIGNAL_REQ))
    {
        if(IsSignalSet(SCSI_SIGNAL_CD))
            m_cdrom->SetIRQ(CDROM_IRQ_STATUS_AND_MSG_IN);
        else
            m_cdrom->SetIRQ(CDROM_IRQ_DATA_IN);
    }
    else
    {
        m_cdrom->ClearIRQ(CDROM_IRQ_STATUS_AND_MSG_IN);
        m_cdrom->ClearIRQ(CDROM_IRQ_DATA_IN);
    }
}

INLINE u8 ScsiController::ReadData()
{
    //Debug("SCSI Read data: %02X %04X %04X", m_bus.db, m_bus.signals, m_huc6280->GetState()->PC->GetValue());
    return m_bus.db;
}

INLINE void ScsiController::WriteData(u8 value)
{
    //Debug("SCSI Write data: %02X", value);
    m_bus.db = value;
}

INLINE u8 ScsiController::GetStatus()
{
    return (m_bus.signals & 0xF8);
}

INLINE void ScsiController::SetSignal(u16 signals)
{
    m_previous_signals = m_bus.signals;
    m_bus.signals |= signals;

    if (m_previous_signals != m_bus.signals)
    {
        //Debug("*** SCSI Bus change detected: %02X %02X", m_bus.signals, m_bus.db);
        m_bus_changed = true;
        m_previous_signals =  m_bus.signals;
    }
}

INLINE void ScsiController::ClearSignal(u16 signals)
{
    m_previous_signals = m_bus.signals;
    m_bus.signals &= ~signals;

    if (m_previous_signals != m_bus.signals)
    {
        //Debug("**** SCSI Bus change detected: %02X %02X", m_bus.signals, m_bus.db);
        m_bus_changed = true;
        m_previous_signals =  m_bus.signals;
    }
}

INLINE bool ScsiController::IsSignalSet(ScsiSignal signal)
{
    return (m_bus.signals & signal) != 0;
}

INLINE void ScsiController::NextEvent(ScsiEvent event, u32 cycles)
{
    m_next_event = event;
    m_next_event_cycles = cycles;
}

INLINE void ScsiController::RunEvent()
{
    switch (m_next_event)
    {
        case SCSI_EVENT_SET_COMMAND_PHASE:
            //Debug("SCSI Event Set command phase");
            SetPhase(SCSI_PHASE_COMMAND);
            break;
        case SCSI_EVENT_SET_REQ_SIGNAL:
            //Debug("SCSI Event Set REQ signal");
            SetSignal(SCSI_SIGNAL_REQ);
            break;
        case SCSI_EVENT_SET_GOOD_STATUS:
            //Debug("SCSI Event Set good status");
            StartStatus(SCSI_STATUS_GOOD);
            break;
        case SCSI_EVENT_SET_DATA_IN_PHASE:
            //Debug("SCSI Event Set data in phase");
            SetPhase(SCSI_PHASE_DATA_IN);
            break;
        default:
            break;
    }

    NextEvent(SCSI_EVENT_NONE, 0);
}

INLINE void ScsiController::AutoAck()
{
    if (IsSignalSet(SCSI_SIGNAL_REQ) && IsSignalSet(SCSI_SIGNAL_IO) && !IsSignalSet(SCSI_SIGNAL_CD))
    {
        m_auto_ack_cycles = TimeToCycles(1);
        SetSignal(SCSI_SIGNAL_ACK);
        //Debug("SCSI Auto ACK (set) %02X", m_bus.signals);
    }
}

INLINE u8 ScsiController::CommandLength(ScsiCommand command)
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

#endif /* SCSI_CONTROLLER_INLINE_H */