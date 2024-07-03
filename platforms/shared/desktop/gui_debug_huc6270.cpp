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

#include "../../../src/geargrafx.h"

// static void debug_window_vram_registers(void);
// static void debug_window_vram(void);
// static void debug_window_vram_background(void);
// static void debug_window_vram_tiles(void);
// static void debug_window_vram_sprites(void);
// static void debug_window_vram_regs(void);

// static void debug_window_vram_registers(void)
// {
//     ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
//     ImGui::SetNextWindowPos(ImVec2(567, 560), ImGuiCond_FirstUseEver);
//     ImGui::SetNextWindowSize(ImVec2(260, 329), ImGuiCond_FirstUseEver);

//     ImGui::Begin("VDP Registers", &config_debug.show_video_registers);

//     debug_window_vram_regs();

//     ImGui::End();
//     ImGui::PopStyleVar();
// }

// static void debug_window_vram(void)
// {
//     ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
//     ImGui::SetNextWindowPos(ImVec2(896, 31), ImGuiCond_FirstUseEver);
//     ImGui::SetNextWindowSize(ImVec2(668, 640), ImGuiCond_FirstUseEver);

//     ImGui::Begin("VDP Viewer", &config_debug.show_video);

//     ImGui::PushFont(gui_default_font);

//     Video* video = emu_get_core()->GetVideo();
//     u8* regs = video->GetRegisters();

//     ImGui::TextColored(cyan, "  VIDEO MODE:");ImGui::SameLine();
//     ImGui::Text("$%02X", video->GetMode()); ImGui::SameLine();

//     ImGui::TextColored(magenta, "  M1:");ImGui::SameLine();
//     ImGui::Text("%d", (regs[1] >> 4) & 0x01); ImGui::SameLine();
//     ImGui::TextColored(magenta, "  M2:");ImGui::SameLine();
//     ImGui::Text("%d", (regs[0] >> 1) & 0x01); ImGui::SameLine();
//     ImGui::TextColored(magenta, "  M3:");ImGui::SameLine();
//     ImGui::Text("%d", (regs[1] >> 3) & 0x01);

//     ImGui::PopFont();

//     if (ImGui::BeginTabBar("##vram_tabs", ImGuiTabBarFlags_None))
//     {
//         if (ImGui::BeginTabItem("Name Table"))
//         {
//             debug_window_vram_background();
//             ImGui::EndTabItem();
//         }

//         if (ImGui::BeginTabItem("Pattern Table"))
//         {
//             debug_window_vram_tiles();
//             ImGui::EndTabItem();
//         }

//         if (ImGui::BeginTabItem("Sprites"))
//         {
//             debug_window_vram_sprites();
//             ImGui::EndTabItem();
//         }

//         ImGui::EndTabBar();
//     }

//     ImGui::End();
//     ImGui::PopStyleVar();
// }

// static void debug_window_vram_background(void)
// {
//     Video* video = emu_get_core()->GetVideo();
//     GC_RuntimeInfo runtime;
//     emu_get_runtime(runtime);
//     u8* regs = video->GetRegisters();
//     u8* vram = video->GetVRAM();
//     int mode = video->GetMode();

//     static bool show_grid = true;
//     int cols = (mode == 1) ? 40 : 32;
//     int rows = 24;
//     float scale = 2.0f;
//     float tile_width = (mode == 1) ? 6.0f : 8.0f;
//     float size_h = ((mode == 1) ? (6.0f * 40.0f) : 256.0f) * scale;
//     float size_v = 8.0f * rows * scale;
//     float spacing_h = ((mode == 1) ? 6.0f : 8.0f) * scale;
//     float spacing_v = 8.0f * scale;
//     float uv_h = (mode == 1) ? 0.0234375f : 1.0f / 32.0f;
//     float uv_v = 1.0f / 32.0f;

//     ImGui::Checkbox("Show Grid##grid_bg", &show_grid);

//     ImGui::PushFont(gui_default_font);

//     ImGui::Columns(2, "bg", false);
//     ImGui::SetColumnOffset(1, size_h + 10.0f);

//     ImVec2 p = ImGui::GetCursorScreenPos();
//     ImDrawList* draw_list = ImGui::GetWindowDrawList();
//     ImGuiIO& io = ImGui::GetIO();

//     ImGui::Image((void*)(intptr_t)renderer_emu_debug_vram_background, ImVec2(size_h, size_v), ImVec2(0.0f, 0.0f), ImVec2(uv_h * cols, uv_v * rows));

