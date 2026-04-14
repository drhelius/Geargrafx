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

#define GUI_DEBUG_TRACE_LOGGER_IMPORT
#include "gui_debug_trace_logger.h"

#include "imgui.h"
#include "gui.h"
#include "gui_filedialogs.h"
#include "gui_debug_constants.h"
#include "gui_debug_text.h"
#include "config.h"
#include "emu.h"
#include "gui_debug.h"

static bool trace_logger_enabled = false;
static u64 trace_logger_start_total = 0;

static void trace_logger_menu(void);
static void trace_logger_sync_flags(void);
static void format_entry_text(const GG_Trace_Entry& entry, char* buf, int buf_size);
static void format_cpu_entry(const GG_Trace_Entry& entry, char* buf, int buf_size);
static void render_entry_colored(const GG_Trace_Entry& entry, u32 index);
static void render_cpu_entry_colored(const GG_Trace_Entry& entry);

void gui_debug_window_trace_logger(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(340, 168), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(544, 362), ImGuiCond_FirstUseEver);

    ImGui::Begin("Trace Logger", &config_debug.show_trace_logger, ImGuiWindowFlags_MenuBar);

    trace_logger_menu();

    TraceLogger* tl = emu_get_core()->GetTraceLogger();

    if (ImGui::Button(trace_logger_enabled ? "Stop" : "Start"))
    {
        trace_logger_enabled = !trace_logger_enabled;
        if (trace_logger_enabled)
        {
            trace_logger_start_total = tl->GetTotalLogged();
            trace_logger_sync_flags();
        }
        else
            tl->SetEnabledFlags(0);
    }

    ImGui::SameLine();

    if (ImGui::Button("Clear"))
    {
        gui_debug_trace_logger_clear();
    }

    ImGui::SameLine();
    ImGui::Text("Entries: %u / %d", tl->GetCount(), TRACE_BUFFER_SIZE);

    if (trace_logger_enabled)
        trace_logger_sync_flags();

    if (ImGui::BeginChild("##logger", ImVec2(ImGui::GetContentRegionAvail().x, 0), true, ImGuiWindowFlags_HorizontalScrollbar))
    {
        ImGui::PushFont(gui_default_font);

        u32 count = tl->GetCount();

        ImGuiListClipper clipper;
        clipper.Begin((int)count, ImGui::GetTextLineHeightWithSpacing());

        while (clipper.Step())
        {
            for (int item = clipper.DisplayStart; item < clipper.DisplayEnd; item++)
            {
                const GG_Trace_Entry& entry = tl->GetEntry((u32)item);
                u64 entry_number = tl->GetTotalLogged() - (u64)count + (u64)item - trace_logger_start_total;
                render_entry_colored(entry, (u32)entry_number);
            }
        }

        ImGui::PopFont();
    }

    ImGui::EndChild();

    ImGui::End();
    ImGui::PopStyleVar();
}

void gui_debug_trace_logger_clear(void)
{
    emu_get_core()->GetTraceLogger()->Reset();
    trace_logger_start_total = 0;
}

void gui_debug_save_log(const char* file_path)
{
    FILE* file = fopen_utf8(file_path, "w");

    if (file != NULL)
    {
        TraceLogger* tl = emu_get_core()->GetTraceLogger();
        u32 count = tl->GetCount();
        char buf[256];

        for (u32 i = 0; i < count; i++)
        {
            const GG_Trace_Entry& entry = tl->GetEntry(i);
            format_entry_text(entry, buf, sizeof(buf));
            if (config_debug.trace_counter)
                fprintf(file, "%06u %s\n", i, buf);
            else
                fprintf(file, "%s\n", buf);
        }

        fclose(file);
    }
}

