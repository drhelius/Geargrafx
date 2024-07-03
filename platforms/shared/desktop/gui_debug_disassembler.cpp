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

#include "../../../src/geargrafx.h"

// struct DebugSymbol
// {
//     int bank;
//     u16 address;
//     std::string text;
// };

// struct DisassmeblerLine
// {
//     bool is_symbol;
//     bool is_breakpoint;
//     Memory::stDisassembleRecord* record;
//     std::string symbol;
// };

// static std::vector<DebugSymbol> symbols;
// static Memory::stDisassembleRecord* selected_record = NULL;
static char brk_address_cpu[5] = "";
static char brk_address_mem[10] = "";
static bool brk_new_mem_read = true;
static bool brk_new_mem_write = true;
static char goto_address[5] = "";
static bool goto_address_requested = false;
static u16 goto_address_target = 0;
static bool goto_back_requested = false;
static int goto_back = 0;

static void debug_window_disassembler(void);
static void add_symbol(const char* line);
static void add_breakpoint_cpu(void);
static void add_breakpoint_mem(void);
static void request_goto_address(u16 addr);
static bool is_return_instruction(u8 opcode1, u8 opcode2);

void gui_debug_reset(void)
{
    // gui_debug_reset_breakpoints_cpu();
    // gui_debug_reset_breakpoints_mem();
    // gui_debug_reset_symbols();
    // selected_record = NULL;
}

void gui_debug_reset_symbols(void)
{
    // symbols.clear();

    // for (int i = 0; i < gui_debug_symbols_count; i++)
    //     add_symbol(gui_debug_symbols[i]);
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
    // if (IsValidPointer(selected_record))
    // {
    //     bool found = false;
    //     std::vector<Memory::stDisassembleRecord*>* breakpoints = emu_get_core()->GetMemory()->GetBreakpointsCPU();

    //     for (long unsigned int b = 0; b < breakpoints->size(); b++)
    //     {
    //         if ((*breakpoints)[b] == selected_record)
    //         {
    //             found = true;
    //              InitPointer((*breakpoints)[b]);
    //             break;
    //         }
    //     }

    //     if (!found)
    //     {
    //         breakpoints->push_back(selected_record);
    //     }
    // }
}

void gui_debug_runtocursor(void)
{
    // if (IsValidPointer(selected_record))
    // {
    //     emu_get_core()->GetMemory()->SetRunToBreakpoint(selected_record);
    //     emu_debug_continue();
    // }
}

void gui_debug_reset_breakpoints_cpu(void)
{
    // emu_get_core()->GetMemory()->GetBreakpointsCPU()->clear();
    // brk_address_cpu[0] = 0;
}

void gui_debug_reset_breakpoints_mem(void)
{
    // emu_get_core()->GetMemory()->GetBreakpointsMem()->clear();
    // brk_address_mem[0] = 0;
}

void gui_debug_go_back(void)
{
    goto_back_requested = true;
}