//     if (show_grid)
//     {
//         float x = p.x;
//         for (int n = 0; n <= cols; n++)
//         {
//             draw_list->AddLine(ImVec2(x, p.y), ImVec2(x, p.y + size_v), ImColor(dark_gray), 1.0f);
//             x += spacing_h;
//         }

//         float y = p.y;  
//         for (int n = 0; n <= rows; n++)
//         {
//             draw_list->AddLine(ImVec2(p.x, y), ImVec2(p.x + size_h, y), ImColor(dark_gray), 1.0f);
//             y += spacing_v;
//         }
//     }

//     int name_table_addr = regs[2] << 10;
//     int color_table_addr = regs[3] << 6;
//     if (mode == 2)
//         color_table_addr &= 0x2000;

//     ImGui::TextColored(cyan, " Name Table Addr:"); ImGui::SameLine();
//     ImGui::Text("$%04X", name_table_addr);

//     float mouse_x = io.MousePos.x - p.x;
//     float mouse_y = io.MousePos.y - p.y;

//     int tile_x = -1;
//     int tile_y = -1;
//     if (ImGui::IsWindowHovered() && (mouse_x >= 0.0f) && (mouse_x < size_h) && (mouse_y >= 0.0f) && (mouse_y < size_v))
//     {
//         tile_x = (int)(mouse_x / spacing_h);
//         tile_y = (int)(mouse_y / spacing_v);

//         draw_list->AddRect(ImVec2(p.x + (tile_x * spacing_h), p.y + (tile_y * spacing_v)), ImVec2(p.x + ((tile_x + 1) * spacing_h), p.y + ((tile_y + 1) * spacing_v)), ImColor(cyan), 2.0f, ImDrawFlags_RoundCornersAll, 2.0f);

//         ImGui::NextColumn();

//         ImGui::Image((void*)(intptr_t)renderer_emu_debug_vram_background, ImVec2(tile_width * 16.0f, 128.0f), ImVec2(uv_h * tile_x, uv_v * tile_y), ImVec2(uv_h * (tile_x + 1), uv_v * (tile_y + 1)));

//         ImGui::TextColored(yellow, "INFO:");

//         ImGui::TextColored(cyan, " X:"); ImGui::SameLine();
//         ImGui::Text("$%02X", tile_x);
//         ImGui::TextColored(cyan, " Y:"); ImGui::SameLine();
//         ImGui::Text("$%02X", tile_y);

//         int pattern_table_addr = regs[4] << 11;
//         int region = (tile_y & 0x18) << 5;

//         int tile_number = (tile_y * cols) + tile_x;
//         int name_tile_addr = (name_table_addr + tile_number) & 0x3FFF;
//         int name_tile = vram[name_tile_addr];

//         if (mode == 2)
//         {
//             pattern_table_addr &= 0x2000;
//             name_tile += region;
//         }
//         else if(mode == 4)
//         {
//             pattern_table_addr &= 0x2000;
//         }

//         int tile_addr = (pattern_table_addr + (name_tile << 3)) & 0x3FFF;

//         int color_mask = ((regs[3] & 0x7F) << 3) | 0x07;

//         int color_tile_addr = 0;

//         if (mode == 2)
//             color_tile_addr = color_table_addr + ((name_tile & color_mask) << 3);
//         else if (mode == 0)
//             color_tile_addr = color_table_addr + (name_tile >> 3);

//         ImGui::TextColored(cyan, " Name Addr:"); ImGui::SameLine();
//         ImGui::Text(" $%04X", name_tile_addr);
//         ImGui::TextColored(cyan, " Tile Number:"); ImGui::SameLine();
//         ImGui::Text("$%03X", name_tile);
//         ImGui::TextColored(cyan, " Tile Addr:"); ImGui::SameLine();
//         ImGui::Text(" $%04X", tile_addr);
//         ImGui::TextColored(cyan, " Color Addr:"); ImGui::SameLine();
//         ImGui::Text("$%04X", color_tile_addr);

//         if (ImGui::IsMouseClicked(0))
//         {
//             mem_edit_select = 4;
//             mem_edit[4].JumpToAddress(name_tile_addr);
//         }
//     }

//     ImGui::Columns(1);

//     ImGui::PopFont();
// }

// static void debug_window_vram_tiles(void)
// {
//     Video* video = emu_get_core()->GetVideo();
//     u8* regs = video->GetRegisters();
//     int mode = video->GetMode();

