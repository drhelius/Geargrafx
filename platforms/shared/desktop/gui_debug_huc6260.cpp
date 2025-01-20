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
#include "../../../src/geargrafx.h"
#include "gui_debug_constants.h"
#include "gui.h"
#include "config.h"
#include "emu.h"
#include "utils.h"

static ImVec4 color_333_to_float(u16 color);

void gui_debug_window_huc6260_info(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(75, 228), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(280, 174), ImGuiCond_FirstUseEver);
    ImGui::Begin("HuC6260 Info", &config_debug.show_huc6260_info);

    ImGui::PushFont(gui_default_font);

    GeargrafxCore* core = emu_get_core();
    HuC6260* huc6260 = core->GetHuC6260();
    HuC6260::HuC6260_State* huc6260_state = huc6260->GetState();

    ImGui::TextColored(magenta, "SPEED    "); ImGui::SameLine();
    const char* speed[] = { "5.36 MHz", "7.16 MHz", "10.8 MHz" };
    ImGui::TextColored(orange, "%s", speed[huc6260->GetSpeed()]);

    ImGui::TextColored(magenta, "SIGNALS  "); ImGui::SameLine();
    ImGui::TextColored(*huc6260_state->HSYNC ? gray : green, "HSYNC"); ImGui::SameLine();
    ImGui::TextColored(*huc6260_state->VSYNC ? gray : green, "VSYNC");

    int vpos = *huc6260_state->VPOS;// - 259;
    // if (vpos < 0)
    //     vpos += 263;

    ImGui::TextColored(magenta, "HPOS,VPOS"); ImGui::SameLine();
    ImGui::TextColored(white, "%03X,%03X (%03d,%03d)", *huc6260_state->HPOS, vpos, *huc6260_state->HPOS, vpos);

    ImGui::TextColored(magenta, "PIXEL    "); ImGui::SameLine();
    ImGui::TextColored(white, "%0X", *huc6260_state->PIXEL_INDEX);

    ImGui::TextColored(magenta, "CTRL REG "); ImGui::SameLine();
    ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", *huc6260_state->CR, BYTE_TO_BINARY(*huc6260_state->CR));

    ImGui::TextColored(magenta, "CTA      "); ImGui::SameLine();
    ImGui::Text("$%04X (" BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED ")", *huc6260_state->CTA, BYTE_TO_BINARY(*huc6260_state->CTA >> 8), BYTE_TO_BINARY(*huc6260_state->CTA & 0xFF));

    ImGui::TextColored(magenta, "BLUR     "); ImGui::SameLine();
    ImGui::TextColored(IsSetBit(*huc6260_state->CR, 2) ? green : gray, "%s", IsSetBit(*huc6260_state->CR, 2) ? "ON" : "OFF");

    ImGui::TextColored(magenta, "B&W      "); ImGui::SameLine();
    ImGui::TextColored(IsSetBit(*huc6260_state->CR, 7) ? green : gray, "%s", IsSetBit(*huc6260_state->CR, 7) ? "ON" : "OFF");

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}

void gui_debug_window_huc6260_palettes(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(59, 70), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(526, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin("HuC6260 Palettes", &config_debug.show_huc6260_palettes);

    GeargrafxCore* core = emu_get_core();
    HuC6260* huc6260 = core->GetHuC6260();
    u16* color_table = huc6260->GetColorTable();

    if (ImGui::BeginTabBar("##palette_tabs", ImGuiTabBarFlags_None))
    {
        if (ImGui::BeginTabItem("Background", NULL, ImGuiTabItemFlags_None))
        {
            ImGui::BeginChild("background_palettes", ImVec2(0, 0.0f), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);
            ImGui::PushFont(gui_default_font);

            ImGui::NewLine();

            for (int row = 0; row < 16; row++)
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
                    snprintf(id, 16, "##bg_pal_%d_%d", row, col);
                    ImGui::ColorEdit3(id, (float*)&float_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoTooltip);
                    if (col != 15)
                        ImGui::SameLine(0, 10);
                }

                ImGui::Text("     "); ImGui::SameLine(0,0);

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

                ImGui::NewLine();
            }

            ImGui::NewLine();

            ImGui::PopFont();
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Sprites", NULL, ImGuiTabItemFlags_None))
        {
            ImGui::BeginChild("sprite_palettes", ImVec2(0, 0.0f), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);
            ImGui::PushFont(gui_default_font);

            ImGui::NewLine();

            for (int row = 16; row < 32; row++)
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
                    snprintf(id, 16, "##spr_pal_%d_%d", row, col);
                    ImGui::ColorEdit3(id, (float*)&float_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoTooltip);
                    if (col != 15)
                        ImGui::SameLine(0, 10);
                }

                ImGui::Text("     "); ImGui::SameLine(0,0);

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

                ImGui::NewLine();
            }

            ImGui::NewLine();

            ImGui::PopFont();
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

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