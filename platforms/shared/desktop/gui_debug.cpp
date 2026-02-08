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

#include <fstream>
#include "geargrafx.h"
#include "imgui.h"
#include "gui_debug_disassembler.h"
#include "gui_debug_huc6280.h"
#include "gui_debug_huc6270.h"
#include "gui_debug_huc6260.h"
#include "gui_debug_huc6202.h"
#include "gui_debug_memory.h"
#include "gui_debug_psg.h"
#include "gui_debug_cdrom.h"
#include "gui_debug_cdrom_audio.h"
#include "gui_debug_adpcm.h"
#include "gui_debug_trace_logger.h"
#include "emu.h"
#include "config.h"


void gui_debug_init(void)
{
    gui_debug_disassembler_init();
    gui_debug_psg_init();
    gui_debug_cdrom_audio_init();
    gui_debug_adpcm_init();
    gui_debug_memory_reset();
}

void gui_debug_destroy(void)
{
    gui_debug_disassembler_destroy();
    gui_debug_psg_destroy();
    gui_debug_cdrom_audio_destroy();
    gui_debug_adpcm_destroy();
}

void gui_debug_reset(void)
{
    gui_debug_disassembler_reset();
    gui_debug_memory_reset();
    gui_debug_reset_breakpoints();
    gui_debug_reset_symbols();
}

void gui_debug_callback(void)
{
    gui_debug_trace_logger_update();
}

void gui_debug_windows(void)
{
    if (config_debug.debug)
    {
        if (config_debug.show_processor)
            gui_debug_window_huc6280();
        if (config_debug.show_memory)
            gui_debug_window_memory();
        if (config_debug.show_disassembler)
            gui_debug_window_disassembler();
        if (config_debug.show_call_stack)
            gui_debug_window_call_stack();
        if (config_debug.show_breakpoints)
            gui_debug_window_breakpoints();
        if (config_debug.show_symbols)
            gui_debug_window_symbols();
        if (config_debug.show_huc6260_info)
            gui_debug_window_huc6260_info();
        if (config_debug.show_huc6260_palettes)
            gui_debug_window_huc6260_palettes();
        if (config_debug.show_huc6270_1_registers)
            gui_debug_window_huc6270_registers(1);
        if (config_debug.show_huc6270_1_background)
            gui_debug_window_huc6270_background(1);
        if (config_debug.show_huc6270_1_sprites)
            gui_debug_window_huc6270_sprites(1);
        if (config_debug.show_huc6270_1_info)
            gui_debug_window_huc6270_info(1);
        if (emu_get_core()->GetMedia()->IsSGX())
        {
            if (config_debug.show_huc6202_info)
                gui_debug_window_huc6202_info();
            if (config_debug.show_huc6270_2_registers)
                gui_debug_window_huc6270_registers(2);
            if (config_debug.show_huc6270_2_background)
                gui_debug_window_huc6270_background(2);
            if (config_debug.show_huc6270_2_sprites)
                gui_debug_window_huc6270_sprites(2);
            if (config_debug.show_huc6270_2_info)
                gui_debug_window_huc6270_info(2);
        }
        if (config_debug.show_psg)
            gui_debug_window_psg();
        if (config_debug.show_cdrom && emu_get_core()->GetMedia()->IsCDROM())
            gui_debug_window_cdrom();
        if (config_debug.show_cdrom_audio && emu_get_core()->GetMedia()->IsCDROM())
            gui_debug_window_cdrom_audio();
        if (config_debug.show_adpcm && emu_get_core()->GetMedia()->IsCDROM())
            gui_debug_window_adpcm();
        if (config_debug.show_arcade_card && emu_get_core()->GetMedia()->IsArcadeCard())
            gui_debug_window_arcade_card();
        if (config_debug.show_trace_logger)
            gui_debug_window_trace_logger();

        gui_debug_memory_watches_window();
        gui_debug_memory_search_window();
    }
}

static const char* GGDEBUG_MAGIC = "GGDEBUG1";
static const int GGDEBUG_MAGIC_LEN = 8;

