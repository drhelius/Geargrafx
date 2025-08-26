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
#include "gui_debug_memory.h"
#include "gui.h"
#include "config.h"
#include "emu.h"
#include "renderer.h"
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
        ImGui::TextColored(violet, "%s", k_register_names_aligned[i]); ImGui::SameLine();
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

static const float k_scale_levels[3] = { 1.0f, 1.5f, 2.0f };

void gui_debug_window_huc6270_background(int vdc)
{
    if (vdc < 1 || vdc > 2)
        return;

    GeargrafxCore* core = emu_get_core();
    HuC6270* huc6270 = vdc == 1 ? core->GetHuC6270_1() : core->GetHuC6270_2();
    HuC6270::HuC6270_State* huc6270_state = huc6270->GetState();
    bool* show = ((vdc == 1) ? &config_debug.show_huc6270_1_background : &config_debug.show_huc6270_2_background);
    char title[32];
    if (core->GetMedia()->IsSGX())
        snprintf(title, sizeof(title), "HuC6270 (%d) Background", vdc);
    else
        strncpy_fit(title, "HuC6270 Background", sizeof(title));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(63.0f + ((vdc == 1) ? 0 : 108), 35.0f + ((vdc == 1) ? 0 : 44)), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(545, 614), ImGuiCond_FirstUseEver);
    ImGui::Begin(title, show);

    u16* vram = huc6270->GetVRAM();
    int screen_reg = (huc6270_state->R[HUC6270_REG_MWR] >> 4) & 0x07;
    int screen_size_x = k_huc6270_screen_size_x[screen_reg];
    int screen_size_y = k_huc6270_screen_size_y[screen_reg];
    int bat_size = screen_size_x * screen_size_y;

    static bool show_grid = true;
    static int zoom = 2;
    ImVec4 grid_color = dark_gray;
    grid_color.w = 0.3f;
    float scale = k_scale_levels[zoom];
    float size_h = 8.0f * screen_size_x * scale;
    float size_v = 8.0f * screen_size_y * scale;
    float spacing_h = 8.0f * scale;
    float spacing_v = 8.0f * scale;
    float texture_size_h = 1024.0f;
    float texture_size_v = 512.0f;

    if (ImGui::BeginTable("regs", 2, ImGuiTableFlags_BordersInnerH |ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoPadOuterX))
    {
        ImGui::TableSetupColumn("one", ImGuiTableColumnFlags_WidthFixed, 200.0f);
        ImGui::TableSetupColumn("two", ImGuiTableColumnFlags_WidthFixed); 

        ImGui::TableNextColumn();

        ImGui::PushItemWidth(60.0f);
        ImGui::Combo("Zoom##zoom_bg", &zoom, "1x\0""1.5x\0""2x\0\0");
        ImGui::Checkbox("Show Grid##grid_bg", &show_grid);

        ImGui::TableNextColumn();
        ImGui::PushFont(gui_default_font);

        ImGui::TextColored(violet, "ENABLED  ");

        ImGui::SameLine();
        ImGui::TextColored(huc6270_state->R[HUC6270_REG_CR] & 0x0080 ? green : gray, "%s", huc6270_state->R[HUC6270_REG_CR] & 0x0080 ? "YES" : "NO");

        ImGui::SameLine();
        ImGui::TextColored(violet, "        SCREEN");

        ImGui::SameLine();
        ImGui::TextColored(white, "%dx%d", k_huc6270_screen_size_x[(huc6270_state->R[HUC6270_REG_MWR] >> 4) & 0x07], k_huc6270_screen_size_y[(huc6270_state->R[HUC6270_REG_MWR] >> 4) & 0x07]);

        ImGui::TextColored(violet, "SCROLL X "); 

        ImGui::SameLine();
        ImGui::TextColored(white, "%02X (%03d)", huc6270_state->R[HUC6270_REG_BXR], huc6270_state->R[HUC6270_REG_BXR]);

        ImGui::TextColored(violet, "SCROLL Y "); 

        ImGui::SameLine();
        ImGui::TextColored(white, "%02X (%03d)", huc6270_state->R[HUC6270_REG_BYR], huc6270_state->R[HUC6270_REG_BYR]);

        ImGui::PopFont();

        ImGui::EndTable();
    }

    ImGui::Separator();


    if (ImGui::BeginChild("##bg", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav))
    {
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        ImGui::Image((ImTextureID)(intptr_t)renderer_emu_debug_huc6270_background[vdc - 1], ImVec2(size_h, size_v), ImVec2(0.0f, 0.0f), ImVec2(emu_debug_background_buffer_width[vdc - 1] / texture_size_h, emu_debug_background_buffer_height[vdc - 1] / texture_size_v));

        if (show_grid)
        {
            float x = p.x;
            for (int n = 0; n <= screen_size_x; n++)
            {
                draw_list->AddLine(ImVec2(x, p.y), ImVec2(x, p.y + size_v), ImColor(grid_color), 1.0f);
                x += spacing_h;
            }

            float y = p.y;
            for (int n = 0; n <= screen_size_y; n++)
            {
                draw_list->AddLine(ImVec2(p.x, y), ImVec2(p.x + size_h, y), ImColor(grid_color), 1.0f);
                y += spacing_v;
            }
        }

        if (ImGui::IsItemHovered())
        {
            ImVec2 mouse_pos = ImGui::GetMousePos();
            ImVec2 rel_pos = ImVec2((mouse_pos.x - p.x) / scale, (mouse_pos.y - p.y) / scale);
            int x = (int)(rel_pos.x / 8.0f);
            int y = (int)(rel_pos.y / 8.0f);
            int i = (screen_size_x * y) + x;
            if (i >= 0 && i < bat_size)
            {
                ImVec2 tile_pos = ImVec2(p.x + (x * 8.0f * scale), p.y + (y * 8.0f * scale));
                ImVec2 tile_size = ImVec2(8.0f * scale, 8.0f * scale);
                draw_list->AddRect(tile_pos, ImVec2(tile_pos.x + tile_size.x, tile_pos.y + tile_size.y), ImColor(cyan), 2.0f, ImDrawFlags_RoundCornersAll, 2.0f);

                u16 bat_entry = vram[i];
                int tile_index = bat_entry & 0x07FF;
                int color_table = (bat_entry >> 12) & 0x0F;

                ImGui::BeginTooltip();

                float tile_scale = 16.0f;
                float tile_width = 8.0f * tile_scale;
                float tile_height = 8.0f * tile_scale;
                float tile_uv_h = (i % screen_size_x) * 8.0f;
                float tile_uv_v = (i / screen_size_x) * 8.0f;

                ImGui::Image((ImTextureID)(intptr_t)renderer_emu_debug_huc6270_background[vdc - 1], ImVec2(tile_width, tile_height), ImVec2(tile_uv_h / texture_size_h, tile_uv_v / texture_size_v), ImVec2((tile_uv_h + 8) / texture_size_h, (tile_uv_v + 8) / texture_size_v));

                ImGui::PushFont(gui_default_font);

                ImGui::TextColored(magenta, "TILE INDEX   ");
                ImGui::SameLine();
                ImGui::TextColored(white, "%03X", tile_index);

                ImGui::TextColored(magenta, "TILE ADDRESS ");
                ImGui::SameLine();
                ImGui::TextColored(white, "%03X", tile_index * 16);

                ImGui::TextColored(magenta, "COLOR TABLE  ");
                ImGui::SameLine();
                ImGui::TextColored(white, "%01X", color_table);

                ImGui::PopFont();

                ImGui::EndTooltip();

                if (ImGui::IsMouseClicked(0))
                {
                    gui_debug_memory_goto((vdc == 1) ? MEMORY_EDITOR_VRAM_1 : MEMORY_EDITOR_VRAM_2, tile_index * 16);
                }
            }
        }
    }

    ImGui::EndChild();
    ImGui::End();
    ImGui::PopStyleVar();
}

