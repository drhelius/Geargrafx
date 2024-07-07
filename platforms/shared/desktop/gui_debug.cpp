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

#define GUI_DEBUG_IMPORT
#include "gui_debug.h"

#include "../../../src/geargrafx.h"
#include "imgui/imgui.h"
#include "gui_debug_disassembler.h"
#include "gui_debug_huc6280.h"
#include "gui_debug_huc6270.h"
#include "gui_debug_memory.h"
#include "gui_debug_constants.h"
#include "config.h"
#include "emu.h"
#include "gui.h"
#include "utils.h"

static MemEditor mem_edit[5];
static int mem_edit_select = -1;
static int current_mem_edit = 0;

static void debug_window_processor(void);
static void debug_window_memory(void);

void gui_debug_windows(void)
{
    if (config_debug.debug)
    {
        if (config_debug.show_processor)
            debug_window_processor();
        if (config_debug.show_memory)
            debug_window_memory();
        if (config_debug.show_disassembler)
            gui_debug_window_disassembler();
        // if (config_debug.show_video)
        //     debug_window_vram();
        // if (config_debug.show_video_registers)
        //     debug_window_vram_registers();
    }
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

static void debug_window_memory(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(567, 249), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(324, 308), ImGuiCond_FirstUseEver);

    ImGui::Begin("Memory Editor", &config_debug.show_memory);

    // GearcolecoCore* core = emu_get_core();
    // Memory* memory = core->GetMemory();
    // Cartridge* cart = core->GetCartridge();
    // Video* video = core->GetVideo();

    ImGui::PushFont(gui_default_font);

    // ImGui::TextColored(cyan, "  ROM: ");ImGui::SameLine();

    // ImGui::TextColored(magenta, "BANK");ImGui::SameLine();
    // ImGui::Text("$%02X", memory->GetRomBank()); ImGui::SameLine();
    // ImGui::TextColored(magenta, "  ADDRESS");ImGui::SameLine();
    // ImGui::Text("$%05X", memory->GetRomBankAddress());

    ImGui::PopFont();

    if (ImGui::BeginTabBar("##memory_tabs", ImGuiTabBarFlags_None))
    {
        // if (ImGui::BeginTabItem("BIOS", NULL, mem_edit_select == 0 ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None))
        // {
        //     ImGui::PushFont(gui_default_font);
        //     if (mem_edit_select == 0)
        //         mem_edit_select = -1;
        //     current_mem_edit = 0;
        //     mem_edit[current_mem_edit].Draw(memory->GetBios(), 0x2000, 0);
        //     ImGui::PopFont();
        //     ImGui::EndTabItem();
        // }

        // if (ImGui::BeginTabItem("RAM", NULL, mem_edit_select == 1 ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None))
        // {
        //     ImGui::PushFont(gui_default_font);
        //      if (mem_edit_select == 1)
        //         mem_edit_select = -1;
        //     current_mem_edit = 1;
        //     mem_edit[current_mem_edit].Draw(memory->GetRam(), 0x400, 0x7000);
        //     ImGui::PopFont();
        //     ImGui::EndTabItem();
        // }

        // if (ImGui::BeginTabItem("SGM RAM", NULL, mem_edit_select == 2 ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None))
        // {
        //     ImGui::PushFont(gui_default_font);
        //     if (mem_edit_select == 2)
        //         mem_edit_select = -1;
        //     current_mem_edit = 2;
        //     mem_edit[current_mem_edit].Draw(memory->GetSGMRam(), 0x8000, 0x0000);
        //     ImGui::PopFont();
        //     ImGui::EndTabItem();
        // }

        // if (IsValidPointer(cart->GetROM()) && ImGui::BeginTabItem("ROM", NULL, mem_edit_select == 3 ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None))
        // {
        //     ImGui::PushFont(gui_default_font);
        //     if (mem_edit_select == 3)
        //         mem_edit_select = -1;
        //     current_mem_edit = 3;
        //     mem_edit[current_mem_edit].Draw(cart->GetROM(), cart->GetROMSize(), 0x0000);
        //     ImGui::PopFont();
        //     ImGui::EndTabItem();
        // }

        // if (ImGui::BeginTabItem("VRAM", NULL, mem_edit_select == 4 ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None))
        // {
        //     ImGui::PushFont(gui_default_font);
        //     if (mem_edit_select == 4)
        //         mem_edit_select = -1;
        //     current_mem_edit = 4;
        //     mem_edit[current_mem_edit].Draw(video->GetVRAM(), 0x4000, 0);
        //     ImGui::PopFont();
        //     ImGui::EndTabItem();
        // }

        ImGui::EndTabBar();
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

static void debug_window_processor(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(6, 31), ImGuiCond_FirstUseEver);

    ImGui::Begin("HuC6280", &config_debug.show_processor, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize);

    ImGui::PushFont(gui_default_font);

    GeargrafxCore* core = emu_get_core();
    HuC6280* processor = core->GetHuC6280();
    HuC6280::Processor_State* proc_state = processor->GetState();
    Memory* memory = core->GetMemory();

    ImGui::TextColored(cyan, "      STATUS");
    ImGui::TextColored(orange, "  N V T B D I Z C");
    ImGui::Text("  " BYTE_TO_BINARY_PATTERN_ALL_SPACED, BYTE_TO_BINARY(proc_state->P->GetValue()));

    ImGui::Columns(2, "registers");
    ImGui::Separator();
    ImGui::TextColored(cyan, "  A"); ImGui::SameLine();
    ImGui::Text(" $%02X", proc_state->A->GetValue());
    ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->A->GetValue()));

    ImGui::NextColumn();
    ImGui::TextColored(cyan, "  S"); ImGui::SameLine();
    ImGui::Text(" $%02X", proc_state->S->GetValue());
    ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->S->GetValue()));

    ImGui::NextColumn();
    ImGui::Separator();
    ImGui::TextColored(cyan, "  X"); ImGui::SameLine();
    ImGui::Text(" $%02X", proc_state->X->GetValue());
    ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->X->GetValue()));

    ImGui::NextColumn();
    ImGui::TextColored(cyan, "  Y"); ImGui::SameLine();
    ImGui::Text(" $%02X", proc_state->Y->GetValue());
    ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->Y->GetValue()));

    ImGui::NextColumn();
    ImGui::Separator();
    ImGui::TextColored(cyan, "MPR0"); ImGui::SameLine();
    ImGui::Text("$%02X", memory->GetMpr(0));
    ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(memory->GetMpr(0)));

    ImGui::NextColumn();
    ImGui::TextColored(cyan, "MPR1"); ImGui::SameLine();
    ImGui::Text("$%02X", memory->GetMpr(1));
    ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(memory->GetMpr(1)));

    ImGui::NextColumn();
    ImGui::Separator();
    ImGui::TextColored(cyan, "MPR2"); ImGui::SameLine();
    ImGui::Text("$%02X", memory->GetMpr(2));
    ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(memory->GetMpr(2)));

    ImGui::NextColumn();
    ImGui::TextColored(cyan, "MPR3"); ImGui::SameLine();
    ImGui::Text("$%02X", memory->GetMpr(3));
    ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(memory->GetMpr(3)));

    ImGui::NextColumn();
    ImGui::Separator();
    ImGui::TextColored(cyan, "MPR4"); ImGui::SameLine();
    ImGui::Text("$%02X", memory->GetMpr(4));
    ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(memory->GetMpr(4)));

    ImGui::NextColumn();
    ImGui::TextColored(cyan, "MPR5"); ImGui::SameLine();
    ImGui::Text("$%02X", memory->GetMpr(5));
    ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(memory->GetMpr(5)));

    ImGui::NextColumn();
    ImGui::Separator();
    ImGui::TextColored(cyan, "MPR6"); ImGui::SameLine();
    ImGui::Text("$%02X", memory->GetMpr(6));
    ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(memory->GetMpr(6)));

    ImGui::NextColumn();
    ImGui::TextColored(cyan, "MPR7"); ImGui::SameLine();
    ImGui::Text("$%02X", memory->GetMpr(7));
    ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(memory->GetMpr(7)));

    ImGui::NextColumn();
    ImGui::Columns(1);

    ImGui::Separator();
    ImGui::TextColored(yellow, "    SP"); ImGui::SameLine();
    ImGui::Text("= $%04X", 0x2100 | proc_state->S->GetValue());
    ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(0x21), BYTE_TO_BINARY(proc_state->S->GetValue()));

    ImGui::Separator();
    ImGui::TextColored(yellow, "    PC"); ImGui::SameLine();
    ImGui::Text("= $%04X", proc_state->PC->GetValue());
    ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->PC->GetHigh()), BYTE_TO_BINARY(proc_state->PC->GetLow()));

    ImGui::Separator();

    ImGui::TextColored(!*proc_state->SPEED ? green : gray, " 1.79 MHz"); ImGui::SameLine();
    ImGui::TextColored(*proc_state->SPEED ? green : gray, "7.16 MHz");

    // ImGui::TextColored(*proc_state->IFF1 ? green : gray, " IFF1"); ImGui::SameLine();
    // ImGui::TextColored(*proc_state->IFF2 ? green : gray, " IFF2"); ImGui::SameLine();
    // ImGui::TextColored(*proc_state->Halt ? green : gray, " HALT");
    
    // ImGui::TextColored(*proc_state->INT ? green : gray, "    INT"); ImGui::SameLine();
    // ImGui::TextColored(*proc_state->NMI ? green : gray, "  NMI");

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}



