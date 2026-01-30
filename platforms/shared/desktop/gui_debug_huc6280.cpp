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
#include "gui_debug_widgets.h"
#include "gui.h"
#include "config.h"
#include "emu.h"
#include "utils.h"

enum HuC6280RegId
{
    HuC6280RegId_A = 0,
    HuC6280RegId_S = 1,
    HuC6280RegId_X = 2,
    HuC6280RegId_Y = 3,
    HuC6280RegId_P = 4,
    HuC6280RegId_PC = 5,
    HuC6280RegId_IO = 6,
    HuC6280RegId_TIM = 7,
    HuC6280RegId_TIMC = 8,
    HuC6280RegId_TIMR = 9,
    HuC6280RegId_IDR = 10,
    HuC6280RegId_IRR = 11,
    HuC6280RegId_MPR0 = 12,
    HuC6280RegId_MPR1 = 13,
    HuC6280RegId_MPR2 = 14,
    HuC6280RegId_MPR3 = 15,
    HuC6280RegId_MPR4 = 16,
    HuC6280RegId_MPR5 = 17,
    HuC6280RegId_MPR6 = 18,
    HuC6280RegId_MPR7 = 19
};

static void HuC6280WriteCallback8(u16 reg_id, u8 value, void* user_data)
{
    GeargrafxCore* core = (GeargrafxCore*)user_data;
    HuC6280* processor = core->GetHuC6280();
    HuC6280::HuC6280_State* proc_state = processor->GetState();
    Memory* memory = core->GetMemory();
    Input* input = core->GetInput();

    switch (reg_id)
    {
        case HuC6280RegId_A: proc_state->A->SetValue(value); break;
        case HuC6280RegId_S: proc_state->S->SetValue(value); break;
        case HuC6280RegId_X: proc_state->X->SetValue(value); break;
        case HuC6280RegId_Y: proc_state->Y->SetValue(value); break;
        case HuC6280RegId_P: proc_state->P->SetValue(value); break;
        case HuC6280RegId_IO: input->SetIORegister(value); break;
        case HuC6280RegId_TIM: processor->WriteTimerRegister(0x0C01, value); break;
        case HuC6280RegId_TIMC: *proc_state->TIMER_COUNTER = value & 0x7F; break;
        case HuC6280RegId_TIMR: processor->WriteTimerRegister(0x0C00, value); break;
        case HuC6280RegId_IDR: processor->WriteInterruptRegister(0x1402, value); break;
        case HuC6280RegId_IRR: processor->WriteInterruptRegister(0x1403, value); break;
        case HuC6280RegId_MPR0: memory->SetMpr(0, value); break;
        case HuC6280RegId_MPR1: memory->SetMpr(1, value); break;
        case HuC6280RegId_MPR2: memory->SetMpr(2, value); break;
        case HuC6280RegId_MPR3: memory->SetMpr(3, value); break;
        case HuC6280RegId_MPR4: memory->SetMpr(4, value); break;
        case HuC6280RegId_MPR5: memory->SetMpr(5, value); break;
        case HuC6280RegId_MPR6: memory->SetMpr(6, value); break;
        case HuC6280RegId_MPR7: memory->SetMpr(7, value); break;
    }
}

static void HuC6280WriteCallback1(u16 reg_id, u8 bit_index, bool value, void* user_data)
{
    GeargrafxCore* core = (GeargrafxCore*)user_data;
    HuC6280::HuC6280_State* proc_state = core->GetHuC6280()->GetState();

    if (reg_id == HuC6280RegId_P)
    {
        u8 p = proc_state->P->GetValue();
        if (value)
            p |= (1 << bit_index);
        else
            p &= ~(1 << bit_index);
        proc_state->P->SetValue(p);
    }
}

