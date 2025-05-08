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

#define GUI_DEBUG_HUC6202_IMPORT
#include "gui_debug_huc6202.h"

#include "imgui/imgui.h"
#include "../../../src/geargrafx.h"
#include "gui_debug_constants.h"
#include "gui.h"
#include "config.h"
#include "emu.h"
#include "utils.h"

void gui_debug_window_huc6202_info(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(75, 410), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(280, 220), ImGuiCond_FirstUseEver);
    ImGui::Begin("HuC6202 Info", &config_debug.show_huc6202_info);

    ImGui::PushFont(gui_default_font);

    GeargrafxCore* core = emu_get_core();
    HuC6202* huc6202 = core->GetHuC6202();
    HuC6202::HuC6202_State* huc6202_state = huc6202->GetState();

    ImGui::TextColored(violet, "SELECTED VDC "); ImGui::SameLine();
    ImGui::TextColored(yellow, "%d", *huc6202_state->VDC2_SELECTED ? 2 : 1);

    ImGui::TextColored(violet, "WINDOW 1     "); ImGui::SameLine();
    ImGui::TextColored(white, "$%03X (%d)", *huc6202_state->WINDOW_1, *huc6202_state->WINDOW_1);
    ImGui::TextColored(violet, "WINDOW 2     "); ImGui::SameLine();
    ImGui::TextColored(white, "$%03X (%d)", *huc6202_state->WINDOW_2, *huc6202_state->WINDOW_2);

    ImGui::TextColored(violet, "PRIORITY 1   "); ImGui::SameLine();
    ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", *huc6202_state->PRIORITY_1, BYTE_TO_BINARY(*huc6202_state->PRIORITY_1));
    ImGui::TextColored(violet, "PRIORITY 2   "); ImGui::SameLine();
    ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", *huc6202_state->PRIORITY_2, BYTE_TO_BINARY(*huc6202_state->PRIORITY_2));

    ImGui::TextColored(violet, "IRQ VDC 1    "); ImGui::SameLine();
    ImGui::TextColored(*huc6202_state->IRQ1_1 ? green : gray, "%s", "ASSERTED");
    ImGui::TextColored(violet, "IRQ VDC 2    "); ImGui::SameLine();
    ImGui::TextColored(*huc6202_state->IRQ1_2 ? green : gray, "%s", "ASSERTED");

    ImGui::NewLine(); ImGui::TextColored(cyan, "WINDOW REGIONS"); ImGui::Separator();

    const char* window_names[] = { "NONE", "WINDOW 1", "WINDOW 2", "BOTH" };
    const char* priority_modes[] = { "DEFAULT", "SPR2 ABOVE BG1", "SPR1 BEHIND BG2" };

    for (int i = 0; i < 4; i++) {
        ImGui::TextColored(magenta, "%s:", window_names[i]);

        ImGui::TextColored(violet, " VDC 1"); ImGui::SameLine();
        ImGui::TextColored(huc6202_state->WINDOW_PRIORITY[i].vdc_1_enabled ? green : gray, "%s", huc6202_state->WINDOW_PRIORITY[i].vdc_1_enabled ? "ON " : "OFF"); ImGui::SameLine();

        ImGui::TextColored(violet, "VDC 2"); ImGui::SameLine();
        ImGui::TextColored(huc6202_state->WINDOW_PRIORITY[i].vdc_2_enabled ? green : gray, "%s", huc6202_state->WINDOW_PRIORITY[i].vdc_2_enabled ? "ON " : "OFF");  ImGui::SameLine();

        ImGui::TextColored(violet, "MODE"); ImGui::SameLine();
        ImGui::TextColored(white, "%s", priority_modes[huc6202_state->WINDOW_PRIORITY[i].priority_mode]);
    }

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}