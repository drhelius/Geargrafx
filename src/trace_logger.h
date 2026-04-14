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

#ifndef TRACE_LOGGER_H
#define TRACE_LOGGER_H

#include "common.h"

#define TRACE_BUFFER_SIZE 100000

enum GG_Trace_Type : u8
{
    TRACE_CPU = 0,
    TRACE_CPU_IRQ,
    TRACE_VDC,
    TRACE_INPUT,
    TRACE_TIMER,
    TRACE_CDROM,
    TRACE_PSG,
    TRACE_ADPCM,
    TRACE_VCE,
    TRACE_SCSI,
    TRACE_TYPE_COUNT,
};

#define TRACE_FLAG_CPU          (1 << TRACE_CPU)
#define TRACE_FLAG_CPU_IRQ      (1 << TRACE_CPU_IRQ)
#define TRACE_FLAG_VDC          (1 << TRACE_VDC)
#define TRACE_FLAG_INPUT        (1 << TRACE_INPUT)
#define TRACE_FLAG_TIMER        (1 << TRACE_TIMER)
#define TRACE_FLAG_CDROM        (1 << TRACE_CDROM)
#define TRACE_FLAG_PSG          (1 << TRACE_PSG)
#define TRACE_FLAG_ADPCM        (1 << TRACE_ADPCM)
#define TRACE_FLAG_VCE          (1 << TRACE_VCE)
#define TRACE_FLAG_SCSI         (1 << TRACE_SCSI)
#define TRACE_FLAG_ALL          0xFFFF

enum GG_Trace_VDC_Event : u8
{
    TRACE_VDC_REG_WRITE = 0,
    TRACE_VDC_VBLANK_IRQ,
    TRACE_VDC_SCANLINE_IRQ,
    TRACE_VDC_OVERFLOW_IRQ,
    TRACE_VDC_SPRITE_COLLISION_IRQ,
    TRACE_VDC_SATB_DMA_END_IRQ,
    TRACE_VDC_VRAM_DMA_END_IRQ,
    TRACE_VDC_VRAM_DMA_START,
    TRACE_VDC_SATB_DMA_START,
};

enum GG_Trace_VCE_Event : u8
{
    TRACE_VCE_CONTROL_WRITE = 0,
    TRACE_VCE_COLOR_WRITE,
    TRACE_VCE_HSYNC,
    TRACE_VCE_VSYNC_START,
    TRACE_VCE_VSYNC_END,
};

enum GG_Trace_CDROM_Event : u8
{
    TRACE_CDROM_IRQ = 0,
    TRACE_CDROM_FADER,
    TRACE_CDROM_RESET,
};

enum GG_Trace_SCSI_Event : u8
{
    TRACE_SCSI_COMMAND = 0,
    TRACE_SCSI_PHASE_CHANGE,
    TRACE_SCSI_STATUS,
    TRACE_SCSI_WARNING,
    TRACE_SCSI_ERROR,
};

enum GG_Trace_SCSI_Problem : u8
{
    TRACE_SCSI_PROBLEM_UNKNOWN_COMMAND = 0,
    TRACE_SCSI_PROBLEM_COMMAND_OVERFLOW,
    TRACE_SCSI_PROBLEM_SELECTION_DURING_DATA_IN,
    TRACE_SCSI_PROBLEM_INVALID_READ_REQUEST,
    TRACE_SCSI_PROBLEM_INVALID_AUDIO_START_LBA,
    TRACE_SCSI_PROBLEM_UNKNOWN_AUDIO_STOP_MODE,
    TRACE_SCSI_PROBLEM_UNKNOWN_TOC_MODE,
    TRACE_SCSI_PROBLEM_LOAD_SECTOR_BUFFER_BUSY,
    TRACE_SCSI_PROBLEM_UNKNOWN_AUDIO_LBA_MODE,
    TRACE_SCSI_PROBLEM_CLAMPED_COMMAND_SIZE,
    TRACE_SCSI_PROBLEM_CLAMPED_DATA_SIZE,
    TRACE_SCSI_PROBLEM_CLAMPED_DATA_OFFSET,
};

struct GG_Trace_Entry
{
    GG_Trace_Type type;
    u64 cycle;
    union
    {
        struct
        {
            u16 pc;
            u8 bank;
            u8 a, x, y, s, p;
        } cpu;

        struct
        {
            u16 pc;
            u16 vector;
            u8 irq_mask;
        } irq;

        struct
        {
            u8 reg;
            u16 value;
            u8 chip;
            u8 event;
        } vdc;

        struct
        {
            u8 value;
            u8 port;
        } input;

        struct
        {
            u8 counter;
            u8 reload;
        } timer;

        struct
        {
            u8 irq_type;
            u8 active;
            u8 enabled;
            u8 event;
        } cdrom;

        struct
        {
            u8 channel;
            u8 reg;
            u8 value;
        } psg;

        struct
        {
            u8 reg;
            u8 value;
        } adpcm;

        struct
        {
            u8 reg;
            u16 value;
            u8 event;
        } vce;

        struct
        {
            u8 command;
            u8 event;
            u8 phase;
            u8 status;
            u32 param;
        } scsi;
    };
};

class TraceLogger
{
public:
    TraceLogger();
    ~TraceLogger();
    void Reset();
    INLINE bool IsEnabled(GG_Trace_Type type) const;
    INLINE void TraceLog(const GG_Trace_Entry& entry);
    void SetEnabledFlags(u32 flags);
    u32 GetEnabledFlags() const;
    const GG_Trace_Entry* GetBuffer() const;
    u32 GetCount() const;
    u32 GetPosition() const;
    u64 GetTotalLogged() const;
    const GG_Trace_Entry& GetEntry(u32 index) const;

private:
    GG_Trace_Entry* m_buffer;
    u32 m_position;
    u32 m_count;
    u32 m_enabled_flags;
    u64 m_total_logged;
};

INLINE bool TraceLogger::IsEnabled(GG_Trace_Type type) const
{
#if !defined(GG_DISABLE_DISASSEMBLER)
    return (m_enabled_flags & (1 << type)) != 0;
#else
    UNUSED(type);
    return false;
#endif
}

INLINE void TraceLogger::TraceLog(const GG_Trace_Entry& entry)
{
#if !defined(GG_DISABLE_DISASSEMBLER)
    m_buffer[m_position] = entry;
    m_position = (m_position + 1) % TRACE_BUFFER_SIZE;
    if (m_count < TRACE_BUFFER_SIZE)
        m_count++;
    m_total_logged++;
#else
    UNUSED(entry);
#endif
}

#endif /* TRACE_LOGGER_H */