static void trace_logger_menu(void)
{
    ImGui::BeginMenuBar();

    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Save Log As..."))
        {
            gui_file_dialog_save_log();
        }

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("CPU"))
    {
        ImGui::MenuItem("Instruction Counter", "", &config_debug.trace_counter);
        ImGui::MenuItem("Bank Number", "", &config_debug.trace_bank);
        ImGui::MenuItem("Registers", "", &config_debug.trace_registers);
        ImGui::MenuItem("Flags", "", &config_debug.trace_flags);
        ImGui::MenuItem("Bytes", "", &config_debug.trace_bytes);

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Filter"))
    {
        ImGui::MenuItem("CPU", "", &config_debug.trace_cpu);
        ImGui::MenuItem("IRQs", "", &config_debug.trace_cpu_irq);
        ImGui::MenuItem("VDC", "", &config_debug.trace_vdc);
        ImGui::MenuItem("VCE", "", &config_debug.trace_vce);
        ImGui::MenuItem("Input", "", &config_debug.trace_input);
        ImGui::MenuItem("Timer", "", &config_debug.trace_timer);
        ImGui::MenuItem("CD-ROM", "", &config_debug.trace_cdrom);
        ImGui::MenuItem("PSG", "", &config_debug.trace_psg);
        ImGui::MenuItem("ADPCM", "", &config_debug.trace_adpcm);
        ImGui::MenuItem("SCSI", "", &config_debug.trace_scsi);

        ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
}

static void trace_logger_sync_flags(void)
{
    u32 flags = 0;
    if (config_debug.trace_cpu)       flags |= TRACE_FLAG_CPU;
    if (config_debug.trace_cpu_irq)   flags |= TRACE_FLAG_CPU_IRQ;
    if (config_debug.trace_vdc)       flags |= TRACE_FLAG_VDC;
    if (config_debug.trace_input)     flags |= TRACE_FLAG_INPUT;
    if (config_debug.trace_timer)     flags |= TRACE_FLAG_TIMER;
    if (config_debug.trace_cdrom)     flags |= TRACE_FLAG_CDROM;
    if (config_debug.trace_psg)       flags |= TRACE_FLAG_PSG;
    if (config_debug.trace_adpcm)     flags |= TRACE_FLAG_ADPCM;
    if (config_debug.trace_vce)       flags |= TRACE_FLAG_VCE;
    if (config_debug.trace_scsi)      flags |= TRACE_FLAG_SCSI;
    emu_get_core()->GetTraceLogger()->SetEnabledFlags(flags);
}

static void format_cpu_entry(const GG_Trace_Entry& entry, char* buf, int buf_size)
{
    Memory* memory = emu_get_core()->GetMemory();
    GG_Disassembler_Record* record = memory->GetDisassemblerRecord(entry.cpu.pc, entry.cpu.bank);

    char instr[64] = "???";
    char bytes[25] = "";
    if (IsValidPointer(record))
    {
        strncpy(instr, record->name, sizeof(instr) - 1);
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
        strncpy(bytes, record->bytes, sizeof(bytes) - 1);
        bytes[sizeof(bytes) - 1] = '\0';
    }

    char bank[8] = "";
    if (config_debug.trace_bank)
        snprintf(bank, sizeof(bank), "%02X:", entry.cpu.bank);

    char registers[40] = "";
    if (config_debug.trace_registers)
        snprintf(registers, sizeof(registers), "A:%02X X:%02X Y:%02X S:%02X  ",
                 entry.cpu.a, entry.cpu.x, entry.cpu.y, entry.cpu.s);

    char flags[20] = "";
    if (config_debug.trace_flags)
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
             bank, entry.cpu.pc,
             registers, flags, instr,
             config_debug.trace_bytes ? bytes : "");
}

