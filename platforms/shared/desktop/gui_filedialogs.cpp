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
#include "gui_debug_memory.h"
#include "gui_debug_disassembler.h"
#include "gui_debug_trace_logger.h"
#include "gui_menus.h"
#include "application.h"
#include "config.h"
#include "emu.h"
#include "nfd/nfd.h"
#include "nfd/nfd_sdl2.h"

static void file_dialog_set_native_window(SDL_Window* window, nfdwindowhandle_t* native_window);

void gui_file_dialog_open_rom(void)
{
    nfdchar_t *outPath;
    nfdfilteritem_t filterItem[1] = { { "ROM/CD Files", "pce,sgx,cue,rom,bin,zip" } };
    nfdopendialogu8args_t args = { };
    args.filterList = filterItem;
    args.filterCount = 1;
    args.defaultPath = config_emulator.last_open_path.c_str();
    file_dialog_set_native_window(application_sdl_window, &args.parentWindow);

    nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);
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
    nfdopendialogu8args_t args = { };
    args.filterList = filterItem;
    args.filterCount = 1;
    args.defaultPath = config_emulator.last_open_path.c_str();
    file_dialog_set_native_window(application_sdl_window, &args.parentWindow);

    nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);
    if (result == NFD_OKAY)
    {
        emu_load_ram(outPath);
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
    nfdsavedialogu8args_t args = { };
    args.filterList = filterItem;
    args.filterCount = 1;
    args.defaultPath = config_emulator.last_open_path.c_str();
    args.defaultName = NULL;
    file_dialog_set_native_window(application_sdl_window, &args.parentWindow);

    nfdresult_t result = NFD_SaveDialogU8_With(&outPath, &args);
    if (result == NFD_OKAY)
    {
        emu_save_ram(outPath);
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
    nfdopendialogu8args_t args = { };
    args.filterList = filterItem;
    args.filterCount = 1;
    args.defaultPath = config_emulator.last_open_path.c_str();
    file_dialog_set_native_window(application_sdl_window, &args.parentWindow);

    nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);
    if (result == NFD_OKAY)
    {
        std::string message("Loading state from ");
        message += outPath;
        gui_set_status_message(message.c_str(), 3000);
        emu_load_state_file(outPath);
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
    nfdsavedialogu8args_t args = { };
    args.filterList = filterItem;
    args.filterCount = 1;
    args.defaultPath = config_emulator.last_open_path.c_str();
    args.defaultName = NULL;
    file_dialog_set_native_window(application_sdl_window, &args.parentWindow);

    nfdresult_t result = NFD_SaveDialogU8_With(&outPath, &args);
    if (result == NFD_OKAY)
    {
        std::string message("Saving state to ");
        message += outPath;
        gui_set_status_message(message.c_str(), 3000);
        emu_save_state_file(outPath);
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
    nfdpickfolderu8args_t args = { };
    args.defaultPath = config_emulator.savestates_path.c_str();
    file_dialog_set_native_window(application_sdl_window, &args.parentWindow);

    nfdresult_t result = NFD_PickFolderU8_With(&outPath, &args);
    if (result == NFD_OKAY)
    {
        strncpy_fit(gui_savestates_path, outPath, sizeof(gui_savestates_path));
        config_emulator.savestates_path.assign(outPath);
        update_savestates_data();
        NFD_FreePath(outPath);
    }
    else if (result != NFD_CANCEL)
    {
        Log("Savestate Path Error: %s", NFD_GetError());
    }
}

void gui_file_dialog_choose_screenshot_path(void)
{
    nfdchar_t *outPath;
    nfdpickfolderu8args_t args = { };
    args.defaultPath = config_emulator.screenshots_path.c_str();
    file_dialog_set_native_window(application_sdl_window, &args.parentWindow);

    nfdresult_t result = NFD_PickFolderU8_With(&outPath, &args);
    if (result == NFD_OKAY)
    {
        strncpy_fit(gui_screenshots_path, outPath, sizeof(gui_screenshots_path));
        config_emulator.screenshots_path.assign(outPath);
        NFD_FreePath(outPath);
    }
    else if (result != NFD_CANCEL)
    {
        Log("Screenshot Path Error: %s", NFD_GetError());
    }
}

void gui_file_dialog_choose_backup_ram_path(void)
{
    nfdchar_t *outPath;
    nfdpickfolderu8args_t args = { };
    args.defaultPath = config_emulator.backup_ram_path.c_str();
    file_dialog_set_native_window(application_sdl_window, &args.parentWindow);

    nfdresult_t result = NFD_PickFolderU8_With(&outPath, &args);
    if (result == NFD_OKAY)
    {
        strncpy_fit(gui_backup_ram_path, outPath, sizeof(gui_backup_ram_path));
        config_emulator.backup_ram_path.assign(outPath);
        NFD_FreePath(outPath);
    }
    else if (result != NFD_CANCEL)
    {
        Log("Backup RAM Path Error: %s", NFD_GetError());
    }
}

void gui_file_dialog_load_bios(bool syscard)
{
    char* bios_path = syscard ? gui_syscard_bios_path : gui_gameexpress_bios_path;
    std::string* bios_config_path = syscard ? &config_emulator.syscard_bios_path : &config_emulator.gameexpress_bios_path;
    nfdchar_t *outPath;
    nfdfilteritem_t filterItem[1] = { { "BIOS Files", "pce,bin,rom,bios" } };
    nfdopendialogu8args_t args = { };
    args.filterList = filterItem;
    args.filterCount = 1;
    args.defaultPath = config_emulator.last_open_path.c_str();
    file_dialog_set_native_window(application_sdl_window, &args.parentWindow);

    nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);
    if (result == NFD_OKAY)
    {
        bios_config_path->assign(outPath);
        strcpy(bios_path, bios_config_path->c_str());
        gui_load_bios(outPath, syscard);
        NFD_FreePath(outPath);
    }
    else if (result != NFD_CANCEL)
    {
        Log("Load Bios Error: %s", NFD_GetError());
    }
}

void gui_file_dialog_load_symbols(void)
{
    nfdchar_t *outPath;
    nfdfilteritem_t filterItem[1] = { { "Symbol Files", "sym" } };
    nfdopendialogu8args_t args = { };
    args.filterList = filterItem;
    args.filterCount = 1;
    args.defaultPath = NULL;
    file_dialog_set_native_window(application_sdl_window, &args.parentWindow);

    nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);
    if (result == NFD_OKAY)
    {
        gui_debug_reset_symbols();
        gui_debug_load_symbols_file(outPath);
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
    nfdsavedialogu8args_t args = { };
    args.filterList = filterItem;
    args.filterCount = 1;
    args.defaultPath = NULL;
    args.defaultName = NULL;
    file_dialog_set_native_window(application_sdl_window, &args.parentWindow);

    nfdresult_t result = NFD_SaveDialogU8_With(&outPath, &args);
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

void gui_file_dialog_save_memory_dump(bool binary)
{
    nfdchar_t *outPath;
    nfdfilteritem_t filterItem[1] = { { "Memory Dump Files", binary ? "bin" : "txt" } };
    nfdsavedialogu8args_t args = { };
    args.filterList = filterItem;
    args.filterCount = 1;
    args.defaultPath = NULL;
    args.defaultName = NULL;
    file_dialog_set_native_window(application_sdl_window, &args.parentWindow);

    nfdresult_t result = NFD_SaveDialogU8_With(&outPath, &args);
    if (result == NFD_OKAY)
    {
        gui_debug_memory_save_dump(outPath, binary);
        NFD_FreePath(outPath);
    }
    else if (result != NFD_CANCEL)
    {
        Log("Save Memory Dump Error: %s", NFD_GetError());
    }
}

void gui_file_dialog_save_disassembler(bool full)
{
    nfdchar_t *outPath;
    nfdfilteritem_t filterItem[1] = { { "Disassembler Files", "txt" } };
    nfdsavedialogu8args_t args = { };
    args.filterList = filterItem;
    args.filterCount = 1;
    args.defaultPath = NULL;
    args.defaultName = NULL;
    file_dialog_set_native_window(application_sdl_window, &args.parentWindow);

    nfdresult_t result = NFD_SaveDialogU8_With(&outPath, &args);
    if (result == NFD_OKAY)
    {
        gui_debug_save_disassembler(outPath, full);
        NFD_FreePath(outPath);
    }
    else if (result != NFD_CANCEL)
    {
        Log("Save Disassembler Error: %s", NFD_GetError());
    }
}

void gui_file_dialog_save_log(void)
{
    nfdchar_t *outPath;
    nfdfilteritem_t filterItem[1] = { { "Log Files", "txt" } };
    nfdsavedialogu8args_t args = { };
    args.filterList = filterItem;
    args.filterCount = 1;
    args.defaultPath = NULL;
    args.defaultName = NULL;
    file_dialog_set_native_window(application_sdl_window, &args.parentWindow);

    nfdresult_t result = NFD_SaveDialogU8_With(&outPath, &args);
    if (result == NFD_OKAY)
    {
        gui_debug_save_log(outPath);
        NFD_FreePath(outPath);
    }
    else if (result != NFD_CANCEL)
    {
        Log("Save Log Error: %s", NFD_GetError());
    }
}

static void file_dialog_set_native_window(SDL_Window* window, nfdwindowhandle_t* native_window)
{
    if (!NFD_GetNativeWindowFromSDLWindow(window, native_window))
    {
        Log("NFD_GetNativeWindowFromSDLWindow failed: %s\n", SDL_GetError());
    }
}