void gui_debug_save_settings(const char* file_path)
{
    std::ofstream file(file_path, std::ios::binary);
    if (!file.is_open())
    {
        Log("Failed to open debug settings file for writing: %s", file_path);
        return;
    }

    file.write(GGDEBUG_MAGIC, GGDEBUG_MAGIC_LEN);

    GeargrafxCore* core = emu_get_core();
    HuC6280* processor = core->GetHuC6280();

    std::vector<HuC6280::GG_Breakpoint>* breakpoints = processor->GetBreakpoints();
    int bp_count = (int)breakpoints->size();
    file.write((const char*)&bp_count, sizeof(int));
    for (int i = 0; i < bp_count; i++)
    {
        HuC6280::GG_Breakpoint& bp = (*breakpoints)[i];
        file.write((const char*)&bp.enabled, sizeof(bool));
        file.write((const char*)&bp.type, sizeof(int));
        file.write((const char*)&bp.address1, sizeof(u16));
        file.write((const char*)&bp.address2, sizeof(u16));
        file.write((const char*)&bp.read, sizeof(bool));
        file.write((const char*)&bp.write, sizeof(bool));
        file.write((const char*)&bp.execute, sizeof(bool));
        file.write((const char*)&bp.range, sizeof(bool));
    }

    file.write((const char*)&emu_debug_irq_breakpoints, sizeof(bool));

    void* bookmarks_ptr = NULL;
    int bookmark_count = gui_debug_get_disassembler_bookmarks(&bookmarks_ptr);
    file.write((const char*)&bookmark_count, sizeof(int));
    if (bookmark_count > 0 && bookmarks_ptr != NULL)
    {
        struct DasmBookmark { u16 address; char name[32]; };
        std::vector<DasmBookmark>* bm_vec = (std::vector<DasmBookmark>*)bookmarks_ptr;
        for (int i = 0; i < bookmark_count; i++)
        {
            file.write((const char*)&(*bm_vec)[i].address, sizeof(u16));
            file.write((*bm_vec)[i].name, 32);
        }
    }

    gui_debug_memory_save_settings(file);

    file.close();

    Log("Debug settings saved to: %s", file_path);
}

void gui_debug_load_settings(const char* file_path)
{
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open())
    {
        Log("Failed to open debug settings file for reading: %s", file_path);
        return;
    }

    char magic[8];
    file.read(magic, GGDEBUG_MAGIC_LEN);
    if (memcmp(magic, GGDEBUG_MAGIC, GGDEBUG_MAGIC_LEN) != 0)
    {
        Log("Invalid debug settings file: %s", file_path);
        file.close();
        return;
    }

    GeargrafxCore* core = emu_get_core();
    HuC6280* processor = core->GetHuC6280();

    processor->ResetBreakpoints();
    int bp_count = 0;
    file.read((char*)&bp_count, sizeof(int));
    std::vector<HuC6280::GG_Breakpoint>* breakpoints = processor->GetBreakpoints();
    for (int i = 0; i < bp_count; i++)
    {
        HuC6280::GG_Breakpoint bp;
        file.read((char*)&bp.enabled, sizeof(bool));
        file.read((char*)&bp.type, sizeof(int));
        file.read((char*)&bp.address1, sizeof(u16));
        file.read((char*)&bp.address2, sizeof(u16));
        file.read((char*)&bp.read, sizeof(bool));
        file.read((char*)&bp.write, sizeof(bool));
        file.read((char*)&bp.execute, sizeof(bool));
        file.read((char*)&bp.range, sizeof(bool));
        breakpoints->push_back(bp);
    }

    file.read((char*)&emu_debug_irq_breakpoints, sizeof(bool));

    gui_debug_reset_disassembler_bookmarks();
    int bookmark_count = 0;
    file.read((char*)&bookmark_count, sizeof(int));
    for (int i = 0; i < bookmark_count; i++)
    {
        u16 address;
        char name[32];
        file.read((char*)&address, sizeof(u16));
        file.read(name, 32);
        gui_debug_add_disassembler_bookmark(address, name);
    }

    gui_debug_memory_load_settings(file);

    file.close();

    Log("Debug settings loaded from: %s", file_path);
}

static std::string get_auto_debug_settings_path(void)
{
    GeargrafxCore* core = emu_get_core();
    if (!core || !core->GetMedia() || strlen(core->GetMedia()->GetFileName()) == 0)
        return "";

    std::string filename = core->GetMedia()->GetFileName();
    std::string::size_type dot = filename.find_last_of('.');
    if (dot != std::string::npos)
        filename = filename.substr(0, dot);
    filename += ".ggdebug";

    std::string path = config_root_path;
    path += filename;
    return path;
}

void gui_debug_auto_save_settings(void)
{
    if (!config_debug.auto_debug_settings)
        return;

    std::string path = get_auto_debug_settings_path();
    if (path.empty())
        return;

    gui_debug_save_settings(path.c_str());
}

void gui_debug_auto_load_settings(void)
{
    if (!config_debug.auto_debug_settings)
        return;

    std::string path = get_auto_debug_settings_path();
    if (path.empty())
        return;

    std::ifstream test(path, std::ios::binary);
    if (!test.is_open())
        return;
    test.close();

    gui_debug_load_settings(path.c_str());
}
