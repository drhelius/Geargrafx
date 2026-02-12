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

#define GUI_DEBUG_HUC6270_IMPORT
#include "gui_debug_huc6270.h"

#include "imgui.h"
#include "geargrafx.h"
#include "gui_debug_constants.h"
#include "gui.h"
#include "config.h"
#include "emu.h"
#include "utils.h"

void gui_debug_window_huc6270_info(int vdc)
{
    if (vdc < 1 || vdc > 2)
        return;

    GeargrafxCore* core = emu_get_core();
    HuC6260* huc6260 = core->GetHuC6260();
    HuC6270* huc6270 = vdc == 1 ? core->GetHuC6270_1() : core->GetHuC6270_2();
    HuC6270::HuC6270_State* huc6270_state = huc6270->GetState();
    bool* show = ((vdc == 1) ? &config_debug.show_huc6270_1_info : &config_debug.show_huc6270_2_info);
    char title[32];
    if (core->GetMedia()->IsSGX())
        snprintf(title, sizeof(title), "HuC6270 (%d) Info", vdc);
    else
        strncpy_fit(title, "HuC6270 Info", sizeof(title));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(214.0f + ((vdc == 1) ? 0 : 224), 45.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(216, 618), ImGuiCond_FirstUseEver);
    ImGui::Begin(title, show);

    ImGui::PushFont(gui_default_font);

    ImGui::TextColored(violet, "SPEED   "); ImGui::SameLine();
    const char* speed[] = { "5.36 MHz", "7.16 MHz", "10.8 MHz" };
    ImGui::TextColored(orange, "%s", speed[huc6260->GetSpeed()]);

    ImGui::TextColored(violet, "X,Y     "); ImGui::SameLine();
    ImGui::TextColored(white, "%03X,%03X (%03d,%03d)", *huc6270_state->HPOS, *huc6270_state->VPOS, *huc6270_state->HPOS, *huc6270_state->VPOS);

    const char* h_states[] = { "HDS", "HDW", "HDE", "HSW" };
    assert(*huc6270_state->H_STATE < 4);
    ImGui::TextColored(violet, "H STATE "); ImGui::SameLine();
    ImGui::TextColored(blue, "%s", h_states[*huc6270_state->H_STATE]);

    const char* v_states[] = { "VDS", "VDW", "VCR", "VSW" };
    assert(*huc6270_state->V_STATE < 4);
    ImGui::TextColored(violet, "V STATE "); ImGui::SameLine();
    ImGui::TextColored(blue, "%s", v_states[*huc6270_state->V_STATE]);

    ImGui::NewLine(); ImGui::TextColored(cyan, "CONTROL REGISTER"); ImGui::Separator();

    ImGui::TextColored(violet, "BACKGRND"); ImGui::SameLine();
    ImGui::TextColored(huc6270_state->R[HUC6270_REG_CR] & 0x0080 ? green : gray, "%s", huc6270_state->R[HUC6270_REG_CR] & 0x0080 ? "ON " : "OFF"); ImGui::SameLine();

    ImGui::TextColored(violet, " SPRITES"); ImGui::SameLine();
    ImGui::TextColored(huc6270_state->R[HUC6270_REG_CR] & 0x0040 ? green : gray, "%s", huc6270_state->R[HUC6270_REG_CR] & 0x0040 ? "ON" : "OFF");

    ImGui::TextColored(violet, "DISP OUT"); ImGui::SameLine();
    const char* disp_output[] = { "DISP", "~BURST", "~INTHSYNC", "INVALID" };
    u16 disp_out_value = (huc6270_state->R[HUC6270_REG_CR] >> 8) & 0x03;
    ImGui::TextColored(disp_out_value == 3 ? red : white, "%s", disp_output[disp_out_value]);

    ImGui::TextColored(violet, "R/W INC "); ImGui::SameLine();
    ImGui::TextColored(white, "%02X", k_huc6270_read_write_increment[(huc6270_state->R[HUC6270_REG_CR] >> 11) & 0x03]);

    ImGui::TextColored(violet, "INT REQ "); ImGui::SameLine();
    ImGui::TextColored(huc6270_state->R[HUC6270_REG_CR] & HUC6270_CONTROL_COLLISION ? yellow : gray, "COLL"); ImGui::SameLine();
    ImGui::TextColored(huc6270_state->R[HUC6270_REG_CR] & HUC6270_CONTROL_OVERFLOW ? yellow : gray, "OVER"); ImGui::SameLine();
    ImGui::TextColored(huc6270_state->R[HUC6270_REG_CR] & HUC6270_CONTROL_SCANLINE ? yellow : gray, "SCAN"); ImGui::SameLine();
    ImGui::TextColored(huc6270_state->R[HUC6270_REG_CR] & HUC6270_CONTROL_VBLANK ? yellow : gray, "VERT");

    ImGui::NewLine(); ImGui::TextColored(cyan, "STATUS REGISTER"); ImGui::Separator(); 

    ImGui::TextColored(violet, "INT ACT "); ImGui::SameLine();
    ImGui::TextColored(*huc6270_state->SR & HUC6270_STATUS_COLLISION ? green : gray, "COLL"); ImGui::SameLine();
    ImGui::TextColored(*huc6270_state->SR & HUC6270_STATUS_OVERFLOW ? green : gray, "OVER"); ImGui::SameLine();
    ImGui::TextColored(*huc6270_state->SR & HUC6270_STATUS_SCANLINE ? green : gray, "SCAN"); ImGui::SameLine();
    ImGui::TextColored(*huc6270_state->SR & HUC6270_STATUS_VBLANK ? green : gray, "VERT");

    ImGui::TextColored(violet, "SAT TX  "); ImGui::SameLine();
    ImGui::TextColored(*huc6270_state->SR & HUC6270_STATUS_SAT_END ? green : gray, "%s", *huc6270_state->SR & HUC6270_STATUS_SAT_END ? "YES" : "NO ");  ImGui::SameLine();

    ImGui::TextColored(violet, " VRAM TX "); ImGui::SameLine();
    ImGui::TextColored(*huc6270_state->SR & HUC6270_STATUS_VRAM_END ? green : gray, "%s", *huc6270_state->SR & HUC6270_STATUS_VRAM_END ? "YES" : "NO");

    ImGui::NewLine(); ImGui::TextColored(cyan, "DISPLAY GEOMETRY"); ImGui::Separator(); 

    ImGui::TextColored(violet, "SCREEN           "); ImGui::SameLine();
    ImGui::TextColored(white, "%dx%d", k_huc6270_screen_size_x[(huc6270_state->R[HUC6270_REG_MWR] >> 4) & 0x07], k_huc6270_screen_size_y[(huc6270_state->R[HUC6270_REG_MWR] >> 4) & 0x07]);

    ImGui::TextColored(violet, "VRAM WIDTH MODE  "); ImGui::SameLine();
    ImGui::TextColored(white, "%d", huc6270_state->R[HUC6270_REG_MWR] & 0x03);

    ImGui::TextColored(violet, "SPRITE WIDTH MODE"); ImGui::SameLine();
    ImGui::TextColored(white, "%d", (huc6270_state->R[HUC6270_REG_MWR] >> 2) & 0x03);

    ImGui::TextColored(violet, "CG MODE          "); ImGui::SameLine();
    ImGui::TextColored(white, "%d", (huc6270_state->R[HUC6270_REG_MWR] >> 7) & 0x01);

    ImGui::TextColored(violet, "HDS"); ImGui::SameLine();
    ImGui::TextColored(white, "%02X", (huc6270_state->R[HUC6270_REG_HSR] >> 8) & 0x7F); ImGui::SameLine();

    ImGui::TextColored(violet, "HDW"); ImGui::SameLine();
    ImGui::TextColored(white, "%02X", huc6270_state->R[HUC6270_REG_HDR] & 0x7F); ImGui::SameLine();

    ImGui::TextColored(violet, "HDE"); ImGui::SameLine();
    ImGui::TextColored(white, "%02X", (huc6270_state->R[HUC6270_REG_HDR] >> 8) & 0x7F); ImGui::SameLine();

    ImGui::TextColored(violet, "HSW"); ImGui::SameLine();
    ImGui::TextColored(white, "%02X", huc6270_state->R[HUC6270_REG_HSR] & 0x1F);

    ImGui::TextColored(violet, "VSW"); ImGui::SameLine();
    ImGui::TextColored(white, "%02X", huc6270_state->R[HUC6270_REG_VSR] & 0x1F); ImGui::SameLine();

    ImGui::TextColored(violet, "VDS"); ImGui::SameLine();
    ImGui::TextColored(white, "%02X", (huc6270_state->R[HUC6270_REG_VSR] >> 8) & 0xFF); ImGui::SameLine();

    ImGui::TextColored(violet, "VDW"); ImGui::SameLine();
    ImGui::TextColored(white, "%02X", huc6270_state->R[HUC6270_REG_VDR] & 0x1FF); ImGui::SameLine();

    ImGui::TextColored(violet, "VCR"); ImGui::SameLine();
    ImGui::TextColored(white, "%02X", huc6270_state->R[HUC6270_REG_VCR] & 0xFF);

    ImGui::NewLine(); ImGui::TextColored(cyan, "SCROLLING"); ImGui::Separator();

    ImGui::TextColored(violet, "X,Y     "); ImGui::SameLine();
    ImGui::TextColored(white, "%03X,%03X (%04d,%04d)", huc6270_state->R[HUC6270_REG_BXR], huc6270_state->R[HUC6270_REG_BYR], huc6270_state->R[HUC6270_REG_BXR], huc6270_state->R[HUC6270_REG_BYR]);

    ImGui::TextColored(violet, "LINE DET"); ImGui::SameLine();
    ImGui::TextColored(white, "%03X (%04d)", huc6270_state->R[HUC6270_REG_RCR], huc6270_state->R[HUC6270_REG_RCR]); 

    ImGui::NewLine(); ImGui::TextColored(cyan, "TRANSFER CONTROL"); ImGui::Separator(); 

    ImGui::TextColored(violet, "SAT IRQ "); ImGui::SameLine();
    ImGui::TextColored(huc6270_state->R[HUC6270_REG_DCR] & 0x0001 ? green : gray, "%s", huc6270_state->R[HUC6270_REG_DCR] & 0x0001 ? "ON" : "OFF");

    ImGui::TextColored(violet, "VRAM IRQ"); ImGui::SameLine();
    ImGui::TextColored(huc6270_state->R[HUC6270_REG_DCR] & 0x0002 ? green : gray, "%s", huc6270_state->R[HUC6270_REG_DCR] & 0x0002 ? "ON" : "OFF");

    ImGui::TextColored(violet, "SRC     "); ImGui::SameLine();
    ImGui::TextColored(white, "%s", huc6270_state->R[HUC6270_REG_DCR] & 0x0004 ? "DEC" : "INC");

    ImGui::TextColored(violet, "DEST    "); ImGui::SameLine();
    ImGui::TextColored(white, "%s", huc6270_state->R[HUC6270_REG_DCR] & 0x0008 ? "DEC" : "INC");

    ImGui::TextColored(violet, "SAT TX  "); ImGui::SameLine();
    ImGui::TextColored(huc6270_state->R[HUC6270_REG_DCR] & 0x0010 ? green : gray, "%s", huc6270_state->R[HUC6270_REG_DCR] & 0x0010 ? "AUTO" : "OFF");

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}

