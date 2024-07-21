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
#include "imgui/colors.h"
#include "../../../src/geargrafx.h"
#include "gui.h"
#include "config.h"
#include "emu.h"
#include "utils.h"

void gui_debug_window_huc6280(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(6, 31), ImGuiCond_FirstUseEver);

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
        ImGui::Text("= $%04X", 0x2100 | proc_state->S->GetValue());
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

            ImGui::TableNextColumn();
            ImGui::TextColored(violet, "MPR1"); ImGui::SameLine();
            ImGui::Text(" $%02X", memory->GetMpr(1));
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(memory->GetMpr(1)));
    
            ImGui::TableNextColumn();
            ImGui::TextColored(violet, "MPR2"); ImGui::SameLine();
            ImGui::Text(" $%02X", memory->GetMpr(2));
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(memory->GetMpr(2)));

            ImGui::TableNextColumn();
            ImGui::TextColored(violet, "MPR3"); ImGui::SameLine();
            ImGui::Text(" $%02X", memory->GetMpr(3));
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(memory->GetMpr(3)));

            ImGui::TableNextColumn();
            ImGui::TextColored(violet, "MPR4"); ImGui::SameLine();
            ImGui::Text(" $%02X", memory->GetMpr(4));
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(memory->GetMpr(4)));

            ImGui::TableNextColumn();
            ImGui::TextColored(violet, "MPR5"); ImGui::SameLine();
            ImGui::Text(" $%02X", memory->GetMpr(5));
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(memory->GetMpr(5)));

            ImGui::TableNextColumn();
            ImGui::TextColored(violet, "MPR6"); ImGui::SameLine();
            ImGui::Text(" $%02X", memory->GetMpr(6));
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(memory->GetMpr(6)));

            ImGui::TableNextColumn();
            ImGui::TextColored(violet, "MPR7"); ImGui::SameLine();
            ImGui::Text(" $%02X", memory->GetMpr(7));
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(memory->GetMpr(7)));

            ImGui::TableNextColumn();
            ImGui::TextColored(red, "I/O "); ImGui::SameLine();
            ImGui::Text(" $%02X", input->GetIORegister());
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(input->GetIORegister()));

            ImGui::TableNextColumn();
            ImGui::TextColored(blue, "TIM  "); ImGui::SameLine();
            ImGui::Text("$%02X", *proc_state->TIMER ? 1 : 0);
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(*proc_state->TIMER ? 1 : 0));

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
        ImGui::TextColored(*proc_state->NMI ? green : gray, " NMI"); ImGui::SameLine();
        ImGui::TextColored(*proc_state->TIMER_IRQ ? green : gray, "TIQ"); ImGui::SameLine();
        ImGui::TextColored(*proc_state->IRQ1 ? green : gray, "IRQ1"); ImGui::SameLine();
        ImGui::TextColored(*proc_state->IRQ2 ? green : gray, "IRQ2"); 

        ImGui::TableNextColumn();
        ImGui::TextColored(!*proc_state->SPEED ? green : gray, " 1.79 MHz"); ImGui::SameLine();
        ImGui::TextColored(*proc_state->SPEED ? green : gray, "7.16 MHz");

        // ImGui::TextColored(*proc_state->IFF1 ? green : gray, " IFF1"); ImGui::SameLine();
        // ImGui::TextColored(*proc_state->IFF2 ? green : gray, " IFF2"); ImGui::SameLine();
        // ImGui::TextColored(*proc_state->Halt ? green : gray, " HALT");

        // ImGui::TextColored(*proc_state->INT ? green : gray, "    INT"); ImGui::SameLine();
        // ImGui::TextColored(*proc_state->NMI ? green : gray, "  NMI");

        ImGui::EndTable();
    }

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}