static void debug_window_disassembler(void)
{
    // ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    // ImGui::SetNextWindowPos(ImVec2(159, 31), ImGuiCond_FirstUseEver);
    // ImGui::SetNextWindowSize(ImVec2(401, 641), ImGuiCond_FirstUseEver);

    // ImGui::Begin("Disassembler", &config_debug.show_disassembler);

    // GearcolecoCore* core = emu_get_core();
    // Processor* processor = core->GetProcessor();
    // Processor::ProcessorState* proc_state = processor->GetState();
    // Memory* memory = core->GetMemory();
    // std::vector<Memory::stDisassembleRecord*>* breakpoints_cpu = memory->GetBreakpointsCPU();
    // std::vector<Memory::stMemoryBreakpoint>* breakpoints_mem = memory->GetBreakpointsMem();

    // int pc = proc_state->PC->GetValue();

    // if (ImGui::Button("Step Over"))
    //     emu_debug_step();
    // ImGui::SameLine();
    // if (ImGui::Button("Step Frame"))
    //     emu_debug_next_frame();
    // ImGui::SameLine();
    // if (ImGui::Button("Continue"))
    //     emu_debug_continue(); 
    // ImGui::SameLine();
    // if (ImGui::Button("Run To Cursor"))
    //     gui_debug_runtocursor();

    // static bool follow_pc = true;
    // static bool show_mem = true;
    // static bool show_symbols = true;
    // static bool show_segment = true;

    // ImGui::Checkbox("Follow PC", &follow_pc); ImGui::SameLine();
    // ImGui::Checkbox("Opcodes", &show_mem);  ImGui::SameLine();
    // ImGui::Checkbox("Symbols", &show_symbols);  ImGui::SameLine();
    // ImGui::Checkbox("Segments", &show_segment);

    // ImGui::Separator();

    // ImGui::Text("Go To Address: ");
    // ImGui::SameLine();
    // ImGui::PushItemWidth(45);
    // if (ImGui::InputTextWithHint("##goto_address", "XXXX", goto_address, IM_ARRAYSIZE(goto_address), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue))
    // {
    //     try
    //     {
    //         request_goto_address((u16)std::stoul(goto_address, 0, 16));
    //         follow_pc = false;
    //     }
    //     catch(const std::invalid_argument&)
    //     {
    //     }
    //     goto_address[0] = 0;
    // }
    // ImGui::PopItemWidth();
    // ImGui::SameLine();
    // if (ImGui::Button("Go", ImVec2(30, 0)))
    // {
    //     try
    //     {
    //         request_goto_address((u16)std::stoul(goto_address, 0, 16));
    //         follow_pc = false;
    //     }
    //     catch(const std::invalid_argument&)
    //     {
    //     }
    //     goto_address[0] = 0;
    // }

    // ImGui::SameLine();
    // if (ImGui::Button("Back", ImVec2(50, 0)))
    // {
    //     goto_back_requested = true;
    //     follow_pc = false;
    // }

    // ImGui::Separator();

    // if (ImGui::CollapsingHeader("Processor Breakpoints"))
    // {
    //     ImGui::Checkbox("Disable All##disable_all_cpu", &emu_debug_disable_breakpoints_cpu);

    //     ImGui::Columns(2, "breakpoints_cpu");
    //     ImGui::SetColumnOffset(1, 85);

    //     ImGui::Separator();

    //     if (IsValidPointer(selected_record))
    //         sprintf(brk_address_cpu, "%04X", selected_record->address);

    //     ImGui::PushItemWidth(70);
    //     if (ImGui::InputTextWithHint("##add_breakpoint_cpu", "XXXX", brk_address_cpu, IM_ARRAYSIZE(brk_address_cpu), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue))
    //     {
    //         add_breakpoint_cpu();
    //     }
    //     ImGui::PopItemWidth();


    //     if (ImGui::Button("Add##add_cpu", ImVec2(70, 0)))
    //     {
    //         add_breakpoint_cpu();
    //     }

    //     if (ImGui::Button("Clear All##clear_all_cpu", ImVec2(70, 0)))
    //     {
    //         gui_debug_reset_breakpoints_cpu();
    //     }

    //     ImGui::NextColumn();

    //     ImGui::BeginChild("breakpoints_cpu", ImVec2(0, 80), false);

    //     int remove = -1;

    //     for (long unsigned int b = 0; b < breakpoints_cpu->size(); b++)
    //     {
    //         if (!IsValidPointer((*breakpoints_cpu)[b]))
    //             continue;

    //         ImGui::PushID(b);
    //         if (ImGui::SmallButton("X"))
    //         {
    //            remove = b;
    //            ImGui::PopID();
    //            continue;
    //         }

    //         ImGui::PopID();

    //         ImGui::PushFont(gui_default_font);
    //         ImGui::SameLine();
    //         ImGui::TextColored(red, "%04X", (*breakpoints_cpu)[b]->address);
    //         ImGui::SameLine();
    //         ImGui::TextColored(gray, "%s", (*breakpoints_cpu)[b]->name);
    //         ImGui::PopFont();
    //     }

    //     if (remove >= 0)
    //     {
    //         breakpoints_cpu->erase(breakpoints_cpu->begin() + remove);
    //     }

    //     ImGui::EndChild();
    //     ImGui::Columns(1);
    //     ImGui::Separator();

    // }

    // if (ImGui::CollapsingHeader("Memory Breakpoints"))
    // {
    //     ImGui::Checkbox("Disable All##diable_all_mem", &emu_debug_disable_breakpoints_mem);

    //     ImGui::Columns(2, "breakpoints_mem");
    //     ImGui::SetColumnOffset(1, 100);

    //     ImGui::Separator();

    //     ImGui::PushItemWidth(85);
    //     if (ImGui::InputTextWithHint("##add_breakpoint_mem", "XXXX-XXXX", brk_address_mem, IM_ARRAYSIZE(brk_address_mem), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue))
    //     {
    //         add_breakpoint_mem();
    //     }
    //     ImGui::PopItemWidth();

    //     if (ImGui::IsItemHovered())
    //         ImGui::SetTooltip("Use XXXX format for single addresses or XXXX-XXXX for address ranges");

    //     ImGui::Checkbox("Read", &brk_new_mem_read);
    //     ImGui::Checkbox("Write", &brk_new_mem_write);

    //     if (ImGui::Button("Add##add_mem", ImVec2(85, 0)))
    //     {
    //         add_breakpoint_mem();
    //     }

    //     if (ImGui::Button("Clear All##clear_all_mem", ImVec2(85, 0)))
    //     {
    //         gui_debug_reset_breakpoints_mem();
    //     }

    //     ImGui::NextColumn();

    //     ImGui::BeginChild("breakpoints_mem", ImVec2(0, 130), false);

    //     int remove = -1;

    //     for (long unsigned int b = 0; b < breakpoints_mem->size(); b++)
    //     {
    //         ImGui::PushID(10000 + b);
    //         if (ImGui::SmallButton("X"))
    //         {
    //            remove = b;
    //            ImGui::PopID();
    //            continue;
    //         }

    //         ImGui::PopID();

    //         ImGui::PushFont(gui_default_font);
    //         ImGui::SameLine();
    //         if ((*breakpoints_mem)[b].range)
    //             ImGui::TextColored(red, "%04X-%04X", (*breakpoints_mem)[b].address1, (*breakpoints_mem)[b].address2);
    //         else
    //             ImGui::TextColored(red, "%04X", (*breakpoints_mem)[b].address1);
    //         if ((*breakpoints_mem)[b].read)
    //         {
    //             ImGui::SameLine(); ImGui::TextColored(gray, "R");
    //         }
    //         if ((*breakpoints_mem)[b].write)
    //         {
    //             ImGui::SameLine(); ImGui::TextColored(gray, "W");
    //         }
    //         ImGui::PopFont();
    //     }

    //     if (remove >= 0)
    //     {
    //         breakpoints_mem->erase(breakpoints_mem->begin() + remove);
    //     }

    //     ImGui::EndChild();
    //     ImGui::Columns(1);
    //     ImGui::Separator();
    // }

    // ImGui::PushFont(gui_default_font);

    // bool window_visible = ImGui::BeginChild("##dis", ImVec2(ImGui::GetContentRegionAvail().x, 0), true, 0);
    
    // if (window_visible)
    // {
    //     int dis_size = 0;
    //     int pc_pos = 0;
    //     int goto_address_pos = 0;
        
    //     std::vector<DisassmeblerLine> vec(0x10000);
        
    //     for (int i = 0; i < 0x10000; i++)
    //     {
    //         Memory::stDisassembleRecord* record = memory->GetDisassembleRecord(i, false);

    //         if (IsValidPointer(record) && (record->name[0] != 0))
    //         {
    //             for (long unsigned int s = 0; s < symbols.size(); s++)
    //             {
    //                 if ((symbols[s].bank == record->bank) && (symbols[s].address == i) && show_symbols)
    //                 {
    //                     vec[dis_size].is_symbol = true;
    //                     vec[dis_size].symbol = symbols[s].text;
    //                     dis_size ++;
    //                 }
    //             }

    //             vec[dis_size].is_symbol = false;
    //             vec[dis_size].record = record;

    //             if (vec[dis_size].record->address == pc)
    //                 pc_pos = dis_size;

    //             if (goto_address_requested && (vec[dis_size].record->address <= goto_address_target))
    //                 goto_address_pos = dis_size;

    //             vec[dis_size].is_breakpoint = false;

    //             for (long unsigned int b = 0; b < breakpoints_cpu->size(); b++)
    //             {
    //                 if ((*breakpoints_cpu)[b] == vec[dis_size].record)
    //                 {
    //                     vec[dis_size].is_breakpoint = true;
    //                     break;
    //                 }
    //             }

    //             dis_size++;
    //         }
    //     }

    //     if (follow_pc)
    //     {
    //         float window_offset = ImGui::GetWindowHeight() / 2.0f;
    //         float offset = window_offset - (ImGui::GetTextLineHeightWithSpacing() - 2.0f);
    //         ImGui::SetScrollY((pc_pos * ImGui::GetTextLineHeightWithSpacing()) - offset);
    //     }

    //     if (goto_address_requested)
    //     {
    //         goto_address_requested = false;
    //         goto_back = (int)ImGui::GetScrollY();
    //         ImGui::SetScrollY((goto_address_pos * ImGui::GetTextLineHeightWithSpacing()) + 2);
    //     }

    //     if (goto_back_requested)
    //     {
    //         goto_back_requested = false;
    //         ImGui::SetScrollY((float)goto_back);
    //     }

    //     ImGuiListClipper clipper;
    //     clipper.Begin(dis_size, ImGui::GetTextLineHeightWithSpacing());

    //     while (clipper.Step())
    //     {
    //         for (int item = clipper.DisplayStart; item < clipper.DisplayEnd; item++)
    //         {
    //             if (vec[item].is_symbol)
    //             {
    //                 ImGui::TextColored(green, "%s:", vec[item].symbol.c_str());
    //                 continue;
    //             }

    //             ImGui::PushID(item);

    //             bool is_selected = (selected_record == vec[item].record);

    //             if (ImGui::Selectable("", is_selected, ImGuiSelectableFlags_AllowDoubleClick))
    //             {
    //                 if (ImGui::IsMouseDoubleClicked(0) && vec[item].record->jump)
    //                 {
    //                     follow_pc = false;
    //                     request_goto_address(vec[item].record->jump_address);
    //                 }
    //                 else if (is_selected)
    //                 {
    //                     InitPointer(selected_record);
    //                     brk_address_cpu[0] = 0;
    //                 }
    //                 else
    //                     selected_record = vec[item].record;
    //             }

    //             if (is_selected)
    //                 ImGui::SetItemDefaultFocus();

    //             ImVec4 color_segment = vec[item].is_breakpoint ? red : magenta;
    //             ImVec4 color_addr = vec[item].is_breakpoint ? red : cyan;
    //             ImVec4 color_opcodes = vec[item].is_breakpoint ? red : gray;
    //             ImVec4 color_name = vec[item].is_breakpoint ? red : white;

    //             if (show_segment)
    //             {
    //                 ImGui::SameLine();
    //                 ImGui::TextColored(color_segment, "%s", vec[item].record->segment);
    //             }

    //             ImGui::SameLine();
    //             if (strcmp(vec[item].record->segment, "ROM") == 0)
    //                 ImGui::TextColored(color_addr, "%02X:%04X ", vec[item].record->bank, vec[item].record->address);
    //             else
    //                 ImGui::TextColored(color_addr, "  %04X ", vec[item].record->address);


    //             if (show_mem)
    //             {
    //                 ImGui::SameLine();
    //                 ImGui::TextColored(color_opcodes, "%s ", vec[item].record->bytes);
    //             }

    //             ImGui::SameLine();
    //             if (vec[item].record->address == pc)
    //             {
    //                 ImGui::TextColored(yellow, "->");
    //                 color_name = yellow;
    //             }
    //             else
    //             {
    //                 ImGui::TextColored(yellow, "  ");
    //             }

    //             ImGui::SameLine();
    //             ImGui::TextColored(color_name, "%s", vec[item].record->name);

    //             bool is_ret = is_return_instruction(vec[item].record->opcodes[0], vec[item].record->opcodes[1]);
    //             if (is_ret)
    //             {
    //                 ImGui::PushStyleColor(ImGuiCol_Separator, (vec[item].record->opcodes[0] == 0xC9) ? gray : dark_gray);
    //                 ImGui::Separator();
    //                 ImGui::PopStyleColor();
    //             }

    //             ImGui::PopID();
    //         }
    //     }
    // }

    // ImGui::EndChild();
    
    // ImGui::PopFont();

    // ImGui::End();

    // ImGui::PopStyleVar();
}

