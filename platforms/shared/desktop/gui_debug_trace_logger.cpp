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

#include <deque>
#include "imgui/imgui.h"
#include "gui.h"
#include "config.h"
#include "emu.h"

static bool trace_logger_enabled = false;
static int trace_logger_count = 10000;
static std::deque<std::string> trace_logger_lines;

static void trace_logger_menu(void);

void gui_debug_window_trace_logger(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(625, 321), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(343, 262), ImGuiCond_FirstUseEver);

    ImGui::Begin("Trace Logger", &config_debug.show_trace_logger, ImGuiWindowFlags_MenuBar);

    trace_logger_menu();

    ImGui::Text("Log last: ");

    ImGui::SameLine();

    if (ImGui::InputInt("lines", &trace_logger_count, 1, 1000, ImGuiInputTextFlags_AllowTabInput))
    {
        if (trace_logger_count < 1)
            trace_logger_count = 1;
        else if (trace_logger_count > 100000)
            trace_logger_count = 100000;

        if ((int)trace_logger_lines.size() > trace_logger_count)
        {
            int diff = trace_logger_lines.size() - trace_logger_count;
            trace_logger_lines.erase(trace_logger_lines.begin(), trace_logger_lines.begin() + diff);
        }
    }

    ImGui::SameLine();

    if (ImGui::Button(trace_logger_enabled ? "Stop" : "Start"))
    {
        trace_logger_enabled = !trace_logger_enabled;
    }

    if (ImGui::BeginChild("##logger", ImVec2(ImGui::GetContentRegionAvail().x, 0), true, 0))
    {
        ImGui::PushFont(gui_default_font);

        ImGuiListClipper clipper;
        clipper.Begin(trace_logger_lines.size(), ImGui::GetTextLineHeightWithSpacing());

        while (clipper.Step())
        {
            for (int item = clipper.DisplayStart; item < clipper.DisplayEnd; item++)
            {
                ImGui::Text("%s", trace_logger_lines[item].c_str());
            }
        }

        ImGui::PopFont();

        ImGui::EndChild();
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

void gui_debug_trace_logger_update(GeargrafxCore::GG_Debug_State* state)
{
    if (trace_logger_enabled)
    {
        if ((int)trace_logger_lines.size() >= trace_logger_count)
        {
            trace_logger_lines.pop_front();
        }

        Memory* memory = emu_get_core()->GetMemory();
        Memory::GG_Disassembler_Record* record = memory->GetDisassemblerRecord(state->PC);

        if (!IsValidPointer(record))
            return;

        char bank[32];
        snprintf(bank, sizeof(bank), "%02X", record->bank);

        char registers[40];
        snprintf(registers, sizeof(registers), "A: %02X  X: %02X  Y: %02X  S: %02X",
            state->A, state->X, state->Y, state->S);

        char flags[32];
        snprintf(flags, sizeof(flags), "P: %c%c%c%c%c%c%c%c",
            (state->P & FLAG_NEGATIVE) ? 'N' : 'n',
            (state->P & FLAG_OVERFLOW) ? 'V' : 'v',
            (state->P & FLAG_TRANSFER) ? 'T' : 't',
            (state->P & FLAG_BREAK) ? 'B' : 'b',
            (state->P & FLAG_DECIMAL) ? 'D' : 'd',
            (state->P & FLAG_INTERRUPT) ? 'I' : 'i',
            (state->P & FLAG_ZERO) ? 'Z' : 'z',
            (state->P & FLAG_CARRY) ? 'C' : 'c');

        std::string instr = record->name;
        instr.erase(std::remove(instr.begin(), instr.end(), '{'), instr.end());
        instr.erase(std::remove(instr.begin(), instr.end(), '}'), instr.end());

        char line[256];
        snprintf(line, sizeof(line), "%s:%04X   %s   %s   %s   %s", bank, state->PC, registers, flags, instr.c_str(), record->bytes);

        trace_logger_lines.push_back(line);
    }
}

static void trace_logger_menu(void)
{
    ImGui::BeginMenuBar();

    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Save Log As..."))
        {
            
        }

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Log"))
    {
        ImGui::MenuItem("Bank Number", "", &config_debug.trace_bank, config_debug.debug);
        ImGui::MenuItem("Registers", "", &config_debug.trace_registers, config_debug.debug);
        ImGui::MenuItem("Flags", "", &config_debug.trace_flags, config_debug.debug);
        ImGui::MenuItem("Cycles", "", &config_debug.trace_bank, config_debug.debug);

        ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
}
