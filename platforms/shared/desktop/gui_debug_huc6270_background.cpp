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

#define GUI_DEBUG_HUC6270_BACKGROUND_IMPORT
#include "gui_debug_huc6270_background.h"

#include "imgui.h"
#include "geargrafx.h"
#include "gui_filedialogs.h"
#include "gui_debug_constants.h"
#include "gui_debug_memory.h"
#include "gui.h"
#include "config.h"
#include "emu.h"
#include "ogl_renderer.h"
#include "utils.h"

static void draw_context_menu_background(int vdc);

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

    static bool show_grid[2] = { true, true };
    static int zoom[2] = { 1, 1 };
    ImVec4 grid_color = dark_gray;
    grid_color.w = 0.3f;
    float scale = k_scale_levels[zoom[vdc - 1]];
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
        ImGui::Combo("Zoom##zoom_bg", &zoom[vdc - 1], "1x\0""1.5x\0""2x\0\0");
        ImGui::Checkbox("Show Grid##grid_bg", &show_grid[vdc - 1]);

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

        ImGui::Image((ImTextureID)(intptr_t)ogl_renderer_emu_debug_huc6270_background[vdc - 1], ImVec2(size_h, size_v), ImVec2(0.0f, 0.0f), ImVec2(emu_debug_background_buffer_width[vdc - 1] / texture_size_h, emu_debug_background_buffer_height[vdc - 1] / texture_size_v));

        draw_context_menu_background(vdc);

        if (show_grid[vdc - 1])
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

                ImGui::Image((ImTextureID)(intptr_t)ogl_renderer_emu_debug_huc6270_background[vdc - 1], ImVec2(tile_width, tile_height), ImVec2(tile_uv_h / texture_size_h, tile_uv_v / texture_size_v), ImVec2((tile_uv_h + 8) / texture_size_h, (tile_uv_v + 8) / texture_size_v));

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

static void draw_context_menu_background(int vdc)
{
    char ctx_id[16];
    snprintf(ctx_id, sizeof(ctx_id), "##bg_ctx_%d", vdc);

    if (ImGui::BeginPopupContextItem(ctx_id))
    {
        if (ImGui::Selectable("Save Background As..."))
            gui_file_dialog_save_background(vdc - 1);

        ImGui::EndPopup();
    }
}