static void add_symbol(const char* line)
{
    // Log("Loading symbol %s", line);

    // DebugSymbol s;

    // std::string str(line);

    // str.erase(std::remove(str.begin(), str.end(), '\r'), str.end());
    // str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());

    // size_t first = str.find_first_not_of(' ');
    // if (std::string::npos == first)
    // {
    //     str = "";
    // }
    // else
    // {
    //     size_t last = str.find_last_not_of(' ');
    //     str = str.substr(first, (last - first + 1));
    // }

    // std::size_t comment = str.find(";");

    // if (comment != std::string::npos)
    //     str = str.substr(0 , comment);

    // std::size_t space = str.find(" ");

    // if (space != std::string::npos)
    // {
    //     s.text = str.substr(space + 1 , std::string::npos);
    //     str = str.substr(0, space);

    //     std::size_t separator = str.find(":");

    //     try
    //     {
    //         if (separator != std::string::npos)
    //         {
    //             s.address = (u16)std::stoul(str.substr(separator + 1 , std::string::npos), 0, 16);

    //             s.bank = std::stoul(str.substr(0, separator), 0 , 16);
    //         }
    //         else
    //         {
    //             s.address = (u16)std::stoul(str, 0, 16);
    //             s.bank = 0;
    //         }

    //         symbols.push_back(s);
    //     }
    //     catch(const std::invalid_argument&)
    //     {
    //     }
    // }
}

