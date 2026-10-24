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

#define GUI_CHEATS_IMPORT
#include "gui_cheats.h"

#include <string>
#include <vector>
#include <fstream>
#include "imgui.h"
#include "geargrafx.h"
#include "gui.h"
#include "gui_colors.h"
#include "gui_filedialogs.h"
#include "emu.h"
#include "utils.h"

struct gui_CheatImport
{
    bool used;
    bool enabled;
    std::string code;
    std::string description;
    std::string handler;
    std::string type;
    std::string size;
    std::string address;
    std::string value;
    std::string repeat_count;
    std::string repeat_address;
    std::string repeat_value;
    std::string big_endian;
};

static bool show_cheats = false;
static bool focus_cheats = false;
static int focus_cheat_row = -1;
static gui_CheatEntry cheat_list[GUI_CHEATS_MAX];
static int cheat_count = 0;

static void cheats_apply(void);
static int cheats_add_entry(const char* code, const char* description, bool enabled);
static void cheats_remove_entry(int index);
static std::string cheats_trim(const std::string& str);
static bool cheats_import_handler(const gui_CheatImport& entry, std::string& code);
static bool cheats_import_number(const std::string& str, int default_value, int* result);

void gui_cheats_init(void)
{
    show_cheats = false;
    focus_cheats = false;
    focus_cheat_row = -1;
    cheat_count = 0;
}

void gui_cheats_show(void)
{
    show_cheats = true;
    focus_cheats = true;
}

void gui_cheats_clear(void)
{
    cheat_count = 0;
    focus_cheat_row = -1;
    emu_clear_cheats();
}

int gui_cheats_add(const char* code, const char* description)
{
    int index = cheats_add_entry(code, description, true);

    if (index >= 0)
        cheats_apply();

    return index;
}

bool gui_cheats_remove(int index)
{
    if ((index < 0) || (index >= cheat_count))
        return false;

    cheats_remove_entry(index);
    cheats_apply();
    return true;
}

int gui_cheats_get_count(void)
{
    return cheat_count;
}

const gui_CheatEntry* gui_cheats_get(int index)
{
    if ((index < 0) || (index >= cheat_count))
        return NULL;

    return &cheat_list[index];
}

bool gui_cheats_import(const char* path, int* imported, int* skipped)
{
    std::ifstream file;
    open_ifstream_utf8(file, path, std::ios::in);

    if (!file.is_open())
        return false;

    std::vector<gui_CheatImport> entries;
    gui_CheatImport empty_entry;
    empty_entry.used = false;
    empty_entry.enabled = false;
    std::string line;

    while (std::getline(file, line))
    {
        size_t equal = line.find('=');

        if (equal == std::string::npos)
            continue;

        std::string key = cheats_trim(line.substr(0, equal));
        std::string value = cheats_trim(line.substr(equal + 1));

        if (!value.empty() && (value[0] == '"'))
            value.erase(0, 1);
        if (!value.empty() && (value[value.length() - 1] == '"'))
            value.erase(value.length() - 1);

        if (key.compare(0, 5, "cheat") != 0)
            continue;

        size_t underscore = key.find('_', 5);

        if ((underscore == std::string::npos) || (underscore == 5) || (underscore > 9))
            continue;

        int index = 0;
        bool is_number = true;

        for (size_t i = 5; i < underscore; i++)
        {
            if ((key[i] < '0') || (key[i] > '9'))
            {
                is_number = false;
                break;
            }

            index = (index * 10) + (key[i] - '0');
        }

        if (!is_number)
            continue;

        if ((int)entries.size() <= index)
            entries.resize(index + 1, empty_entry);

        gui_CheatImport& entry = entries[index];
        std::string field = key.substr(underscore + 1);

        if (field == "desc")
            entry.description = value;
        else if (field == "code")
            entry.code = value;
        else if (field == "enable")
            entry.enabled = (value == "true") || (value == "1");
        else if (field == "handler")
            entry.handler = value;
        else if (field == "cheat_type")
            entry.type = value;
        else if (field == "memory_search_size")
            entry.size = value;
        else if (field == "address")
            entry.address = value;
        else if (field == "value")
            entry.value = value;
        else if (field == "repeat_count")
            entry.repeat_count = value;
        else if (field == "repeat_add_to_address")
            entry.repeat_address = value;
        else if (field == "repeat_add_to_value")
            entry.repeat_value = value;
        else if (field == "big_endian")
            entry.big_endian = value;
        else
            continue;

        entry.used = true;
    }

    int imported_count = 0;
    int skipped_count = 0;

    for (size_t i = 0; i < entries.size(); i++)
    {
        if (!entries[i].used)
            continue;

        std::string code = entries[i].code;

        if (code.empty() && !cheats_import_handler(entries[i], code))
            code.clear();

        if (code.empty() || (code.length() >= sizeof(cheat_list[0].code)) ||
            (cheats_add_entry(code.c_str(), entries[i].description.c_str(), entries[i].enabled) < 0))
        {
            skipped_count++;
        }
        else
            imported_count++;
    }

    if (imported_count > 0)
        cheats_apply();

    if (IsValidPointer(imported))
        *imported = imported_count;
    if (IsValidPointer(skipped))
        *skipped = skipped_count;

    return true;
}

