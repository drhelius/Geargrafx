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

#include "imgui/imgui.h"
#include "../../../src/geargrafx.h"
#include "gui_debug_constants.h"
#include "gui.h"
#include "config.h"
#include "emu.h"
#include "utils.h"

static char mpr_name[16] = { };
static char mpr_tooltip[128] = { };

static void get_bank_name(u8 mpr, u8 mpr_value, char *name, char* tooltip);

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
        ImGui::TextColored(orange, "  N V T B D I Z C");
        ImGui::Text("  " BYTE_TO_BINARY_PATTERN_ALL_SPACED, BYTE_TO_BINARY(proc_state->P->GetValue()));

        ImGui::TableNextColumn();
        ImGui::TextColored(yellow, "    PC"); ImGui::SameLine();
        ImGui::Text("= $%04X", proc_state->PC->GetValue());
        ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->PC->GetHigh()), BYTE_TO_BINARY(proc_state->PC->GetLow()));

        ImGui::TableNextColumn();
        ImGui::TextColored(yellow, " PHYS PC"); ImGui::SameLine();
        ImGui::Text("= $%06X", memory->GetPhysicalAddress(proc_state->PC->GetValue()));

        ImGui::TableNextColumn();
        ImGui::TextColored(yellow, "    SP"); ImGui::SameLine();
        ImGui::Text("= $%04X", STACK_ADDR | proc_state->S->GetValue());
        ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(0x21), BYTE_TO_BINARY(proc_state->S->GetValue()));

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

            ImGui::TableNextColumn();
            ImGui::TextColored(violet, "MPR0"); ImGui::SameLine();
            ImGui::Text(" $%02X", memory->GetMpr(0));
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(memory->GetMpr(0)));
            get_bank_name(0, memory->GetMpr(0), mpr_name, mpr_tooltip);
            ImGui::TextColored(gray, " %s", mpr_name);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                ImGui::SetTooltip("%s", mpr_tooltip);

            ImGui::TableNextColumn();
            ImGui::TextColored(violet, "MPR1"); ImGui::SameLine();
            ImGui::Text(" $%02X", memory->GetMpr(1));
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(memory->GetMpr(1)));
            get_bank_name(1, memory->GetMpr(1), mpr_name, mpr_tooltip);
            ImGui::TextColored(gray, " %s", mpr_name);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                ImGui::SetTooltip("%s", mpr_tooltip);
    
            ImGui::TableNextColumn();
            ImGui::TextColored(violet, "MPR2"); ImGui::SameLine();
            ImGui::Text(" $%02X", memory->GetMpr(2));
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(memory->GetMpr(2)));
            get_bank_name(2, memory->GetMpr(2), mpr_name, mpr_tooltip);
            ImGui::TextColored(gray, " %s", mpr_name);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                ImGui::SetTooltip("%s", mpr_tooltip);

            ImGui::TableNextColumn();
            ImGui::TextColored(violet, "MPR3"); ImGui::SameLine();
            ImGui::Text(" $%02X", memory->GetMpr(3));
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(memory->GetMpr(3)));
            get_bank_name(3, memory->GetMpr(3), mpr_name, mpr_tooltip);
            ImGui::TextColored(gray, " %s", mpr_name);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                ImGui::SetTooltip("%s", mpr_tooltip);

            ImGui::TableNextColumn();
            ImGui::TextColored(violet, "MPR4"); ImGui::SameLine();
            ImGui::Text(" $%02X", memory->GetMpr(4));
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(memory->GetMpr(4)));
            get_bank_name(4, memory->GetMpr(4), mpr_name, mpr_tooltip);
            ImGui::TextColored(gray, " %s", mpr_name);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                ImGui::SetTooltip("%s", mpr_tooltip);

            ImGui::TableNextColumn();
            ImGui::TextColored(violet, "MPR5"); ImGui::SameLine();
            ImGui::Text(" $%02X", memory->GetMpr(5));
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(memory->GetMpr(5)));
            get_bank_name(5, memory->GetMpr(5), mpr_name, mpr_tooltip);
            ImGui::TextColored(gray, " %s", mpr_name);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                ImGui::SetTooltip("%s", mpr_tooltip);

            ImGui::TableNextColumn();
            ImGui::TextColored(violet, "MPR6"); ImGui::SameLine();
            ImGui::Text(" $%02X", memory->GetMpr(6));
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(memory->GetMpr(6)));
            get_bank_name(6, memory->GetMpr(6), mpr_name, mpr_tooltip);
            ImGui::TextColored(gray, " %s", mpr_name);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                ImGui::SetTooltip("%s", mpr_tooltip);

            ImGui::TableNextColumn();
            ImGui::TextColored(violet, "MPR7"); ImGui::SameLine();
            ImGui::Text(" $%02X", memory->GetMpr(7));
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(memory->GetMpr(7)));
            get_bank_name(7, memory->GetMpr(7), mpr_name, mpr_tooltip);
            ImGui::TextColored(gray, " %s", mpr_name);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                ImGui::SetTooltip("%s", mpr_tooltip);

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
        ImGui::TextColored(magenta, " SPEED:"); ImGui::SameLine();
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

    // 0x00 - 0x7F
    if (mpr_value < 0x80)
    {
        u32 rom_address = mpr_value << 13;
        snprintf(name, 16, "ROM $%02X", mpr_value);
        snprintf(tooltip, 128, "Range (CPU) $%04X-$%04X \nRange (ROM) $%06X-$%06X",
            cpu_address, cpu_address + 0x1FFF,  rom_address,  rom_address + 0x1FFF);
    }
    // 0x80 - 0xF6
    else if (mpr_value < 0xF7)
    {
        snprintf(name, 16, "UNUSED");
        snprintf(tooltip, 128, "Range (CPU) $%04X-$%04X", cpu_address, cpu_address + 0x1FFF);
    }
    // 0xF7
    else if (mpr_value < 0xF8)
    {
        snprintf(name, 16, "BRAM");
        snprintf(tooltip, 128, "Range (CPU) $%04X-$%04X", cpu_address, cpu_address + 0x1FFF);
    }
    // 0xF8 - 0xFB
    else if (mpr_value < 0xFC)
    {
        u8 ram_bank = mpr_value - 0xF8;
        u16 ram_address = ram_bank << 13;
        snprintf(name, 16, "WRAM $%02X", ram_bank);
        snprintf(tooltip, 128, "Range (CPU) $%04X-$%04X \nRange (WRAM) $%04X-$%04X",
            cpu_address, cpu_address + 0x1FFF,  ram_address,  ram_address + 0x1FFF);
    }
    // 0xFC - 0xFE
    else if (mpr_value < 0xFF)
    {
        snprintf(name, 16, "UNUSED");
        snprintf(tooltip, 128, "Range (CPU) $%04X-$%04X", cpu_address, cpu_address + 0x1FFF);
    }
    // 0xFF
    else
    {
        snprintf(name, 16, "HARDWARE");
        snprintf(tooltip, 128, "Range (CPU) $%04X-$%04X", cpu_address, cpu_address + 0x1FFF);
    }
}
