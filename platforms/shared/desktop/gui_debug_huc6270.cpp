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

#include "imgui/imgui.h"
#include "../../../src/geargrafx.h"
#include "gui_debug_constants.h"
#include "gui.h"
#include "config.h"
#include "emu.h"
#include "renderer.h"

static const char* k_register_names[20] = { "MAWR ", "MARR ", "VWR  ", "???  ", "???  ",
                                     "CR   ", "RCR  ", "BXR  ", "BYR  ", "MWR  ",
                                     "HSR  ", "HDR  ", "VPR  ", "VDR  ", "VCR  ",
                                     "DCR  ", "SOUR ", "DESR ", "LENR ", "DVSSR" };

void gui_debug_window_huc6270_info(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(6, 31), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(401, 641), ImGuiCond_FirstUseEver);
    ImGui::Begin("HuC6270 Info", &config_debug.show_huc6270_sprites);

    ImGui::PushFont(gui_default_font);

    GeargrafxCore* core = emu_get_core();
    HuC6260* huc6260 = core->GetHuC6260();
    HuC6270* huc6270 = core->GetHuC6270();
    HuC6270::HuC6270_State* huc6270_state = huc6270->GetState();

    ImGui::TextColored(magenta, "SPEED   "); ImGui::SameLine();
    const char* speed[] = { "10.8 MHz", "7.16 MHz", "5.36 MHz" };
    ImGui::TextColored(green, "%s", speed[huc6260->GetSpeed()]);

    ImGui::TextColored(magenta, "X,Y     "); ImGui::SameLine();
    ImGui::TextColored(white, "%03d,%03d", *huc6270_state->HPOS, *huc6270_state->VPOS);

    ImGui::NewLine(); ImGui::TextColored(cyan, "CONTROL REGISTRY"); ImGui::Separator(); 

    ImGui::TextColored(magenta, "SPRITES ");ImGui::SameLine();
    ImGui::TextColored(huc6270_state->R[HUC6270_REG_CR] & 0x0040 ? green : gray, "%s", huc6270_state->R[HUC6270_REG_CR] & 0x0040 ? "ON" : "OFF");

    ImGui::TextColored(magenta, "BACKGRND");ImGui::SameLine();
    ImGui::TextColored(huc6270_state->R[HUC6270_REG_CR] & 0x0080 ? green : gray, "%s", huc6270_state->R[HUC6270_REG_CR] & 0x0080 ? "ON" : "OFF");

    ImGui::TextColored(magenta, "DISP OUT");ImGui::SameLine();
    const char* disp_output[] = { "DISP", "~BURST", "~INTHSYNC", "INVALID" };
    u16 disp_out_value = (huc6270_state->R[HUC6270_REG_CR] >> 8) & 0x03;
    ImGui::TextColored(disp_out_value == 3 ? red : white, "%s", disp_output[disp_out_value]);

    ImGui::TextColored(magenta, "R/W INC ");ImGui::SameLine();
    ImGui::TextColored(white, "%02X", k_read_write_increment[(huc6270_state->R[HUC6270_REG_CR] >> 11) & 0x03]);

    ImGui::TextColored(magenta, "INT REQ ");ImGui::SameLine();
    ImGui::TextColored(huc6270_state->R[HUC6270_REG_CR] & HUC6270_CONTROL_COLLISION ? orange : gray, "COLL"); ImGui::SameLine();
    ImGui::TextColored(huc6270_state->R[HUC6270_REG_CR] & HUC6270_CONTROL_OVERFLOW ? orange : gray, "OVER"); ImGui::SameLine();
    ImGui::TextColored(huc6270_state->R[HUC6270_REG_CR] & HUC6270_CONTROL_SCANLINE ? orange : gray, "SCAN"); ImGui::SameLine();
    ImGui::TextColored(huc6270_state->R[HUC6270_REG_CR] & HUC6270_CONTROL_VBLANK ? orange : gray, "VERT");

    ImGui::NewLine(); ImGui::TextColored(cyan, "STATUS REGISTRY"); ImGui::Separator(); 

    ImGui::TextColored(magenta, "INT ACT ");ImGui::SameLine();
    ImGui::TextColored(*huc6270_state->SR & HUC6270_STATUS_COLLISION ? green : gray, "COLL"); ImGui::SameLine();
    ImGui::TextColored(*huc6270_state->SR & HUC6270_STATUS_OVERFLOW ? green : gray, "OVER"); ImGui::SameLine();
    ImGui::TextColored(*huc6270_state->SR & HUC6270_STATUS_SCANLINE ? green : gray, "SCAN"); ImGui::SameLine();
    ImGui::TextColored(*huc6270_state->SR & HUC6270_STATUS_VBLANK ? green : gray, "VERT");

    ImGui::TextColored(magenta, "SAT TX  ");ImGui::SameLine();
    ImGui::TextColored(*huc6270_state->SR & HUC6270_STATUS_SAT_END ? green : gray, "%s", *huc6270_state->SR & HUC6270_STATUS_SAT_END ? "YES" : "NO");

    ImGui::TextColored(magenta, "VRAM TX  ");ImGui::SameLine();
    ImGui::TextColored(*huc6270_state->SR & HUC6270_STATUS_VRAM_END ? green : gray, "%s", *huc6270_state->SR & HUC6270_STATUS_VRAM_END ? "YES" : "NO");

    ImGui::NewLine(); ImGui::TextColored(cyan, "DISPLAY GEOMETRY"); ImGui::Separator(); 

    ImGui::TextColored(magenta, "SCREEN  ");ImGui::SameLine();
    ImGui::TextColored(white, "%dx%d", k_scren_size_x[(huc6270_state->R[HUC6270_REG_MWR] >> 4) & 0x07], k_scren_size_y[(huc6270_state->R[HUC6270_REG_MWR] >> 4) & 0x07]);

    ImGui::NewLine(); ImGui::TextColored(cyan, "SCROLLING"); ImGui::Separator(); 

    ImGui::TextColored(magenta, "SCROLL X");ImGui::SameLine();
    ImGui::TextColored(white, "%02X (%03d)", huc6270_state->R[HUC6270_REG_BXR], huc6270_state->R[HUC6270_REG_BXR]);

    ImGui::TextColored(magenta, "SCROLL Y");ImGui::SameLine();
    ImGui::TextColored(white, "%02X (%03d)", huc6270_state->R[HUC6270_REG_BYR], huc6270_state->R[HUC6270_REG_BYR]); 

    ImGui::TextColored(magenta, "LINE DET");ImGui::SameLine();
    ImGui::TextColored(white, "%04X", huc6270_state->R[HUC6270_REG_RCR]); 

    ImGui::NewLine(); ImGui::TextColored(cyan, "TRANSFER CONTROL"); ImGui::Separator(); 

    ImGui::TextColored(magenta, "SAT IRQ ");ImGui::SameLine();
    ImGui::TextColored(huc6270_state->R[HUC6270_REG_DCR] & 0x0001 ? green : gray, "%s", huc6270_state->R[HUC6270_REG_DCR] & 0x0001 ? "ON" : "OFF");

    ImGui::TextColored(magenta, "VRAM IRQ");ImGui::SameLine();
    ImGui::TextColored(huc6270_state->R[HUC6270_REG_DCR] & 0x0002 ? green : gray, "%s", huc6270_state->R[HUC6270_REG_DCR] & 0x0002 ? "ON" : "OFF");

    ImGui::TextColored(magenta, "SRC     ");ImGui::SameLine();
    ImGui::TextColored(white, "%s", huc6270_state->R[HUC6270_REG_DCR] & 0x0004 ? "DEC" : "INC");

    ImGui::TextColored(magenta, "DEST    ");ImGui::SameLine();
    ImGui::TextColored(white, "%s", huc6270_state->R[HUC6270_REG_DCR] & 0x0008 ? "DEC" : "INC");

    ImGui::TextColored(magenta, "SAT     ");ImGui::SameLine();
    ImGui::TextColored(huc6270_state->R[HUC6270_REG_DCR] & 0x0010 ? green : gray, "%s", huc6270_state->R[HUC6270_REG_DCR] & 0x0010 ? "AUTO" : "OFF");

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}