static void cheats_apply(void)
{
    emu_clear_cheats();

    for (int i = 0; i < cheat_count; i++)
    {
        gui_CheatEntry* entry = &cheat_list[i];
        entry->active = entry->enabled && entry->valid && emu_add_cheat(entry->code);
    }
}

static int cheats_add_entry(const char* code, const char* description, bool enabled)
{
    if (cheat_count >= GUI_CHEATS_MAX)
        return -1;

    gui_CheatEntry* entry = &cheat_list[cheat_count];
    entry->enabled = enabled;
    entry->active = false;
    strncpy_fit(entry->code, IsValidPointer(code) ? code : "", sizeof(entry->code));
    strncpy_fit(entry->description, IsValidPointer(description) ? description : "", sizeof(entry->description));
    entry->valid = GeargrafxCore::IsValidCheat(entry->code);
    cheat_count++;

    return cheat_count - 1;
}

static void cheats_remove_entry(int index)
{
    for (int i = index; i < (cheat_count - 1); i++)
        cheat_list[i] = cheat_list[i + 1];

    cheat_count--;
}

static std::string cheats_trim(const std::string& str)
{
    size_t begin = str.find_first_not_of(" \t\r\n");

    if (begin == std::string::npos)
        return "";

    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(begin, end - begin + 1);
}

static bool cheats_import_handler(const gui_CheatImport& entry, std::string& code)
{
    int handler = 0;
    int type = 0;
    int size = 0;
    int address = 0;
    int value = 0;
    int repeat_count = 0;
    int repeat_address = 0;
    int repeat_value = 0;

    if (!cheats_import_number(entry.handler, 0, &handler) || !cheats_import_number(entry.type, 0, &type) ||
        !cheats_import_number(entry.size, 3, &size) || !cheats_import_number(entry.address, -1, &address) ||
        !cheats_import_number(entry.value, 0, &value) || !cheats_import_number(entry.repeat_count, 1, &repeat_count) ||
        !cheats_import_number(entry.repeat_address, 1, &repeat_address) ||
        !cheats_import_number(entry.repeat_value, 0, &repeat_value))
    {
        return false;
    }

    if ((handler != 1) || (type != 1) || ((size != 3) && (size != 4)) || (repeat_count < 1) || (repeat_count > 16))
        return false;

    int bytes = (size == 4) ? 2 : 1;
    int mask = (size == 4) ? 0xFFFF : 0xFF;
    bool big_endian = (entry.big_endian == "true") || (entry.big_endian == "1");
    char part[24];

    code.clear();

    for (int i = 0; i < repeat_count; i++)
    {
        if ((address < 0) || ((address + bytes) > 0x8000))
            return false;

        int physical = 0x1F0000 + address;

        if (bytes == 1)
            snprintf(part, sizeof(part), "%06X:%02X", physical, value & 0xFF);
        else if (big_endian)
            snprintf(part, sizeof(part), "%06X:%02X+%06X:%02X", physical, (value >> 8) & 0xFF, physical + 1, value & 0xFF);
        else
            snprintf(part, sizeof(part), "%06X:%04X", physical, value & 0xFFFF);

        if (!code.empty())
            code += "+";

        code += part;
        address += repeat_address * bytes;
        value = (value + repeat_value) % mask;
    }

    return true;
}

static bool cheats_import_number(const std::string& str, int default_value, int* result)
{
    if (str.empty())
    {
        *result = default_value;
        return true;
    }

    char* end = NULL;
    long number = strtol(str.c_str(), &end, 10);

    if ((end == str.c_str()) || (*end != 0) || (number < 0) || (number > 0xFFFFFF))
        return false;

    *result = (int)number;
    return true;
}

