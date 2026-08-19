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
    TRACE_SYSTEM,
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
#define TRACE_FLAG_SYSTEM       (1 << TRACE_SYSTEM)
#define TRACE_FLAG_ALL          ((1U << TRACE_TYPE_COUNT) - 1)

static_assert(TRACE_TYPE_COUNT < 32, "Trace category flags exceed u32 width");

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
    TRACE_VDC_VPC_REG_WRITE,
};

enum GG_Trace_VCE_Event : u8
{
    TRACE_VCE_CONTROL_WRITE = 0,
    TRACE_VCE_COLOR_WRITE,
    TRACE_VCE_VSYNC_START,
    TRACE_VCE_VSYNC_END,
};

enum GG_Trace_Input_Event : u8
{
    TRACE_INPUT_READ = 0,
    TRACE_INPUT_WRITE,
};

enum GG_Trace_Input_Source : u8
{
    TRACE_INPUT_SOURCE_NONE = 0,
    TRACE_INPUT_SOURCE_GAMEPAD,
    TRACE_INPUT_SOURCE_MOUSE,
    TRACE_INPUT_SOURCE_MB128,
};

enum GG_Trace_Timer_Event : u8
{
    TRACE_TIMER_IRQ_REQUEST = 0,
    TRACE_TIMER_RELOAD_WRITE,
    TRACE_TIMER_CONTROL_WRITE,
};

enum GG_Trace_ADPCM_Event : u8
{
    TRACE_ADPCM_REG_WRITE = 0,
    TRACE_ADPCM_DMA_STATE,
    TRACE_ADPCM_PLAY_REQUEST,
    TRACE_ADPCM_PLAY_START,
    TRACE_ADPCM_PLAY_STOP,
    TRACE_ADPCM_READ_COMPLETE,
    TRACE_ADPCM_WRITE_COMPLETE,
    TRACE_ADPCM_HALF_IRQ,
    TRACE_ADPCM_END_IRQ,
};

enum GG_Trace_CDROM_Event : u8
{
    TRACE_CDROM_IRQ_SET = 0,
    TRACE_CDROM_IRQ_CLEAR,
    TRACE_CDROM_IRQ_ENABLE,
    TRACE_CDROM_FADER,
    TRACE_CDROM_RESET,
    TRACE_CDROM_AUDIO_START,
    TRACE_CDROM_AUDIO_SEEK_END,
    TRACE_CDROM_AUDIO_STATE,
    TRACE_CDROM_AUDIO_STOP_LBA,
    TRACE_CDROM_AUDIO_BOUNDARY,
};

enum GG_Trace_SCSI_Event : u8
{
    TRACE_SCSI_COMMAND = 0,
    TRACE_SCSI_PHASE_CHANGE,
    TRACE_SCSI_STATUS,
    TRACE_SCSI_RESPONSE,
    TRACE_SCSI_RESPONSE_BYTE,
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
    TRACE_SCSI_PROBLEM_READ_PAST_END,
    TRACE_SCSI_PROBLEM_READ_SECTOR_FAILED,
};

enum GG_Trace_PSG_Event : u8
{
    TRACE_PSG_CHANNEL_SELECT = 0,
    TRACE_PSG_MAIN_BALANCE,
    TRACE_PSG_FREQUENCY_L,
    TRACE_PSG_FREQUENCY_H,
    TRACE_PSG_CONTROL,
    TRACE_PSG_BALANCE,
    TRACE_PSG_WAVE_DDA,
    TRACE_PSG_NOISE,
    TRACE_PSG_LFO_FREQUENCY,
    TRACE_PSG_LFO_CONTROL,
};

enum GG_Trace_System_Event : u8
{
    TRACE_SYSTEM_MPR_WRITE = 0,
    TRACE_SYSTEM_SF2_MAPPER,
    TRACE_SYSTEM_IRQ_MASK_WRITE,
    TRACE_SYSTEM_IRQ_ACK,
};

static_assert(TRACE_VDC_VPC_REG_WRITE < 32 && TRACE_VCE_VSYNC_END < 32 &&
    TRACE_INPUT_WRITE < 32 && TRACE_TIMER_CONTROL_WRITE < 32 &&
    TRACE_ADPCM_END_IRQ < 32 && TRACE_CDROM_AUDIO_BOUNDARY < 32 &&
    TRACE_SCSI_ERROR < 32 && TRACE_PSG_LFO_CONTROL < 32 &&
    TRACE_SYSTEM_IRQ_ACK < 32, "Trace event filters exceed u32 width");

#define TRACE_EVENT_FLAG(event)              (1U << (event))

