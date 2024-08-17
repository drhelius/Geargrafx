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

#define GUI_DEBUG_DISASSEMBLER_IMPORT
#include "gui_debug_disassembler.h"

#include "imgui/imgui.h"
#include "imgui/fonts/IconsMaterialDesign.h"
#include "../../../src/geargrafx.h"
#include "gui_debug_constants.h"
#include "gui.h"
#include "config.h"
#include "emu.h"

struct DebugSymbol
{
    int bank;
    u16 address;
    char text[64];
};

struct DisassemblerLine
{
    u16 address;
    bool is_symbol;
    bool is_breakpoint;
    Memory::GG_Disassembler_Record* record;
    char symbol[64];
};

static std::vector<DebugSymbol> fixed_symbols;
static std::vector<DebugSymbol> dynamic_symbols;
static std::vector<DisassemblerLine> disassembler_lines(0x10000);
static int selected_address = -1;
static char new_breakpoint_buffer[10] = "";
static bool new_breakpoint_read = false;
static bool new_breakpoint_write = false;
static bool new_breakpoint_execute = true;
static char goto_address[5] = "";
static bool goto_address_requested = false;
static u16 goto_address_target = 0;
static bool goto_back_requested = false;
static int goto_back = 0;
static bool show_mem = true;
static bool show_symbols = true;
static bool show_segment = true;
static bool show_bank = true;
static int pc_pos = 0;
static int goto_address_pos = 0;

static void show_controls(void);
static void show_breakpoints(void);
static void prepare_drawable_lines(void);
static void show_disassembly(void);
static void add_symbol(const char* line);
static void add_auto_symbol(Memory::GG_Disassembler_Record* record, u16 address);
static void add_breakpoint(void);
static void request_goto_address(u16 addr);
static bool is_return_instruction(u8 opcode);

void gui_debug_reset(void)
{
    gui_debug_reset_breakpoints();
    gui_debug_reset_symbols();
    selected_address = -1;
}

void gui_debug_reset_symbols(void)
{
    fixed_symbols.clear();
    dynamic_symbols.clear();

    for (int i = 0; i < gui_debug_symbols_count; i++)
        add_symbol(gui_debug_symbols[i]);
}

void gui_debug_reset_breakpoints(void)
{
    emu_get_core()->GetHuC6280()->ResetBreakpoints();
    new_breakpoint_buffer[0] = 0;
}

void gui_debug_load_symbols_file(const char* path)
{
    // Log("Loading symbol file %s", path);

    // std::ifstream file(path);

    // if (file.is_open())
    // {
    //     std::string line;
    //     bool valid_section = true;

    //     while (std::getline(file, line))
    //     {
    //         size_t comment = line.find_first_of(';');
    //         if (comment != std::string::npos)
    //             line = line.substr(0, comment);
    //         line = line.erase(0, line.find_first_not_of(" \t\r\n"));
    //         line = line.erase(line.find_last_not_of(" \t\r\n") + 1);

    //         if (line.empty())
    //             continue;

    //         if (line.find("[") != std::string::npos)
    //         {
    //             valid_section = (line.find("[labels]") != std::string::npos);
    //             continue;
    //         }

    //         if (valid_section)
    //             add_symbol(line.c_str());
    //     }

    //     file.close();
    // }
}

void gui_debug_toggle_breakpoint(void)
{
    if (selected_address > 0)
    {
        if (emu_get_core()->GetHuC6280()->IsBreakpoint(selected_address))
            emu_get_core()->GetHuC6280()->RemoveBreakpoint(selected_address);
        else
            emu_get_core()->GetHuC6280()->AddBreakpoint(selected_address);
    }
}

void gui_debug_runtocursor(void)
{
    if (selected_address > 0)
    {
        emu_get_core()->GetHuC6280()->AddRunToBreakpoint(selected_address);
    }

    emu_debug_continue();
}

void gui_debug_go_back(void)
{
    goto_back_requested = true;
}

void gui_debug_window_disassembler(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(159, 31), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(463, 553), ImGuiCond_FirstUseEver);

    ImGui::Begin("Disassembler", &config_debug.show_disassembler);

    show_controls();

    ImGui::Separator();

    show_breakpoints();
    show_disassembly();

    ImGui::End();
    ImGui::PopStyleVar();
}

