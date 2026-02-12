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

#define GUI_DEBUG_HUC6270_TILES_IMPORT
#include "gui_debug_huc6270_tiles.h"

#include "imgui.h"
#include "geargrafx.h"
#include "gui_debug_constants.h"
#include "gui_debug_memory.h"
#include "gui.h"
#include "config.h"
#include "emu.h"
#include "ogl_renderer.h"

#define TILES_ACROSS 32
#define TILES_DOWN 64
#define TOTAL_TILES (TILES_ACROSS * TILES_DOWN)
#define TILE_TEXTURE_WIDTH (TILES_ACROSS * 8)
#define TILE_TEXTURE_HEIGHT (TILES_DOWN * 8)

static const float k_scale_levels[4] = { 1.0f, 1.5f, 2.0f, 3.0f };

void gui_debug_window_huc6270_tiles(int vdc)
{
    if (vdc < 1 || vdc > 2)
        return;

    GeargrafxCore* core = emu_get_core();
    HuC6270* huc6270 = vdc == 1 ? core->GetHuC6270_1() : core->GetHuC6270_2();
    u16* vram = huc6270->GetVRAM();
    bool* show = ((vdc == 1) ? &config_debug.show_huc6270_1_tiles : &config_debug.show_huc6270_2_tiles);
    char title[32];
    if (core->GetMedia()->IsSGX())
        snprintf(title, sizeof(title), "HuC6270 (%d) Tiles", vdc);
    else
        strncpy_fit(title, "HuC6270 Tiles", sizeof(title));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(130.0f + ((vdc == 1) ? 0 : 80), 60.0f + ((vdc == 1) ? 0 : 40)), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(440, 510), ImGuiCond_FirstUseEver);
    ImGui::Begin(title, show);

    static bool show_grid = true;
    static int zoom = 1;
    static int palette = 0;
    ImVec4 grid_color = dark_gray;
    grid_color.w = 0.3f;
    float scale = k_scale_levels[zoom];
    float size_h = (float)TILE_TEXTURE_WIDTH * scale;
    float size_v = (float)TILE_TEXTURE_HEIGHT * scale;
    float spacing = 8.0f * scale;

    if (ImGui::BeginTable("tile_opts", 2, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoPadOuterX))
    {
        ImGui::TableSetupColumn("one", ImGuiTableColumnFlags_WidthFixed, 200.0f);
        ImGui::TableSetupColumn("two", ImGuiTableColumnFlags_WidthFixed);

        ImGui::TableNextColumn();

        ImGui::PushItemWidth(60.0f);
        ImGui::Combo("Zoom##zoom_tiles", &zoom, "1x\0""1.5x\0""2x\0""3x\0\0");
        ImGui::PopItemWidth();
        ImGui::Checkbox("Show Grid##grid_tiles", &show_grid);

        ImGui::TableNextColumn();

        ImGui::PushFont(gui_default_font);

        ImGui::TextColored(violet, "PALETTE  ");
        ImGui::SameLine();
        ImGui::PushItemWidth(120.0f);
        ImGui::SliderInt("##tile_pal", &palette, 0, 15, "%d");
        ImGui::PopItemWidth();

        emu_debug_tiles_palette[vdc - 1] = palette;

        ImGui::TextColored(violet, "TILES    ");
        ImGui::SameLine();
        ImGui::TextColored(white, "%d (%dx%d)", TOTAL_TILES, TILES_ACROSS, TILES_DOWN);

        ImGui::TextColored(violet, "VRAM     ");
        ImGui::SameLine();
        ImGui::TextColored(white, "0x0000 - 0x7FFF");

        ImGui::PopFont();

        ImGui::EndTable();
    }

    ImGui::Separator();

    if (ImGui::BeginChild("##tiles", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav))
    {
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        ImGui::Image((ImTextureID)(intptr_t)ogl_renderer_emu_debug_huc6270_tiles[vdc - 1], ImVec2(size_h, size_v));

        if (show_grid)
        {
            float x = p.x;
            for (int n = 0; n <= TILES_ACROSS; n++)
            {
                draw_list->AddLine(ImVec2(x, p.y), ImVec2(x, p.y + size_v), ImColor(grid_color), 1.0f);
                x += spacing;
            }

            float y = p.y;
            for (int n = 0; n <= TILES_DOWN; n++)
            {
                draw_list->AddLine(ImVec2(p.x, y), ImVec2(p.x + size_h, y), ImColor(grid_color), 1.0f);
                y += spacing;
            }
        }

        if (ImGui::IsItemHovered())
        {
            ImVec2 mouse_pos = ImGui::GetMousePos();
            ImVec2 rel_pos = ImVec2((mouse_pos.x - p.x) / scale, (mouse_pos.y - p.y) / scale);
            int tile_x = (int)(rel_pos.x / 8.0f);
            int tile_y = (int)(rel_pos.y / 8.0f);

            if (tile_x >= 0 && tile_x < TILES_ACROSS && tile_y >= 0 && tile_y < TILES_DOWN)
            {
                int tile_index = tile_y * TILES_ACROSS + tile_x;

                ImVec2 tile_pos = ImVec2(p.x + (tile_x * 8.0f * scale), p.y + (tile_y * 8.0f * scale));
                ImVec2 tile_end = ImVec2(tile_pos.x + 8.0f * scale, tile_pos.y + 8.0f * scale);
                draw_list->AddRect(tile_pos, tile_end, ImColor(cyan), 2.0f, ImDrawFlags_RoundCornersAll, 2.0f);

                ImGui::BeginTooltip();

                float tile_scale = 16.0f;
                float tile_width = 8.0f * tile_scale;
                float tile_height = 8.0f * tile_scale;
                float tex_w = (float)TILE_TEXTURE_WIDTH;
                float tex_h = (float)TILE_TEXTURE_HEIGHT;
                float uv_x0 = (tile_x * 8.0f) / tex_w;
                float uv_y0 = (tile_y * 8.0f) / tex_h;
                float uv_x1 = ((tile_x + 1) * 8.0f) / tex_w;
                float uv_y1 = ((tile_y + 1) * 8.0f) / tex_h;

                ImGui::Image((ImTextureID)(intptr_t)ogl_renderer_emu_debug_huc6270_tiles[vdc - 1], ImVec2(tile_width, tile_height), ImVec2(uv_x0, uv_y0), ImVec2(uv_x1, uv_y1));

                ImGui::PushFont(gui_default_font);

                int vram_address = tile_index * 16;

                ImGui::TextColored(magenta, "TILE INDEX   ");
                ImGui::SameLine();
                ImGui::TextColored(white, "%03X (%d)", tile_index, tile_index);

                ImGui::TextColored(magenta, "VRAM ADDRESS ");
                ImGui::SameLine();
                ImGui::TextColored(white, "$%04X", vram_address);

                ImGui::TextColored(magenta, "PALETTE      ");
                ImGui::SameLine();
                ImGui::TextColored(white, "%d", palette);

                ImGui::Separator();
                ImGui::TextColored(violet, "RAW DATA (16 words):");

                for (int row = 0; row < 4; row++)
                {
                    ImGui::TextColored(gray, " ");
                    ImGui::SameLine(0, 0);
                    for (int col = 0; col < 4; col++)
                    {
                        int word_idx = vram_address + row * 4 + col;
                        if (word_idx < HUC6270_VRAM_SIZE)
                            ImGui::TextColored(white, "%04X", vram[word_idx]);
                        else
                            ImGui::TextColored(gray, "----");
                        if (col < 3)
                        {
                            ImGui::SameLine();
                        }
                    }
                }

                ImGui::PopFont();

                ImGui::EndTooltip();

                if (ImGui::IsMouseClicked(0))
                {
                    gui_debug_memory_goto((vdc == 1) ? MEMORY_EDITOR_VRAM_1 : MEMORY_EDITOR_VRAM_2, vram_address);
                }
            }
        }
    }

    ImGui::EndChild();
    ImGui::End();
    ImGui::PopStyleVar();
}