#define TRACE_VDC_FILTER_REGISTERS            \
    (TRACE_EVENT_FLAG(TRACE_VDC_REG_WRITE) | TRACE_EVENT_FLAG(TRACE_VDC_VPC_REG_WRITE))
#define TRACE_VDC_FILTER_IRQS                 \
    (TRACE_EVENT_FLAG(TRACE_VDC_VBLANK_IRQ) | TRACE_EVENT_FLAG(TRACE_VDC_SCANLINE_IRQ) | \
     TRACE_EVENT_FLAG(TRACE_VDC_OVERFLOW_IRQ) | TRACE_EVENT_FLAG(TRACE_VDC_SPRITE_COLLISION_IRQ))
#define TRACE_VDC_FILTER_DMA                  \
    (TRACE_EVENT_FLAG(TRACE_VDC_SATB_DMA_END_IRQ) | TRACE_EVENT_FLAG(TRACE_VDC_VRAM_DMA_END_IRQ) | \
     TRACE_EVENT_FLAG(TRACE_VDC_VRAM_DMA_START) | TRACE_EVENT_FLAG(TRACE_VDC_SATB_DMA_START))
#define TRACE_VDC_FILTER_ALL                  \
    (TRACE_VDC_FILTER_REGISTERS | TRACE_VDC_FILTER_IRQS | TRACE_VDC_FILTER_DMA)

#define TRACE_VCE_FILTER_REGISTERS            \
    (TRACE_EVENT_FLAG(TRACE_VCE_CONTROL_WRITE) | TRACE_EVENT_FLAG(TRACE_VCE_COLOR_WRITE))
#define TRACE_VCE_FILTER_TIMING               \
    (TRACE_EVENT_FLAG(TRACE_VCE_VSYNC_START) | TRACE_EVENT_FLAG(TRACE_VCE_VSYNC_END))
#define TRACE_VCE_FILTER_ALL                  \
    (TRACE_VCE_FILTER_REGISTERS | TRACE_VCE_FILTER_TIMING)

#define TRACE_INPUT_FILTER_READS              TRACE_EVENT_FLAG(TRACE_INPUT_READ)
#define TRACE_INPUT_FILTER_WRITES             TRACE_EVENT_FLAG(TRACE_INPUT_WRITE)
#define TRACE_INPUT_FILTER_ALL                (TRACE_INPUT_FILTER_READS | TRACE_INPUT_FILTER_WRITES)

#define TRACE_TIMER_FILTER_IRQS               TRACE_EVENT_FLAG(TRACE_TIMER_IRQ_REQUEST)
#define TRACE_TIMER_FILTER_REGISTERS          \
    (TRACE_EVENT_FLAG(TRACE_TIMER_RELOAD_WRITE) | TRACE_EVENT_FLAG(TRACE_TIMER_CONTROL_WRITE))
#define TRACE_TIMER_FILTER_ALL                (TRACE_TIMER_FILTER_IRQS | TRACE_TIMER_FILTER_REGISTERS)

#define TRACE_CDROM_FILTER_IRQS               \
    (TRACE_EVENT_FLAG(TRACE_CDROM_IRQ_SET) | TRACE_EVENT_FLAG(TRACE_CDROM_IRQ_CLEAR) | \
     TRACE_EVENT_FLAG(TRACE_CDROM_IRQ_ENABLE))
#define TRACE_CDROM_FILTER_CONTROL            \
    (TRACE_EVENT_FLAG(TRACE_CDROM_FADER) | TRACE_EVENT_FLAG(TRACE_CDROM_RESET))
#define TRACE_CDROM_FILTER_AUDIO              \
    (TRACE_EVENT_FLAG(TRACE_CDROM_AUDIO_START) | TRACE_EVENT_FLAG(TRACE_CDROM_AUDIO_SEEK_END) | \
     TRACE_EVENT_FLAG(TRACE_CDROM_AUDIO_STATE) | TRACE_EVENT_FLAG(TRACE_CDROM_AUDIO_STOP_LBA) | \
     TRACE_EVENT_FLAG(TRACE_CDROM_AUDIO_BOUNDARY))
#define TRACE_CDROM_FILTER_ALL                (TRACE_CDROM_FILTER_IRQS | TRACE_CDROM_FILTER_CONTROL | TRACE_CDROM_FILTER_AUDIO)

#define TRACE_PSG_FILTER_GLOBAL               \
    (TRACE_EVENT_FLAG(TRACE_PSG_CHANNEL_SELECT) | TRACE_EVENT_FLAG(TRACE_PSG_MAIN_BALANCE) | \
     TRACE_EVENT_FLAG(TRACE_PSG_LFO_FREQUENCY) | TRACE_EVENT_FLAG(TRACE_PSG_LFO_CONTROL))