static void show_controls(void)
{
    ImGui::PushFont(gui_material_icons_font);

    if (ImGui::Button(ICON_MD_PLAY_ARROW))
    {
        emu_debug_continue();
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Continue (F5)");
    }

    ImGui::SameLine();
    if (ImGui::Button(ICON_MD_STOP))
    {
        emu_debug_break();
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Break (F7)");
    }

    ImGui::SameLine();
    if (ImGui::Button(ICON_MD_REDO))
    {
        emu_debug_step_over();
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Step Over (F10)");
    }

    ImGui::SameLine();
    if (ImGui::Button(ICON_MD_FILE_DOWNLOAD))
    {
        emu_debug_step_into();
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Step Into (F11)");
    }

    ImGui::SameLine();
    if (ImGui::Button(ICON_MD_FILE_UPLOAD))
    {
        emu_debug_step_out();
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Step Out (Shift+F11)");
    }

    ImGui::SameLine();
    if (ImGui::Button(ICON_MD_KEYBOARD_TAB))
    {
        gui_debug_runtocursor();
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Run to Cursor (F8)");
    }

    ImGui::SameLine();
    if (ImGui::Button(ICON_MD_INPUT))
    {
        emu_debug_step_frame();
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Step Frame (F6)");
    }

    ImGui::SameLine();
    if (ImGui::Button(ICON_MD_REPLAY))
    {
        emu_reset();
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Reset (CTRL+R)");
    }

    ImGui::PopFont();

    ImGui::Checkbox("Opcodes", &show_mem);  ImGui::SameLine();
    ImGui::Checkbox("Symbols", &show_symbols);  ImGui::SameLine();
    ImGui::Checkbox("Segment", &show_segment); ImGui::SameLine();
    ImGui::Checkbox("Bank", &show_bank);

    ImGui::Separator();

    ImGui::PushItemWidth(45);
    if (ImGui::InputTextWithHint("##goto_address", "XXXX", goto_address, IM_ARRAYSIZE(goto_address), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue))
    {
        try
        {
            request_goto_address((u16)std::stoul(goto_address, 0, 16));
        }
        catch(const std::invalid_argument&)
        {
        }
        goto_address[0] = 0;
    }
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Button("Go To Address", ImVec2(120, 0)))
    {
        try
        {
            request_goto_address((u16)std::stoul(goto_address, 0, 16));
        }
        catch(const std::invalid_argument&)
        {
        }
        goto_address[0] = 0;
    }

    ImGui::SameLine();
    if (ImGui::Button("Go Back", ImVec2(80, 0)))
    {
        goto_back_requested = true;
    }
}

static void show_breakpoints(void)
{
    if (ImGui::CollapsingHeader("Breakpoints"))
    {
        ImGui::Checkbox("Disable All##disable_mem", &emu_debug_disable_breakpoints);

        ImGui::Columns(2, "breakpoints");
        ImGui::SetColumnOffset(1, 100);

        ImGui::Separator();

        ImGui::PushItemWidth(85);
        if (ImGui::InputTextWithHint("##add_breakpoint", "XXXX-XXXX", new_breakpoint_buffer, IM_ARRAYSIZE(new_breakpoint_buffer), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue))
        {
            add_breakpoint();
        }
        ImGui::PopItemWidth();

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Use XXXX format for single addresses or XXXX-XXXX for address ranges");

        ImGui::Checkbox("Read", &new_breakpoint_read);
        ImGui::Checkbox("Write", &new_breakpoint_write);
        ImGui::Checkbox("Execute", &new_breakpoint_execute);

        if (ImGui::Button("Add##add", ImVec2(85, 0)))
        {
            add_breakpoint();
        }

        if (ImGui::Button("Clear All##clear_all", ImVec2(85, 0)))
        {
            gui_debug_reset_breakpoints();
        }

        ImGui::NextColumn();

        ImGui::BeginChild("breakpoints", ImVec2(0, 130), false);
        ImGui::PushFont(gui_default_font);

        int remove = -1;
        std::vector<HuC6280::GG_Breakpoint>* breakpoints = emu_get_core()->GetHuC6280()->GetBreakpoints();

        for (long unsigned int b = 0; b < breakpoints->size(); b++)
        {
            HuC6280::GG_Breakpoint* brk = &(*breakpoints)[b];

            ImGui::PushID(10000 + b);
            if (ImGui::SmallButton("X"))
            {
               remove = b;
               ImGui::PopID();
               continue;
            }

            ImGui::PopID();

            ImGui::SameLine();
            if ((*breakpoints)[b].range)
                ImGui::TextColored(red, "%04X-%04X", brk->address1, brk->address2);
            else
                ImGui::TextColored(red, "%04X", brk->address1);

            ImGui::SameLine(); ImGui::TextColored(brk->read ? orange : gray, " R");
            ImGui::SameLine(); ImGui::TextColored(brk->write ? orange : gray, "W");
            ImGui::SameLine(); ImGui::TextColored(brk->execute ? orange : gray, "X ");

            Memory::GG_Disassembler_Record* record = emu_get_core()->GetMemory()->GetDisassemblerRecord(brk->address1);

            if (brk->execute && IsValidPointer(record))
            {
                ImGui::SameLine(); ImGui::TextColored(white, "%s", record->name);
            }
        }

        ImGui::PopFont();

        if (remove >= 0)
        {
            breakpoints->erase(breakpoints->begin() + remove);
        }

        ImGui::EndChild();
        ImGui::Columns(1);
        ImGui::Separator();
    }
}