void gui_cheats_window(void)
{
    if (!show_cheats)
        return;

    if (focus_cheats)
    {
        ImGui::SetNextWindowFocus();
        focus_cheats = false;
    }

    ImGui::SetNextWindowSize(ImVec2(620.0f, 300.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(560.0f, 180.0f), ImVec2(1200.0f, 800.0f));

    if (!ImGui::Begin("Cheats", &show_cheats))
    {
        ImGui::End();
        return;
    }

    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows) ||
        ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
    {
        gui_in_use = true;
    }

    ImGuiStyle& style = ImGui::GetStyle();
    float right_edge = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x;
    ImGui::TextDisabled("AAAAAA:VV  AAAAAA?CC:VV  (join with +)");

    float import_button_width = ImGui::CalcTextSize("Import .cht...").x + (style.FramePadding.x * 2.0f);
    float add_button_width = ImGui::CalcTextSize("Add Cheat").x + (style.FramePadding.x * 2.0f);
    ImGui::SameLine();
    ImGui::SetCursorPosX(right_edge - add_button_width - style.ItemSpacing.x - import_button_width);

    bool maximum_reached = cheat_count >= GUI_CHEATS_MAX;
    ImGui::BeginDisabled(maximum_reached);
    bool import_cheats = ImGui::Button("Import .cht...##import_cheats");
    ImGui::EndDisabled();

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip(maximum_reached ? "Maximum of 50 cheats" : "Import cheats from a RetroArch .cht file");

    ImGui::SameLine();
    ImGui::BeginDisabled(maximum_reached);
    bool add_cheat = ImGui::Button("Add Cheat##add_cheat");
    ImGui::EndDisabled();

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip(maximum_reached ? "Maximum of 50 cheats" : "Add cheat");

    if (import_cheats)
        gui_file_dialog_import_cheats();

    if (add_cheat)
        focus_cheat_row = cheats_add_entry("", "", true);

    float footer_height = (cheat_count == 0) ? 0.0f : ImGui::GetFrameHeightWithSpacing();
    float table_height = ImGui::GetContentRegionAvail().y - footer_height;

    if (table_height < 80.0f)
        table_height = 80.0f;

    ImGuiTableFlags table_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY;

    bool apply_changes = false;
    int remove_cheat = -1;

    if (ImGui::BeginTable("cheats_table", 4, table_flags, ImVec2(0.0f, table_height)))
    {
        ImGui::TableSetupColumn("Enabled", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 60.0f);
        ImGui::TableSetupColumn("Code", ImGuiTableColumnFlags_WidthFixed, 220.0f);
        ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 30.0f);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        for (int i = 0; i < cheat_count; i++)
        {
            gui_CheatEntry* entry = &cheat_list[i];
            bool invalid = (entry->code[0] != '\0') && !entry->valid;
            bool rejected = entry->enabled && entry->valid && !entry->active;

            ImGui::PushID(i);
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::BeginDisabled(!entry->valid);

            if (ImGui::Checkbox("##enabled", &entry->enabled))
                apply_changes = true;
            ImGui::EndDisabled();

            ImGui::TableSetColumnIndex(1);
            if (invalid || rejected)
            {
                ImGui::PushStyleColor(ImGuiCol_Border, red);
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
            }

            if (focus_cheat_row == i)
            {
                ImGui::SetKeyboardFocusHere();
                focus_cheat_row = -1;
            }

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            bool code_edited = ImGui::InputTextWithHint("##code", "Code", entry->code, IM_ARRAYSIZE(entry->code), ImGuiInputTextFlags_CharsUppercase);

            if (invalid || rejected)
            {
                ImGui::PopStyleVar();
                ImGui::PopStyleColor();
            }

            if (invalid && ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Use AAAAAA:VV or AAAAAA?CC:VV, join codes with +.");
            }
            else if (rejected && ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("This code does not apply to the loaded game.");
            }

            if (code_edited)
            {
                entry->valid = GeargrafxCore::IsValidCheat(entry->code);
                apply_changes = true;
            }

            ImGui::TableSetColumnIndex(2);
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::InputTextWithHint("##description", "Description", entry->description, IM_ARRAYSIZE(entry->description));

            ImGui::TableSetColumnIndex(3);
            bool remove = ImGui::SmallButton("X##remove_cheat");

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Remove Cheat");

            if (remove)
                remove_cheat = i;

            ImGui::PopID();
        }

        if (cheat_count == 0)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(1);
            ImGui::TextDisabled("No cheats");
        }

        ImGui::EndTable();
    }

    if (remove_cheat >= 0)
    {
        cheats_remove_entry(remove_cheat);
        apply_changes = true;
    }

    if (apply_changes)
        cheats_apply();

    if ((cheat_count > 0) && ImGui::Button("Remove All"))
        gui_cheats_clear();

    ImGui::End();
}
