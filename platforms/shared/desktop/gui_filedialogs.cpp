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

#define GUI_FILEDIALOGS_IMPORT
#include "gui_filedialogs.h"
#include "gui.h"
#include "gui_actions.h"
#include "config.h"
#include "nfd/nfd.h"

void gui_file_dialog_open_rom(void)
{
    nfdchar_t *outPath;
    nfdfilteritem_t filterItem[1] = { { "ROM Files", "col,cv,rom,bin,zip" } };
    nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, config_emulator.last_open_path.c_str());
    if (result == NFD_OKAY)
    {
        std::string path = outPath;
        std::string::size_type pos = path.find_last_of("\\/");
        config_emulator.last_open_path.assign(path.substr(0, pos));
        gui_load_rom(outPath);
        NFD_FreePath(outPath);
    }
    else if (result != NFD_CANCEL)
    {
        Log("Open ROM Error: %s", NFD_GetError());
    }
}

void gui_file_dialog_load_ram(void)
{
    nfdchar_t *outPath;
    nfdfilteritem_t filterItem[1] = { { "RAM Files", "sav" } };
    nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, NULL);
    if (result == NFD_OKAY)
    {
        // Cartridge::ForceConfiguration config;
        // config.region = get_region(config_emulator.region);
        // config.type = Cartridge::CartridgeNotSupported;

        // emu_load_ram(outPath, config);
        NFD_FreePath(outPath);
    }
    else if (result != NFD_CANCEL)
    {
        Log("Load RAM Error: %s", NFD_GetError());
    }
}

void gui_file_dialog_save_ram(void)
{
    nfdchar_t *outPath;
    nfdfilteritem_t filterItem[1] = { { "RAM Files", "sav" } };
    nfdresult_t result = NFD_SaveDialog(&outPath, filterItem, 1, NULL, NULL);
    if (result == NFD_OKAY)
    {
        // emu_save_ram(outPath);
        NFD_FreePath(outPath);
    }
    else if (result != NFD_CANCEL)
    {
        Log("Save RAM Error: %s", NFD_GetError());
    }
}

void gui_file_dialog_load_state(void)
{
    nfdchar_t *outPath;
    nfdfilteritem_t filterItem[1] = { { "Save State Files", "state" } };
    nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, NULL);
    if (result == NFD_OKAY)
    {
        std::string message("Loading state from ");
        message += outPath;
        gui_set_status_message(message.c_str(), 3000);
        // emu_load_state_file(outPath);
        NFD_FreePath(outPath);
    }
    else if (result != NFD_CANCEL)
    {
        Log("Load State Error: %s", NFD_GetError());
    }
}

void gui_file_dialog_save_state(void)
{
    nfdchar_t *outPath;
    nfdfilteritem_t filterItem[1] = { { "Save State Files", "state" } };
    nfdresult_t result = NFD_SaveDialog(&outPath, filterItem, 1, NULL, NULL);
    if (result == NFD_OKAY)
    {
        std::string message("Saving state to ");
        message += outPath;
        gui_set_status_message(message.c_str(), 3000);
        // emu_save_state_file(outPath);
        NFD_FreePath(outPath);
    }
    else if (result != NFD_CANCEL)
    {
        Log("Save State Error: %s", NFD_GetError());
    }
}

void gui_file_dialog_choose_savestate_path(void)
{
    nfdchar_t *outPath;
    nfdresult_t result = NFD_PickFolder(&outPath, config_emulator.savestates_path.c_str());
    if (result == NFD_OKAY)
    {
        config_emulator.savestates_path.assign(outPath);
        NFD_FreePath(outPath);
    }
    else if (result != NFD_CANCEL)
    {
        Log("Savestate Path Error: %s", NFD_GetError());
    }
}


void gui_file_dialog_load_symbols(void)
{
    nfdchar_t *outPath;
    nfdfilteritem_t filterItem[1] = { { "Symbol Files", "sym" } };
    nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, NULL);
    if (result == NFD_OKAY)
    {
        // gui_debug_reset_symbols();
        // gui_debug_load_symbols_file(outPath);
        NFD_FreePath(outPath);
    }
    else if (result != NFD_CANCEL)
    {
        Log("Load Symbols Error: %s", NFD_GetError());
    }
}

void gui_file_dialog_save_screenshot(void)
{
    nfdchar_t *outPath;
    nfdfilteritem_t filterItem[1] = { { "PNG Files", "png" } };
    nfdresult_t result = NFD_SaveDialog(&outPath, filterItem, 1, NULL, NULL);
    if (result == NFD_OKAY)
    {
        gui_action_save_screenshot(outPath);
        NFD_FreePath(outPath);
    }
    else if (result != NFD_CANCEL)
    {
        Log("Save Screenshot Error: %s", NFD_GetError());
    }
}