void gui_debug_window_huc6270_registers(int vdc)
{
    if (vdc < 1 || vdc > 2)
        return;

    GeargrafxCore* core = emu_get_core();
    HuC6270* huc6270 = vdc == 1 ? core->GetHuC6270_1() : core->GetHuC6270_2();
    HuC6270::HuC6270_State* huc6270_state = huc6270->GetState();
    bool* show = ((vdc == 1) ? &config_debug.show_huc6270_1_registers : &config_debug.show_huc6270_2_registers);
    char title[32];
    if (core->GetMedia()->IsSGX())
        snprintf(title, sizeof(title), "HuC6270 (%d) Registers", vdc);
    else
        strncpy_fit(title, "HuC6270 Registers", sizeof(title));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(93.0f + ((vdc == 1) ? 0 : 274), 79), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(284, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin(title, show);

    ImGui::PushFont(gui_default_font);

    ImGui::TextColored(magenta, "ADDRESS"); ImGui::SameLine();
    ImGui::TextColored(magenta, "  "); ImGui::SameLine();
    ImGui::TextColored(white, "$%04X ", *huc6270_state->AR); ImGui::SameLine(0, 0);
    ImGui::TextColored(gray, "(" BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED ")", BYTE_TO_BINARY(*huc6270_state->AR >> 8), BYTE_TO_BINARY(*huc6270_state->AR & 0xFF));

    ImGui::TextColored(magenta, "STATUS"); ImGui::SameLine();
    ImGui::TextColored(magenta, "   "); ImGui::SameLine();
    ImGui::TextColored(white, "$%04X ", *huc6270_state->SR); ImGui::SameLine(0, 0);
    ImGui::TextColored(gray, "(" BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED ")", BYTE_TO_BINARY(*huc6270_state->SR >> 8), BYTE_TO_BINARY(*huc6270_state->SR & 0xFF));
    ImGui::Separator();

    for (int i = 0; i < 20; i++)
    {
        if (i == 3 || i == 4)
            continue;

        ImGui::TextColored(cyan, "R%02X ", i); ImGui::SameLine();
        ImGui::TextColored(violet, "%s", k_register_names_aligned[i]); ImGui::SameLine();
        ImGui::TextColored(white, "$%04X ", huc6270_state->R[i]); ImGui::SameLine(0, 0);
        ImGui::TextColored(gray, "(" BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED ")", BYTE_TO_BINARY(huc6270_state->R[i] >> 8), BYTE_TO_BINARY(huc6270_state->R[i] & 0xFF));

        if (i == 2)
        {
            ImGui::TextColored(cyan, "R%02X ", i); ImGui::SameLine();
            ImGui::TextColored(violet, "VRR  "); ImGui::SameLine();
            ImGui::TextColored(white, "$%04X ", *huc6270_state->READ_BUFFER); ImGui::SameLine(0, 0);
            ImGui::TextColored(gray, "(" BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED ")", BYTE_TO_BINARY(*huc6270_state->READ_BUFFER >> 8), BYTE_TO_BINARY(*huc6270_state->READ_BUFFER & 0xFF));
        }
    }

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}