//     static bool show_grid = true;
//     int lines = 32;
//     float scale = 2.0f;
//     float width = 8.0f * 32.0f * scale;
//     float height = 8.0f * lines * scale;
//     float spacing = 8.0f * scale;
//     ImDrawList* draw_list = ImGui::GetWindowDrawList();
//     ImGuiIO& io = ImGui::GetIO();
//     ImVec2 p;

//     ImGui::Checkbox("Show Grid##grid_tiles", &show_grid);

//     ImGui::PushFont(gui_default_font);

//     ImGui::Columns(2, "tiles", false);
//     ImGui::SetColumnOffset(1, width + 10.0f);

//     p = ImGui::GetCursorScreenPos();

//     ImGui::Image((void*)(intptr_t)renderer_emu_debug_vram_tiles, ImVec2(width, height), ImVec2(0.0f, 0.0f), ImVec2(1.0f, (1.0f / 32.0f) * lines));

//     if (show_grid)
//     {
//         float x = p.x;
//         for (int n = 0; n <= 32; n++)
//         {
//             draw_list->AddLine(ImVec2(x, p.y), ImVec2(x, p.y + height), ImColor(dark_gray), 1.0f);
//             x += spacing;
//         }

//         float y = p.y;  
//         for (int n = 0; n <= lines; n++)
//         {
//             draw_list->AddLine(ImVec2(p.x, y), ImVec2(p.x + width, y), ImColor(dark_gray), 1.0f);
//             y += spacing;
//         }
//     }

//     int pattern_table_addr = (regs[4] & (mode == 2 ? 0x04 : 0x07)) << 11;

//     ImGui::TextColored(cyan, " Pattern Table Addr:"); ImGui::SameLine();
//     ImGui::Text("$%04X", pattern_table_addr);

//     float mouse_x = io.MousePos.x - p.x;
//     float mouse_y = io.MousePos.y - p.y;

//     int tile_x = -1;
//     int tile_y = -1;

//     if (ImGui::IsWindowHovered() && (mouse_x >= 0.0f) && (mouse_x < width) && (mouse_y >= 0.0f) && (mouse_y < height))
//     {
//         tile_x = (int)(mouse_x / spacing);
//         tile_y = (int)(mouse_y / spacing);

//         draw_list->AddRect(ImVec2(p.x + (tile_x * spacing), p.y + (tile_y * spacing)), ImVec2(p.x + ((tile_x + 1) * spacing), p.y + ((tile_y + 1) * spacing)), ImColor(cyan), 2.0f, ImDrawFlags_RoundCornersAll, 2.0f);

//         ImGui::NextColumn();

//         ImGui::Image((void*)(intptr_t)renderer_emu_debug_vram_tiles, ImVec2(128.0f, 128.0f), ImVec2((1.0f / 32.0f) * tile_x, (1.0f / 32.0f) * tile_y), ImVec2((1.0f / 32.0f) * (tile_x + 1), (1.0f / 32.0f) * (tile_y + 1)));

//         ImGui::TextColored(yellow, "DETAILS:");

//         int tile = (tile_y << 5) + tile_x;

//         int tile_addr = (pattern_table_addr + (tile << 3)) & 0x3FFF;

//         ImGui::TextColored(cyan, " Tile Number:"); ImGui::SameLine();
//         ImGui::Text("$%03X", tile); 
//         ImGui::TextColored(cyan, " Tile Addr:"); ImGui::SameLine();
//         ImGui::Text("$%04X", tile_addr); 

//         if (ImGui::IsMouseClicked(0))
//         {
//             mem_edit_select = 4;
//             mem_edit[4].JumpToAddress(tile_addr);
//         }
//     }

//     ImGui::Columns(1);

//     ImGui::PopFont();
// }

// static void debug_window_vram_sprites(void)
// {
//     float scale = 4.0f;
//     float size_8 = 8.0f * scale;
//     float size_16 = 16.0f * scale;

//     GearcolecoCore* core = emu_get_core();
//     Video* video = core->GetVideo();
//     u8* regs = video->GetRegisters();
//     u8* vram = video->GetVRAM();
//     GC_RuntimeInfo runtime;
//     emu_get_runtime(runtime);
//     bool sprites_16 = IsSetBit(regs[1], 1);

//     float width = 0.0f;
//     float height = 0.0f;

//     width = sprites_16 ? size_16 : size_8;
//     height = sprites_16 ? size_16 : size_8;

//     ImVec2 p[64];

//     ImGuiIO& io = ImGui::GetIO();

