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
#include "gui_debug_text.h"
#include "gui.h"
#include "gui_filedialogs.h"
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
    bool is_breakpoint;
    Memory::GG_Disassembler_Record* record;
    char name_enhanced[64];
    int name_real_length;
    DebugSymbol* symbol;
};

struct DisassemblerBookmark
{
    u16 address;
    char name[32];
};

static DebugSymbol*** fixed_symbols = NULL;
static DebugSymbol*** dynamic_symbols = NULL;
static std::vector<DisassemblerLine> disassembler_lines(0x10000);
static std::vector<DisassemblerBookmark> bookmarks;
static int selected_address = -1;
static int new_breakpoint_type = HuC6280::HuC6280_BREAKPOINT_TYPE_ROMRAM;
static char new_breakpoint_buffer[10] = "";
static bool new_breakpoint_read = false;
static bool new_breakpoint_write = false;
static bool new_breakpoint_execute = true;
static char goto_address[5] = "";
static bool goto_address_requested = false;
static u16 goto_address_target = 0;
static bool goto_back_requested = false;
static int goto_back = 0;
static int pc_pos = 0;
static int goto_address_pos = 0;
static bool add_bookmark = false;

static void draw_controls(void);
static void draw_breakpoints(void);
static void prepare_drawable_lines(void);
static void draw_disassembly(void);
static void draw_context_menu(DisassemblerLine* line);
static void add_symbol(const char* line);
static void add_auto_symbol(Memory::GG_Disassembler_Record* record, u16 address);
static void add_breakpoint(int type);
static void request_goto_address(u16 addr);
static bool is_return_instruction(u8 opcode);
static void replace_symbols(DisassemblerLine* line, const char* color);
static void replace_labels(DisassemblerLine* line, const char* color, const char* original_color);
static void draw_instruction_name(DisassemblerLine* line, bool is_pc);
static void disassembler_menu(void);
static void add_bookmark_popup(void);

void gui_debug_disassembler_init(void)
{
    fixed_symbols = new DebugSymbol**[0x100];
    dynamic_symbols = new DebugSymbol**[0x100];

    for (int i = 0; i < 0x100; i++)
    {
        fixed_symbols[i] = new DebugSymbol*[0x10000];
        dynamic_symbols[i] = new DebugSymbol*[0x10000];
    }

    for (int i = 0; i < 0x100; i++)
    {
        for (int j = 0; j < 0x10000; j++)
        {
            InitPointer(fixed_symbols[i][j]);
            InitPointer(dynamic_symbols[i][j]);
        }
    }
}

void gui_debug_disassembler_destroy(void)
{
    for (int i = 0; i < 0x100; i++)
    {
        for (int j = 0; j < 0x10000; j++)
        {
            SafeDelete(fixed_symbols[i][j]);
            SafeDelete(dynamic_symbols[i][j]);
        }

        SafeDeleteArray(fixed_symbols[i]);
        SafeDeleteArray(dynamic_symbols[i]);
    }

    SafeDeleteArray(fixed_symbols);
    SafeDeleteArray(dynamic_symbols);
}

void gui_debug_reset(void)
{
    gui_debug_reset_breakpoints();
    gui_debug_reset_symbols();
    selected_address = -1;
}

void gui_debug_reset_symbols(void)
{
    for (int i = 0; i < 0x100; i++)
    {
        for (int j = 0; j < 0x10000; j++)
        {
            SafeDelete(fixed_symbols[i][j]);
            SafeDelete(dynamic_symbols[i][j]);
        }
    }
}

void gui_debug_reset_breakpoints(void)
{
    emu_get_core()->GetHuC6280()->ResetBreakpoints();
    new_breakpoint_buffer[0] = 0;
}