static void HuC6280WriteCallback16(u16 reg_id, u16 value, void* user_data)
{
    GeargrafxCore* core = (GeargrafxCore*)user_data;
    HuC6280::HuC6280_State* proc_state = core->GetHuC6280()->GetState();

    switch (reg_id)
    {
        case HuC6280RegId_PC: proc_state->PC->SetValue(value); break;
    }
}

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
        u8 p = proc_state->P->GetValue();
        ImGui::Text(" ");
        ImGui::SameLine(0, 0); ImGui::TextColored(orange, "N");
        ImGui::SameLine(); ImGui::TextColored(orange, "V");
        ImGui::SameLine(); ImGui::TextColored(orange, "T");
        ImGui::SameLine(); ImGui::TextColored(orange, "B");
        ImGui::SameLine(); ImGui::TextColored(orange, "D");
        ImGui::SameLine(); ImGui::TextColored(orange, "I");
        ImGui::SameLine(); ImGui::TextColored(orange, "Z");
        ImGui::SameLine(); ImGui::TextColored(orange, "C");
        ImGui::Text(" ");
        ImGui::SameLine(0, 0);
        EditableRegister1(HuC6280RegId_P, 7, (p >> 7) & 1, HuC6280WriteCallback1, core);
        ImGui::SameLine(); EditableRegister1(HuC6280RegId_P, 6, (p >> 6) & 1, HuC6280WriteCallback1, core);
        ImGui::SameLine(); EditableRegister1(HuC6280RegId_P, 5, (p >> 5) & 1, HuC6280WriteCallback1, core);
        ImGui::SameLine(); EditableRegister1(HuC6280RegId_P, 4, (p >> 4) & 1, HuC6280WriteCallback1, core);
        ImGui::SameLine(); EditableRegister1(HuC6280RegId_P, 3, (p >> 3) & 1, HuC6280WriteCallback1, core);
        ImGui::SameLine(); EditableRegister1(HuC6280RegId_P, 2, (p >> 2) & 1, HuC6280WriteCallback1, core);
        ImGui::SameLine(); EditableRegister1(HuC6280RegId_P, 1, (p >> 1) & 1, HuC6280WriteCallback1, core);
        ImGui::SameLine(); EditableRegister1(HuC6280RegId_P, 0, p & 1, HuC6280WriteCallback1, core);

        ImGui::TableNextColumn();
        ImGui::TextColored(yellow, "    PC"); ImGui::SameLine();
        ImGui::Text(" "); ImGui::SameLine(0, 0);
        EditableRegister16(NULL, NULL, HuC6280RegId_PC, proc_state->PC->GetValue(), HuC6280WriteCallback16, core, EditableRegisterFlags_None);
        ImGui::TextColored(gray, BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->PC->GetHigh()), BYTE_TO_BINARY(proc_state->PC->GetLow()));

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
        ImGui::TextColored(gray, BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(0x21), BYTE_TO_BINARY(proc_state->S->GetValue()));
        if (ImGui::IsItemClicked())
            gui_debug_memory_goto(MEMORY_EDITOR_RAM, (STACK_ADDR - 0x2000) | proc_state->S->GetValue());

        ImGui::TableNextColumn();

        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(2.0f, 2.0f));

        if (ImGui::BeginTable("regs", 2, ImGuiTableFlags_BordersInnerH |ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoPadOuterX))
        {
            ImGui::TableNextColumn();
            ImGui::TextColored(cyan, " A"); ImGui::SameLine();
            ImGui::Text("  "); ImGui::SameLine(0, 0);
            EditableRegister8(NULL, NULL, HuC6280RegId_A, proc_state->A->GetValue(), HuC6280WriteCallback8, core, EditableRegisterFlags_None);
            ImGui::TextColored(gray, BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->A->GetValue()));

            ImGui::TableNextColumn();
            ImGui::TextColored(cyan, " S"); ImGui::SameLine();
            ImGui::Text("  "); ImGui::SameLine(0, 0);
            EditableRegister8(NULL, NULL, HuC6280RegId_S, proc_state->S->GetValue(), HuC6280WriteCallback8, core, EditableRegisterFlags_None);
            ImGui::TextColored(gray, BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->S->GetValue()));

            ImGui::TableNextColumn();
            ImGui::TextColored(cyan, " X"); ImGui::SameLine();
            ImGui::Text("  "); ImGui::SameLine(0, 0);
            EditableRegister8(NULL, NULL, HuC6280RegId_X, proc_state->X->GetValue(), HuC6280WriteCallback8, core, EditableRegisterFlags_None);
            ImGui::TextColored(gray, BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->X->GetValue()));

            ImGui::TableNextColumn();
            ImGui::TextColored(cyan, " Y"); ImGui::SameLine();
            ImGui::Text("  "); ImGui::SameLine(0, 0);
            EditableRegister8(NULL, NULL, HuC6280RegId_Y, proc_state->Y->GetValue(), HuC6280WriteCallback8, core, EditableRegisterFlags_None);
            ImGui::TextColored(gray, BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->Y->GetValue()));

            for (u8 i = 0; i < 8; i++)
            {
                char label[8];
                snprintf(label, sizeof(label), "MPR%d", i);
                ImGui::TableNextColumn();
                ImGui::TextColored(violet, "%s", label); ImGui::SameLine();
                if (ImGui::IsItemClicked())
                    goto_address(memory->GetMpr(i));
                ImGui::Text(""); ImGui::SameLine(0, 0);
                EditableRegister8(NULL, NULL, HuC6280RegId_MPR0 + i, memory->GetMpr(i), HuC6280WriteCallback8, core, EditableRegisterFlags_None);
                if (ImGui::IsItemClicked())
                    goto_address(memory->GetMpr(i));
                ImGui::TextColored(gray, BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(memory->GetMpr(i)));
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
            EditableRegister8(NULL, NULL, HuC6280RegId_IO, input->GetIORegister(), HuC6280WriteCallback8, core, EditableRegisterFlags_None);
            ImGui::TextColored(gray, BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(input->GetIORegister()));

            ImGui::TableNextColumn();
            u8 tim = (*proc_state->TIMER) ? 0x01 : 0x00;
            ImGui::TextColored(blue, "TIM "); ImGui::SameLine();
            EditableRegister8(NULL, NULL, HuC6280RegId_TIM, tim, HuC6280WriteCallback8, core, EditableRegisterFlags_None);
            ImGui::TextColored(gray, BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(tim));

            ImGui::TableNextColumn();
            u8 timc = processor->ReadTimerRegister();
            ImGui::TextColored(blue, "TIMC"); ImGui::SameLine();
            EditableRegister8(NULL, NULL, HuC6280RegId_TIMC, timc, HuC6280WriteCallback8, core, EditableRegisterFlags_None);
            ImGui::TextColored(gray, BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(timc));

            ImGui::TableNextColumn();
            ImGui::TextColored(blue, "TIMR"); ImGui::SameLine();
            EditableRegister8(NULL, NULL, HuC6280RegId_TIMR, *proc_state->TIMER_RELOAD, HuC6280WriteCallback8, core, EditableRegisterFlags_None);
            ImGui::TextColored(gray, BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(*proc_state->TIMER_RELOAD));

            ImGui::TableNextColumn();
            ImGui::TextColored(magenta, "IDR "); ImGui::SameLine();
            EditableRegister8(NULL, NULL, HuC6280RegId_IDR, *proc_state->IDR, HuC6280WriteCallback8, core, EditableRegisterFlags_None);
            ImGui::TextColored(gray, BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(*proc_state->IDR));

            ImGui::TableNextColumn();
            ImGui::TextColored(magenta, "IRR "); ImGui::SameLine();
            EditableRegister8(NULL, NULL, HuC6280RegId_IRR, *proc_state->IRR, HuC6280WriteCallback8, core, EditableRegisterFlags_None);
            ImGui::TextColored(gray, BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(*proc_state->IRR));

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