//     ImGui::PushFont(gui_default_font);

//     ImGui::Columns(2, "spr", false);
//     ImGui::SetColumnOffset(1, sprites_16 ? 330.0f : 200.0f);

//     ImGui::BeginChild("sprites", ImVec2(0, 0.0f), true);
//     bool window_hovered = ImGui::IsWindowHovered();

//     for (int s = 0; s < 32; s++)
//     {
//         p[s] = ImGui::GetCursorScreenPos();

//         ImGui::Image((void*)(intptr_t)renderer_emu_debug_vram_sprites[s], ImVec2(width, height), ImVec2(0.0f, 0.0f), ImVec2((1.0f / 16.0f) * (width / scale), (1.0f / 16.0f) * (height / scale)));

//         float mouse_x = io.MousePos.x - p[s].x;
//         float mouse_y = io.MousePos.y - p[s].y;

//         if (window_hovered && (mouse_x >= 0.0f) && (mouse_x < width) && (mouse_y >= 0.0f) && (mouse_y < height))
//         {
//             ImDrawList* draw_list = ImGui::GetWindowDrawList();
//             draw_list->AddRect(ImVec2(p[s].x, p[s].y), ImVec2(p[s].x + width, p[s].y + height), ImColor(cyan), 2.0f, ImDrawFlags_RoundCornersAll, 3.0f);
//         }

//         if (s % 4 < 3)
//             ImGui::SameLine();
//     }

//     ImGui::EndChild();

//     ImGui::NextColumn();

//     ImVec2 p_screen = ImGui::GetCursorScreenPos();

//     float screen_scale = 1.0f;
//     float tex_h = (float)runtime.screen_width / (float)(GC_RESOLUTION_WIDTH_WITH_OVERSCAN);
//     float tex_v = (float)runtime.screen_height / (float)(GC_RESOLUTION_HEIGHT_WITH_OVERSCAN);

//     ImGui::Image((void*)(intptr_t)renderer_emu_texture, ImVec2(runtime.screen_width * screen_scale, runtime.screen_height * screen_scale), ImVec2(0, 0), ImVec2(tex_h, tex_v));

//     for (int s = 0; s < 64; s++)
//     {
//         if ((p[s].x == 0) && (p[s].y == 0))
//             continue;

//         float mouse_x = io.MousePos.x - p[s].x;
//         float mouse_y = io.MousePos.y - p[s].y;

//         if (window_hovered && (mouse_x >= 0.0f) && (mouse_x < width) && (mouse_y >= 0.0f) && (mouse_y < height))
//         {
//             int x = 0;
//             int y = 0;
//             int tile = 0;
//             int sprite_tile_addr = 0;
//             int sprite_shift = 0;
//             int sprite_color = 0;
//             float real_x = 0.0f;
//             float real_y = 0.0f;

//             u16 sprite_attribute_addr = (regs[5] & 0x7F) << 7;
//             u16 sprite_pattern_addr = (regs[6] & 0x07) << 11;
//             int sprite_attribute_offset = sprite_attribute_addr + (s << 2);
//             tile = vram[sprite_attribute_offset + 2];
//             sprite_tile_addr = sprite_pattern_addr + (tile << 3);
//             sprite_shift = (vram[sprite_attribute_offset + 3] & 0x80) ? 32 : 0;
//             sprite_color = vram[sprite_attribute_offset + 3] & 0x0F;
//             x = vram[sprite_attribute_offset + 1];
//             y = vram[sprite_attribute_offset];

//             int final_y = (y + 1) & 0xFF;

//             if (final_y >= 0xE0)
//                 final_y = -(0x100 - final_y);

//             real_x = (float)(x - sprite_shift);
//             real_y = (float)final_y;

//             float max_width = 8.0f;
//             float max_height = sprites_16 ? 16.0f : 8.0f;

//             if (sprites_16)
//                 max_width = 16.0f;

//             if(IsSetBit(regs[1], 0))
//             {
//                 max_width *= 2.0f;
//                 max_height *= 2.0f;
//             }

//             float rectx_min = p_screen.x + (real_x * screen_scale);
//             float rectx_max = p_screen.x + ((real_x + max_width) * screen_scale);
//             float recty_min = p_screen.y + (real_y * screen_scale);
//             float recty_max = p_screen.y + ((real_y + max_height) * screen_scale);

