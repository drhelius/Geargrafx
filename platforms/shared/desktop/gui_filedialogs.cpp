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
#include "gui_debug.h"
#include "gui_menus.h"
#include "application.h"
#include "config.h"
#include "emu.h"
#include "nfd.h"
#include "nfd_sdl2.h"

static void file_dialog_set_native_window(SDL_Window* window, nfdwindowhandle_t* native_window);

void gui_file_dialog_open_rom(void)
{
    nfdchar_t *outPath;
    nfdfilteritem_t filterItem[1] = { { "ROM/CD Files", "pce,sgx,hes,cue,chd,zip" } };
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
        Error("Open ROM Error: %s", NFD_GetError());
    }
}

void gui_file_dialog_load_ram(void)
{
    nfdchar_t *outPath;
    nfdfilteritem_t filterItem[1] = { { "BRAM Files", "sav,bram,ram,srm" } };
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
        Error("Load BRAM Error: %s", NFD_GetError());
    }
}

void gui_file_dialog_save_ram(void)
{
    nfdchar_t *outPath;
    nfdfilteritem_t filterItem[1] = { { "BRAM Files", "sav,bram,srm" } };
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
        Error("Save BRAM Error: %s", NFD_GetError());
    }
}

void gui_file_dialog_load_state(void)
{
    nfdchar_t *outPath;
    nfdfilteritem_t filterItem[1] = { { "Save State Files", "state,state1,state2,state3,state4,state5" } };
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
        Error("Load State Error: %s", NFD_GetError());
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
        Error("Save State Error: %s", NFD_GetError());
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
        Error("Savestate Path Error: %s", NFD_GetError());
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
        Error("Screenshot Path Error: %s", NFD_GetError());
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
        Error("Backup RAM Path Error: %s", NFD_GetError());
    }
}

void gui_file_dialog_choose_mb128_path(void)
{
    nfdchar_t *outPath;
    nfdpickfolderu8args_t args = { };
    args.defaultPath = config_emulator.mb128_path.c_str();
    file_dialog_set_native_window(application_sdl_window, &args.parentWindow);

    nfdresult_t result = NFD_PickFolderU8_With(&outPath, &args);
    if (result == NFD_OKAY)
    {
        strncpy_fit(gui_mb128_path, outPath, sizeof(gui_mb128_path));
        config_emulator.mb128_path.assign(outPath);
        NFD_FreePath(outPath);
    }
    else if (result != NFD_CANCEL)
    {
        Error("MB128 Path Error: %s", NFD_GetError());
    }
}

void gui_file_dialog_load_bios(bool syscard)
{
    char* bios_path = syscard ? gui_syscard_bios_path : gui_gameexpress_bios_path;
    std::string* bios_config_path = syscard ? &config_emulator.syscard_bios_path : &config_emulator.gameexpress_bios_path;
    nfdchar_t *outPath;
    nfdfilteritem_t filterItem[1] = { { "BIOS Files", "pce,rom,bios" } };
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
        Error("Load Bios Error: %s", NFD_GetError());
    }
}

void gui_file_dialog_load_symbols(void)
{
    nfdchar_t *outPath;
    nfdopendialogu8args_t args = { };
    args.filterList = NULL;
    args.filterCount = 0;
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
        Error("Load Symbols Error: %s", NFD_GetError());
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
        Error("Save Screenshot Error: %s", NFD_GetError());
    }
}

void gui_file_dialog_save_vgm(void)
{
    nfdchar_t *outPath;
    nfdfilteritem_t filterItem[1] = { { "VGM Files", "vgm" } };
    nfdsavedialogu8args_t args = { };
    args.filterList = filterItem;
    args.filterCount = 1;
    args.defaultPath = NULL;
    args.defaultName = NULL;
    file_dialog_set_native_window(application_sdl_window, &args.parentWindow);

    nfdresult_t result = NFD_SaveDialogU8_With(&outPath, &args);
    if (result == NFD_OKAY)
    {
        emu_start_vgm_recording(outPath);
        gui_set_status_message("VGM recording started", 3000);
        NFD_FreePath(outPath);
    }
    else if (result != NFD_CANCEL)
    {
        Error("Save VGM Error: %s", NFD_GetError());
    }
}

void gui_file_dialog_save_sprite(int vdc, int index)
{
    char default_name[32];
    snprintf(default_name, 32, "sprite_vdc%d_id%02d.png", vdc, index);

    nfdchar_t *outPath;
    nfdfilteritem_t filterItem[1] = { { "PNG Files", "png" } };
    nfdsavedialogu8args_t args = { };
    args.filterList = filterItem;
    args.filterCount = 1;
    args.defaultPath = NULL;
    args.defaultName = default_name;
    file_dialog_set_native_window(application_sdl_window, &args.parentWindow);

    nfdresult_t result = NFD_SaveDialogU8_With(&outPath, &args);
    if (result == NFD_OKAY)
    {
        gui_action_save_sprite(outPath, vdc, index);
        NFD_FreePath(outPath);
    }
    else if (result != NFD_CANCEL)
    {
        Error("Save Sprite Error: %s", NFD_GetError());
    }
}