static void add_breakpoint_cpu(void)
{
    // int input_len = (int)strlen(brk_address_cpu);
    // u16 target_address = 0;
    // int target_bank = 0;

    // try
    // {
    //     if ((input_len == 7) && (brk_address_cpu[2] == ':'))
    //     {
    //         std::string str(brk_address_cpu);
    //         std::size_t separator = str.find(":");

    //         if (separator != std::string::npos)
    //         {
    //             target_address = (u16)std::stoul(str.substr(separator + 1 , std::string::npos), 0, 16);

    //             target_bank = std::stoul(str.substr(0, separator), 0 , 16);
    //             target_bank &= 0xFF;
    //         }
    //     } 
    //     else if (input_len == 4)
    //     {
    //         target_bank = 0; 
    //         target_address = (u16)std::stoul(brk_address_cpu, 0, 16);
    //     }
    //     else
    //     {
    //         return;
    //     }
    // }
    // catch(const std::invalid_argument&)
    // {
    //     return;
    // }

    // Memory::stDisassembleRecord* record = emu_get_core()->GetMemory()->GetDisassembleRecord(target_address, true);

    // brk_address_cpu[0] = 0;

    // bool found = false;
    // std::vector<Memory::stDisassembleRecord*>* breakpoints = emu_get_core()->GetMemory()->GetBreakpointsCPU();

    // if (IsValidPointer(record))
    // {
    //     for (long unsigned int b = 0; b < breakpoints->size(); b++)
    //     {
    //         if ((*breakpoints)[b] == record)
    //         {
    //             found = true;
    //             break;
    //         }
    //     }
    // }

    // if (!found)
    // {
    //     breakpoints->push_back(record);
    // }
}

