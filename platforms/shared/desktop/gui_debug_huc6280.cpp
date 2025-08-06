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

#define GUI_DEBUG_HUC6280_IMPORT
#include "gui_debug_huc6280.h"

#include "imgui.h"
#include "geargrafx.h"
#include "gui_debug_constants.h"
#include "gui_debug_memory.h"
#include "gui.h"
#include "config.h"
#include "emu.h"
#include "utils.h"

static char mpr_name[16] = { };
static char mpr_tooltip[128] = { };

static void get_bank_name(u8 mpr, u8 mpr_value, char *name, char* tooltip);
static void goto_address(u8 mpr_value);

void gui_debug_window_huc6280(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(3, 26), ImGuiCond_FirstUseEver);

    ImGui::Begin("HuC6280", &config_debug.show_processor, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize);

    ImGui::PushFont(gui_default_font);

    GeargrafxCore* core = emu_get_core();
    HuC6280* processor = core->GetHuC6280();
    HuC6280::HuC6280_State* proc_state = processor->GetState();
    Memory* memory = core->GetMemory();
    Input* input = core->GetInput();

    if (ImGui::BeginTable("huc6280", 1, ImGuiTableFlags_BordersInnerH))
    {
        ImGui::TableNextColumn();
        ImGui::TextColored(cyan, "      STATUS");
        ImGui::TextColored(magenta, "  N V T B D I Z C");
        ImGui::Text("  " BYTE_TO_BINARY_PATTERN_ALL_SPACED, BYTE_TO_BINARY(proc_state->P->GetValue()));

        ImGui::TableNextColumn();
        ImGui::TextColored(yellow, "    PC"); ImGui::SameLine();
        ImGui::Text("= $%04X", proc_state->PC->GetValue());
        ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->PC->GetHigh()), BYTE_TO_BINARY(proc_state->PC->GetLow()));

        ImGui::TableNextColumn();
        ImGui::TextColored(yellow, " PHYS PC"); ImGui::SameLine();
        if (ImGui::IsItemClicked())
        {
            gui_debug_memory_goto(MEMORY_EDITOR_ROM, memory->GetPhysicalAddress(proc_state->PC->GetValue()));
        }
        ImGui::Text("= $%06X", memory->GetPhysicalAddress(proc_state->PC->GetValue()));
        if (ImGui::IsItemClicked())
        {
            gui_debug_memory_goto(MEMORY_EDITOR_ROM, memory->GetPhysicalAddress(proc_state->PC->GetValue()));
        }

        ImGui::TableNextColumn();
        ImGui::TextColored(yellow, "    SP"); ImGui::SameLine();
        if (ImGui::IsItemClicked())
            gui_debug_memory_goto(MEMORY_EDITOR_RAM, (STACK_ADDR - 0x2000) | proc_state->S->GetValue());
        ImGui::Text("= $%04X", STACK_ADDR | proc_state->S->GetValue());
        if (ImGui::IsItemClicked())
            gui_debug_memory_goto(MEMORY_EDITOR_RAM, (STACK_ADDR - 0x2000) | proc_state->S->GetValue());
        ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(0x21), BYTE_TO_BINARY(proc_state->S->GetValue()));
        if (ImGui::IsItemClicked())
            gui_debug_memory_goto(MEMORY_EDITOR_RAM, (STACK_ADDR - 0x2000) | proc_state->S->GetValue());

        ImGui::TableNextColumn();

        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(2.0f, 2.0f));

        if (ImGui::BeginTable("regs", 2, ImGuiTableFlags_BordersInnerH |ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoPadOuterX))
        {
            ImGui::TableNextColumn();
            ImGui::TextColored(cyan, " A"); ImGui::SameLine();
            ImGui::Text("   $%02X", proc_state->A->GetValue());
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->A->GetValue()));

            ImGui::TableNextColumn();
            ImGui::TextColored(cyan, " S"); ImGui::SameLine();
            ImGui::Text("   $%02X", proc_state->S->GetValue());
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->S->GetValue()));

            ImGui::TableNextColumn();
            ImGui::TextColored(cyan, " X"); ImGui::SameLine();
            ImGui::Text("   $%02X", proc_state->X->GetValue());
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->X->GetValue()));

            ImGui::TableNextColumn();
            ImGui::TextColored(cyan, " Y"); ImGui::SameLine();
            ImGui::Text("   $%02X", proc_state->Y->GetValue());
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->Y->GetValue()));

            for (u8 i = 0; i < 8; i++)
            {
                char label[8];
                snprintf(label, sizeof(label), "MPR%d", i);
                ImGui::TableNextColumn();
                ImGui::TextColored(violet, "%s", label); ImGui::SameLine();
                if (ImGui::IsItemClicked())
                    goto_address(memory->GetMpr(i));
                ImGui::Text(" $%02X", memory->GetMpr(i));
                if (ImGui::IsItemClicked())
                    goto_address(memory->GetMpr(i));
                ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(memory->GetMpr(i)));
                get_bank_name(i, memory->GetMpr(i), mpr_name, mpr_tooltip);
                if (ImGui::IsItemClicked())
                    goto_address(memory->GetMpr(i));
                ImGui::TextColored(brown, " %s", mpr_name);
                if (ImGui::IsItemClicked())
                    goto_address(memory->GetMpr(i));
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, yellow);
                    ImGui::SetTooltip("%s", mpr_tooltip);
                    ImGui::PopStyleColor();
                }
            }

            ImGui::TableNextColumn();
            ImGui::TextColored(red, "I/O "); ImGui::SameLine();
            ImGui::Text(" $%02X", input->GetIORegister());
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(input->GetIORegister()));

            ImGui::TableNextColumn();
            u8 tim = (*proc_state->TIMER) ? 0x01 : 0x00;
            ImGui::TextColored(blue, "TIM  "); ImGui::SameLine();
            ImGui::Text("$%02X", tim);
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(tim));

            ImGui::TableNextColumn();
            ImGui::TextColored(blue, "TIMC"); ImGui::SameLine();
            ImGui::Text(" $%02X", *proc_state->TIMER_COUNTER);
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(*proc_state->TIMER_COUNTER));

            ImGui::TableNextColumn();
            ImGui::TextColored(blue, "TIMR"); ImGui::SameLine();
            ImGui::Text(" $%02X", *proc_state->TIMER_RELOAD);
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(*proc_state->TIMER_RELOAD));

            ImGui::TableNextColumn();
            ImGui::TextColored(magenta, "IDR "); ImGui::SameLine();
            ImGui::Text(" $%02X", *proc_state->IDR);
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(*proc_state->IDR));

            ImGui::TableNextColumn();
            ImGui::TextColored(magenta, "IRR "); ImGui::SameLine();
            ImGui::Text(" $%02X", *proc_state->IRR);
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(*proc_state->IRR));

            ImGui::EndTable();
        }

        ImGui::PopStyleVar();

        ImGui::TableNextColumn();

        ImGui::TextColored(magenta, "IRQ1:"); ImGui::SameLine();
        ImGui::TextColored(*proc_state->IDR & 0x02 ? gray : green, *proc_state->IDR & 0x02 ? "OFF" : "ON "); ImGui::SameLine();
        ImGui::TextColored(*proc_state->IRR & 0x02 ? green : gray, "ASSERTED");

        ImGui::TextColored(magenta, "IRQ2:"); ImGui::SameLine();
        ImGui::TextColored(*proc_state->IDR & 0x01 ? gray : green, *proc_state->IDR & 0x01 ? "OFF" : "ON "); ImGui::SameLine();
        ImGui::TextColored(*proc_state->IRR & 0x01 ? green : gray, "ASSERTED");

        ImGui::TextColored(magenta, "TIQ: "); ImGui::SameLine();
        ImGui::TextColored(*proc_state->IDR & 0x04 ? gray : green, *proc_state->IDR & 0x04 ? "OFF" : "ON "); ImGui::SameLine();
        ImGui::TextColored(*proc_state->IRR & 0x04 ? green : gray, "ASSERTED");

        ImGui::TableNextColumn();
        ImGui::TextColored(input->GetSel() ? green : gray, " I/O SEL"); ImGui::SameLine();
        ImGui::TextColored(input->GetClr() ? green : gray, " I/O CLR"); ImGui::SameLine();

        ImGui::TableNextColumn();
        ImGui::TextColored(violet, " SPEED:"); ImGui::SameLine();
        ImGui::TextColored(orange, *proc_state->SPEED ? " 7.16 MHz" : " 1.79 MHz");

        ImGui::EndTable();
    }

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}

