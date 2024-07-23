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

#define GUI_DEBUG_MEMORY_IMPORT
#include "gui_debug_memory.h"

#include "../../../src/geargrafx.h"
#include "imgui/imgui.h"
#include "gui_debug_memeditor.h"
#include "config.h"
#include "gui.h"
#include "emu.h"

static MemEditor mem_edit[5];
static int mem_edit_select = -1;
static int current_mem_edit = 0;

void gui_debug_window_memory(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(567, 249), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(324, 308), ImGuiCond_FirstUseEver);

    ImGui::Begin("Memory Editor", &config_debug.show_memory);

    GeargrafxCore* core = emu_get_core();
    Memory* memory = core->GetMemory();
    Cartridge* cart = core->GetCartridge();
    HuC6270* huc6270 = core->GetHuC6270();

    if (ImGui::BeginTabBar("##memory_tabs", ImGuiTabBarFlags_None))
    {
        if (ImGui::BeginTabItem("RAM", NULL, mem_edit_select == 0 ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None))
        {
            ImGui::PushFont(gui_default_font);
             if (mem_edit_select == 0)
                mem_edit_select = -1;
            current_mem_edit = 0;
            mem_edit[current_mem_edit].Draw(memory->GetWram(), 0x2000);
            ImGui::PopFont();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("ZERO PAGE", NULL, mem_edit_select == 1 ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None))
        {
            ImGui::PushFont(gui_default_font);
             if (mem_edit_select == 1)
                mem_edit_select = -1;
            current_mem_edit = 1;
            mem_edit[current_mem_edit].Draw(memory->GetWram(), 0x100);
            ImGui::PopFont();
            ImGui::EndTabItem();
        }

        if (IsValidPointer(cart->GetROM()) && ImGui::BeginTabItem("ROM", NULL, mem_edit_select == 2 ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None))
        {
            ImGui::PushFont(gui_default_font);
            if (mem_edit_select == 2)
                mem_edit_select = -1;
            current_mem_edit = 2;
            mem_edit[current_mem_edit].Draw(cart->GetROM(), cart->GetROMSize());
            ImGui::PopFont();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("VRAM", NULL, mem_edit_select == 3 ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None))
        {
            ImGui::PushFont(gui_default_font);
            if (mem_edit_select == 3)
                mem_edit_select = -1;
            current_mem_edit = 3;
            mem_edit[current_mem_edit].Draw((u8*)huc6270->GetVRAM(), HUC6270_VRAM_SIZE, 0, 2);
            ImGui::PopFont();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

void gui_debug_copy_memory(void)
{
    int size = 0;
    u8* data = NULL;
    mem_edit[current_mem_edit].Copy(&data, &size);

    if (IsValidPointer(data))
    {
        std::string text;

        for (int i = 0; i < size; i++)
        {
            char byte[3];
            sprintf(byte, "%02X", data[i]);
            if (i > 0)
                text += " ";
            text += byte;
        }

        SDL_SetClipboardText(text.c_str());
    }
}

void gui_debug_paste_memory(void)
{
    char* clipboard = SDL_GetClipboardText();

    if (IsValidPointer(clipboard))
    {
        std::string text(clipboard);

        text.erase(std::remove(text.begin(), text.end(), ' '), text.end());

        size_t buffer_size = text.size() / 2;
        u8* data = new u8[buffer_size];

        for (size_t i = 0; i < buffer_size; i ++)
        {
            std::string byte = text.substr(i * 2, 2);

            try
            {
                data[i] = (u8)std::stoul(byte, 0, 16);
            }
            catch(const std::invalid_argument&)
            {
                delete[] data;
                return;
            }
        }

        mem_edit[current_mem_edit].Paste(data, buffer_size);

        delete[] data;
    }

    SDL_free(clipboard);
}