#define TRACE_PSG_FILTER_FREQUENCY            \
    (TRACE_EVENT_FLAG(TRACE_PSG_FREQUENCY_L) | TRACE_EVENT_FLAG(TRACE_PSG_FREQUENCY_H))
#define TRACE_PSG_FILTER_CHANNEL              \
    (TRACE_EVENT_FLAG(TRACE_PSG_CONTROL) | TRACE_EVENT_FLAG(TRACE_PSG_BALANCE))
#define TRACE_PSG_FILTER_WAVE                 TRACE_EVENT_FLAG(TRACE_PSG_WAVE_DDA)
#define TRACE_PSG_FILTER_NOISE                TRACE_EVENT_FLAG(TRACE_PSG_NOISE)
#define TRACE_PSG_FILTER_ALL                  \
    (TRACE_PSG_FILTER_GLOBAL | TRACE_PSG_FILTER_FREQUENCY | TRACE_PSG_FILTER_CHANNEL | \
     TRACE_PSG_FILTER_WAVE | TRACE_PSG_FILTER_NOISE)

#define TRACE_ADPCM_FILTER_REGISTERS          TRACE_EVENT_FLAG(TRACE_ADPCM_REG_WRITE)
#define TRACE_ADPCM_FILTER_DMA                TRACE_EVENT_FLAG(TRACE_ADPCM_DMA_STATE)
#define TRACE_ADPCM_FILTER_PLAYBACK           \
    (TRACE_EVENT_FLAG(TRACE_ADPCM_PLAY_REQUEST) | TRACE_EVENT_FLAG(TRACE_ADPCM_PLAY_START) | \
     TRACE_EVENT_FLAG(TRACE_ADPCM_PLAY_STOP))
#define TRACE_ADPCM_FILTER_TRANSFERS          \
    (TRACE_EVENT_FLAG(TRACE_ADPCM_READ_COMPLETE) | TRACE_EVENT_FLAG(TRACE_ADPCM_WRITE_COMPLETE))
#define TRACE_ADPCM_FILTER_IRQS               \
    (TRACE_EVENT_FLAG(TRACE_ADPCM_HALF_IRQ) | TRACE_EVENT_FLAG(TRACE_ADPCM_END_IRQ))
#define TRACE_ADPCM_FILTER_ALL                \
    (TRACE_ADPCM_FILTER_REGISTERS | TRACE_ADPCM_FILTER_DMA | TRACE_ADPCM_FILTER_PLAYBACK | \
     TRACE_ADPCM_FILTER_TRANSFERS | TRACE_ADPCM_FILTER_IRQS)

#define TRACE_SCSI_FILTER_COMMANDS            TRACE_EVENT_FLAG(TRACE_SCSI_COMMAND)
#define TRACE_SCSI_FILTER_PHASES              TRACE_EVENT_FLAG(TRACE_SCSI_PHASE_CHANGE)
#define TRACE_SCSI_FILTER_RESPONSES           \
    (TRACE_EVENT_FLAG(TRACE_SCSI_STATUS) | TRACE_EVENT_FLAG(TRACE_SCSI_RESPONSE))
#define TRACE_SCSI_FILTER_RESPONSE_BYTES      TRACE_EVENT_FLAG(TRACE_SCSI_RESPONSE_BYTE)
#define TRACE_SCSI_FILTER_PROBLEMS            \
    (TRACE_EVENT_FLAG(TRACE_SCSI_WARNING) | TRACE_EVENT_FLAG(TRACE_SCSI_ERROR))
#define TRACE_SCSI_FILTER_ALL                 \
    (TRACE_SCSI_FILTER_COMMANDS | TRACE_SCSI_FILTER_PHASES | TRACE_SCSI_FILTER_RESPONSES | \
     TRACE_SCSI_FILTER_RESPONSE_BYTES | TRACE_SCSI_FILTER_PROBLEMS)

#define TRACE_SYSTEM_FILTER_MPR               TRACE_EVENT_FLAG(TRACE_SYSTEM_MPR_WRITE)
#define TRACE_SYSTEM_FILTER_MAPPER            TRACE_EVENT_FLAG(TRACE_SYSTEM_SF2_MAPPER)
#define TRACE_SYSTEM_FILTER_INTERRUPTS        \
    (TRACE_EVENT_FLAG(TRACE_SYSTEM_IRQ_MASK_WRITE) | TRACE_EVENT_FLAG(TRACE_SYSTEM_IRQ_ACK))