void gui_debug_window_huc6270_sprites(int vdc)
{
    if (vdc < 1 || vdc > 2)
        return;

    GeargrafxCore* core = emu_get_core();
    HuC6270* huc6270 = vdc == 1 ? core->GetHuC6270_1() : core->GetHuC6270_2();
    u16* sat = huc6270->GetSAT();
    GG_Runtime_Info runtime;
    emu_get_runtime(runtime);
    bool* show = ((vdc == 1) ? &config_debug.show_huc6270_1_sprites : &config_debug.show_huc6270_2_sprites);
    char title[32];
    if (core->GetMedia()->IsSGX())
        snprintf(title, sizeof(title), "HuC6270 (%d) Sprites", vdc);
    else
        strncpy_fit(title, "HuC6270 Sprites", sizeof(title));

    ImVec4 cyan = ImVec4(0.0f, 1.0f, 1.0f, 1.0f);
    float scale = 4.0f;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(78.0f + ((vdc == 1) ? 0 : 64), 56.0f + ((vdc == 1) ? 0 : 34)), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(546, 500), ImGuiCond_FirstUseEver);
    ImGui::Begin(title, show);

    ImGui::PushFont(gui_default_font);

    ImGuiIO& io = ImGui::GetIO();

    ImGui::Columns(2, "spr", false);
    ImGui::SetColumnOffset(1, 180.0f);

    ImGui::BeginChild("sprites", ImVec2(0, 0.0f), ImGuiChildFlags_Border);
    bool window_hovered = ImGui::IsWindowHovered();

    ImVec2 p[64];

    for (int s = 0; s < 64; s++)
    {
        p[s] = ImGui::GetCursorScreenPos();

        u16 sprite_flags = sat[(s * 4) + 3] & 0xB98F;
        float fwidth = k_huc6270_sprite_width[(sprite_flags >> 8) & 0x01] * scale;
        float fheight = k_huc6270_sprite_height[(sprite_flags >> 12) & 0x03] * scale;
        float tex_h = fwidth / 32.0f / scale;
        float tex_v = fheight / 64.0f / scale;

        ImGui::Image((ImTextureID)(intptr_t)renderer_emu_debug_huc6270_sprites[vdc - 1][s], ImVec2(fwidth, fheight), ImVec2(0.0f, 0.0f), ImVec2(tex_h, tex_v));

        float mouse_x = io.MousePos.x - p[s].x;
        float mouse_y = io.MousePos.y - p[s].y;

        if (window_hovered && (mouse_x >= 0.0f) && (mouse_x < fwidth) && (mouse_y >= 0.0f) && (mouse_y < fheight))
        {
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            draw_list->AddRect(ImVec2(p[s].x, p[s].y), ImVec2(p[s].x + fwidth, p[s].y + fheight), ImColor(cyan), 2.0f, ImDrawFlags_RoundCornersAll, 3.0f);
        }
    }

    ImGui::EndChild();

    ImGui::NextColumn();

    ImVec2 p_screen = ImGui::GetCursorScreenPos();

    float screen_scale = 1.0f;
    float tex_h = (float)runtime.screen_width / (float)(SYSTEM_TEXTURE_WIDTH);
    float tex_v = (float)runtime.screen_height / (float)(SYSTEM_TEXTURE_HEIGHT);

    ImGui::Image((ImTextureID)(intptr_t)renderer_emu_texture, ImVec2(runtime.screen_width * screen_scale, runtime.screen_height * screen_scale), ImVec2(0, 0), ImVec2(tex_h, tex_v));

    for (int s = 0; s < 64; s++)
    {
        float mouse_x = io.MousePos.x - p[s].x;
        float mouse_y = io.MousePos.y - p[s].y;
        u16 sprite_flags = sat[(s * 4) + 3] & 0xB98F;
        int width = k_huc6270_sprite_width[(sprite_flags >> 8) & 0x01];
        int height = k_huc6270_sprite_height[(sprite_flags >> 12) & 0x03];
        float fwidth = width * scale;
        float fheight = height * scale;

        if (window_hovered && (mouse_x >= 0.0f) && (mouse_x < fwidth) && (mouse_y >= 0.0f) && (mouse_y < fheight))
        {
            int sprite_y = (sat[s * 4] & 0x03FF) + 3;
            int sprite_x = sat[(s * 4) + 1] & 0x03FF;
            u16 pattern = (sat[(s * 4) + 2] >> 1) & 0x03FF;

            bool h_flip = (sprite_flags & 0x0800) != 0;
            bool v_flip = (sprite_flags & 0x8000) != 0;

            int palette = sprite_flags & 0x0F;
            bool priority = (sprite_flags & 0x0080) != 0;

            float real_x = (float)(sprite_x - 32);
            float real_y = (float)(sprite_y - 64);

            float rectx_min = p_screen.x + (real_x * screen_scale);
            float rectx_max = p_screen.x + ((real_x + width) * screen_scale);
            float recty_min = p_screen.y + (real_y * screen_scale);
            float recty_max = p_screen.y + ((real_y + height) * screen_scale);

            rectx_min = fminf(fmaxf(rectx_min, p_screen.x), p_screen.x + (runtime.screen_width * screen_scale));
            rectx_max = fminf(fmaxf(rectx_max, p_screen.x), p_screen.x + (runtime.screen_width * screen_scale));
            recty_min = fminf(fmaxf(recty_min, p_screen.y), p_screen.y + (runtime.screen_height * screen_scale));
            recty_max = fminf(fmaxf(recty_max, p_screen.y), p_screen.y + (runtime.screen_height * screen_scale));

            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            draw_list->AddRect(ImVec2(rectx_min, recty_min), ImVec2(rectx_max, recty_max), ImColor(cyan), 2.0f, ImDrawFlags_RoundCornersAll, 2.0f);

            ImGui::NewLine();

            ImGui::TextColored(cyan, "DETAILS:");
            ImGui::Separator();
            ImGui::TextColored(violet, " SAT ENTRY:"); ImGui::SameLine();
            ImGui::Text("%d", s);

            ImGui::TextColored(violet, " SPRITE X: "); ImGui::SameLine();
            ImGui::Text("%03X (%d)", sprite_x, sprite_x);

            ImGui::TextColored(violet, " SPRITE Y: "); ImGui::SameLine();
            ImGui::Text("%03X (%d)", sprite_y, sprite_y);

            ImGui::TextColored(violet, " SIZE:     "); ImGui::SameLine();
            ImGui::Text("%dx%d", width, height);

            ImGui::TextColored(violet, " PATTERN:  "); ImGui::SameLine();
            ImGui::Text("%03X (%d)", pattern, pattern);

            ImGui::TextColored(violet, " VRAM ADDR:"); ImGui::SameLine();
            ImGui::Text("$%04X", pattern << 6);

            ImGui::TextColored(violet, " PALETTE:  "); ImGui::SameLine();
            ImGui::Text("%01X (%d)", palette, palette);

            ImGui::TextColored(violet, " H FLIP:   "); ImGui::SameLine();
            ImGui::TextColored(h_flip ? green : gray, "%s", h_flip ? "YES" : "NO ");

            ImGui::TextColored(violet, " V FLIP:   "); ImGui::SameLine();
            ImGui::TextColored(v_flip ? green : gray, "%s", v_flip ? "YES" : "NO ");

            ImGui::TextColored(violet, " PRIORITY: "); ImGui::SameLine();
            ImGui::TextColored(priority ? green : gray, "%s", priority ? "YES" : "NO ");

            if (ImGui::IsMouseClicked(0))
            {
                gui_debug_memory_goto((vdc == 1) ? MEMORY_EDITOR_VRAM_1 : MEMORY_EDITOR_VRAM_2, pattern << 6);
            }
        }
    }

    ImGui::Columns(1);

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}