void gui_debug_window_huc6270_registers(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(6, 31), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(401, 641), ImGuiCond_FirstUseEver);
    ImGui::Begin("HuC6270 Registers", &config_debug.show_huc6270_registers);

    ImGui::PushFont(gui_default_font);

    GeargrafxCore* core = emu_get_core();
    HuC6270* huc6270 = core->GetHuC6270();
    HuC6270::HuC6270_State* huc6270_state = huc6270->GetState();

    ImGui::TextColored(magenta, "ADDRESS"); ImGui::SameLine();
    ImGui::TextColored(magenta, "  "); ImGui::SameLine();
    ImGui::Text("$%04X (" BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED ")", *huc6270_state->AR, BYTE_TO_BINARY(*huc6270_state->AR >> 8), BYTE_TO_BINARY(*huc6270_state->AR & 0xFF));

    ImGui::TextColored(magenta, "STATUS"); ImGui::SameLine();
    ImGui::TextColored(magenta, "   "); ImGui::SameLine();
    ImGui::Text("$%04X (" BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED ")", *huc6270_state->SR, BYTE_TO_BINARY(*huc6270_state->SR >> 8), BYTE_TO_BINARY(*huc6270_state->SR & 0xFF));

    ImGui::Separator();

    for (int i = 0; i < 20; i++)
    {
        if (i == 3 || i == 4)
            continue;

        ImGui::TextColored(cyan, "R%02X ", i); ImGui::SameLine();
        ImGui::TextColored(violet, "%s", k_register_names[i]); ImGui::SameLine();
        ImGui::Text("$%04X (" BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED ")", huc6270_state->R[i], BYTE_TO_BINARY(huc6270_state->R[i] >> 8), BYTE_TO_BINARY(huc6270_state->R[i] & 0xFF));

        if (i == 2)
        {
            ImGui::TextColored(cyan, "R%02X ", i); ImGui::SameLine();
            ImGui::TextColored(violet, "VRR  "); ImGui::SameLine();
            ImGui::Text("$%04X (" BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED ")", *huc6270_state->READ_BUFFER, BYTE_TO_BINARY(*huc6270_state->READ_BUFFER >> 8), BYTE_TO_BINARY(*huc6270_state->READ_BUFFER & 0xFF));
        }
    }

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}

void gui_debug_window_huc6270_background(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(6, 31), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(401, 641), ImGuiCond_FirstUseEver);
    ImGui::Begin("HuC6270 Background", &config_debug.show_huc6270_background);

    ImGui::PushFont(gui_default_font);

    GeargrafxCore* core = emu_get_core();
    HuC6270* huc6270 = core->GetHuC6270();

    ImGui::TextColored(magenta, "BACKGROUND");

    ImGui::Image((void*)(intptr_t)renderer_emu_debug_huc6270_background, ImVec2(emu_debug_background_buffer_width, emu_debug_background_buffer_height), ImVec2(0.0f, 0.0f), ImVec2(emu_debug_background_buffer_width / 1024.0f, emu_debug_background_buffer_height / 512.0f));

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}

void gui_debug_window_huc6270_sprites(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(6, 31), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(401, 641), ImGuiCond_FirstUseEver);
    ImGui::Begin("HuC6270 Sprites", &config_debug.show_huc6270_sprites);

    ImGui::PushFont(gui_default_font);

    GeargrafxCore* core = emu_get_core();
    HuC6270* huc6270 = core->GetHuC6270();

    ImGui::TextColored(magenta, "SPRITES");

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}