void gui_debug_load_symbols_file(const char* file_path)
{
    std::ifstream file(file_path);

    if (file.is_open())
    {
        Log("Loading symbol file %s", file_path);

        std::string line;
        bool valid_section = true;

        while (std::getline(file, line))
        {
            size_t comment = line.find_first_of(';');
            if (comment != std::string::npos)
                line = line.substr(0, comment);
            line = line.erase(0, line.find_first_not_of(" \t\r\n"));
            line = line.erase(line.find_last_not_of(" \t\r\n") + 1);
            while (line[0] == ' ')
                line = line.substr(1);

            if (line.find("Bank") == 0)
                continue;

            if (line.find("-") == 0)
                continue;

            if (line.empty())
                continue;

            if (line.find("[") != std::string::npos)
            {
                valid_section = (line.find("[labels]") != std::string::npos);
                continue;
            }

            if (valid_section)
                add_symbol(line.c_str());
        }

        file.close();
    }
    else
    {
        Debug("Symbol file %s not found", file_path);
    }
}

void gui_debug_toggle_breakpoint(void)
{
    if (selected_address > 0)
    {
        if (emu_get_core()->GetHuC6280()->IsBreakpoint(HuC6280::HuC6280_BREAKPOINT_TYPE_ROMRAM, selected_address))
            emu_get_core()->GetHuC6280()->RemoveBreakpoint(HuC6280::HuC6280_BREAKPOINT_TYPE_ROMRAM, selected_address);
        else
            emu_get_core()->GetHuC6280()->AddBreakpoint(selected_address);
    }
}

void gui_debug_add_bookmark(void)
{
    add_bookmark = true;
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
    ImGui::SetNextWindowPos(ImVec2(155, 26), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(458, 553), ImGuiCond_FirstUseEver);

    ImGui::Begin("Disassembler", &config_debug.show_disassembler, ImGuiWindowFlags_MenuBar);

    disassembler_menu();
    draw_controls();

    ImGui::Separator();

    draw_breakpoints();
    draw_disassembly();

    add_bookmark_popup();

    ImGui::End();
    ImGui::PopStyleVar();
}

void gui_debug_save_disassembler(const char* file_path)
{
    FILE* file = fopen(file_path, "w");

    Memory* memory = emu_get_core()->GetMemory();
    Memory::GG_Disassembler_Record** records = memory->GetAllDisassemblerRecords();

    for (int i = 0; i < 0x200000; i++)
    {
        Memory::GG_Disassembler_Record* record = records[i];

        if (IsValidPointer(record) && (record->name[0] != 0))
        {
            if (record->subroutine || record->irq)
                fprintf(file, "\n");

            char name[64];
            strcpy(name, record->name);
            RemoveColorFromString(name);

            int len = (int)strlen(name);
            char spaces[32];
            int offset = 28 - len;
            if (offset < 0)
                offset = 0;
            for (int i = 0; i < offset; i++)
                spaces[i] = ' ';
            spaces[offset] = 0;

            fprintf(file, "%06X-%02X:    %s%s;%s\n", i, record->bank, name, spaces, record->bytes);

            if (is_return_instruction(record->opcodes[0]))
                fprintf(file, "\n");
        }
    }

    fclose(file);
}

