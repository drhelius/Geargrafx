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
    HuC6280::Processor_State* proc_state = processor->GetState();
    Memory* memory = core->GetMemory();

    ImGui::TextColored(cyan, "      STATUS");
    ImGui::TextColored(orange, "  N V T B D I Z C");
    ImGui::Text("  " BYTE_TO_BINARY_PATTERN_ALL_SPACED, BYTE_TO_BINARY(proc_state->P->GetValue()));

    ImGui::Columns(2, "registers");
    ImGui::Separator();
    ImGui::TextColored(cyan, "  A"); ImGui::SameLine();
    ImGui::Text(" $%02X", proc_state->A->GetValue());
    ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->A->GetValue()));

    ImGui::NextColumn();
    ImGui::TextColored(cyan, "  S"); ImGui::SameLine();
    ImGui::Text(" $%02X", proc_state->S->GetValue());
    ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->S->GetValue()));

    ImGui::NextColumn();
    ImGui::Separator();
    ImGui::TextColored(cyan, "  X"); ImGui::SameLine();
    ImGui::Text(" $%02X", proc_state->X->GetValue());
    ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->X->GetValue()));

    ImGui::NextColumn();
    ImGui::TextColored(cyan, "  Y"); ImGui::SameLine();
    ImGui::Text(" $%02X", proc_state->Y->GetValue());
    ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->Y->GetValue()));

    ImGui::NextColumn();
    ImGui::Separator();
    ImGui::TextColored(violet, "MPR0"); ImGui::SameLine();
    ImGui::Text("$%02X", memory->GetMpr(0));
    ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(memory->GetMpr(0)));

    ImGui::NextColumn();
    ImGui::TextColored(violet, "MPR1"); ImGui::SameLine();
    ImGui::Text("$%02X", memory->GetMpr(1));
    ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(memory->GetMpr(1)));

    ImGui::NextColumn();
    ImGui::Separator();
    ImGui::TextColored(violet, "MPR2"); ImGui::SameLine();
    ImGui::Text("$%02X", memory->GetMpr(2));
    ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(memory->GetMpr(2)));

    ImGui::NextColumn();
    ImGui::TextColored(violet, "MPR3"); ImGui::SameLine();
    ImGui::Text("$%02X", memory->GetMpr(3));
    ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(memory->GetMpr(3)));

    ImGui::NextColumn();
    ImGui::Separator();
    ImGui::TextColored(violet, "MPR4"); ImGui::SameLine();
    ImGui::Text("$%02X", memory->GetMpr(4));
    ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(memory->GetMpr(4)));

    ImGui::NextColumn();
    ImGui::TextColored(violet, "MPR5"); ImGui::SameLine();
    ImGui::Text("$%02X", memory->GetMpr(5));
    ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(memory->GetMpr(5)));

    ImGui::NextColumn();
    ImGui::Separator();
    ImGui::TextColored(violet, "MPR6"); ImGui::SameLine();
    ImGui::Text("$%02X", memory->GetMpr(6));
    ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(memory->GetMpr(6)));

    ImGui::NextColumn();
    ImGui::TextColored(violet, "MPR7"); ImGui::SameLine();
    ImGui::Text("$%02X", memory->GetMpr(7));
    ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(memory->GetMpr(7)));

    ImGui::NextColumn();
    ImGui::Columns(1);

    ImGui::Separator();
    ImGui::TextColored(yellow, "    SP"); ImGui::SameLine();
    ImGui::Text("= $%04X", 0x2100 | proc_state->S->GetValue());
    ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(0x21), BYTE_TO_BINARY(proc_state->S->GetValue()));

    ImGui::Separator();
    ImGui::TextColored(yellow, "    PC"); ImGui::SameLine();
    ImGui::Text("= $%04X", proc_state->PC->GetValue());
    ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->PC->GetHigh()), BYTE_TO_BINARY(proc_state->PC->GetLow()));

    ImGui::Separator();
    ImGui::TextColored(yellow, " PHYS PC"); ImGui::SameLine();
    ImGui::Text("= $%06X", memory->GetPhysicalAddress(proc_state->PC->GetValue()));

    ImGui::Separator();

    ImGui::TextColored(!*proc_state->SPEED ? green : gray, " 1.79 MHz"); ImGui::SameLine();
    ImGui::TextColored(*proc_state->SPEED ? green : gray, "7.16 MHz");

    // ImGui::TextColored(*proc_state->IFF1 ? green : gray, " IFF1"); ImGui::SameLine();
    // ImGui::TextColored(*proc_state->IFF2 ? green : gray, " IFF2"); ImGui::SameLine();
    // ImGui::TextColored(*proc_state->Halt ? green : gray, " HALT");
    
    // ImGui::TextColored(*proc_state->INT ? green : gray, "    INT"); ImGui::SameLine();
    // ImGui::TextColored(*proc_state->NMI ? green : gray, "  NMI");

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}

#include "../../../src/geargrafx.h"