static void prepare_drawable_lines(void)
{
    Memory* memory = emu_get_core()->GetMemory();
    HuC6280* processor = emu_get_core()->GetHuC6280();
    HuC6280::HuC6280_State* proc_state = processor->GetState();
    u16 pc = proc_state->PC->GetValue();

    disassembler_lines.clear();
    dynamic_symbols.clear();
    pc_pos = 0;
    goto_address_pos = 0;

    for (int i = 0; i < 0x10000; i++)
    {
        Memory::GG_Disassembler_Record* record = memory->GetDisassemblerRecord(i);

        if (IsValidPointer(record) && (record->name[0] != 0))
            add_auto_symbol(record, i);
    }

    for (int i = 0; i < 0x10000; i++)
    {
        Memory::GG_Disassembler_Record* record = memory->GetDisassemblerRecord(i);

        if (IsValidPointer(record) && (record->name[0] != 0))
        {
            for (long unsigned int s = 0; s < fixed_symbols.size(); s++)
            {
                if ((fixed_symbols[s].bank == record->bank) && (fixed_symbols[s].address == i) && show_symbols)
                {
                    DisassemblerLine line;
                    line.address = (u16)i;
                    line.is_symbol = true;
                    snprintf(line.symbol, 64, "%s", fixed_symbols[s].text);
                    disassembler_lines.push_back(line);
                }
            }

            for (long unsigned int s = 0; s < dynamic_symbols.size(); s++)
            {
                if ((dynamic_symbols[s].bank == record->bank) && (dynamic_symbols[s].address == i) && show_symbols)
                {
                    DisassemblerLine line;
                    line.address = (u16)i;
                    line.is_symbol = true;
                    snprintf(line.symbol, 64, "%s", dynamic_symbols[s].text);
                    disassembler_lines.push_back(line);
                }
            }

            DisassemblerLine line;
            line.address = (u16)i;
            line.is_symbol = false;
            line.is_breakpoint = false;
            line.record = record;

            std::vector<HuC6280::GG_Breakpoint>* breakpoints = emu_get_core()->GetHuC6280()->GetBreakpoints();

            for (long unsigned int b = 0; b < breakpoints->size(); b++)
            {
                HuC6280::GG_Breakpoint* brk = &(*breakpoints)[b];

                if (brk->execute && (brk->address1 == i))
                {
                    line.is_breakpoint = true;
                    break;
                }
            }

            if ((u16)i == pc)
                pc_pos = disassembler_lines.size();

            if (goto_address_requested && (i <= goto_address_target))
            {
                goto_address_pos = disassembler_lines.size();
                if ((goto_address_pos > 0) && disassembler_lines[goto_address_pos - 1].is_symbol)
                    goto_address_pos--;
            }

            disassembler_lines.push_back(line);
        }
    }
}