void gui_file_dialog_save_all_sprites(int vdc)
{
    nfdchar_t *outPath;
    nfdpickfolderu8args_t args = { };
    args.defaultPath = NULL;
    file_dialog_set_native_window(application_sdl_window, &args.parentWindow);

    nfdresult_t result = NFD_PickFolderU8_With(&outPath, &args);
    if (result == NFD_OKAY)
    {
        gui_action_save_all_sprites(outPath, vdc);
        NFD_FreePath(outPath);
    }
    else if (result != NFD_CANCEL)
    {
        Error("Save All Sprites Error: %s", NFD_GetError());
    }
}

void gui_file_dialog_save_background(int vdc)
{
    char default_name[32];
    snprintf(default_name, 32, "background_vdc%d.png", vdc);

    nfdchar_t *outPath;
    nfdfilteritem_t filterItem[1] = { { "PNG Files", "png" } };
    nfdsavedialogu8args_t args = { };
    args.filterList = filterItem;
    args.filterCount = 1;
    args.defaultPath = NULL;
    args.defaultName = default_name;
    file_dialog_set_native_window(application_sdl_window, &args.parentWindow);

    nfdresult_t result = NFD_SaveDialogU8_With(&outPath, &args);
    if (result == NFD_OKAY)
    {
        gui_action_save_background(outPath, vdc);
        NFD_FreePath(outPath);
    }
    else if (result != NFD_CANCEL)
    {
        Error("Save Background Error: %s", NFD_GetError());
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
        Error("Save Memory Dump Error: %s", NFD_GetError());
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
        Error("Save Disassembler Error: %s", NFD_GetError());
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
        Error("Save Log Error: %s", NFD_GetError());
    }
}

void gui_file_dialog_save_debug_settings(void)
{
    nfdchar_t *outPath;
    nfdfilteritem_t filterItem[1] = { { "Debug Settings Files", "ggdebug" } };
    nfdsavedialogu8args_t args = { };
    args.filterList = filterItem;
    args.filterCount = 1;
    args.defaultPath = config_emulator.last_open_path.c_str();

    std::string default_name;
    GeargrafxCore* core = emu_get_core();
    if (core && core->GetMedia() && strlen(core->GetMedia()->GetFileName()) > 0)
    {
        default_name = core->GetMedia()->GetFileName();
        std::string::size_type dot = default_name.find_last_of('.');
        if (dot != std::string::npos)
            default_name = default_name.substr(0, dot);
        default_name += ".ggdebug";
    }

    args.defaultName = default_name.empty() ? NULL : default_name.c_str();
    file_dialog_set_native_window(application_sdl_window, &args.parentWindow);

    nfdresult_t result = NFD_SaveDialogU8_With(&outPath, &args);
    if (result == NFD_OKAY)
    {
        gui_debug_save_settings(outPath);
        gui_set_status_message("Debug settings saved", 3000);
        NFD_FreePath(outPath);
    }
    else if (result != NFD_CANCEL)
    {
        Error("Save Debug Settings Error: %s", NFD_GetError());
    }
}

void gui_file_dialog_load_debug_settings(void)
{
    nfdchar_t *outPath;
    nfdfilteritem_t filterItem[1] = { { "Debug Settings Files", "ggdebug" } };
    nfdopendialogu8args_t args = { };
    args.filterList = filterItem;
    args.filterCount = 1;
    args.defaultPath = config_emulator.last_open_path.c_str();
    file_dialog_set_native_window(application_sdl_window, &args.parentWindow);

    nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);
    if (result == NFD_OKAY)
    {
        gui_debug_load_settings(outPath);
        gui_set_status_message("Debug settings loaded", 3000);
        NFD_FreePath(outPath);
    }
    else if (result != NFD_CANCEL)
    {
        Error("Load Debug Settings Error: %s", NFD_GetError());
    }
}

void gui_file_dialog_load_palette(void)
{
    nfdchar_t *outPath;
    nfdfilteritem_t filterItem[1] = { { "Palette Files", "pal,bin" } };
    nfdopendialogu8args_t args = { };
    args.filterList = filterItem;
    args.filterCount = 1;
    args.defaultPath = config_emulator.last_open_path.c_str();
    file_dialog_set_native_window(application_sdl_window, &args.parentWindow);

    nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);
    if (result == NFD_OKAY)
    {
        gui_load_palette(outPath);
        NFD_FreePath(outPath);
    }
    else if (result != NFD_CANCEL)
    {
        Error("Load Palette Error: %s", NFD_GetError());
    }
}

static void file_dialog_set_native_window(SDL_Window* window, nfdwindowhandle_t* native_window)
{
    if (!NFD_GetNativeWindowFromSDLWindow(window, native_window))
    {
        Log("NFD_GetNativeWindowFromSDLWindow failed: %s\n", SDL_GetError());
    }
}
