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

#define GUI_DEBUG_HUC6260_IMPORT
#include "gui_debug_huc6260.h"

#include "imgui/imgui.h"
#include "imgui/colors.h"
#include "../../../src/geargrafx.h"
#include "gui.h"
#include "config.h"
#include "emu.h"

static ImVec4 color_333_to_float(u16 color);

void gui_debug_window_huc6260(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(6, 31), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(401, 641), ImGuiCond_FirstUseEver);
    ImGui::Begin("HuC6260", &config_debug.show_huc6260);

    ImGui::PushFont(gui_default_font);

    GeargrafxCore* core = emu_get_core();
    HuC6260* huc6260 = core->GetHuC6260();
    HuC6260::HuC6260_State* huc6260_state = huc6260->GetState();
    u16* color_table = huc6260->GetColorTable();

    ImGui::TextColored(magenta, " CR    "); ImGui::SameLine();
    ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", *huc6260_state->CR, BYTE_TO_BINARY(*huc6260_state->CR));

    ImGui::TextColored(magenta, " CTA   "); ImGui::SameLine();
    ImGui::Text("$%04X (" BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED ")", *huc6260_state->CTA, BYTE_TO_BINARY(*huc6260_state->CTA >> 8), BYTE_TO_BINARY(*huc6260_state->CTA & 0xFF));

    ImGui::TextColored(magenta, " B&W   "); ImGui::SameLine();
    ImGui::TextColored(IsSetBit(*huc6260_state->CR, 7) ? green : gray, "%s", IsSetBit(*huc6260_state->CR, 7) ? "ON" : "OFF");

    ImGui::TextColored(magenta, " BLUR  "); ImGui::SameLine();
    ImGui::TextColored(IsSetBit(*huc6260_state->CR, 2) ? green : gray, "%s", IsSetBit(*huc6260_state->CR, 2) ? "ON" : "OFF");

    ImGui::TextColored(magenta, " SPEED "); ImGui::SameLine();
    const char* speed[] = { "10.8 MHz", "7.16 MHz", "5.36 MHz" };
    ImGui::TextColored(green, "%s", speed[huc6260->GetSpeed()]);

    ImGui::NewLine();

    ImGui::PushStyleColor(ImGuiCol_Text, cyan);
    ImGui::SeparatorText("BACKGROUND PALETTE");
    ImGui::PopStyleColor();

    ImGui::NewLine(); ImGui::Text(" "); ImGui::SameLine(0,0);

    for (int row = 0; row < 32; row++)
    {
        for (int col = 0; col < 16; col++)
        {
            int i = row * 16 + col;
            if (col == 0)
            {
                ImGui::TextColored(white, "%03X:", i); ImGui::SameLine();
            }

            u16 color = color_table[i];
            ImVec4 float_color = color_333_to_float(color);
            char id[16];
            sprintf(id, "##pal_%d_%d", row, col);
            ImGui::ColorEdit3(id, (float*)&float_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoTooltip);
            if (col != 15)
                ImGui::SameLine(0, 10);
        }

        ImGui::Text("      "); ImGui::SameLine(0,0);

        for (int col = 0; col < 16; col++)
        {
            u16 color = color_table[row * 16 + col];
            u8 color_green = (color >> 6) & 0x07;
            u8 color_red = (color >> 3) & 0x07;
            u8 color_blue = color & 0x07;

            ImGui::TextColored(green, "%01X", color_green); ImGui::SameLine(0,0);
            ImGui::TextColored(red, "%01X", color_red); ImGui::SameLine(0,0);
            ImGui::TextColored(blue, "%01X", color_blue); ImGui::SameLine();
        }

        if (row == 15)
        {
            ImGui::NewLine();
            ImGui::NewLine();
            ImGui::PushStyleColor(ImGuiCol_Text, cyan);
            ImGui::SeparatorText("SPRITES PALETTE");
            ImGui::PopStyleColor();
        }

        ImGui::NewLine(); ImGui::Text(" "); ImGui::SameLine(0,0);
    }

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}

static ImVec4 color_333_to_float(u16 color)
{
    ImVec4 ret;
    ret.w = 0;
    ret.z = (1.0f / 7.0f) * (color & 0x7);
    ret.x = (1.0f / 7.0f) * ((color >> 3) & 0x7);
    ret.y = (1.0f / 7.0f) * ((color >> 6) & 0x7);
    return ret;
}