static void get_bank_name(u8 mpr, u8 mpr_value, char *name, char* tooltip)
{
    u16 cpu_address = mpr << 13;

    GeargrafxCore* core = emu_get_core();
    Memory* memory = core->GetMemory();
    Media* media = core->GetMedia();
    Memory::MemoryBankType bank_type = memory->GetBankType(mpr_value);

    switch (bank_type)
    {
    case Memory::MEMORY_BANK_TYPE_ROM:
        // ROM
        {
            u32 rom_address = mpr_value << 13;
            snprintf(name, 16, "ROM $%02X", mpr_value);
            snprintf(tooltip, 128, "Range (CPU) $%04X-$%04X \nRange (ROM) $%06X-$%06X",
                cpu_address, cpu_address + 0x1FFF, rom_address, rom_address + 0x1FFF);
        }
        break;
    case Memory::MEMORY_BANK_TYPE_BIOS:
        // BIOS
        {
            u32 rom_address = mpr_value << 13;
            snprintf(name, 16, "BIOS $%02X", mpr_value);
            snprintf(tooltip, 128, "Range (CPU) $%04X-$%04X \nRange (BIOS) $%06X-$%06X",
                cpu_address, cpu_address + 0x1FFF, rom_address, rom_address + 0x1FFF);
        }
        break;
    case Memory::MEMORY_BANK_TYPE_CARD_RAM:
        // Card RAM
        {
            int card_ram_start = memory->GetCardRAMStart();
            int card_ram_size = memory->GetCardRAMSize();
            u32 card_ram_address = ((mpr_value - card_ram_start) * 0x2000) % card_ram_size;
            snprintf(name, 16, "CARD RAM");
            snprintf(tooltip, 128, "Range (CPU) $%04X-$%04X \nRange (CARD RAM) $%06X-$%06X",
                cpu_address, cpu_address + 0x1FFF, card_ram_address, card_ram_address + 0x1FFF);
        }
        break;
    case Memory::MEMORY_BANK_TYPE_BACKUP_RAM:
        // Backup RAM
        snprintf(name, 16, "BRAM");
        snprintf(tooltip, 128, "Range (CPU) $%04X-$%04X \nBackup RAM", 
            cpu_address, cpu_address + 0x1FFF);
        break;
    case Memory::MEMORY_BANK_TYPE_WRAM:
        // WRAM
        {
            u8 ram_bank = mpr_value - 0xF8;
            u16 ram_address = ram_bank << 13;

            if (media->IsSGX())
            {
                snprintf(name, 16, "WRAM $%02X", ram_bank);
                snprintf(tooltip, 128, "Range (CPU) $%04X-$%04X \nRange (WRAM) $%04X-$%04X",
                    cpu_address, cpu_address + 0x1FFF, ram_address, ram_address + 0x1FFF);
            }
            else
            {
                snprintf(name, 16, "WRAM $00");
                snprintf(tooltip, 128, "Range (CPU) $%04X-$%04X \nRange (WRAM) $0000-$1FFF",
                    cpu_address, cpu_address + 0x1FFF);
            }
        }
        break;
    case Memory::MEMORY_BANK_TYPE_CDROM_RAM:
        // CDROM RAM
        {
            u32 cdrom_ram_address = (mpr_value - 0x80) * 0x2000;
            snprintf(name, 16, "CD RAM");
            snprintf(tooltip, 128, "Range (CPU) $%04X-$%04X \nRange (CDROM RAM) $%06X-$%06X",
                cpu_address, cpu_address + 0x1FFF, cdrom_ram_address, cdrom_ram_address + 0x1FFF);
        }
        break;
    case Memory::MEMORY_BANK_TYPE_UNUSED:
    default:
        // Hardware registers at 0xFF or unused
        if (mpr_value == 0xFF)
        {
            snprintf(name, 16, "HARDWARE");
            snprintf(tooltip, 128, "Range (CPU) $%04X-$%04X", cpu_address, cpu_address + 0x1FFF);
        }
        else
        {
            snprintf(name, 16, "UNUSED");
            snprintf(tooltip, 128, "Range (CPU) $%04X-$%04X", cpu_address, cpu_address + 0x1FFF);
        }
        break;
    }
}

