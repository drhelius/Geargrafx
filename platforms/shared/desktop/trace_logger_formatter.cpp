/*
 * Geargrafx - PC Engine / TurboGrafx Emulator
 * Copyright (C) 2026  Ignacio Sanchez

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

#include "trace_logger_formatter.h"
#include "huc6280.h"
#include "cdrom.h"
#include <cstdio>
#include <cstring>

void trace_log_format_cpu_bytes(const GG_Trace_Entry& entry, char* buffer, size_t buffer_size)
{
    static const char k_hex[] = "0123456789ABCDEF";
    int pos = 0;
    u8 size = MIN(entry.cpu.size, (u8)sizeof(entry.cpu.opcodes));
    for (u8 i = 0; i < size && (size_t)(pos + 3) < buffer_size; i++)
    {
        u8 value = entry.cpu.opcodes[i];
        buffer[pos++] = k_hex[value >> 4];
        buffer[pos++] = k_hex[value & 0x0F];
        buffer[pos++] = ' ';
    }
    buffer[pos] = '\0';
}

void trace_log_format_cycle_prefix(const GG_Trace_Entry& entry, bool previous_cycle_valid,
    u64 previous_cycle, char* buffer, size_t buffer_size)
{
    if (previous_cycle_valid && entry.cycle >= previous_cycle)
    {
        snprintf(buffer, buffer_size, "@%012llu +%-12llu ",
                 (unsigned long long)entry.cycle,
                 (unsigned long long)(entry.cycle - previous_cycle));
    }
    else if (previous_cycle_valid)
    {
        snprintf(buffer, buffer_size, "@%012llu RESET         ",
                 (unsigned long long)entry.cycle);
    }
    else
    {
        snprintf(buffer, buffer_size, "@%012llu               ",
                 (unsigned long long)entry.cycle);
    }
}

static void format_cpu_entry(const GG_Trace_Entry& entry,
    const GG_Trace_Format_Options& options, char* buf, int buf_size)
{
    char instr[64] = "???";
    char bytes[25] = "";

    if (entry.cpu.name[0] != 0)
    {
        strncpy(instr, entry.cpu.name, sizeof(instr) - 1);
        instr[sizeof(instr) - 1] = '\0';

        char* p = instr;
        while (*p)
        {
            if (*p == '{')
            {
                char* end = strchr(p, '}');
                if (end)
                    memmove(p, end + 1, strlen(end + 1) + 1);
                else
                    break;
            }
            else
                p++;
        }
    }

    trace_log_format_cpu_bytes(entry, bytes, sizeof(bytes));

    char bank[8] = "";
    if (options.bank)
        snprintf(bank, sizeof(bank), "%02X:", entry.cpu.bank);

    char registers[40] = "";
    if (options.registers)
    {
        snprintf(registers, sizeof(registers), "A:%02X X:%02X Y:%02X S:%02X  ",
                 entry.cpu.a, entry.cpu.x, entry.cpu.y, entry.cpu.s);
    }

    char flags[20] = "";
    if (options.flags)
    {
        u8 p = entry.cpu.p;
        snprintf(flags, sizeof(flags), "%c%c%c%c%c%c%c%c  ",
                 (p & FLAG_NEGATIVE) ? 'N' : 'n',
                 (p & FLAG_OVERFLOW) ? 'V' : 'v',
                 (p & FLAG_TRANSFER) ? 'T' : 't',
                 (p & FLAG_BREAK) ? 'B' : 'b',
                 (p & FLAG_DECIMAL) ? 'D' : 'd',
                 (p & FLAG_INTERRUPT) ? 'I' : 'i',
                 (p & FLAG_ZERO) ? 'Z' : 'z',
                 (p & FLAG_CARRY) ? 'C' : 'c');
    }

    snprintf(buf, buf_size, "%s%04X  %s%s%-24s %s",
             bank, entry.cpu.pc, registers, flags, instr,
             options.bytes ? bytes : "");
}

void trace_logger_format_entry(const GG_Trace_Entry& entry,
    const GG_Trace_Format_Options& options, char* buffer, size_t buffer_size)
{
    char* buf = buffer;
    int buf_size = (int)buffer_size;

    if (options.cycles)
    {
        GG_Trace_Format_Options body_options = options;
        body_options.cycles = false;
        char body[GG_TRACE_FORMAT_BUFFER_SIZE];
        char prefix[64];
        trace_logger_format_entry(entry, body_options, body, sizeof(body));
        trace_log_format_cycle_prefix(entry, options.previous_cycle_valid,
                                      options.previous_cycle, prefix, sizeof(prefix));
        snprintf(buffer, buffer_size, "%s%s", prefix, body);
        return;
    }

    switch (entry.type)
    {
        case TRACE_CPU:
            format_cpu_entry(entry, options, buffer, (int)buffer_size);
            break;
        case TRACE_CPU_IRQ:
        {
            const char* irq_name = "???";
            if (entry.irq.vector == 0xFFFA) irq_name = "TIQ";
            else if (entry.irq.vector == 0xFFF8) irq_name = "IRQ1";
            else if (entry.irq.vector == 0xFFF6) irq_name = "IRQ2";
            snprintf(buf, buf_size, "  [CPU]  IRQ       %s  PC:$%04X  Vector:$%04X  Mask:%02X",
                     irq_name, entry.irq.pc, entry.irq.vector, entry.irq.irq_mask);
            break;
        }
        case TRACE_VDC:
        {
            static const char* k_vdc_reg_names[] = {
                "MAWR", "MARR", "VWR", "???", "???", "CR", "RCR", "BXR",
                "BYR", "MWR", "HSR", "HDR", "VSR", "VDR", "VCR", "DCR",
                "SOUR", "DESR", "LENR", "DVSSR"
            };
            const char* chip_name = entry.vdc.chip == 0 ? "VDC1" : "VDC2";
            switch (entry.vdc.event)
            {
                case TRACE_VDC_REG_WRITE:
                {
                    const char* reg_name = entry.vdc.reg < 20 ? k_vdc_reg_names[entry.vdc.reg] : "???";
                    snprintf(buf, buf_size, "  [%s]  REG       %s($%02X) %s:$%02X  Effective:$%04X",
                             chip_name, reg_name, entry.vdc.reg,
                             entry.vdc.msb ? "MSB" : "LSB", entry.vdc.raw, entry.vdc.value);
                    break;
                }
                case TRACE_VDC_VBLANK_IRQ:
                    snprintf(buf, buf_size, "  [%s]  VBLANK    IRQ", chip_name);
                    break;
                case TRACE_VDC_SCANLINE_IRQ:
                    snprintf(buf, buf_size, "  [%s]  SCANLINE  IRQ  RCR=%d", chip_name, entry.vdc.value);
                    break;
                case TRACE_VDC_OVERFLOW_IRQ:
                    snprintf(buf, buf_size, "  [%s]  OVERFLOW  IRQ", chip_name);
                    break;
                case TRACE_VDC_SPRITE_COLLISION_IRQ:
                    snprintf(buf, buf_size, "  [%s]  SPRITE    COLLISION IRQ", chip_name);
                    break;
                case TRACE_VDC_SATB_DMA_END_IRQ:
                    snprintf(buf, buf_size, "  [%s]  SATB DMA  END IRQ", chip_name);
                    break;
                case TRACE_VDC_VRAM_DMA_END_IRQ:
                    snprintf(buf, buf_size, "  [%s]  VRAM DMA  END IRQ", chip_name);
                    break;
                case TRACE_VDC_VRAM_DMA_START:
                    snprintf(buf, buf_size, "  [%s]  VRAM DMA  START  Source:$%04X  Dest:$%04X  Words:%u  DCR:$%04X",
                             chip_name, entry.vdc.value, entry.vdc.value2,
                             entry.vdc.param, entry.vdc.value3);
                    break;
                case TRACE_VDC_SATB_DMA_START:
                    snprintf(buf, buf_size, "  [%s]  SATB DMA  START  DVSSR=$%04X", chip_name, entry.vdc.value);
                    break;
                case TRACE_VDC_VPC_REG_WRITE:
                {
                    static const char* k_vpc_reg_names[] = {
                        "PRIORITY_1", "PRIORITY_2", "WINDOW_1_L", "WINDOW_1_H",
                        "WINDOW_2_L", "WINDOW_2_H", "VDC_SELECT"
                    };
                    const char* reg_name = entry.vdc.reg >= 0x08 && entry.vdc.reg <= 0x0E ?
                        k_vpc_reg_names[entry.vdc.reg - 0x08] : "UNKNOWN";
                    snprintf(buf, buf_size, "  [VPC]   REG       %s($%02X) Write:$%02X  Effective:$%03X",
                             reg_name, entry.vdc.reg, entry.vdc.raw, entry.vdc.value);
                    break;
                }
                default:
                    snprintf(buf, buf_size, "  [%s]  ???", chip_name);
                    break;
            }
            break;
        }
        case TRACE_INPUT:
        {
            const char* source = "NONE";
            if (entry.input.source == TRACE_INPUT_SOURCE_GAMEPAD) source = "GAMEPAD";
            else if (entry.input.source == TRACE_INPUT_SOURCE_MOUSE) source = "MOUSE";
            else if (entry.input.source == TRACE_INPUT_SOURCE_MB128) source = "MB128";

            if (entry.input.event == TRACE_INPUT_WRITE)
            {
                if (entry.input.port < GG_MAX_GAMEPADS)
                {
                    snprintf(buf, buf_size, "  [INPUT] WRITE   Data:$%02X  Pad:%u  SEL:%u  CLR:%u  Extra:%u",
                             entry.input.value, entry.input.port + 1,
                             entry.input.state & 0x01,
                             (entry.input.state >> 1) & 0x01,
                             (entry.input.state >> 2) & 0x01);
                }
                else
                {
                    snprintf(buf, buf_size, "  [INPUT] WRITE   Data:$%02X  Pad:NONE  SEL:%u  CLR:%u  Extra:%u",
                             entry.input.value, entry.input.state & 0x01,
                             (entry.input.state >> 1) & 0x01,
                             (entry.input.state >> 2) & 0x01);
                }
            }
            else if (entry.input.port < GG_MAX_GAMEPADS && entry.input.source != TRACE_INPUT_SOURCE_MB128)
            {
                snprintf(buf, buf_size, "  [INPUT] READ    %s Pad:%u  Data:$%02X  SEL:%u  CLR:%u  Extra:%u",
                         source, entry.input.port + 1, entry.input.value,
                         entry.input.state & 0x01,
                         (entry.input.state >> 1) & 0x01,
                         (entry.input.state >> 2) & 0x01);
            }
            else
            {
                snprintf(buf, buf_size, "  [INPUT] READ    %s  Data:$%02X  SEL:%u  CLR:%u  Extra:%u",
                         source, entry.input.value, entry.input.state & 0x01,
                         (entry.input.state >> 1) & 0x01,
                         (entry.input.state >> 2) & 0x01);
            }
            break;
        }
        case TRACE_TIMER:
            if (entry.timer.event == TRACE_TIMER_IRQ_REQUEST)
            {
                snprintf(buf, buf_size, "  [TIMER] UNDERFLOW  IRQ REQUEST  Reload:$%02X",
                         entry.timer.reload);
            }
            else if (entry.timer.event == TRACE_TIMER_RELOAD_WRITE)
            {
                snprintf(buf, buf_size, "  [TIMER] RELOAD  Write:$%02X  Reload:$%02X  Counter:$%02X",
                         entry.timer.value, entry.timer.reload, entry.timer.counter);
            }
            else if (entry.timer.event == TRACE_TIMER_CONTROL_WRITE)
            {
                snprintf(buf, buf_size, "  [TIMER] CONTROL Write:$%02X  Enabled:%u  Counter:$%02X  Reload:$%02X",
                         entry.timer.value, entry.timer.enabled,
                         entry.timer.counter, entry.timer.reload);
            }
            else
                snprintf(buf, buf_size, "  [TIMER] ???");
            break;
        case TRACE_CDROM:
        {
            switch (entry.cdrom.event)
            {
                case TRACE_CDROM_IRQ_SET:
                case TRACE_CDROM_IRQ_CLEAR:
                {
                    const char* irq_name = "UNKNOWN";
                    if (entry.cdrom.irq_type == CDROM_IRQ_ADPCM_HALF) irq_name = "ADPCM_HALF";
                    else if (entry.cdrom.irq_type == CDROM_IRQ_ADPCM_END) irq_name = "ADPCM_END";
                    else if (entry.cdrom.irq_type == CDROM_IRQ_STATUS_AND_MSG_IN) irq_name = "STATUS_MESSAGE_IN";
                    else if (entry.cdrom.irq_type == CDROM_IRQ_DATA_IN) irq_name = "DATA_IN";
                    snprintf(buf, buf_size, "  [CDROM] IRQ     %s %s  Active:$%02X  Enabled:$%02X",
                             entry.cdrom.event == TRACE_CDROM_IRQ_SET ? "SET" : "CLEAR",
                             irq_name, entry.cdrom.active, entry.cdrom.enabled);
                    break;
                }
                case TRACE_CDROM_IRQ_ENABLE:
                    snprintf(buf, buf_size, "  [CDROM] IRQ     ENABLE Write:$%02X  Mask:$%02X  Active:$%02X",
                             entry.cdrom.irq_type, entry.cdrom.enabled, entry.cdrom.active);
                    break;
                case TRACE_CDROM_FADER:
                {
                    u8 v = entry.cdrom.irq_type;
                    snprintf(buf, buf_size, "  [CDROM] FADER    %s  %s  %s",
                             IS_SET_BIT(v, 3) ? "ON" : "OFF",
                             IS_SET_BIT(v, 1) ? "ADPCM" : "CD",
                             IS_SET_BIT(v, 2) ? "FAST" : "SLOW");
                    break;
                }
                case TRACE_CDROM_RESET:
                    snprintf(buf, buf_size, "  [CDROM] RESET   Write:$%02X  Asserted:%u  Active:$%02X",
                             entry.cdrom.irq_type, (entry.cdrom.irq_type >> 1) & 1,
                             entry.cdrom.active);
                    break;
                case TRACE_CDROM_AUDIO_START:
                case TRACE_CDROM_AUDIO_SEEK_END:
                case TRACE_CDROM_AUDIO_STATE:
                case TRACE_CDROM_AUDIO_STOP_LBA:
                case TRACE_CDROM_AUDIO_BOUNDARY:
                {
                    static const char* k_audio_states[] = { "PLAYING", "IDLE", "PAUSED", "STOPPED" };
                    static const char* k_stop_events[] = { "STOP", "LOOP", "IRQ" };
                    const char* state = entry.cdrom.state < 4 ? k_audio_states[entry.cdrom.state] : "UNKNOWN";
                    const char* stop_event = entry.cdrom.irq_type < 3 ? k_stop_events[entry.cdrom.irq_type] : "UNKNOWN";
                    if (entry.cdrom.event == TRACE_CDROM_AUDIO_START)
                    {
                        snprintf(buf, buf_size, "  [CDROM] AUDIO   START  State:%s  LBA:%u  SeekCycles:%u",
                                 state, entry.cdrom.lba, entry.cdrom.param);
                    }
                    else if (entry.cdrom.event == TRACE_CDROM_AUDIO_SEEK_END)
                    {
                        snprintf(buf, buf_size, "  [CDROM] AUDIO   SEEK END  State:%s  LBA:%u",
                                 state, entry.cdrom.lba);
                    }
                    else if (entry.cdrom.event == TRACE_CDROM_AUDIO_STATE)
                    {
                        snprintf(buf, buf_size, "  [CDROM] AUDIO   STATE  %s  LBA:%u",
                                 state, entry.cdrom.lba);
                    }
                    else if (entry.cdrom.event == TRACE_CDROM_AUDIO_STOP_LBA)
                    {
                        snprintf(buf, buf_size, "  [CDROM] AUDIO   LIMIT  %s  Start:%u  Stop:%u",
                                 stop_event, entry.cdrom.param, entry.cdrom.lba);
                    }
                    else
                    {
                        snprintf(buf, buf_size, "  [CDROM] AUDIO   BOUNDARY  %s  State:%s  Stop:%u  Next:%u",
                                 stop_event, state, entry.cdrom.lba, entry.cdrom.param);
                    }
                    break;
                }
                default:
                    snprintf(buf, buf_size, "  [CDROM] ???");
                    break;
            }
            break;
        }
        case TRACE_PSG:
        {
            static const char* k_psg_reg_names[] = {
                "CHANNEL_SELECT", "MAIN_BALANCE", "FREQUENCY_L", "FREQUENCY_H",
                "CONTROL", "BALANCE", "WAVE_DDA", "NOISE",
                "LFO_FREQUENCY", "LFO_CONTROL"
            };

            const char* reg_name = entry.psg.reg < 10 ? k_psg_reg_names[entry.psg.reg] : "UNKNOWN";
            if (entry.psg.reg == 0)
            {
                snprintf(buf, buf_size, "  [PSG]   %-14s Channel:%u  Write:$%02X",
                         reg_name, entry.psg.value & 0x07, entry.psg.value);
            }
            else if (entry.psg.reg == 1 || entry.psg.reg == 8 || entry.psg.reg == 9)
            {
                snprintf(buf, buf_size, "  [PSG]   %-14s Write:$%02X",
                         reg_name, entry.psg.value);
            }
            else
            {
                snprintf(buf, buf_size, "  [PSG]   CH %u  %-14s Write:$%02X",
                         entry.psg.channel, reg_name, entry.psg.value);
            }
            break;
        }
        case TRACE_ADPCM:
        {
            char state[64];
            snprintf(state, sizeof(state), "  Playing:%u Pending:%u HalfIRQ:%u EndIRQ:%u",
                     entry.adpcm.state & 0x01 ? 1 : 0,
                     entry.adpcm.state & 0x02 ? 1 : 0,
                     entry.adpcm.state & 0x04 ? 1 : 0,
                     entry.adpcm.state & 0x08 ? 1 : 0);
            if (entry.adpcm.event == TRACE_ADPCM_REG_WRITE)
            {
                static const char* k_adpcm_reg_names[] = {
                    "ADDRESS_L", "ADDRESS_H", "DATA", "DMA", "RESERVED", "CONTROL", "SAMPLE_RATE"
                };
                const char* reg_name = entry.adpcm.reg >= 0x08 && entry.adpcm.reg <= 0x0E ?
                    k_adpcm_reg_names[entry.adpcm.reg - 0x08] : "INVALID";
                if (entry.adpcm.reg == 0x0D)
                {
                    snprintf(buf, buf_size, "  [ADPCM] WRITE   %s($%02X)=$%02X  Play:%u  Repeat:%u  Reset:%u%s",
                             reg_name, entry.adpcm.reg, entry.adpcm.value,
                             (entry.adpcm.value >> 5) & 1,
                             (entry.adpcm.value >> 6) & 1,
                             (entry.adpcm.value >> 7) & 1, state);
                }
                else if (entry.adpcm.reg == 0x0E)
                {
                    u8 rate = entry.adpcm.value & 0x0F;
                    snprintf(buf, buf_size, "  [ADPCM] WRITE   %s($%02X)=$%02X  Rate:%uHz%s",
                             reg_name, entry.adpcm.reg, entry.adpcm.value,
                             32000 / (16 - rate), state);
                }
                else
                {
                    snprintf(buf, buf_size, "  [ADPCM] WRITE   %s($%02X)=$%02X%s",
                             reg_name, entry.adpcm.reg, entry.adpcm.value, state);
                }
            }
            else if (entry.adpcm.event == TRACE_ADPCM_DMA_STATE)
            {
                snprintf(buf, buf_size, "  [ADPCM] DMA     State:$%02X  Active:%u  Length:%u%s",
                         entry.adpcm.value, entry.adpcm.value & 0x03 ? 1 : 0,
                         entry.adpcm.length, state);
            }
            else if (entry.adpcm.event == TRACE_ADPCM_PLAY_REQUEST)
            {
                snprintf(buf, buf_size, "  [ADPCM] PLAY    REQUEST  Address:$%04X  Length:%u%s",
                         entry.adpcm.address, entry.adpcm.length, state);
            }
            else if (entry.adpcm.event == TRACE_ADPCM_PLAY_START)
            {
                snprintf(buf, buf_size, "  [ADPCM] PLAY    START  Address:$%04X  Length:%u%s",
                         entry.adpcm.address, entry.adpcm.length, state);
            }
            else if (entry.adpcm.event == TRACE_ADPCM_PLAY_STOP)
            {
                snprintf(buf, buf_size, "  [ADPCM] PLAY    STOP  Address:$%04X  Length:%u%s",
                         entry.adpcm.address, entry.adpcm.length, state);
            }
            else if (entry.adpcm.event == TRACE_ADPCM_READ_COMPLETE ||
                     entry.adpcm.event == TRACE_ADPCM_WRITE_COMPLETE)
            {
                snprintf(buf, buf_size, "  [ADPCM] %s  Address:$%04X  Data:$%02X  Length:%u%s",
                         entry.adpcm.event == TRACE_ADPCM_READ_COMPLETE ? "READ " : "WRITE",
                         entry.adpcm.address, entry.adpcm.value, entry.adpcm.length, state);
            }
            else if (entry.adpcm.event == TRACE_ADPCM_HALF_IRQ ||
                     entry.adpcm.event == TRACE_ADPCM_END_IRQ)
            {
                snprintf(buf, buf_size, "  [ADPCM] IRQ     %s %s  Address:$%04X  Length:%u%s",
                         entry.adpcm.event == TRACE_ADPCM_HALF_IRQ ? "HALF" : "END",
                         entry.adpcm.value ? "SET" : "CLEAR",
                         entry.adpcm.address, entry.adpcm.length, state);
            }
            else
                snprintf(buf, buf_size, "  [ADPCM] ???");
            break;
        }
        case TRACE_SYSTEM:
        {
            if (entry.system.event == TRACE_SYSTEM_MPR_WRITE)
            {
                snprintf(buf, buf_size,
                         "  [MPR]   TAM     Mask:$%02X  MPR%u:$%02X->$%02X  Logical:$%04X-$%04X  Physical:$%06X",
                         entry.system.mask, entry.system.index,
                         entry.system.old_value, entry.system.new_value,
                         entry.system.address, entry.system.address + 0x1FFF,
                         entry.system.physical);
            }
            else if (entry.system.event == TRACE_SYSTEM_SF2_MAPPER)
            {
                snprintf(buf, buf_size,
                         "  [SF2]   LATCH   Address:$%04X  Block:$%02X->$%02X  PhysicalBase:$%06X",
                         entry.system.address, entry.system.old_value,
                         entry.system.new_value, entry.system.physical);
            }
            else if (entry.system.event == TRACE_SYSTEM_IRQ_MASK_WRITE)
            {
                snprintf(buf, buf_size,
                         "  [IRQ]   MASK    Write:$%02X  Mask:$%02X->$%02X  Request:$%02X  Active:$%02X",
                         entry.system.raw, entry.system.old_value,
                         entry.system.new_value, entry.system.request,
                         entry.system.state);
            }
            else if (entry.system.event == TRACE_SYSTEM_IRQ_ACK)
            {
                snprintf(buf, buf_size,
                         "  [IRQ]   ACK     Write:$%02X  Request:$%02X->$%02X  Mask:$%02X  Active:$%02X",
                         entry.system.raw, entry.system.old_value,
                         entry.system.new_value, entry.system.mask,
                         entry.system.state);
            }
            else
                snprintf(buf, buf_size, "  [SYSTEM] ???");
            break;
        }
        case TRACE_VCE:
        {
            switch (entry.vce.event)
            {
                case TRACE_VCE_CONTROL_WRITE:
                {
                    static const char* k_speed_names[] = { "5.36MHz", "7.16MHz", "10.8MHz", "10.8MHz" };
                    u8 speed = entry.vce.value & 0x03;
                    snprintf(buf, buf_size, "  [VCE]  CONTROL   Speed:%s  Blur:%d  B&W:%d",
                             k_speed_names[speed],
                             (entry.vce.value >> 2) & 1,
                             (entry.vce.value >> 7) & 1);
                    break;
                }
                case TRACE_VCE_COLOR_WRITE:
                    snprintf(buf, buf_size, "  [VCE]  COLOR     Addr:$%03X=$%03X",
                             entry.vce.reg, entry.vce.value & 0x1FF);
                    break;
                case TRACE_VCE_VSYNC_START:
                    snprintf(buf, buf_size, "  [VCE]  VSYNC     START  Line:%d", entry.vce.value);
                    break;
                case TRACE_VCE_VSYNC_END:
                    snprintf(buf, buf_size, "  [VCE]  VSYNC     END    Line:%d", entry.vce.value);
                    break;
                default:
                    snprintf(buf, buf_size, "  [VCE]  ???");
                    break;
            }
            break;
        }
        case TRACE_SCSI:
        {
            static const char* k_scsi_cmd_names[] = {
                "TEST_UNIT_READY", NULL, NULL, "REQUEST_SENSE",
                NULL, NULL, NULL, NULL, "READ"
            };
            static const char* k_scsi_phase_names[] = {
                "BUS FREE", "SELECTION", "MESSAGE OUT", "COMMAND", "DATA IN",
                "DATA OUT", "MESSAGE IN", "STATUS", "BUSY"
            };
            static const char* k_scsi_status_names[] = {
                "GOOD", "???", "CHECK_CONDITION", "???", "CONDITION_MET", "???", "???", "???",
                "BUSY"
            };
            static const char* k_scsi_problem_names[] = {
                "UNKNOWN_COMMAND",
                "COMMAND_OVERFLOW",
                "SELECTION_DURING_DATA_IN",
                "INVALID_READ_REQUEST",
                "INVALID_AUDIO_START_LBA",
                "UNKNOWN_AUDIO_STOP_MODE",
                "UNKNOWN_TOC_MODE",
                "LOAD_SECTOR_BUFFER_BUSY",
                "UNKNOWN_AUDIO_LBA_MODE",
                "CLAMPED_COMMAND_SIZE",
                "CLAMPED_DATA_SIZE",
                "CLAMPED_DATA_OFFSET",
                "READ_PAST_END",
                "READ_SECTOR_FAILED"
            };
            switch (entry.scsi.event)
            {
                case TRACE_SCSI_COMMAND:
                {
                    const char* cmd_name = NULL;
                    char bytes[49] = "";
                    int bytes_pos = 0;
                    for (u8 i = 0; i < entry.scsi.size && (bytes_pos + 3) < (int)sizeof(bytes); i++)
                    {
                        bytes_pos += snprintf(bytes + bytes_pos, sizeof(bytes) - bytes_pos,
                                              "%02X ", entry.scsi.data[i]);
                    }
                    if (entry.scsi.command < 9)
                        cmd_name = k_scsi_cmd_names[entry.scsi.command];
                    else if (entry.scsi.command == 0xD8) cmd_name = "AUDIO_START";
                    else if (entry.scsi.command == 0xD9) cmd_name = "AUDIO_STOP";
                    else if (entry.scsi.command == 0xDA) cmd_name = "AUDIO_PAUSE";
                    else if (entry.scsi.command == 0xDD) cmd_name = "READ_SUBCODE_Q";
                    else if (entry.scsi.command == 0xDE) cmd_name = "READ_TOC";
                    if (cmd_name)
                    {
                        if (entry.scsi.command == 0x08 && entry.scsi.size >= 5)
                        {
                            u32 lba = ((entry.scsi.data[1] & 0x1F) << 16) |
                                      (entry.scsi.data[2] << 8) | entry.scsi.data[3];
                            u16 count = entry.scsi.data[4] == 0 ? 256 : entry.scsi.data[4];
                            snprintf(buf, buf_size, "  [SCSI] CMD      %s  LBA:%u  Count:%u  Bytes:%s",
                                     cmd_name, lba, count, bytes);
                        }
                        else if ((entry.scsi.command == 0xD8 || entry.scsi.command == 0xD9) &&
                                 entry.scsi.size >= 10)
                        {
                            snprintf(buf, buf_size, "  [SCSI] CMD      %s  Mode:$%02X  AddressMode:%u  Address:%02X:%02X:%02X:%02X  Bytes:%s",
                                     cmd_name, entry.scsi.data[1], entry.scsi.data[9] >> 6,
                                     entry.scsi.data[2], entry.scsi.data[3],
                                     entry.scsi.data[4], entry.scsi.data[5], bytes);
                        }
                        else if (entry.scsi.command == 0xDE && entry.scsi.size >= 3)
                        {
                            snprintf(buf, buf_size, "  [SCSI] CMD      %s  Mode:$%02X  Track:$%02X  Bytes:%s",
                                     cmd_name, entry.scsi.data[1], entry.scsi.data[2], bytes);
                        }
                        else
                            snprintf(buf, buf_size, "  [SCSI] CMD      %s  Bytes:%s", cmd_name, bytes);
                    }
                    else
                        snprintf(buf, buf_size, "  [SCSI] CMD      $%02X  Bytes:%s", entry.scsi.command, bytes);
                    break;
                }
                case TRACE_SCSI_PHASE_CHANGE:
                {
                    const char* phase_name = entry.scsi.phase < 9 ? k_scsi_phase_names[entry.scsi.phase] : "???";
                    snprintf(buf, buf_size, "  [SCSI] PHASE    %s", phase_name);
                    break;
                }
                case TRACE_SCSI_STATUS:
                {
                    const char* status_name = entry.scsi.status < 9 ? k_scsi_status_names[entry.scsi.status] : NULL;
                    if (status_name != NULL)
                        snprintf(buf, buf_size, "  [SCSI] STATUS   PREPARE %s  Len:%u", status_name, entry.scsi.param);
                    else
                        snprintf(buf, buf_size, "  [SCSI] STATUS   PREPARE $%02X  Len:%u", entry.scsi.status, entry.scsi.param);
                    break;
                }
                case TRACE_SCSI_RESPONSE:
                {
                    char bytes[49] = "";
                    int bytes_pos = 0;
                    for (u8 i = 0; i < entry.scsi.size && (bytes_pos + 3) < (int)sizeof(bytes); i++)
                    {
                        bytes_pos += snprintf(bytes + bytes_pos, sizeof(bytes) - bytes_pos,
                                              "%02X ", entry.scsi.data[i]);
                    }

                    if (entry.scsi.command == 0xDD && entry.scsi.size >= 10)
                    {
                        snprintf(buf, buf_size, "  [SCSI] RESPONSE SUBCODE_Q  State:$%02X  ADRCTL:$%02X  Track:$%02X  Index:$%02X  Rel:%02X:%02X:%02X  Abs:%02X:%02X:%02X",
                                 entry.scsi.data[0], entry.scsi.data[1],
                                 entry.scsi.data[2], entry.scsi.data[3],
                                 entry.scsi.data[4], entry.scsi.data[5], entry.scsi.data[6],
                                 entry.scsi.data[7], entry.scsi.data[8], entry.scsi.data[9]);
                    }
                    else if (entry.scsi.command == 0xDE)
                    {
                        snprintf(buf, buf_size, "  [SCSI] RESPONSE READ_TOC  Mode:$%02X  Track:$%02X  Bytes:%s",
                                 entry.scsi.param >> 8, entry.scsi.param & 0xFF, bytes);
                    }
                    else
                    {
                        snprintf(buf, buf_size, "  [SCSI] RESPONSE CMD:$%02X  Bytes:%s",
                                 entry.scsi.command, bytes);
                    }
                    break;
                }
                case TRACE_SCSI_RESPONSE_BYTE:
                {
                    const char* phase_name = entry.scsi.phase < 9 ? k_scsi_phase_names[entry.scsi.phase] : "???";
                    snprintf(buf, buf_size, "  [SCSI] BYTE     %s  Offset:%u  Data:$%02X  REQ",
                             phase_name, entry.scsi.param, entry.scsi.status);
                    break;
                }
                case TRACE_SCSI_WARNING:
                case TRACE_SCSI_ERROR:
                {
                    const char* severity = entry.scsi.event == TRACE_SCSI_ERROR ? "ERROR" : "WARN";
                    const u8 problem_count = sizeof(k_scsi_problem_names) / sizeof(k_scsi_problem_names[0]);
                    const char* problem = entry.scsi.status < problem_count ? k_scsi_problem_names[entry.scsi.status] : "UNKNOWN";

                    switch (entry.scsi.status)
                    {
                        case TRACE_SCSI_PROBLEM_COMMAND_OVERFLOW:
                            snprintf(buf, buf_size, "  [SCSI] %s    %s  CMD:$%02X  Size:%u  Byte:$%02X",
                                     severity, problem, entry.scsi.command,
                                     entry.scsi.param >> 8, entry.scsi.param & 0xFF);
                            break;
                        case TRACE_SCSI_PROBLEM_INVALID_READ_REQUEST:
                            snprintf(buf, buf_size, "  [SCSI] %s    %s  LBA:%u  Count:%u",
                                     severity, problem, entry.scsi.param & 0xFFFFFF,
                                     entry.scsi.param >> 24);
                            break;
                        case TRACE_SCSI_PROBLEM_LOAD_SECTOR_BUFFER_BUSY:
                            snprintf(buf, buf_size, "  [SCSI] %s    %s  Size:%u  Offset:%u",
                                     severity, problem, entry.scsi.param >> 16,
                                     entry.scsi.param & 0xFFFF);
                            break;
                        case TRACE_SCSI_PROBLEM_CLAMPED_DATA_OFFSET:
                            snprintf(buf, buf_size, "  [SCSI] %s    %s  Offset:%u  Size:%u",
                                     severity, problem, entry.scsi.param >> 16,
                                     entry.scsi.param & 0xFFFF);
                            break;
                        case TRACE_SCSI_PROBLEM_READ_PAST_END:
                        case TRACE_SCSI_PROBLEM_READ_SECTOR_FAILED:
                            snprintf(buf, buf_size, "  [SCSI] %s    %s  LBA:%u",
                                     severity, problem, entry.scsi.param);
                            break;
                        default:
                            snprintf(buf, buf_size, "  [SCSI] %s    %s  CMD:$%02X  Param:%u",
                                     severity, problem, entry.scsi.command, entry.scsi.param);
                            break;
                    }
                    break;
                }
                default:
                    snprintf(buf, buf_size, "  [SCSI] ???");
                    break;
            }
            break;
        }
        default:
            snprintf(buf, buf_size, "  [???]");
            break;
    }
}