static void show_disassembly(void)
{
    ImGui::PushFont(gui_default_font);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, mid_gray);

    bool window_visible = ImGui::BeginChild("##dis", ImVec2(ImGui::GetContentRegionAvail().x, 0), true, 0);

    if (window_visible)
    {
        HuC6280* processor = emu_get_core()->GetHuC6280();
        HuC6280::HuC6280_State* proc_state = processor->GetState();
        u16 pc = proc_state->PC->GetValue();

        prepare_drawable_lines();

        if (emu_debug_pc_changed)
        {
            emu_debug_pc_changed = false;
            float window_offset = ImGui::GetWindowHeight() / 2.0f;
            float offset = window_offset - (ImGui::GetTextLineHeightWithSpacing() - 2.0f);
            ImGui::SetScrollY((pc_pos * ImGui::GetTextLineHeightWithSpacing()) - offset);
        }

        if (goto_address_requested)
        {
            goto_address_requested = false;
            goto_back = (int)ImGui::GetScrollY();
            ImGui::SetScrollY((goto_address_pos * ImGui::GetTextLineHeightWithSpacing()) + 2);
        }

        if (goto_back_requested)
        {
            goto_back_requested = false;
            ImGui::SetScrollY((float)goto_back);
        }

        ImGuiListClipper clipper;
        clipper.Begin(disassembler_lines.size(), ImGui::GetTextLineHeightWithSpacing());

        while (clipper.Step())
        {
            for (int item = clipper.DisplayStart; item < clipper.DisplayEnd; item++)
            {
                DisassemblerLine line = disassembler_lines[item];

                if (line.is_symbol)
                {
                    ImGui::TextColored(green, "%s:", line.symbol);
                    continue;
                }

                ImGui::PushID(item);

                bool is_selected = (selected_address == line.address);

                if (ImGui::Selectable("", is_selected, ImGuiSelectableFlags_AllowDoubleClick))
                {
                    if (ImGui::IsMouseDoubleClicked(0) && line.record->jump)
                    {
                        request_goto_address(line.record->jump_address);
                    }
                    else if (is_selected)
                    {
                        selected_address = -1;
                        new_breakpoint_buffer[0] = 0;
                    }
                    else
                        selected_address = line.address;
                }

                bool enable_bg_color = false;
                ImVec4 bg_color;

                if (is_selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
                else if (line.is_breakpoint && !ImGui::IsItemHovered())
                {
                    enable_bg_color = true;
                    bg_color = dark_red;
                }
                else if ((line.address == pc) && !ImGui::IsItemHovered())
                {
                    enable_bg_color = true;
                    bg_color = dark_yellow;
                }
                else if (line.record->subroutine && !ImGui::IsItemHovered())
                {
                    enable_bg_color = true;
                    bg_color = dark_gray;
                }

                if (enable_bg_color)
                {
                    ImVec2 p_min = ImGui::GetItemRectMin();
                    ImVec2 p_max = ImGui::GetItemRectMax();
                    ImGui::GetWindowDrawList()->AddRectFilled(p_min, p_max, ImGui::GetColorU32(bg_color));
                }

                ImVec4 color_segment = line.is_breakpoint ? red : magenta;
                ImVec4 color_bank = line.is_breakpoint ? red : violet;
                ImVec4 color_addr = line.is_breakpoint ? red : cyan;
                ImVec4 color_mem = line.is_breakpoint ? red : mid_gray;
                ImVec4 color_name = line.is_breakpoint ? red : white;

                if (show_segment)
                {
                    ImGui::SameLine();
                    ImGui::TextColored(color_segment, "%s", line.record->segment);
                }

                if (show_bank)
                {
                    ImGui::SameLine();
                    ImGui::TextColored(color_bank, "%02X", line.record->bank);
                }

                ImGui::SameLine();
                ImGui::TextColored(color_addr, "%04X", line.address);

                ImGui::SameLine();
                if (line.address == pc)
                {
                    ImGui::TextColored(yellow, " ->");
                    color_name = yellow;
                }
                else
                {
                    ImGui::TextColored(yellow, "   ");
                }

                ImGui::SameLine();
                ImGui::TextColored(color_name, "%s", line.record->name);

                if (show_mem)
                {
                    char spaces[32];
                    int offset = 21 - strlen(line.record->name);
                    if (offset < 0)
                        offset = 0;
                    for (int i = 0; i < offset; i++)
                        spaces[i] = ' ';
                    spaces[offset] = 0;
                    ImGui::SameLine();
                    ImGui::TextColored(color_mem, "%s;%s", spaces, line.record->bytes);
                }

                bool is_ret = is_return_instruction(line.record->opcodes[0]);
                if (is_ret)
                {
                    ImGui::PushStyleColor(ImGuiCol_Separator, dark_green);
                    ImGui::Separator();
                    ImGui::PopStyleColor();
                }

                ImGui::PopID();
            }
        }
    }

    ImGui::EndChild();

    ImGui::PopStyleColor();
    ImGui::PopFont();
}