#define TRACE_SYSTEM_FILTER_ALL               \
    (TRACE_SYSTEM_FILTER_MPR | TRACE_SYSTEM_FILTER_MAPPER | TRACE_SYSTEM_FILTER_INTERRUPTS)

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
            u8 size;
            u8 opcodes[7];
        } cpu;

        struct
        {
            u16 pc;
            u16 vector;
            u8 irq_mask;
        } irq;

        struct
        {
            u32 param;
            u8 reg;
            u16 value;
            u16 value2;
            u16 value3;
            u8 chip;
            u8 event;
            u8 raw;
            u8 msb;
        } vdc;

        struct
        {
            u8 value;
            u8 port;
            u8 event;
            u8 source;
            u8 state;
        } input;

        struct
        {
            u8 counter;
            u8 reload;
            u8 value;
            u8 event;
            u8 enabled;
        } timer;

        struct
        {
            u32 lba;
            u32 param;
            u8 irq_type;
            u8 active;
            u8 enabled;
            u8 event;
            u8 state;
        } cdrom;

        struct
        {
            u8 channel;
            u8 reg;
            u8 value;
        } psg;

        struct
        {
            u32 length;
            u16 address;
            u8 reg;
            u8 value;
            u8 event;
            u8 state;
        } adpcm;

        struct
        {
            u16 reg;
            u16 value;
            u8 event;
        } vce;

        struct
        {
            u8 command;
            u8 event;
            u8 phase;
            u8 status;
            u8 size;
            u8 data[16];
            u32 param;
        } scsi;

        struct
        {
            u32 physical;
            u16 address;
            u8 event;
            u8 index;
            u8 mask;
            u8 raw;
            u8 old_value;
            u8 new_value;
            u8 request;
            u8 state;
        } system;
    };
};

static_assert(sizeof(GG_Trace_Entry) <= 48, "Trace entry exceeds memory budget");

class TraceLogger
{
public:
    TraceLogger(const u64* master_clock_cycles = NULL);
    ~TraceLogger();
    void Reset();
    bool SetCapacity(u32 capacity);
    INLINE bool IsEnabled(GG_Trace_Type type) const;
    INLINE bool IsEventEnabled(GG_Trace_Type type, u8 event) const;
    INLINE void TraceLog(const GG_Trace_Entry& entry);
    void SetEnabledFlags(u32 flags);
    void SetEventFilter(GG_Trace_Type type, u32 filter);
    u32 GetEnabledFlags() const;
    u32 GetEventFilter(GG_Trace_Type type) const;
    const GG_Trace_Entry* GetBuffer() const;
    u32 GetCount() const;
    u32 GetCapacity() const;
    u32 GetPosition() const;
    u64 GetTotalLogged() const;
    u64 GetSequence() const;
    const GG_Trace_Entry& GetEntry(u32 index) const;

private:
    GG_Trace_Entry* m_buffer;
    u32 m_position;
    u32 m_count;
    u32 m_capacity;
    u32 m_enabled_flags;
    u32 m_event_filters[TRACE_TYPE_COUNT];
    u64 m_total_logged;
    u64 m_sequence;
    const u64* m_master_clock_cycles;
};

INLINE bool TraceLogger::IsEnabled(GG_Trace_Type type) const
{
#if !defined(GG_DISABLE_DISASSEMBLER)
    return type < TRACE_TYPE_COUNT && (m_enabled_flags & (1 << type)) != 0 && m_buffer;
#else
    UNUSED(type);
    return false;
#endif
}

INLINE bool TraceLogger::IsEventEnabled(GG_Trace_Type type, u8 event) const
{
#if !defined(GG_DISABLE_DISASSEMBLER)
    return IsEnabled(type) && event < 32 && (m_event_filters[type] & TRACE_EVENT_FLAG(event)) != 0;
#else
    UNUSED(type);
    UNUSED(event);
    return false;
#endif
}

INLINE void TraceLogger::TraceLog(const GG_Trace_Entry& entry)
{
#if !defined(GG_DISABLE_DISASSEMBLER)
    m_buffer[m_position] = entry;
    if (IsValidPointer(m_master_clock_cycles))
        m_buffer[m_position].cycle = *m_master_clock_cycles;
    m_position++;
    if (m_position == m_capacity)
        m_position = 0;
    if (m_count < m_capacity)
        m_count++;
    m_total_logged++;
    m_sequence++;
#else
    UNUSED(entry);
#endif
}

#endif /* TRACE_LOGGER_H */