static void draw_controls(void)
{
    ImGui::PushFont(gui_material_icons_font);

    if (ImGui::Button(ICON_MD_PLAY_ARROW))
    {
        emu_debug_continue();
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Start (F5)");
    }

    ImGui::SameLine();
    if (ImGui::Button(ICON_MD_STOP))
    {
        emu_debug_break();
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Stop (F7)");
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
    if (ImGui::Button(ICON_MD_INPUT))
    {
        emu_debug_step_frame();
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Step Frame (F6)");
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
    if (ImGui::Button(ICON_MD_REPLAY))
    {
        emu_reset();
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Reset (CTRL+R)");
    }

    ImGui::PopFont();
}

static const char* k_breakpoint_types[] = { "ROM/RAM ", "VRAM    ", "PALETTE ", "6270 REG", "6260 REG" };

static void draw_breakpoints(void)
{
    if (ImGui::CollapsingHeader("Breakpoints"))
    {
        ImGui::Checkbox("Break On IRQs##irq_break", &emu_debug_irq_breakpoints); ImGui::SameLine();
        ImGui::Checkbox("Disable All##disable_mem", &emu_debug_disable_breakpoints); ImGui::SameLine();

        if (ImGui::Button("Remove All##clear_all", ImVec2(85, 0)))
        {
            gui_debug_reset_breakpoints();
        }

        ImGui::Columns(2, "breakpoints");
        ImGui::SetColumnOffset(1, 130);

        ImGui::Separator();

        ImGui::PushItemWidth(120);
        ImGui::Combo("Type##type", &new_breakpoint_type, "ROM/RAM\0VRAM\0Palette RAM\0HuC6270 Reg\0HuC6260 Reg\0");

        ImGui::PushItemWidth(85);
        if (ImGui::InputTextWithHint("##add_breakpoint", "XXXX-XXXX", new_breakpoint_buffer, IM_ARRAYSIZE(new_breakpoint_buffer), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue))
        {
            add_breakpoint(new_breakpoint_type);
        }
        ImGui::PopItemWidth();

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Use hex XXXX format for single addresses or XXXX-XXXX for address ranges");

        ImGui::Checkbox("Read", &new_breakpoint_read);
        ImGui::Checkbox("Write", &new_breakpoint_write);

        if (new_breakpoint_type == HuC6280::HuC6280_BREAKPOINT_TYPE_ROMRAM)
            ImGui::Checkbox("Execute", &new_breakpoint_execute);

        if (ImGui::Button("Add##add", ImVec2(85, 0)))
        {
            add_breakpoint(new_breakpoint_type);
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

            ImGui::PushID(20000 + b);
            if (ImGui::SmallButton(brk->enabled ? "-" : "+"))
            {
                brk->enabled = !brk->enabled;
            }
            ImGui::PopID();

            ImGui::SameLine(); ImGui::TextColored(brk->enabled ? red : gray, "%s", k_breakpoint_types[brk->type]); ImGui::SameLine();

            if ((*breakpoints)[b].range)
                ImGui::TextColored(brk->enabled ? cyan : gray, "%04X-%04X", brk->address1, brk->address2);
            else
                ImGui::TextColored(brk->enabled ? cyan : gray, "%04X", brk->address1);

            ImGui::SameLine(); ImGui::TextColored(brk->enabled && brk->read ? orange : gray, " R");
            ImGui::SameLine(0, 2); ImGui::TextColored(brk->enabled && brk->write ? orange : gray, "W");

            if (brk->type == HuC6280::HuC6280_BREAKPOINT_TYPE_ROMRAM)
            {
                ImGui::SameLine(0, 2); ImGui::TextColored(brk->enabled && brk->execute ? orange : gray, "X");
            }

            Memory::GG_Disassembler_Record* record = emu_get_core()->GetMemory()->GetDisassemblerRecord(brk->address1);

            if (brk->execute && IsValidPointer(record))
            {
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Text, brk->enabled ? white : gray);
                TextColoredEx(" %s", record->name);
                ImGui::PopStyleColor();
            }
            else if (!brk->range && (brk->type == HuC6280::HuC6280_BREAKPOINT_TYPE_HUC6270_REGISTER) && (brk->address1 < 20))
            {
                ImGui::SameLine();
                ImGui::TextColored(brk->enabled ? violet : gray, " %s", k_register_names[brk->address1]);
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
            bool fixed_symbol_found = false;
            if (config_debug.dis_show_symbols)
            {
                DebugSymbol* symbol = fixed_symbols[record->bank][i];

                if (IsValidPointer(symbol))
                {
                    DisassemblerLine line;
                    line.address = (u16)i;
                    line.symbol = symbol;
                    disassembler_lines.push_back(line);
                    fixed_symbol_found = true;
                }
            }

            if (config_debug.dis_show_symbols && config_debug.dis_show_auto_symbols && !fixed_symbol_found)
            {
                DebugSymbol* symbol = dynamic_symbols[record->bank][i];

                if (IsValidPointer(symbol))
                {
                    DisassemblerLine line;
                    line.address = (u16)i;
                    line.symbol = symbol;
                    disassembler_lines.push_back(line);
                }
            }

            DisassemblerLine line;
            line.address = (u16)i;
            line.symbol = NULL;
            line.is_breakpoint = false;
            line.record = record;
            snprintf(line.name_enhanced, 64, "%s", line.record->name);

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
                pc_pos = (int)disassembler_lines.size();

            if (goto_address_requested && (i <= goto_address_target))
            {
                goto_address_pos = (int)disassembler_lines.size();
                if ((goto_address_pos > 0) && disassembler_lines[goto_address_pos - 1].symbol)
                    goto_address_pos--;
            }

            disassembler_lines.push_back(line);
        }
    }
}

static void draw_disassembly(void)
{
    ImGui::PushFont(gui_default_font);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, mid_gray);

    bool window_visible = ImGui::BeginChild("##dis", ImVec2(ImGui::GetContentRegionAvail().x, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

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
        clipper.Begin((int)disassembler_lines.size(), ImGui::GetTextLineHeightWithSpacing());

        while (clipper.Step())
        {
            for (int item = clipper.DisplayStart; item < clipper.DisplayEnd; item++)
            {
                DisassemblerLine line = disassembler_lines[item];

                if (line.symbol)
                {
                    ImGui::TextColored(green, "%s:", line.symbol->text);
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

                draw_context_menu(&line);

                ImVec4 color_segment = line.is_breakpoint ? red : magenta;
                ImVec4 color_bank = line.is_breakpoint ? red : violet;
                ImVec4 color_addr = line.is_breakpoint ? red : cyan;
                ImVec4 color_mem = line.is_breakpoint ? red : mid_gray;

                if (config_debug.dis_show_segment)
                {
                    ImGui::SameLine();
                    ImGui::TextColored(color_segment, "%s", line.record->segment);
                }

                if (config_debug.dis_show_bank)
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
                }
                else
                {
                    ImGui::TextColored(yellow, "   ");
                }

                ImGui::SameLine();
                draw_instruction_name(&line, line.address == pc);

                if (config_debug.dis_show_mem)
                {
                    int len = line.name_real_length;
                    char spaces[32];
                    int offset = 28 - len;
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

static void draw_context_menu(DisassemblerLine* line)
{
    ImGui::PopFont();
    if (ImGui::BeginPopupContextItem())
    {
        selected_address = line->address;

        if (ImGui::Selectable("Run To Cursor"))
        {
            gui_debug_runtocursor();
        }

        if (ImGui::Selectable("Add Bookmark..."))
        {
            gui_debug_add_bookmark();
        }

        if (ImGui::Selectable("Toggle Breakpoint"))
        {
            gui_debug_toggle_breakpoint();
        }

        ImGui::EndPopup();
    }
    ImGui::PushFont(gui_default_font);
}

static void add_symbol(const char* line)
{
    Debug("Loading symbol %s", line);

    DebugSymbol s;
    std::string str(line);

    str.erase(std::remove(str.begin(), str.end(), '\r'), str.end());
    str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
    std::replace(str.begin(), str.end(), '\t', ' ');

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

    std::size_t space = str.find_last_of(" ");

    if (space != std::string::npos)
    {
        snprintf(s.text, 64, "%s", str.substr(space + 1 , std::string::npos).c_str());
        str = str.substr(0, space);

        std::replace(str.begin(), str.end(), ' ', ':');

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

            DebugSymbol* new_symbol = new DebugSymbol;
            new_symbol->address = s.address;
            new_symbol->bank = s.bank;
            snprintf(new_symbol->text, 64, "%s", s.text);

            fixed_symbols[s.bank][s.address] = new_symbol;
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
            snprintf(s.text, 64, "SUB_%02X_%04X", record->jump_bank, record->jump_address);
        else
            snprintf(s.text, 64, "TAG_%02X_%04X", record->jump_bank, record->jump_address);
        insert = true;
    }
    else if (record->irq > 0)
    {
        snprintf(s.text, 64, k_irq_symbol_format[record->irq], record->bank, address);
        insert = true;
    }

    if (insert)
    {
        DebugSymbol* new_symbol = dynamic_symbols[s.bank][s.address];

        if (IsValidPointer(new_symbol))
        {
           if (record->subroutine)
               snprintf(dynamic_symbols[s.bank][s.address]->text, 64, "SUB_%02X_%04X", record->jump_bank, record->jump_address);
        }
        else
        {
            new_symbol = new DebugSymbol;
            new_symbol->address = s.address;
            new_symbol->bank = s.bank;
            snprintf(new_symbol->text, 64, "%s", s.text);

            dynamic_symbols[s.bank][s.address] = new_symbol;
        }
    }
}

static void add_breakpoint(int type)
{
    bool read = new_breakpoint_read;
    bool write = new_breakpoint_write;
    bool execute = new_breakpoint_execute;

    if (type != HuC6280::HuC6280_BREAKPOINT_TYPE_ROMRAM)
    {
        if (!read && !write)
            return;
        execute = false;
    }

    if (emu_get_core()->GetHuC6280()->AddBreakpoint(type, new_breakpoint_buffer, read, write, execute))
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

static void replace_symbols(DisassemblerLine* line, const char* color)
{
    bool symbol_found = false;

    DebugSymbol* fixed_symbol = fixed_symbols[line->record->jump_bank][line->record->jump_address];

    if (IsValidPointer(fixed_symbol))
    {
        std::string instr = line->record->name;
        std::string symbol = fixed_symbol->text;
        char jump_address[6];
        snprintf(jump_address, 6, "$%04X", line->record->jump_address);
        size_t pos = instr.find(jump_address);
        if (pos != std::string::npos)
        {
            instr.replace(pos, 5, color + symbol);
            snprintf(line->name_enhanced, 64, "%s", instr.c_str());
            symbol_found = true;
        }
    }

    if (symbol_found)
        return;

    DebugSymbol* dynamic_symbol = dynamic_symbols[line->record->jump_bank][line->record->jump_address];

    if (IsValidPointer(dynamic_symbol))
    {
        std::string instr = line->record->name;
        std::string symbol = dynamic_symbol->text;
        char jump_address[6];
        snprintf(jump_address, 6, "$%04X", line->record->jump_address);
        size_t pos = instr.find(jump_address);
        if (pos != std::string::npos)
        {
            instr.replace(pos, 5, color + symbol);
            snprintf(line->name_enhanced, 64, "%s", instr.c_str());
        }

    }
}

static void replace_labels(DisassemblerLine* line, const char* color, const char* original_color)
{
    u16 hardware_offset = 0x0000;

    for (int i = 0; i < 8; i++)
    {
        if (emu_get_core()->GetMemory()->GetMpr(i) == 0xFF)
        {
            hardware_offset = i * 0x2000;
            break;
        }
    }

    for (int i = 0; i < k_debug_label_count; i++)
    {
        std::string instr = line->record->name;
        std::string label = k_debug_labels[i].label;
        char label_address[6];
        snprintf(label_address, 6, "$%04X", k_debug_labels[i].address + hardware_offset);
        size_t pos = instr.find(label_address);
        if (pos != std::string::npos)
        {
            instr.replace(pos, 5, color + label + label_address + original_color);
            snprintf(line->name_enhanced, 64, "%s", instr.c_str());
        }
    }
}

static const char* const c_yellow = "{FFE60C}";
static const char* const c_white = "{FFFFFF}";
static const char* const c_red = "{FA2573}";
static const char* const c_green = "{1AE51A}";
static const char* const c_blue = "{3466FF}";
static const char* const c_orange = "{FD9820}";

static void draw_instruction_name(DisassemblerLine* line, bool is_pc)
{
    const char* color;
    const char* symbol_color;
    const char* label_color;
    const char* extra_color;

    if (is_pc)
    {
        color = c_yellow;
        symbol_color = c_yellow;
        label_color = c_yellow;
        extra_color = c_yellow;
    }
    else if (line->is_breakpoint)
    {
        color = c_red;
        symbol_color = c_red;
        label_color = c_red;
        extra_color = c_red;
    }
    else
    {
        color = c_white;
        symbol_color = c_green;
        label_color = c_orange;
        extra_color = c_blue;
    }

    if (config_debug.dis_replace_symbols && line->record->jump)
    {
        replace_symbols(line, symbol_color);
    }

    if (config_debug.dis_replace_labels)
    {
        replace_labels(line, label_color, color);
    }

    std::string instr = line->name_enhanced;
    size_t pos = instr.find("{}");
    if (pos != std::string::npos)
        instr.replace(pos, 2, extra_color);

    line->name_real_length = TextColoredEx("%s%s", color, instr.c_str());
}

static void disassembler_menu(void)
{
    ImGui::BeginMenuBar();

    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Save Code As..."))
        {
            gui_file_dialog_save_disassembler();
        }

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View"))
    {
        ImGui::MenuItem("Opcodes", NULL, &config_debug.dis_show_mem);
        ImGui::MenuItem("Symbols", NULL, &config_debug.dis_show_symbols);
        ImGui::MenuItem("Segment", NULL, &config_debug.dis_show_segment);
        ImGui::MenuItem("Bank", NULL, &config_debug.dis_show_bank);

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Go"))
    {
        if (ImGui::MenuItem("Back", "CTRL+BACKSPACE"))
        {
            gui_debug_go_back();
        }

        if (ImGui::BeginMenu("Go To Address..."))
        {
            ImGui::PushItemWidth(45);
            if (ImGui::InputTextWithHint("##goto_address", "XXXX", goto_address, IM_ARRAYSIZE(goto_address), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase))
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
            if (ImGui::Button("Go!", ImVec2(40, 0)))
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

            ImGui::EndMenu();
        }

        ImGui::EndMenu();
    }


    if (ImGui::BeginMenu("Run"))
    {
        if (ImGui::MenuItem("Start", "F5"))
        {
            emu_debug_continue();
        }

        if (ImGui::MenuItem("Stop", "F7"))
        {
            emu_debug_break();
        }

        if (ImGui::MenuItem("Step Over", "F10"))
        {
            emu_debug_step_over();
        }

        if (ImGui::MenuItem("Step Into", "F11"))
        {
            emu_debug_step_into();
        }

        if (ImGui::MenuItem("Step Out", "Shift+F11"))
        {
            emu_debug_step_out();
        }

        if (ImGui::MenuItem("Step Frame", "F6"))
        {
            emu_debug_step_frame();
        }

        if (ImGui::MenuItem("Run to Cursor", "F8"))
        {
            gui_debug_runtocursor();
        }

        if (ImGui::MenuItem("Reset", "CTRL+R"))
        {
            emu_reset();
        }

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Breakpoints"))
    {
        if (ImGui::MenuItem("Toggle Selected Line", "F9"))
        {
            gui_debug_toggle_breakpoint();
        }

        ImGui::MenuItem("Break On IRQs", 0, &emu_debug_irq_breakpoints);

        ImGui::Separator();

        if (ImGui::MenuItem("Remove All"))
        {
            gui_debug_reset_breakpoints();
        }

        ImGui::MenuItem("Disable All", 0, &emu_debug_disable_breakpoints);

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Bookmarks"))
    {
        if (ImGui::MenuItem("Add Bookmark..."))
        {
            gui_debug_add_bookmark();
        }

        if (ImGui::MenuItem("Remove All"))
        {
            bookmarks.clear();
        }

        if (bookmarks.size() > 0)
            ImGui::Separator();

        for (long unsigned int i = 0; i < bookmarks.size(); i++)
        {
            char label[80];
            snprintf(label, 80, "$%04X: %s", bookmarks[i].address, bookmarks[i].name);
            if (ImGui::MenuItem(label))
            {
                request_goto_address(bookmarks[i].address);
            }
        }

        ImGui::EndMenu();
    }

    bool open_symbols = false;

    if (ImGui::BeginMenu("Symbols"))
    {
        ImGui::MenuItem("Automatic Symbols", NULL, &config_debug.dis_show_auto_symbols);
        ImGui::MenuItem("Replace Address With Symbol", NULL, &config_debug.dis_replace_symbols);
        ImGui::MenuItem("Replace Address With Label", NULL, &config_debug.dis_replace_labels);

        ImGui::Separator();

        if (ImGui::MenuItem("Load Symbols..."))
        {
            open_symbols = true;
        }

        if (ImGui::MenuItem("Clear Symbols"))
        {
            gui_debug_reset_symbols();
        }

        ImGui::EndMenu();
    }

    if (open_symbols)
        gui_file_dialog_load_symbols();

    ImGui::EndMenuBar();
}

static void add_bookmark_popup(void)
{
    if (add_bookmark)
    {
        ImGui::OpenPopup("Add Bookmark");
        add_bookmark = false;
    }

    if (ImGui::BeginPopupModal("Add Bookmark", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        static char address[5] = "";
        static char name[32] = "";
        u16 bookmark_address = (u16)selected_address;

        if (bookmark_address > 0)
            snprintf(address, 5, "%04X", bookmark_address);

        ImGui::Text("Name:");
        ImGui::PushItemWidth(200);ImGui::SetItemDefaultFocus();
        ImGui::InputText("##name", name, IM_ARRAYSIZE(name));

        ImGui::Text("Address:");
        ImGui::PushItemWidth(50);
        ImGui::InputTextWithHint("##bookaddr", "XXXX", address, IM_ARRAYSIZE(address), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase);

        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2(90, 0)))
        {
            try
            {
                bookmark_address = (int)std::stoul(address, 0, 16);

                if (strlen(name) == 0)
                {
                    Memory* memory = emu_get_core()->GetMemory();
                    Memory::GG_Disassembler_Record* record = memory->GetDisassemblerRecord(bookmark_address);

                    if (IsValidPointer(record) && (record->name[0] != 0))
                    {
                        std::string instr = record->name;
                        size_t pos = instr.find("{}");
                        if (pos != std::string::npos)
                            instr.replace(pos, 2, "");
                        snprintf(name, 32, "%s", instr.c_str());
                    }
                    else
                    {
                        snprintf(name, 32, "Bookmark_%04X", bookmark_address);
                    }
                }

                DisassemblerBookmark bookmark;
                bookmark.address = bookmark_address;
                snprintf(bookmark.name, 32, "%s", name);
                bookmarks.push_back(bookmark);
                ImGui::CloseCurrentPopup();

                address[0] = 0;
                name[0] = 0;
            }
            catch(const std::invalid_argument&)
            {
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(90, 0))) { ImGui::CloseCurrentPopup(); }

        ImGui::EndPopup();
    }
}

void gui_debug_window_call_stack(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(340, 168), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(330, 240), ImGuiCond_FirstUseEver);

    ImGui::Begin("Call Stack", &config_debug.show_call_stack);

    GeargrafxCore* core = emu_get_core();
    Memory* memory = core->GetMemory();
    HuC6280* processor = core->GetHuC6280();
    std::stack<HuC6280::GG_CallStackEntry> temp_stack = *processor->GetDisassemblerCallStack();

    char symbol_text[64] = { };

    ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable;

    if (ImGui::BeginTable("call_stack", 3, flags))
    {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Function", ImGuiTableColumnFlags_WidthStretch, 2.0f);
        ImGui::TableSetupColumn("Source", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupColumn("Return", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableHeadersRow();

        ImGui::PushFont(gui_default_font);

        while (!temp_stack.empty())
        {
            ImGui::TableNextRow();

            HuC6280::GG_CallStackEntry entry = temp_stack.top();
            temp_stack.pop();

            Memory::GG_Disassembler_Record* record = memory->GetDisassemblerRecord(entry.dest);

            if (IsValidPointer(record) && (record->name[0] != 0))
            {
                DebugSymbol* symbol = fixed_symbols[record->bank][entry.dest];

                if (IsValidPointer(symbol))
                    snprintf(symbol_text, sizeof(symbol_text), "%s", symbol->text);
                else 
                {
                    DebugSymbol* symbol = dynamic_symbols[record->bank][entry.dest];

                    if (IsValidPointer(symbol))
                        snprintf(symbol_text, sizeof(symbol_text), "%s", symbol->text);
                    else
                        symbol_text[0] = 0;
                }
            }

            ImGui::TableNextColumn();
            ImGui::TextColored(cyan, "$%04X", entry.dest);
            ImGui::SameLine();
            ImGui::TextColored(green, "%s", symbol_text);

            ImGui::TableNextColumn();
            ImGui::TextColored(cyan, "$%04X", entry.src);

            ImGui::TableNextColumn();
            ImGui::TextColored(cyan, "$%04X", entry.back);
        }

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextColored(gray, "----- Bottom of Stack");
        ImGui::TableNextColumn();
        ImGui::TextColored(gray, "-----");
        ImGui::TableNextColumn();
        ImGui::TextColored(gray, "-----");

        ImGui::PopFont();

        ImGui::EndTable();
    }

    ImGui::End();
    ImGui::PopStyleVar();
}