static void add_symbol(const char* line)
{
    Debug("Loading symbol %s", line);

    DebugSymbol s;
    std::string str(line);

    str.erase(std::remove(str.begin(), str.end(), '\r'), str.end());
    str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());

    size_t first = str.find_first_not_of(' ');
    if (std::string::npos == first)
    {
        str = "";
    }
    else
    {
        size_t last = str.find_last_not_of(' ');
        str = str.substr(first, (last - first + 1));
    }

    std::size_t comment = str.find(";");

    if (comment != std::string::npos)
        str = str.substr(0 , comment);

    std::size_t space = str.find(" ");

    if (space != std::string::npos)
    {
        snprintf(s.text, 64, "%s", str.substr(space + 1 , std::string::npos).c_str());
        str = str.substr(0, space);

        std::size_t separator = str.find(":");

        try
        {
            if (separator != std::string::npos)
            {
                s.address = (u16)std::stoul(str.substr(separator + 1 , std::string::npos), 0, 16);
                s.bank = std::stoul(str.substr(0, separator), 0 , 16);
            }
            else
            {
                s.address = (u16)std::stoul(str, 0, 16);
                s.bank = 0;
            }

            fixed_symbols.push_back(s);
        }
        catch(const std::invalid_argument&)
        {
        }
    }
}

static const char* k_irq_symbol_format[6] = {
    "????_%02X_%04X",
    "RESET_%02X_%04X",
    "NMI_%02X_%04X",
    "TIMER_IRQ_%02X_%04X",
    "IRQ1_%02X_%04X",
    "IRQ2_BRK_%02X_%04X"
};

static void add_auto_symbol(Memory::GG_Disassembler_Record* record, u16 address)
{
    DebugSymbol s;
    s.bank = record->bank;
    s.address = address;
    bool insert = false;

    if (record->jump)
    {
        s.address = record->jump_address;
        s.bank = record->jump_bank;
        if (record->subroutine)
            snprintf(s.text, 64, "SUBROUTINE_%02X_%04X", record->jump_bank, record->jump_address);
        else
            snprintf(s.text, 64, "LABEL_%02X_%04X", record->jump_bank, record->jump_address);
        insert = true;
    }
    else if (record->irq > 0)
    {
        snprintf(s.text, 64, k_irq_symbol_format[record->irq], record->bank, address);
        insert = true;
    }

    if (insert)
    {
        for (long unsigned int i = 0; i < dynamic_symbols.size(); i++)
        {
            if ((dynamic_symbols[i].bank == s.bank) && (dynamic_symbols[i].address == s.address))
            {
                if (record->subroutine)
                    snprintf(dynamic_symbols[i].text, 64, "SUBROUTINE_%02X_%04X", record->jump_bank, record->jump_address);
                return;
            }
        }

        dynamic_symbols.push_back(s);
    }
}

static void add_breakpoint(void)
{
    if (emu_get_core()->GetHuC6280()->AddBreakpoint(new_breakpoint_buffer, new_breakpoint_read, new_breakpoint_write, new_breakpoint_execute))
        new_breakpoint_buffer[0] = 0;
}

static void request_goto_address(u16 address)
{
    goto_address_requested = true;
    goto_address_target = address;
}

static bool is_return_instruction(u8 opcode)
{
    switch (opcode)
    {
        case 0x60: // RTS
        case 0x40: // RTI
            return true;
        default:
            return false;
    }
}