//             rectx_min = fminf(fmaxf(rectx_min, p_screen.x), p_screen.x + (runtime.screen_width * screen_scale));
//             rectx_max = fminf(fmaxf(rectx_max, p_screen.x), p_screen.x + (runtime.screen_width * screen_scale));
//             recty_min = fminf(fmaxf(recty_min, p_screen.y), p_screen.y + (runtime.screen_height * screen_scale));
//             recty_max = fminf(fmaxf(recty_max, p_screen.y), p_screen.y + (runtime.screen_height * screen_scale));
            
//             ImDrawList* draw_list = ImGui::GetWindowDrawList();
//             draw_list->AddRect(ImVec2(rectx_min, recty_min), ImVec2(rectx_max, recty_max), ImColor(cyan), 2.0f, ImDrawFlags_RoundCornersAll, 2.0f);

//             ImGui::TextColored(yellow, "DETAILS:");
//             ImGui::TextColored(cyan, " Attribute Addr:"); ImGui::SameLine();
//             ImGui::Text("$%04X", sprite_attribute_offset);

//             ImGui::TextColored(cyan, " X:"); ImGui::SameLine();
//             ImGui::Text("$%02X", x);
//             ImGui::TextColored(cyan, " Y:"); ImGui::SameLine();
//             ImGui::Text("$%02X", y);

//             ImGui::TextColored(cyan, " Tile:"); ImGui::SameLine();
//             ImGui::Text("$%02X", tile);

//             ImGui::TextColored(cyan, " Tile Addr:"); ImGui::SameLine();
//             ImGui::Text("$%04X", sprite_tile_addr);

//             ImGui::TextColored(cyan, " Color:"); ImGui::SameLine();
//             ImGui::Text("$%02X", sprite_color);

//             ImGui::TextColored(cyan, " Early Clock:"); ImGui::SameLine();
//             sprite_shift > 0 ? ImGui::TextColored(green, "ON ") : ImGui::TextColored(gray, "OFF");

//             if (ImGui::IsMouseClicked(0))
//             {
//                 mem_edit_select = 4;
//                 mem_edit[4].JumpToAddress(sprite_tile_addr);
//             }
//         }
//     }

//     ImGui::Columns(1);

//     ImGui::PopFont();
// }

// static void debug_window_vram_regs(void)
// {
//     ImGui::PushFont(gui_default_font);

//     Video* video = emu_get_core()->GetVideo();
//     u8* regs = video->GetRegisters();

//     ImGui::TextColored(yellow, "VDP STATE:");

//     ImGui::TextColored(cyan, " PAL (50Hz)       "); ImGui::SameLine();
//     video->IsPAL() ? ImGui::TextColored(green, "YES ") : ImGui::TextColored(gray, "NO  ");
//     ImGui::TextColored(cyan, " LATCH FIRST BYTE "); ImGui::SameLine();
//     video->GetLatch() ? ImGui::TextColored(green, "YES ") : ImGui::TextColored(gray, "NO  ");
//     ImGui::TextColored(cyan, " INTERNAL BUFFER  "); ImGui::SameLine();
//     ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", video->GetBufferReg(), BYTE_TO_BINARY(video->GetBufferReg()));
//     ImGui::TextColored(cyan, " INTERNAL STATUS  "); ImGui::SameLine();
//     ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", video->GetStatusReg(), BYTE_TO_BINARY(video->GetStatusReg()));
//     ImGui::TextColored(cyan, " INTERNAL ADDRESS "); ImGui::SameLine();
//     ImGui::Text("$%04X", video->GetAddressReg());
//     ImGui::TextColored(cyan, " RENDER LINE      "); ImGui::SameLine();
//     ImGui::Text("%d", video->GetRenderLine());
//     ImGui::TextColored(cyan, " CYCLE COUNTER    "); ImGui::SameLine();
//     ImGui::Text("%d", video->GetCycleCounter());

//     ImGui::TextColored(yellow, "VDP REGISTERS:");

//     const char* reg_desc[] = {"CONTROL 0   ", "CONTROL 1   ", "PATTERN NAME", "COLOR TABLE ", "PATTERN GEN ", "SPRITE ATTR ", "SPRITE GEN  ", "COLORS      "};

//     for (int i = 0; i < 8; i++)
//     {
//         ImGui::TextColored(cyan, " $%01X ", i); ImGui::SameLine();
//         ImGui::TextColored(magenta, "%s ", reg_desc[i]); ImGui::SameLine();
//         ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", regs[i], BYTE_TO_BINARY(regs[i]));
//     }

//     ImGui::PopFont();
// }