static void format_entry_text(const GG_Trace_Entry& entry, char* buf, int buf_size)
{
    switch (entry.type)
    {
        case TRACE_CPU:
            format_cpu_entry(entry, buf, buf_size);
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
                    snprintf(buf, buf_size, "  [%s]  REG       %s($%02X)=$%04X",
                             chip_name, reg_name, entry.vdc.reg, entry.vdc.value);
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
                    snprintf(buf, buf_size, "  [%s]  VRAM DMA  START", chip_name);
                    break;
                case TRACE_VDC_SATB_DMA_START:
                    snprintf(buf, buf_size, "  [%s]  SATB DMA  START  DVSSR=$%04X", chip_name, entry.vdc.value);
                    break;
                default:
                    snprintf(buf, buf_size, "  [%s]  ???", chip_name);
                    break;
            }
            break;
        }
        case TRACE_INPUT:
            snprintf(buf, buf_size, "  [INPUT] PORT %d  Data:$%02X",
                     entry.input.port, entry.input.value);
            break;
        case TRACE_TIMER:
            snprintf(buf, buf_size, "  [TIMER] IRQ     Counter:%02X  Reload:%02X",
                     entry.timer.counter, entry.timer.reload);
            break;
        case TRACE_CDROM:
        {
            switch (entry.cdrom.event)
            {
                case TRACE_CDROM_IRQ:
                    snprintf(buf, buf_size, "  [CDROM] IRQ     Type:%02X  Active:%02X  Enabled:%02X",
                             entry.cdrom.irq_type, entry.cdrom.active, entry.cdrom.enabled);
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
                    snprintf(buf, buf_size, "  [CDROM] RESET    $%02X", entry.cdrom.irq_type);
                    break;
                default:
                    snprintf(buf, buf_size, "  [CDROM] ???");
                    break;
            }
            break;
        }
        case TRACE_PSG:
            snprintf(buf, buf_size, "  [PSG]   CH %d    Reg:$%02X=$%02X",
                     entry.psg.channel, entry.psg.reg, entry.psg.value);
            break;
        case TRACE_ADPCM:
            snprintf(buf, buf_size, "  [ADPCM] REG     $%02X=$%02X",
                     entry.adpcm.reg, entry.adpcm.value);
            break;
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
                case TRACE_VCE_HSYNC:
                    snprintf(buf, buf_size, "  [VCE]  HSYNC     Line:%d", entry.vce.value);
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
                "CLAMPED_DATA_OFFSET"
            };
            switch (entry.scsi.event)
            {
                case TRACE_SCSI_COMMAND:
                {
                    const char* cmd_name = NULL;
                    if (entry.scsi.command < 9)
                        cmd_name = k_scsi_cmd_names[entry.scsi.command];
                    else if (entry.scsi.command == 0xD8) cmd_name = "AUDIO_START";
                    else if (entry.scsi.command == 0xD9) cmd_name = "AUDIO_STOP";
                    else if (entry.scsi.command == 0xDA) cmd_name = "AUDIO_PAUSE";
                    else if (entry.scsi.command == 0xDD) cmd_name = "READ_SUBCODE_Q";
                    else if (entry.scsi.command == 0xDE) cmd_name = "READ_TOC";
                    if (cmd_name)
                    {
                        if (entry.scsi.command == 0x08)
                            snprintf(buf, buf_size, "  [SCSI] CMD      %s  LBA:%u", cmd_name, entry.scsi.param);
                        else
                            snprintf(buf, buf_size, "  [SCSI] CMD      %s", cmd_name);
                    }
                    else
                        snprintf(buf, buf_size, "  [SCSI] CMD      $%02X", entry.scsi.command);
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
                        snprintf(buf, buf_size, "  [SCSI] STATUS   %s  Len:%u", status_name, entry.scsi.param);
                    else
                        snprintf(buf, buf_size, "  [SCSI] STATUS   $%02X  Len:%u", entry.scsi.status, entry.scsi.param);
                    break;
                }
                case TRACE_SCSI_WARNING:
                case TRACE_SCSI_ERROR:
                {
                    const char* severity = entry.scsi.event == TRACE_SCSI_ERROR ? "ERROR" : "WARN";
                    const char* problem = entry.scsi.status < 12 ? k_scsi_problem_names[entry.scsi.status] : "UNKNOWN";

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
                        case TRACE_SCSI_PROBLEM_CLAMPED_DATA_OFFSET:
                            snprintf(buf, buf_size, "  [SCSI] %s    %s  Size:%u  Offset:%u",
                                     severity, problem, entry.scsi.param >> 16,
                                     entry.scsi.param & 0xFFFF);
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

static void render_cpu_entry_colored(const GG_Trace_Entry& entry)
{
    Memory* memory = emu_get_core()->GetMemory();
    GG_Disassembler_Record* record = memory->GetDisassemblerRecord(entry.cpu.pc, entry.cpu.bank);

    if (config_debug.trace_bank)
    {
        ImGui::TextColored(violet, "%02X ", entry.cpu.bank);
        ImGui::SameLine(0, 0);
    }

    ImGui::TextColored(cyan, "%04X", entry.cpu.pc);

    if (config_debug.trace_registers)
    {
        ImGui::SameLine(0, 0);
        ImGui::TextColored(magenta, "  A:");
        ImGui::SameLine(0, 0);
        ImGui::TextColored(white, "%02X", entry.cpu.a);
        ImGui::SameLine(0, 0);
        ImGui::TextColored(magenta, " X:");
        ImGui::SameLine(0, 0);
        ImGui::TextColored(white, "%02X", entry.cpu.x);
        ImGui::SameLine(0, 0);
        ImGui::TextColored(magenta, " Y:");
        ImGui::SameLine(0, 0);
        ImGui::TextColored(white, "%02X", entry.cpu.y);
        ImGui::SameLine(0, 0);
        ImGui::TextColored(magenta, " S:");
        ImGui::SameLine(0, 0);
        ImGui::TextColored(white, "%02X", entry.cpu.s);
    }

    if (config_debug.trace_flags)
    {
        u8 p = entry.cpu.p;
        ImGui::SameLine(0, 0);
        ImGui::TextColored(yellow, "  %c%c%c%c%c%c%c%c",
                 (p & FLAG_NEGATIVE) ? 'N' : 'n',
                 (p & FLAG_OVERFLOW) ? 'V' : 'v',
                 (p & FLAG_TRANSFER) ? 'T' : 't',
                 (p & FLAG_BREAK) ? 'B' : 'b',
                 (p & FLAG_DECIMAL) ? 'D' : 'd',
                 (p & FLAG_INTERRUPT) ? 'I' : 'i',
                 (p & FLAG_ZERO) ? 'Z' : 'z',
                 (p & FLAG_CARRY) ? 'C' : 'c');
    }

    if (IsValidPointer(record))
    {
        std::string instr = record->name;
        size_t pos;
        pos = instr.find("{n}");
        if (pos != std::string::npos)
            instr.replace(pos, 3, c_white);
        pos = instr.find("{o}");
        if (pos != std::string::npos)
            instr.replace(pos, 3, c_brown);
        pos = instr.find("{e}");
        if (pos != std::string::npos)
            instr.replace(pos, 3, c_blue);

        ImGui::SameLine(0, 0);
        TextColoredEx("  %s%s", c_white, instr.c_str());

        if (config_debug.trace_bytes)
        {
            float char_width = ImGui::CalcTextSize("A").x;
            float bytes_column = char_width * 31;
            if (config_debug.trace_bank)         bytes_column += char_width * 3;
            if (config_debug.trace_registers)    bytes_column += char_width * 24;
            if (config_debug.trace_flags)        bytes_column += char_width * 10;
            if (config_debug.trace_counter)      bytes_column += char_width * 7;
            ImGui::SameLine(bytes_column);
            ImGui::TextColored(gray, "%s", record->bytes);
        }
    }
    else
    {
        ImGui::SameLine(0, 0);
        ImGui::TextColored(gray, "  ???");
    }
}

static void render_entry_colored(const GG_Trace_Entry& entry, u32 index)
{
    char buf[256];

    if (config_debug.trace_counter)
    {
        ImGui::TextColored(gray, "%06u ", index);
        ImGui::SameLine(0, 0);
    }

    switch (entry.type)
    {
        case TRACE_CPU:
            render_cpu_entry_colored(entry);
            break;
        case TRACE_CPU_IRQ:
            format_entry_text(entry, buf, sizeof(buf));
            ImGui::TextColored(red, "%s", buf);
            break;
        case TRACE_VDC:
            format_entry_text(entry, buf, sizeof(buf));
            ImGui::TextColored(green, "%s", buf);
            break;
        case TRACE_INPUT:
            format_entry_text(entry, buf, sizeof(buf));
            ImGui::TextColored(yellow, "%s", buf);
            break;
        case TRACE_TIMER:
            format_entry_text(entry, buf, sizeof(buf));
            ImGui::TextColored(orange, "%s", buf);
            break;
        case TRACE_CDROM:
            format_entry_text(entry, buf, sizeof(buf));
            ImGui::TextColored(violet, "%s", buf);
            break;
        case TRACE_PSG:
            format_entry_text(entry, buf, sizeof(buf));
            ImGui::TextColored(blue, "%s", buf);
            break;
        case TRACE_ADPCM:
            format_entry_text(entry, buf, sizeof(buf));
            ImGui::TextColored(magenta, "%s", buf);
            break;
        case TRACE_VCE:
            format_entry_text(entry, buf, sizeof(buf));
            ImGui::TextColored(cyan, "%s", buf);
            break;
        case TRACE_SCSI:
            format_entry_text(entry, buf, sizeof(buf));
            ImGui::TextColored(brown, "%s", buf);
            break;
        default:
            break;
    }
}