static void goto_address(u8 mpr_value)
{
    GeargrafxCore* core = emu_get_core();
    Memory* memory = core->GetMemory();
    Memory::MemoryBankType bank_type = memory->GetBankType(mpr_value);

    switch (bank_type)
    {
    case Memory::MEMORY_BANK_TYPE_ROM:
    case Memory::MEMORY_BANK_TYPE_BIOS:
        {
            u32 rom_address = mpr_value << 13;
            gui_debug_memory_goto(MEMORY_EDITOR_ROM, rom_address);
        }
        break;
    case Memory::MEMORY_BANK_TYPE_CARD_RAM:
        {
            int card_ram_start = memory->GetCardRAMStart();
            int card_ram_size = memory->GetCardRAMSize();
            u32 card_ram_address = ((mpr_value - card_ram_start) * 0x2000) % card_ram_size;
            gui_debug_memory_goto(MEMORY_EDITOR_CARD_RAM, card_ram_address);
        }
        break;
    case Memory::MEMORY_BANK_TYPE_BACKUP_RAM:
        {
            gui_debug_memory_goto(MEMORY_EDITOR_BACKUP_RAM, 0);
        }
        break;
    case Memory::MEMORY_BANK_TYPE_CDROM_RAM:
        {
            u32 cdrom_ram_address = (mpr_value - 0x80) * 0x2000;
            gui_debug_memory_goto(MEMORY_EDITOR_CDROM_RAM, cdrom_ram_address);
        }
        break;
    case Memory::MEMORY_BANK_TYPE_WRAM:
        {
            u8 ram_bank = mpr_value - 0xF8;
            u16 ram_address = ram_bank << 13;
            gui_debug_memory_goto(MEMORY_EDITOR_RAM, ram_address);
        }
        break;
    default:
        break;
    }
}