static void add_breakpoint_mem(void)
{
    // int input_len = (int)strlen(brk_address_mem);
    // u16 address1 = 0;
    // u16 address2 = 0;
    // bool range = false;

    // try
    // {
    //     if ((input_len == 9) && (brk_address_mem[4] == '-'))
    //     {
    //         std::string str(brk_address_mem);
    //         std::size_t separator = str.find("-");

    //         if (separator != std::string::npos)
    //         {
    //             address1 = (u16)std::stoul(str.substr(0, separator), 0 , 16);
    //             address2 = (u16)std::stoul(str.substr(separator + 1 , std::string::npos), 0, 16);
    //             range = true;
    //         }
    //     }
    //     else if (input_len == 4)
    //     {
    //         address1 = (u16)std::stoul(brk_address_mem, 0, 16);
    //     }
    //     else
    //     {
    //         return;
    //     }
    // }
    // catch(const std::invalid_argument&)
    // {
    //     return;
    // }

    // bool found = false;
    // std::vector<Memory::stMemoryBreakpoint>* breakpoints = emu_get_core()->GetMemory()->GetBreakpointsMem();

    // for (long unsigned int b = 0; b < breakpoints->size(); b++)
    // {
    //     Memory::stMemoryBreakpoint temp = (*breakpoints)[b];
    //     if ((temp.address1 == address1) && (temp.address2 == address2) && (temp.range == range))
    //     {
    //         found = true;
    //         break;
    //     }
    // }

    // if (!found)
    // {
    //     Memory::stMemoryBreakpoint new_breakpoint;
    //     new_breakpoint.address1 = address1;
    //     new_breakpoint.address2 = address2;
    //     new_breakpoint.range = range;
    //     new_breakpoint.read = brk_new_mem_read;
    //     new_breakpoint.write = brk_new_mem_write;

    //     breakpoints->push_back(new_breakpoint);
    // }

    // brk_address_mem[0] = 0;
}

static void request_goto_address(u16 address)
{
    goto_address_requested = true;
    goto_address_target = address;
}

static bool is_return_instruction(u8 opcode1, u8 opcode2)
{
    return false;
    // switch (opcode1)
    // {
    //     case 0xC9: // RET
    //     case 0xC0: // RET NZ
    //     case 0xC8: // RET Z
    //     case 0xD0: // RET NC
    //     case 0xD8: // RET C
    //     case 0xE0: // RET PO
    //     case 0xE8: // RET PE
    //     case 0xF0: // RET P
    //     case 0xF8: // RET M
    //         return true;
    //     case 0xED: // Extended instructions
    //         if (opcode2 == 0x45 || opcode2 == 0x4D) // RETN, RETI
    //             return true;
    //         else
    //             return false;
    //     default:
    //         return false;
    // }
}