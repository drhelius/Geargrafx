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

#define GUI_MENUS_IMPORT
#include "gui_menus.h"
#include "gui.h"
#include "gui_filedialogs.h"
#include "gui_popups.h"
#include "gui_actions.h"
#include "gui_debug_disassembler.h"
#include "config.h"
#include "application.h"
#include "display.h"
#include "gamepad.h"
#include "emu.h"
#include "ogl_renderer.h"
#include "utils.h"
#include "geargrafx.h"

static bool open_rom = false;
static bool open_ram = false;
static bool save_ram = false;
static bool open_state = false;
static bool save_state = false;
static bool open_about = false;
static bool save_screenshot = false;
static bool save_vgm = false;
static bool choose_savestates_path = false;
static bool choose_screenshots_path = false;
static bool choose_backup_ram_path = false;
static bool choose_mb128_path = false;
static bool open_syscard_bios = false;
static bool open_gameexpress_bios = false;
static bool save_debug_settings = false;
static bool load_debug_settings = false;

static void menu_geargrafx(void);
static void menu_emulator(void);
static void menu_video(void);
static void menu_input(void);
static void menu_audio(void);
static void menu_debug(void);
static void menu_about(void);
static void file_dialogs(void);
static void keyboard_configuration_item(const char* text, SDL_Scancode* key, int player);
static void gamepad_configuration_item(const char* text, int* button, int player);
static void hotkey_configuration_item(const char* text, config_Hotkey* hotkey);
static void gamepad_device_selector(int player);
static void draw_savestate_slot_info(int slot);

void gui_init_menus(void)
{
    gui_shortcut_open_rom = false;
}

void gui_main_menu(void)
{
    open_rom = false;
    open_ram = false;
    save_ram = false;
    open_state = false;
    save_state = false;
    open_about = false;
    save_screenshot = false;
    save_vgm = false;
    choose_savestates_path = false;
    choose_screenshots_path = false;
    gui_main_menu_hovered = false;
    choose_backup_ram_path = false;
    choose_mb128_path = false;
    open_syscard_bios = false;
    open_gameexpress_bios = false;
    save_debug_settings = false;
    load_debug_settings = false;

    if (application_show_menu && ImGui::BeginMainMenuBar())
    {
        gui_main_menu_hovered = ImGui::IsWindowHovered();

        menu_geargrafx();
        menu_emulator();
        menu_video();
        menu_input();
        menu_audio();
        menu_debug();
        menu_about();

        gui_main_menu_height = (int)ImGui::GetWindowSize().y;

        ImGui::EndMainMenuBar();
    }

    file_dialogs();
}

static void menu_geargrafx(void)
{
    if (ImGui::BeginMenu(GG_TITLE))
    {
        gui_in_use = true;

        if (ImGui::MenuItem("Open ROM/CD...", config_hotkeys[config_HotkeyIndex_OpenROM].str))
        {
            open_rom = true;
        }

        if (ImGui::BeginMenu("Open Recent"))
        {
            for (int i = 0; i < config_max_recent_roms; i++)
            {
                if (config_emulator.recent_roms[i].length() > 0)
                {
                    if (ImGui::MenuItem(config_emulator.recent_roms[i].c_str()))
                    {
                        char rom_path[4096];
                        strcpy(rom_path, config_emulator.recent_roms[i].c_str());
                        gui_load_rom(rom_path);
                    }
                }
            }

            ImGui::EndMenu();
        }

        ImGui::Separator();
        
        if (ImGui::MenuItem("Reset", config_hotkeys[config_HotkeyIndex_Reset].str))
        {
            gui_action_reset();
        }

        if (ImGui::MenuItem("Pause", config_hotkeys[config_HotkeyIndex_Pause].str, &config_emulator.paused))
        {
            gui_action_pause();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Fast Forward", config_hotkeys[config_HotkeyIndex_FFWD].str, &config_emulator.ffwd))
        {
            gui_action_ffwd();
        }

        if (ImGui::BeginMenu("Fast Forward Speed"))
        {
            ImGui::PushItemWidth(100.0f);
            ImGui::Combo("##fwd", &config_emulator.ffwd_speed, "X 1.5\0X 2\0X 2.5\0X 3\0Unlimited\0\0");
            ImGui::PopItemWidth();
            ImGui::EndMenu();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Save BRAM As..."))
        {
            save_ram = true;
        }

        if (ImGui::MenuItem("Load BRAM From..."))
        {
            open_ram = true;
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Save State As...")) 
        {
            save_state = true;
        }

        if (ImGui::MenuItem("Load State From..."))
        {
            open_state = true;
        }

        ImGui::Separator();

        if (ImGui::BeginMenu("Save State Slot"))
        {
            ImGui::PushItemWidth(100.0f);
            ImGui::Combo("##slot", &config_emulator.save_slot, "Slot 1\0Slot 2\0Slot 3\0Slot 4\0Slot 5\0\0");
            ImGui::PopItemWidth();

            ImGui::Separator();
            draw_savestate_slot_info(config_emulator.save_slot);

            ImGui::EndMenu();
        }

        if (ImGui::MenuItem("Save State", config_hotkeys[config_HotkeyIndex_SaveState].str))
        {
            std::string message("Saving state to slot ");
            message += std::to_string(config_emulator.save_slot + 1);
            gui_set_status_message(message.c_str(), 3000);
            emu_save_state_slot(config_emulator.save_slot + 1);
        }

        if (ImGui::MenuItem("Load State", config_hotkeys[config_HotkeyIndex_LoadState].str))
        {
            std::string message("Loading state from slot ");
            message += std::to_string(config_emulator.save_slot + 1);
            gui_set_status_message(message.c_str(), 3000);
            emu_load_state_slot(config_emulator.save_slot + 1);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Slot: %d", config_emulator.save_slot + 1);
            ImGui::Separator();
            draw_savestate_slot_info(config_emulator.save_slot);
            ImGui::EndTooltip();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Save Screenshot As..."))
        {
            save_screenshot = true;
        }

        if (ImGui::MenuItem("Save Screenshot", config_hotkeys[config_HotkeyIndex_Screenshot].str))
        {
            gui_action_save_screenshot(NULL);
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Quit", config_hotkeys[config_HotkeyIndex_Quit].str))
        {
            application_trigger_quit();
        }

        ImGui::EndMenu();
    }
}

static void menu_emulator(void)
{
    if (ImGui::BeginMenu("Emulator"))
    {
        gui_in_use = true;

        if (ImGui::BeginMenu("Save States Dir"))
        {
            ImGui::PushItemWidth(220.0f);
            if (ImGui::Combo("##savestate_option", &config_emulator.savestates_dir_option, "Default Location\0Same as ROM\0Custom Location\0\0"))
            {
                update_savestates_data();
            }

            switch ((Directory_Location)config_emulator.savestates_dir_option)
            {
                case Directory_Location_Default:
                {
                    ImGui::Text("%s", config_root_path);
                    break;
                }
                case Directory_Location_ROM:
                {
                    if (!emu_is_empty())
                        ImGui::Text("%s", emu_get_core()->GetMedia()->GetFileDirectory());
                    break;
                }
                case Directory_Location_Custom:
                {
                    if (ImGui::MenuItem("Choose..."))
                    {
                        choose_savestates_path = true;
                    }

                    ImGui::PushItemWidth(450);
                    if (ImGui::InputText("##savestate_path", gui_savestates_path, IM_ARRAYSIZE(gui_savestates_path), ImGuiInputTextFlags_AutoSelectAll))
                    {
                        config_emulator.savestates_path.assign(gui_savestates_path);
                        update_savestates_data();
                    }
                    ImGui::PopItemWidth();
                    break;
                }
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Backup RAM Dir"))
        {
            ImGui::PushItemWidth(220.0f);
            ImGui::Combo("##backup_ram_option", &config_emulator.backup_ram_dir_option, "Default Location\0Same as ROM\0Custom Location\0\0");

            switch ((Directory_Location)config_emulator.backup_ram_dir_option)
            {
                case Directory_Location_Default:
                {
                    ImGui::Text("%s", config_root_path);
                    break;
                }
                case Directory_Location_ROM:
                {
                    if (!emu_is_empty())
                        ImGui::Text("%s", emu_get_core()->GetMedia()->GetFileDirectory());
                    break;
                }
                case Directory_Location_Custom:
                {
                    if (ImGui::MenuItem("Choose..."))
                    {
                        choose_backup_ram_path = true;
                    }

                    ImGui::PushItemWidth(450);
                    if (ImGui::InputText("##backup_ram_path", gui_backup_ram_path, IM_ARRAYSIZE(gui_backup_ram_path), ImGuiInputTextFlags_AutoSelectAll))
                    {
                        config_emulator.backup_ram_path.assign(gui_backup_ram_path);
                    }
                    ImGui::PopItemWidth();
                    break;
                }
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("MB128 Save Dir"))
        {
            ImGui::PushItemWidth(220.0f);
            ImGui::Combo("##mb128_option", &config_emulator.mb128_dir_option, "Default Location\0Custom Location\0\0");

            switch (config_emulator.mb128_dir_option)
            {
                case 0:
                {
                    ImGui::Text("%s", config_root_path);
                    break;
                }
                case 1:
                {
                    if (ImGui::MenuItem("Choose..."))
                    {
                        choose_mb128_path = true;
                    }

                    ImGui::PushItemWidth(450);
                    if (ImGui::InputText("##mb128_path", gui_mb128_path, IM_ARRAYSIZE(gui_mb128_path), ImGuiInputTextFlags_AutoSelectAll))
                    {
                        config_emulator.mb128_path.assign(gui_mb128_path);
                    }
                    ImGui::PopItemWidth();
                    break;
                }
                default:
                    break;
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Screenshots Dir"))
        {
            ImGui::PushItemWidth(220.0f);
            ImGui::Combo("##screenshots_option", &config_emulator.screenshots_dir_option, "Default Location\0Same as ROM\0Custom Location\0\0");

            switch ((Directory_Location)config_emulator.screenshots_dir_option)
            {
                case Directory_Location_Default:
                {
                    ImGui::Text("%s", config_root_path);
                    break;
                }
                case Directory_Location_ROM:
                {
                    if (!emu_is_empty())
                        ImGui::Text("%s", emu_get_core()->GetMedia()->GetFileDirectory());
                    break;
                }
                case Directory_Location_Custom:
                {
                    if (ImGui::MenuItem("Choose..."))
                    {
                        choose_screenshots_path = true;
                    }

                    ImGui::PushItemWidth(450);
                    if (ImGui::InputText("##screenshots_path", gui_screenshots_path, IM_ARRAYSIZE(gui_screenshots_path), ImGuiInputTextFlags_AutoSelectAll))
                    {
                        config_emulator.screenshots_path.assign(gui_screenshots_path);
                    }
                    ImGui::PopItemWidth();
                    break;
                }
            }

            ImGui::EndMenu();
        }

        ImGui::Separator();

        if (ImGui::BeginMenu("BIOS"))
        {
            if (ImGui::BeginMenu("System Card"))
            {
                if (ImGui::MenuItem("Load System Card BIOS..."))
                {
                    open_syscard_bios = true;
                }
                ImGui::PushItemWidth(350);
                if (ImGui::InputText("##syscard_bios_path", gui_syscard_bios_path, IM_ARRAYSIZE(gui_syscard_bios_path), ImGuiInputTextFlags_AutoSelectAll))
                {
                    config_emulator.syscard_bios_path.assign(gui_syscard_bios_path);
                    gui_load_bios(gui_syscard_bios_path, true);
                }
                ImGui::PopItemWidth();

                ImGui::Separator();
                if (emu_get_core()->GetMedia()->IsValidBios(true))
                {
                    ImGui::TextColored(ImVec4(0.10f, 0.90f, 0.10f, 1.0f), "Valid BIOS: %s", emu_get_core()->GetMedia()->GetBiosName(true));
                }
                else
                {
                    ImGui::TextColored(ImVec4(0.98f, 0.15f, 0.45f, 1.0f), "System Card BIOS not loaded or invalid!");
                    ImGui::TextColored(ImVec4(0.98f, 0.15f, 0.45f, 1.0f), "System Card 3.0 recommended for most games.");
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Games Express"))
            {
                if (ImGui::MenuItem("Load Game Express BIOS..."))
                {
                    open_gameexpress_bios = true;
                }
                ImGui::PushItemWidth(350);
                if (ImGui::InputText("##gameexpress_bios_path", gui_gameexpress_bios_path, IM_ARRAYSIZE(gui_gameexpress_bios_path), ImGuiInputTextFlags_AutoSelectAll))
                {
                    config_emulator.gameexpress_bios_path.assign(gui_gameexpress_bios_path);
                    gui_load_bios(gui_gameexpress_bios_path, false);
                }
                ImGui::PopItemWidth();

                ImGui::Separator();
                if (emu_get_core()->GetMedia()->IsValidBios(false))
                {
                    ImGui::TextColored(ImVec4(0.10f, 0.90f, 0.10f, 1.0f), "Valid BIOS: %s", emu_get_core()->GetMedia()->GetBiosName(false));
                }
                else
                {
                    ImGui::TextColored(ImVec4(0.98f, 0.15f, 0.45f, 1.0f), "Game Express BIOS not loaded or invalid!");
                }

                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        ImGui::Separator();

        if (ImGui::BeginMenu("Console Model"))
        {
            ImGui::PushItemWidth(170.0f);
            if (ImGui::Combo("##consoletype", &config_emulator.console_type, "Auto\0PC Engine (JAP)\0SuperGrafx (JAP)\0TurboGrafx-16 (USA)\0\0"))
            {
                emu_set_console_type((GG_Console_Type)config_emulator.console_type);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("It is recommended to leave this option on Auto.");
                ImGui::Text("Many USA games will fail to start on Japanese systems.");
                ImGui::Text("Reset the emulator to apply changes.");
                ImGui::EndTooltip();
            }

            ImGui::EndMenu();
        }

        ImGui::Separator();

        if (ImGui::BeginMenu("CD-ROM Model"))
        {
            ImGui::PushItemWidth(150.0f);
            if (ImGui::Combo("##cdromtype", &config_emulator.cdrom_type, "Auto\0Standard\0Super CD-ROM\0Arcade CD-ROM\0\0"))
            {
                emu_set_cdrom_type((GG_CDROM_Type)config_emulator.cdrom_type);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("It is recommended to leave this option on Auto.");
                ImGui::Text("Reset the emulator to apply changes.");
                ImGui::EndTooltip();
            }

            ImGui::EndMenu();
        }

        if (ImGui::MenuItem("Preload CD-ROM in RAM", "", &config_emulator.preload_cdrom))
        {
            emu_set_preload_cdrom(config_emulator.preload_cdrom);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("This option will preload all CD-ROM tracks in RAM.");
            ImGui::Text("Load a new CD-ROM image to apply changes.");
            ImGui::EndTooltip();
        }

        if (ImGui::MenuItem("Force Backup RAM", "", &config_emulator.backup_ram))
        {
            emu_set_backup_ram(config_emulator.backup_ram);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("This option will add backup RAM support to HuCard games.");
            ImGui::Text("It is recommended to leave this option enabled.");
            ImGui::Text("Reset the emulator to apply changes.");
            ImGui::EndTooltip();
        }

        ImGui::Separator();

        if (ImGui::BeginMenu("Memory Base 128"))
        {
            ImGui::PushItemWidth(100.0f);
            if (ImGui::Combo("##mb128_backup", &config_emulator.mb128_mode, "Auto\0Enabled\0Disabled\0\0"))
            {
                emu_set_mb128_mode((GG_MB128_Mode)config_emulator.mb128_mode);
            }
            ImGui::EndMenu();
        }

        ImGui::TextColored(ImVec4(0.50f, 0.50f, 0.50f, 1.0f), "Status: %s", emu_get_core()->GetInput()->GetMB128()->IsConnected() ? "CONNECTED" : "DISCONNECTED");

        ImGui::Separator();

        ImGui::MenuItem("Show ROM info", "", &config_emulator.show_info);
        ImGui::MenuItem("Status Messages", "", &config_emulator.status_messages);

        ImGui::Separator();

        ImGui::MenuItem("Start Paused", "", &config_emulator.start_paused);
        ImGui::MenuItem("Pause When Inactive", "", &config_emulator.pause_when_inactive);

        ImGui::Separator();

        if (ImGui::BeginMenu("Hotkeys"))
        {
            hotkey_configuration_item("Open ROM:", &config_hotkeys[config_HotkeyIndex_OpenROM]);
            hotkey_configuration_item("Quit:", &config_hotkeys[config_HotkeyIndex_Quit]);
            hotkey_configuration_item("Reset:", &config_hotkeys[config_HotkeyIndex_Reset]);
            hotkey_configuration_item("Pause:", &config_hotkeys[config_HotkeyIndex_Pause]);
            hotkey_configuration_item("Fast Forward:", &config_hotkeys[config_HotkeyIndex_FFWD]);
            hotkey_configuration_item("Save State:", &config_hotkeys[config_HotkeyIndex_SaveState]);
            hotkey_configuration_item("Load State:", &config_hotkeys[config_HotkeyIndex_LoadState]);
            hotkey_configuration_item("Save State Slot 1:", &config_hotkeys[config_HotkeyIndex_SelectSlot1]);
            hotkey_configuration_item("Save State Slot 2:", &config_hotkeys[config_HotkeyIndex_SelectSlot2]);
            hotkey_configuration_item("Save State Slot 3:", &config_hotkeys[config_HotkeyIndex_SelectSlot3]);
            hotkey_configuration_item("Save State Slot 4:", &config_hotkeys[config_HotkeyIndex_SelectSlot4]);
            hotkey_configuration_item("Save State Slot 5:", &config_hotkeys[config_HotkeyIndex_SelectSlot5]);
            hotkey_configuration_item("Screenshot:", &config_hotkeys[config_HotkeyIndex_Screenshot]);
            hotkey_configuration_item("Fullscreen:", &config_hotkeys[config_HotkeyIndex_Fullscreen]);
            hotkey_configuration_item("Show Main Menu:", &config_hotkeys[config_HotkeyIndex_ShowMainMenu]);

            gui_popup_modal_hotkey();

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Debug Hotkeys"))
        {
            hotkey_configuration_item("Reload ROM:", &config_hotkeys[config_HotkeyIndex_ReloadROM]);
            hotkey_configuration_item("Step Into:", &config_hotkeys[config_HotkeyIndex_DebugStepInto]);
            hotkey_configuration_item("Step Over:", &config_hotkeys[config_HotkeyIndex_DebugStepOver]);
            hotkey_configuration_item("Step Out:", &config_hotkeys[config_HotkeyIndex_DebugStepOut]);
            hotkey_configuration_item("Step Frame:", &config_hotkeys[config_HotkeyIndex_DebugStepFrame]);
            hotkey_configuration_item("Continue:", &config_hotkeys[config_HotkeyIndex_DebugContinue]);
            hotkey_configuration_item("Break:", &config_hotkeys[config_HotkeyIndex_DebugBreak]);
            hotkey_configuration_item("Run to Cursor:", &config_hotkeys[config_HotkeyIndex_DebugRunToCursor]);
            hotkey_configuration_item("Toggle Breakpoint:", &config_hotkeys[config_HotkeyIndex_DebugBreakpoint]);
            hotkey_configuration_item("Go Back:", &config_hotkeys[config_HotkeyIndex_DebugGoBack]);

            gui_popup_modal_hotkey();

            ImGui::EndMenu();
        }

        ImGui::Separator();

        ImGui::MenuItem("Single Instance", "", &config_debug.single_instance);
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("RESTART REQUIRED");
            ImGui::NewLine();
            ImGui::Text("When enabled, opening a ROM while another instance is running");
            ImGui::Text("will send the ROM to the running instance instead of");
            ImGui::Text("starting a new one.");
            ImGui::EndTooltip();
        }

        ImGui::EndMenu();
    }
}

static void menu_video(void)
{
    if (ImGui::BeginMenu("Video"))
    {
        gui_in_use = true;

        if (ImGui::MenuItem("Full Screen", config_hotkeys[config_HotkeyIndex_Fullscreen].str, &config_emulator.fullscreen))
        {
            application_trigger_fullscreen(config_emulator.fullscreen);
        }

#if !defined(__APPLE__)
        if (ImGui::BeginMenu("Fullscreen Mode"))
        {
            ImGui::PushItemWidth(130.0f);
            ImGui::Combo("##fullscreen_mode", &config_emulator.fullscreen_mode, "Full Screen Desktop\0Full Screen\0\0");
            ImGui::PopItemWidth();
            ImGui::EndMenu();
        }
#endif

        ImGui::Separator();

        ImGui::MenuItem("Always Show Menu", config_hotkeys[config_HotkeyIndex_ShowMainMenu].str, &config_emulator.always_show_menu);
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("This option will enable menu even in fullscreen.");
            ImGui::Text("Menu always shows in debug mode.");
            ImGui::EndTooltip();
        }

        if (ImGui::MenuItem("Resize Window to Content"))
        {
            if (!config_debug.debug)
            {
                application_trigger_fit_to_content(gui_main_window_width, gui_main_window_height + gui_main_menu_height);
            }
        }

        ImGui::Separator();

        if (ImGui::BeginMenu("Scale"))
        {
            ImGui::PushItemWidth(250.0f);
            ImGui::Combo("##scale", &config_video.scale, "Integer Scale (Auto)\0Integer Scale (Manual)\0Scale to Window Height\0Scale to Window Width & Height\0\0");
            if (config_video.scale == 1)
                ImGui::SliderInt("##scale_manual", &config_video.scale_manual, 1, 10);
            ImGui::PopItemWidth();
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Aspect Ratio"))
        {
            ImGui::PushItemWidth(190.0f);
            ImGui::Combo("##ratio", &config_video.ratio, "Square Pixels (1:1 PAR)\0Standard (4:3 DAR)\0Wide (16:9 DAR)\0Wide (16:10 DAR)\0PCE (6:5 DAR)\0\0");
            ImGui::PopItemWidth();
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Overscan"))
        {
            ImGui::PushItemWidth(100.0f);
            if (ImGui::Combo("##overscan", &config_video.overscan, "Disabled\0Enabled\0\0"))
            {
                emu_set_overscan(config_debug.debug ? 0 : config_video.overscan);
            }
            ImGui::PopItemWidth();
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Scanline Count"))
        {
            ImGui::PushItemWidth(110.0f);
            if (ImGui::Combo("##scanline_mode", &config_video.scanline_mode, "Mode 224p\0Mode 240p\0Manual\0\0"))
            {
                if (config_video.scanline_mode == 0)
                {
                    config_video.scanline_start = 11;
                    config_video.scanline_end = 234;
                }
                else if (config_video.scanline_mode == 1)
                {
                    config_video.scanline_start = 2;
                    config_video.scanline_end = 241;
                }
                emu_set_scanline_start_end(config_video.scanline_start, config_video.scanline_end);
            }

            if (config_video.scanline_mode == 2)
            {
                ImGui::Separator();
                ImGui::TextDisabled("Displaying %d scanlines:", MAX(0, config_video.scanline_end - config_video.scanline_start + 1));
                ImGui::PushItemWidth(250.0f);
                if (ImGui::SliderInt("##scanline_start", &config_video.scanline_start, 0, 241, "Start line = %d"))
                {
                    emu_set_scanline_start_end(
                    config_debug.debug ? 0 : config_video.scanline_start,
                    config_debug.debug ? 241 : config_video.scanline_end);
                }
                if (ImGui::SliderInt("##scanline_end", &config_video.scanline_end, 0, 241, "End line = %d"))
                {
                    emu_set_scanline_start_end(
                    config_debug.debug ? 0 : config_video.scanline_start,
                    config_debug.debug ? 241 : config_video.scanline_end);
                }
                if (ImGui::Button("Show all scanlines", ImVec2(250.0f, 0)))
                {
                    if (!config_debug.debug)
                    {
                        config_video.scanline_start = 0;
                        config_video.scanline_end = 241;
                        emu_set_scanline_start_end(config_video.scanline_start, config_video.scanline_end);
                    }
                }
            }
            ImGui::EndMenu();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Vertical Sync", "", &config_video.sync))
        {
            display_set_vsync(config_video.sync);

            if (config_video.sync)
            {
                config_audio.sync = true;
                config_emulator.ffwd = false;
                emu_audio_reset();
            }
        }

        ImGui::MenuItem("Show FPS", "", &config_video.fps);

        ImGui::Separator();

        if (ImGui::BeginMenu("Color Palette"))
        {
            ImGui::PushItemWidth(180.0f);
            if (ImGui::Combo("##palette", &config_video.palette, "Standard RGB\0Composite RGB\0Custom\0\0"))
            {
                emu_set_palette(config_video.palette);
            }
            ImGui::PopItemWidth();

            if (ImGui::MenuItem("Load Custom Palette..."))
            {
                gui_file_dialog_load_palette();
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Format: 512 RGB entries (R, G, B)");
                ImGui::Text("Size: 1536 bytes (0x600)");
                ImGui::Text("Extensions: .pal, .bin");
                ImGui::EndTooltip();
            }

            ImGui::Separator();

            if (gui_custom_palette_loaded)
            {
                ImGui::TextColored(ImVec4(0.10f, 0.90f, 0.10f, 1.0f), "Custom palette loaded");
            }
            else
            {
                ImGui::TextColored(ImVec4(0.50f, 0.50f, 0.50f, 1.0f), "No custom palette loaded");
            }
            ImGui::EndMenu();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Disable Sprite Limit", "", &config_video.sprite_limit))
        {
            emu_video_no_sprite_limit(config_video.sprite_limit);
        }

        ImGui::MenuItem("Bilinear Filtering", "", &config_video.bilinear);

        if (ImGui::BeginMenu("Screen Ghosting"))
        {
            ImGui::MenuItem("Enable Screen Ghosting", "", &config_video.mix_frames);
            ImGui::SliderFloat("##screen_ghosting", &config_video.mix_frames_intensity, 0.0f, 1.0f, "Intensity = %.2f");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Scanlines"))
        {
            ImGui::MenuItem("Enable Scanlines", "", &config_video.scanlines);
            ImGui::MenuItem("Enable Scanlines Filter", "", &config_video.scanlines_filter);
            ImGui::SliderFloat("##scanlines", &config_video.scanlines_intensity, 0.0f, 1.0f, "Intensity = %.2f");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Low Pass Filter"))
        {
            if (ImGui::MenuItem("Enable Low Pass Filter", "", &config_video.lowpass_filter))
            {
                emu_set_lowpass_filter(config_video.lowpass_filter, config_video.lowpass_intensity, config_video.lowpass_cutoff_mhz,
                    config_video.lowpass_speed[0], config_video.lowpass_speed[1], config_video.lowpass_speed[2]);
            }
            ImGui::Separator();
            ImGui::PushItemWidth(180.0f);
            if (ImGui::SliderFloat("##lpf_intensity", &config_video.lowpass_intensity, 0.0f, 1.0f, "Intensity = %.2f"))
            {
                emu_set_lowpass_filter(config_video.lowpass_filter, config_video.lowpass_intensity, config_video.lowpass_cutoff_mhz,
                    config_video.lowpass_speed[0], config_video.lowpass_speed[1], config_video.lowpass_speed[2]);
            }
            ImGui::PushItemWidth(180.0f);
            if (ImGui::SliderFloat("##lpf_cutoff", &config_video.lowpass_cutoff_mhz, 3.0f, 7.0f, "Cutoff = %.1f MHz"))
            {
                emu_set_lowpass_filter(config_video.lowpass_filter, config_video.lowpass_intensity, config_video.lowpass_cutoff_mhz,
                    config_video.lowpass_speed[0], config_video.lowpass_speed[1], config_video.lowpass_speed[2]);
            }

            ImGui::Separator();
            ImGui::Text("Apply to speeds:");
            if (ImGui::Checkbox("5.36 MHz (256px)", &config_video.lowpass_speed[0]))
            {
                emu_set_lowpass_filter(config_video.lowpass_filter, config_video.lowpass_intensity, config_video.lowpass_cutoff_mhz,
                    config_video.lowpass_speed[0], config_video.lowpass_speed[1], config_video.lowpass_speed[2]);
            }
            if (ImGui::Checkbox("7.16 MHz (341px)", &config_video.lowpass_speed[1]))
            {
                emu_set_lowpass_filter(config_video.lowpass_filter, config_video.lowpass_intensity, config_video.lowpass_cutoff_mhz,
                    config_video.lowpass_speed[0], config_video.lowpass_speed[1], config_video.lowpass_speed[2]);
            }
            if (ImGui::Checkbox("10.8 MHz (512px)", &config_video.lowpass_speed[2]))
            {
                emu_set_lowpass_filter(config_video.lowpass_filter, config_video.lowpass_intensity, config_video.lowpass_cutoff_mhz,
                    config_video.lowpass_speed[0], config_video.lowpass_speed[1], config_video.lowpass_speed[2]);
            }

            ImGui::EndMenu();
        }

        ImGui::Separator();

        if (ImGui::BeginMenu("Background Color"))
        {
            ImGui::ColorEdit3("##normal_bg", config_video.background_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_Float);
            ImGui::SameLine();
            ImGui::Text("Normal Background");

            ImGui::Separator();

            if (ImGui::ColorEdit3("##debugger_bg", config_video.background_color_debugger, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_Float))
            {
                ImGuiStyle& style = ImGui::GetStyle();
                style.Colors[ImGuiCol_DockingEmptyBg] = ImVec4(config_video.background_color_debugger[0], config_video.background_color_debugger[1], config_video.background_color_debugger[2], 1.0f);
            }
            ImGui::SameLine();
            ImGui::Text("Debugger Background");

            ImGui::EndMenu();
        }

        ImGui::EndMenu();
    }
}

static void menu_input(void)
{
    if (ImGui::BeginMenu("Input"))
    {
        gui_in_use = true;

        if (ImGui::MenuItem("Enable Turbo Tap", "", &config_input.turbo_tap))
        {
            emu_set_turbo_tap(config_input.turbo_tap);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("It is recommended to keep this option disabled if");
            ImGui::Text("you are using the emulator in single player only.");
            ImGui::EndTooltip();
        }

        if (ImGui::BeginMenu("Controller"))
        {
            for (int i = 0; i < GG_MAX_GAMEPADS; i++)
            {
                char player_name[32];
                snprintf(player_name, sizeof(player_name), "Player %d", i + 1);

                if (ImGui::BeginMenu(player_name))
                {
                    ImGui::PushItemWidth(200.0f);
                    if (ImGui::Combo("##controller", &config_input.controller_type[i], "Standard Pad (2 buttons)\0Avenue Pad 3 (3 buttons)\0Avenue Pad 6 (6 buttons)\0\0"))
                    {
                        emu_set_pad_type((GG_Controllers)i, (GG_Controller_Type)config_input.controller_type[i]);
                    }
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::BeginTooltip();
                        ImGui::Text("It is recommended to select Avenue Pad 6");
                        ImGui::Text("only for games that support it.");
                        ImGui::EndTooltip();
                    }

                    if (config_input.controller_type[i] == GG_CONTROLLER_AVENUE_PAD_3)
                    {
                        ImGui::Separator();
                        ImGui::TextDisabled("Avenue Pad 3 Switch:");
                        ImGui::PushItemWidth(200.0f);
                        if (ImGui::Combo("##avenue_pad_3", &config_input.avenue_pad_3_button[i], "Auto\0SELECT\0RUN\0\0"))
                        {
                            GG_Keys key = GG_KEY_NONE;
                            if (config_input.avenue_pad_3_button[i] == 1)
                                key = GG_KEY_SELECT;
                            else if (config_input.avenue_pad_3_button[i] == 2)
                                key = GG_KEY_RUN;
                            emu_set_avenue_pad_3_button((GG_Controllers)i, key);
                        }
                        ImGui::PopItemWidth();
                        if (ImGui::IsItemHovered())
                        {
                            ImGui::BeginTooltip();
                            ImGui::Text("\"Auto\" will automatically choose SELECT or RUN");
                            ImGui::Text("depending on the game being played.");
                            ImGui::EndTooltip();
                        }
                    }

                    ImGui::Separator();

                    if (ImGui::MenuItem("Turbo I", "", &config_input.turbo_enabled[i][0]))
                    {
                        emu_set_turbo((GG_Controllers)i, GG_KEY_I, config_input.turbo_enabled[i][0]);
                    }
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::BeginTooltip();
                        ImGui::Text("Click to enable or disable Turbo I for Player %d.", i + 1);
                        ImGui::EndTooltip();
                    }

                    if (ImGui::SliderInt("##turbo_speed_i", &config_input.turbo_speed[i][0], 1, 20, "Turbo I Speed = %d", ImGuiSliderFlags_AlwaysClamp))
                    {
                        emu_set_turbo_speed((GG_Controllers)i, GG_KEY_I, config_input.turbo_speed[i][0]);
                    }
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::BeginTooltip();
                        ImGui::Text("The number of frames between each button I toggle.");
                        ImGui::EndTooltip();
                    }

                    if (ImGui::MenuItem("Turbo II", "", &config_input.turbo_enabled[i][1]))
                    {
                        emu_set_turbo((GG_Controllers)i, GG_KEY_II, config_input.turbo_enabled[i][1]);
                    }
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::BeginTooltip();
                        ImGui::Text("Click to enable or disable Turbo II for Player %d.", i + 1);
                        ImGui::EndTooltip();
                    }

                    if(ImGui::SliderInt("##turbo_speed_ii", &config_input.turbo_speed[i][1], 1, 20, "Turbo II Speed = %d", ImGuiSliderFlags_AlwaysClamp))
                    {
                        emu_set_turbo_speed((GG_Controllers)i, GG_KEY_II, config_input.turbo_speed[i][1]);
                    }
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::BeginTooltip();
                        ImGui::Text("The number of frames between each button II toggle.");
                        ImGui::EndTooltip();
                    }

                    ImGui::EndMenu();
                }
            }

            ImGui::EndMenu();
        }

        ImGui::Separator();

        if (ImGui::BeginMenu("Keyboard"))
        {
            for (int i = 0; i < GG_MAX_GAMEPADS; i++)
            {
                char keyboard_name[32];
                snprintf(keyboard_name, sizeof(keyboard_name), "Player %d", i + 1);

                if (ImGui::BeginMenu(keyboard_name))
                {
                    ImGui::TextDisabled("Keyboard %s", keyboard_name);
                    ImGui::Separator();
                    keyboard_configuration_item("Left:", &config_input_keyboard[i].key_left, i);
                    keyboard_configuration_item("Right:", &config_input_keyboard[i].key_right, i);
                    keyboard_configuration_item("Up:", &config_input_keyboard[i].key_up, i);
                    keyboard_configuration_item("Down:", &config_input_keyboard[i].key_down, i);
                    keyboard_configuration_item("Select:", &config_input_keyboard[i].key_select, i);
                    keyboard_configuration_item("Run:", &config_input_keyboard[i].key_run, i);
                    keyboard_configuration_item("I:", &config_input_keyboard[i].key_I, i);
                    keyboard_configuration_item("II:", &config_input_keyboard[i].key_II, i);
                    ImGui::Separator();
                    ImGui::TextDisabled("Avenue Pad 3/6:");
                    keyboard_configuration_item("III:", &config_input_keyboard[i].key_III, i);
                    ImGui::Separator();
                    ImGui::TextDisabled("Avenue Pad 6:");
                    keyboard_configuration_item("IV:", &config_input_keyboard[i].key_IV, i);
                    keyboard_configuration_item("V:", &config_input_keyboard[i].key_V, i);
                    keyboard_configuration_item("VI:", &config_input_keyboard[i].key_VI, i);
                    ImGui::Separator();
                    ImGui::TextDisabled("Turbo:");
                    keyboard_configuration_item("Toggle Turbo I:", &config_input_keyboard[i].key_toggle_turbo_I, i);
                    keyboard_configuration_item("Toggle Turbo II:", &config_input_keyboard[i].key_toggle_turbo_II, i);

                    gui_popup_modal_keyboard();

                    ImGui::EndMenu();
                }
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Gamepads"))
        {
            for (int i = 0; i < GG_MAX_GAMEPADS; i++)
            {
                char gamepad_name[32];
                snprintf(gamepad_name, sizeof(gamepad_name), "Player %d", i + 1);

                if (ImGui::BeginMenu(gamepad_name))
                {
                    if (!gamepad_controller[i])
                    {
                        ImGui::TextDisabled("This gamepad is not detected");
                        ImGui::Separator();
                    }
                    else if (!config_input.turbo_tap && (i > 0))
                    {
                        ImGui::TextDisabled("Gamepad detected for Player %d", i + 1);
                        ImGui::TextDisabled("But Turbo Tap is disabled:");
                        ImGui::TextDisabled("This gamepad will not be used");
                        ImGui::Separator();
                    }
                    else
                    {
                        ImGui::TextDisabled("Gamepad detected for Player %d", i + 1);
                        ImGui::Separator();
                    }

                    if (ImGui::BeginMenu("Device"))
                    {
                        gamepad_device_selector(i);
                        ImGui::EndMenu();
                    }

                    if (ImGui::BeginMenu("Directional Controls"))
                    {
                        ImGui::PushItemWidth(150.0f);
                        ImGui::Combo("##directional", &config_input_gamepad[i].gamepad_directional, "D-pad\0Left Analog Stick\0\0");
                        ImGui::PopItemWidth();
                        ImGui::EndMenu();
                    }

                    if (ImGui::BeginMenu("Button Configuration"))
                    {
                        ImGui::TextDisabled("Gamepad %s", gamepad_name);
                        ImGui::Separator();
                        gamepad_configuration_item("Select:", &config_input_gamepad[i].gamepad_select, i);
                        gamepad_configuration_item("Run:", &config_input_gamepad[i].gamepad_run, i);
                        gamepad_configuration_item("I:", &config_input_gamepad[i].gamepad_I, i);
                        gamepad_configuration_item("II:", &config_input_gamepad[i].gamepad_II, i);
                        ImGui::Separator();
                        ImGui::TextDisabled("Avenue Pad%s:", config_input.controller_type[i] == 1 ? "" : " (disabled)");
                        gamepad_configuration_item("III:", &config_input_gamepad[i].gamepad_III, i);
                        gamepad_configuration_item("IV:", &config_input_gamepad[i].gamepad_IV, i);
                        gamepad_configuration_item("V:", &config_input_gamepad[i].gamepad_V, i);
                        gamepad_configuration_item("VI:", &config_input_gamepad[i].gamepad_VI, i);
                        ImGui::Separator();
                        ImGui::TextDisabled("Turbo:");
                        gamepad_configuration_item("Toggle Turbo I:", &config_input_gamepad[i].gamepad_toggle_turbo_I, i);
                        gamepad_configuration_item("Toggle Turbo II:", &config_input_gamepad[i].gamepad_toggle_turbo_II, i);

                        gui_popup_modal_gamepad(i);

                        ImGui::EndMenu();
                    }

                    if (ImGui::BeginMenu("Shortcut Configuration"))
                    {
                        ImGui::TextDisabled("Gamepad %s - Shortcuts", gamepad_name);
                        ImGui::Separator();

                        gamepad_configuration_item("Save State:", &config_input_gamepad_shortcuts[i].gamepad_shortcuts[config_HotkeyIndex_SaveState], i);
                        gamepad_configuration_item("Load State:", &config_input_gamepad_shortcuts[i].gamepad_shortcuts[config_HotkeyIndex_LoadState], i);
                        gamepad_configuration_item("Save State Slot 1:", &config_input_gamepad_shortcuts[i].gamepad_shortcuts[config_HotkeyIndex_SelectSlot1], i);
                        gamepad_configuration_item("Save State Slot 2:", &config_input_gamepad_shortcuts[i].gamepad_shortcuts[config_HotkeyIndex_SelectSlot2], i);
                        gamepad_configuration_item("Save State Slot 3:", &config_input_gamepad_shortcuts[i].gamepad_shortcuts[config_HotkeyIndex_SelectSlot3], i);
                        gamepad_configuration_item("Save State Slot 4:", &config_input_gamepad_shortcuts[i].gamepad_shortcuts[config_HotkeyIndex_SelectSlot4], i);
                        gamepad_configuration_item("Save State Slot 5:", &config_input_gamepad_shortcuts[i].gamepad_shortcuts[config_HotkeyIndex_SelectSlot5], i);

                        ImGui::Separator();

                        gamepad_configuration_item("Reset:", &config_input_gamepad_shortcuts[i].gamepad_shortcuts[config_HotkeyIndex_Reset], i);
                        gamepad_configuration_item("Pause:", &config_input_gamepad_shortcuts[i].gamepad_shortcuts[config_HotkeyIndex_Pause], i);
                        gamepad_configuration_item("Fast Forward:", &config_input_gamepad_shortcuts[i].gamepad_shortcuts[config_HotkeyIndex_FFWD], i);
                        gamepad_configuration_item("Screenshot:", &config_input_gamepad_shortcuts[i].gamepad_shortcuts[config_HotkeyIndex_Screenshot], i);

                        gui_popup_modal_gamepad(i);

                        ImGui::EndMenu();
                    }

                    ImGui::EndMenu();
                }
            }

            ImGui::EndMenu();
        }

        ImGui::EndMenu();
    }
}

static void menu_audio(void)
{
    if (ImGui::BeginMenu("Audio"))
    {
        gui_in_use = true;

        if (ImGui::MenuItem("Enable Audio", "", &config_audio.enable))
        {
            emu_audio_mute(!config_audio.enable);
        }

        if (ImGui::MenuItem("HuC6280A PSG", "", &config_audio.huc6280a))
        {
            emu_audio_huc6280a(config_audio.huc6280a);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("When enabled, this option will emulate the HuC6280A audio chip.");
            ImGui::Text("This chip will reduce clicks and pops in the audio output.");
            ImGui::EndTooltip();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Mute PSG", "", &gui_audio_mute_psg, config_audio.enable))
        {
            emu_audio_psg_volume(gui_audio_mute_psg ? 0 : config_audio.psg_volume);
        }

        if (ImGui::MenuItem("Mute CD-ROM", "", &gui_audio_mute_cdrom, config_audio.enable))
        {
            emu_audio_cdrom_volume(gui_audio_mute_cdrom ? 0 : config_audio.cdrom_volume);
        }

        if (ImGui::MenuItem("Mute ADPCM", "", &gui_audio_mute_adpcm, config_audio.enable))
        {
            emu_audio_adpcm_volume(gui_audio_mute_adpcm ? 0 : config_audio.adpcm_volume);
        }

        ImGui::Separator();

        if (ImGui::BeginMenu("PSG Volume", config_audio.enable))
        {
            ImGui::PushItemWidth(200.0f);
            if (ImGui::SliderFloat("##psg_volume", &config_audio.psg_volume, 0.0f, 2.0f, "Volume = %.2f"))
            {
                emu_audio_psg_volume(config_audio.psg_volume);
            }
            ImGui::PopItemWidth();
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Anything above 1.00 may cause clipping.");
                ImGui::EndTooltip();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("CD-ROM Volume", config_audio.enable))
        {
            ImGui::PushItemWidth(200.0f);
            if (ImGui::SliderFloat("##cdrom_volume", &config_audio.cdrom_volume, 0.0f, 2.0f, "Volume = %.2f"))
            {
                emu_audio_cdrom_volume(config_audio.cdrom_volume);
            }
            ImGui::PopItemWidth();
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Anything above 1.00 may cause clipping.");
                ImGui::EndTooltip();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("ADPCM Volume", config_audio.enable))
        {
            ImGui::PushItemWidth(200.0f);
            if (ImGui::SliderFloat("##adpcm_volume", &config_audio.adpcm_volume, 0.0f, 2.0f, "Volume = %.2f"))
            {
                emu_audio_adpcm_volume(config_audio.adpcm_volume);
            }
            ImGui::PopItemWidth();
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Anything above 1.00 may cause clipping.");
                ImGui::EndTooltip();
            }
            ImGui::EndMenu();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Sync With Emulator", "", &config_audio.sync))
        {
            config_emulator.ffwd = false;

            if (!config_audio.sync)
            {
                config_video.sync = false;
                display_set_vsync(false);
            }
        }

#ifndef GG_DISABLE_VGMRECORDER
        ImGui::Separator();

        bool is_recording = emu_is_vgm_recording();

        if (ImGui::MenuItem("Start VGM Recording...", "", false, !is_recording && !emu_is_empty()))
        {
            save_vgm = true;
        }

        if (ImGui::MenuItem("Stop VGM Recording", "", false, is_recording))
        {
            emu_stop_vgm_recording();
            gui_set_status_message("VGM recording stopped", 3000);
        }
#endif

        ImGui::EndMenu();
    }
}

static void menu_debug(void)
{
#if !defined(GG_DISABLE_DISASSEMBLER)
    if (ImGui::BeginMenu("Debug"))
    {
        gui_in_use = true;

        if (ImGui::MenuItem("Enable", "", &config_debug.debug))
        {
            emu_set_overscan(config_debug.debug ? 0 : config_video.overscan);
            emu_set_scanline_start_end(
                config_debug.debug ? 0 : config_video.scanline_start,
                config_debug.debug ? 241 : config_video.scanline_end);
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Save Debug Settings...", "", false, config_debug.debug))
        {
            save_debug_settings = true;
        }

        if (ImGui::MenuItem("Load Debug Settings...", "", false, config_debug.debug))
        {
            load_debug_settings = true;
        }

        ImGui::MenuItem("Auto Save/Load Debug Settings", "", &config_debug.auto_debug_settings, config_debug.debug);

        ImGui::Separator();

        if (ImGui::MenuItem("Reload ROM", config_hotkeys[config_HotkeyIndex_ReloadROM].str, false, config_debug.debug && !emu_is_empty()))
        {
            gui_action_reload_rom();
        }

        ImGui::Separator();

        if (ImGui::BeginMenu("MCP Server", config_debug.debug))
        {
            bool mcp_running = emu_mcp_is_running();
            int transport_mode = emu_mcp_get_transport_mode();
            bool http_running = mcp_running && (transport_mode == 1);
            bool stdio_running = mcp_running && (transport_mode == 0);

            if (ImGui::MenuItem("Start HTTP Server", "", false, !mcp_running))
            {
                emu_mcp_set_transport(1, config_emulator.mcp_tcp_port);
                emu_mcp_start();
            }

            if (ImGui::MenuItem("Stop HTTP Server", "", false, http_running))
            {
                emu_mcp_stop();
            }

            ImGui::Separator();

            if (stdio_running)
                ImGui::TextColored(ImVec4(0.90f, 0.70f, 0.10f, 1.0f), "STDIO mode active");
            else if (http_running)
                ImGui::TextColored(ImVec4(0.10f, 0.90f, 0.10f, 1.0f), "Listening on %d", config_emulator.mcp_tcp_port);
            else
                ImGui::TextColored(ImVec4(0.98f, 0.15f, 0.45f, 1.0f), "Stopped");

            ImGui::Separator();

            ImGui::Text("HTTP Port:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(50);
            if (ImGui::InputInt("##mcp_port", &config_emulator.mcp_tcp_port, 0, 0))
            {
                if (config_emulator.mcp_tcp_port < 1)
                    config_emulator.mcp_tcp_port = 1;
                if (config_emulator.mcp_tcp_port > 65535)
                    config_emulator.mcp_tcp_port = 65535;
            }

            ImGui::EndMenu();
        }

        ImGui::Separator();

        if (ImGui::BeginMenu("Reset Values", config_debug.debug))
        {
            if (ImGui::BeginMenu("CPU Registers"))
            {
                ImGui::PushItemWidth(100.0f);
                if (ImGui::Combo("##init_registers", &config_debug.reset_registers, "Random\0 0x00\0 0xFF\0\0"))
                {
                    emu_set_huc6280_registers_reset_value(get_reset_value(config_debug.reset_registers));
                }
                ImGui::PopItemWidth();
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("MPRs"))
            {
                ImGui::PushItemWidth(100.0f);
                if (ImGui::Combo("##init_mpr", &config_debug.reset_mpr, "Random\0 0x00\0 0xFF\0\0"))
                {
                    emu_set_memory_reset_values(
                            get_reset_value(config_debug.reset_mpr),
                            get_reset_value(config_debug.reset_ram),
                            get_reset_value(config_debug.reset_card_ram),
                            get_reset_value(config_debug.reset_arcade_card));
                }
                ImGui::PopItemWidth();
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("WRAM & CD-ROM RAM"))
            {
                ImGui::PushItemWidth(100.0f);
                if (ImGui::Combo("##init_ram", &config_debug.reset_ram, "Random\0 0x00\0 0xFF\0\0"))
                {
                    emu_set_memory_reset_values(
                            get_reset_value(config_debug.reset_mpr),
                            get_reset_value(config_debug.reset_ram),
                            get_reset_value(config_debug.reset_card_ram),
                            get_reset_value(config_debug.reset_arcade_card));
                }
                ImGui::PopItemWidth();
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Card RAM"))
            {
                ImGui::PushItemWidth(100.0f);
                if (ImGui::Combo("##init_card_ram", &config_debug.reset_card_ram, "Random\0 0x00\0 0xFF\0\0"))
                {
                    emu_set_memory_reset_values(
                            get_reset_value(config_debug.reset_mpr),
                            get_reset_value(config_debug.reset_ram),
                            get_reset_value(config_debug.reset_card_ram),
                            get_reset_value(config_debug.reset_arcade_card));
                }
                ImGui::PopItemWidth();
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Arcade Card RAM"))
            {
                ImGui::PushItemWidth(100.0f);
                if (ImGui::Combo("##init_arcade_card_ram", &config_debug.reset_arcade_card, "Random\0 0x00\0 0xFF\0\0"))
                {
                    emu_set_memory_reset_values(
                            get_reset_value(config_debug.reset_mpr),
                            get_reset_value(config_debug.reset_ram),
                            get_reset_value(config_debug.reset_card_ram),
                            get_reset_value(config_debug.reset_arcade_card));
                }
                ImGui::PopItemWidth();
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Palettes"))
            {
                ImGui::PushItemWidth(100.0f);
                if (ImGui::Combo("##init_color_table", &config_debug.reset_color_table, "Random\0 0x0000\0 0x01FF\0\0"))
                {
                    emu_set_huc6260_color_table_reset_value(get_reset_value(config_debug.reset_color_table));
                }
                ImGui::PopItemWidth();
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        ImGui::Separator();

        ImGui::MenuItem("Show Output Screen", "", &config_debug.show_screen, config_debug.debug);

        if (ImGui::BeginMenu("Output Scale", config_debug.debug))
        {
            ImGui::PushItemWidth(200.0f);
            ImGui::SliderInt("##debug_scale", &config_debug.scale, 1, 10);
            ImGui::PopItemWidth();
            ImGui::EndMenu();
        }

        ImGui::Separator();
        ImGui::MenuItem("Show Disassembler", "", &config_debug.show_disassembler, config_debug.debug);
        ImGui::MenuItem("Show Memory Editor", "", &config_debug.show_memory, config_debug.debug);
        ImGui::MenuItem("Show Trace Logger", "", &config_debug.show_trace_logger, config_debug.debug);

        ImGui::Separator();

        if (ImGui::BeginMenu("HuC6280", config_debug.debug))
        {
            ImGui::MenuItem("Show Status", "", &config_debug.show_processor);
            ImGui::MenuItem("Show Call Stack", "", &config_debug.show_call_stack);
            ImGui::MenuItem("Show Breakpoints", "", &config_debug.show_breakpoints);
            ImGui::MenuItem("Show Symbols", "", &config_debug.show_symbols);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("HuC6260", config_debug.debug))
        {
            ImGui::MenuItem("Show Info", "", &config_debug.show_huc6260_info);
            ImGui::MenuItem("Show Palettes", "", &config_debug.show_huc6260_palettes);
            ImGui::EndMenu();
        }

        if (emu_get_core()->GetMedia()->IsSGX())
        {
            if (ImGui::BeginMenu("HuC6202", config_debug.debug))
            {
                ImGui::MenuItem("Show Info", "", &config_debug.show_huc6202_info);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("HuC6270", config_debug.debug))
            {
                ImGui::MenuItem("Show Info (1)", "", &config_debug.show_huc6270_1_info);
                ImGui::MenuItem("Show Info (2)", "", &config_debug.show_huc6270_2_info);
                ImGui::Separator();
                ImGui::MenuItem("Show Registers (1)", "", &config_debug.show_huc6270_1_registers);
                ImGui::MenuItem("Show Registers (2)", "", &config_debug.show_huc6270_2_registers);
                ImGui::Separator();
                ImGui::MenuItem("Show Background (1)", "", &config_debug.show_huc6270_1_background);
                ImGui::MenuItem("Show Background (2)", "", &config_debug.show_huc6270_2_background);
                ImGui::Separator();
                ImGui::MenuItem("Show Sprites (1)", "", &config_debug.show_huc6270_1_sprites);
                ImGui::MenuItem("Show Sprites (2)", "", &config_debug.show_huc6270_2_sprites);
                ImGui::EndMenu();
            }
        }
        else
        {
            if (ImGui::BeginMenu("HuC6270", config_debug.debug))
            {
                ImGui::MenuItem("Show Info", "", &config_debug.show_huc6270_1_info);
                ImGui::MenuItem("Show Registers", "", &config_debug.show_huc6270_1_registers);
                ImGui::MenuItem("Show Background", "", &config_debug.show_huc6270_1_background);
                ImGui::MenuItem("Show Sprites", "", &config_debug.show_huc6270_1_sprites);
                ImGui::EndMenu();
            }
        }

        if (ImGui::BeginMenu("CD-ROM", config_debug.debug && emu_get_core()->GetMedia()->IsCDROM()))
        {
            ImGui::MenuItem("Show Status", "", &config_debug.show_cdrom);
            ImGui::MenuItem("Show Arcade Card", "", &config_debug.show_arcade_card, emu_get_core()->GetMedia()->IsArcadeCard());
            ImGui::Separator();
            ImGui::MenuItem("Show CD-ROM Audio", "", &config_debug.show_cdrom_audio);
            ImGui::MenuItem("Show ADPCM", "", &config_debug.show_adpcm);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Audio", config_debug.debug))
        {
            ImGui::MenuItem("Show PSG", "", &config_debug.show_psg);
            ImGui::EndMenu();
        }

#if defined(__APPLE__) || defined(_WIN32)
        ImGui::Separator();
        ImGui::MenuItem("Multi-Viewport", "", &config_debug.multi_viewport, config_debug.debug);
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("RESTART REQUIRED");
            ImGui::NewLine();
            ImGui::Text("Enables docking of debug windows outside of main window.");
            ImGui::EndTooltip();
        }
#endif

        ImGui::Separator();

        if (ImGui::BeginMenu("Font Size", config_debug.debug))
        {
            ImGui::PushItemWidth(110.0f);
            if (ImGui::Combo("##font", &config_debug.font_size, "Very Small\0Small\0Medium\0Large\0\0"))
            {
                gui_default_font = gui_default_fonts[config_debug.font_size];
            }
            ImGui::PopItemWidth();
            ImGui::EndMenu();
        }

        ImGui::EndMenu();
    }
#endif
}

static void menu_about(void)
{
    if (ImGui::BeginMenu("About"))
    {
        gui_in_use = true;

        if (ImGui::MenuItem("About " GG_TITLE " " GG_VERSION " ..."))
        {
            open_about = true;
        }
        ImGui::EndMenu();
    }
}

static void file_dialogs(void)
{
    gui_file_dialog_process_results();

    if (open_rom || gui_shortcut_open_rom)
    {
        gui_shortcut_open_rom = false;
        gui_file_dialog_open_rom();
    }
    if (open_ram)
        gui_file_dialog_load_ram();
    if (save_ram)
        gui_file_dialog_save_ram();
    if (open_state)
        gui_file_dialog_load_state();
    if (save_state)
        gui_file_dialog_save_state();
    if (save_screenshot)
        gui_file_dialog_save_screenshot();
    if (save_vgm)
        gui_file_dialog_save_vgm();
    if (choose_savestates_path)
        gui_file_dialog_choose_savestate_path();
    if (choose_screenshots_path)
        gui_file_dialog_choose_screenshot_path();
    if (choose_backup_ram_path)
        gui_file_dialog_choose_backup_ram_path();
    if (choose_mb128_path)
        gui_file_dialog_choose_mb128_path();
    if (open_syscard_bios)
        gui_file_dialog_load_bios(true);
    if (open_gameexpress_bios)
        gui_file_dialog_load_bios(false);
    if (save_debug_settings)
        gui_file_dialog_save_debug_settings();
    if (load_debug_settings)
        gui_file_dialog_load_debug_settings();
    if (open_about)
    {
        gui_dialog_in_use = true;
        ImGui::OpenPopup("About " GG_TITLE);
    }

    gui_popup_modal_about();
}

static void keyboard_configuration_item(const char* text, SDL_Scancode* key, int player)
{
    ImGui::Text("%s", text);
    ImGui::SameLine(120);

    char button_label[256];
    snprintf(button_label, 256, "%s##%s%d", SDL_GetKeyName(SDL_GetKeyFromScancode(*key, SDL_KMOD_NONE, false)), text, player);

    if (ImGui::Button(button_label, ImVec2(90,0)))
    {
        gui_configured_key = key;
        ImGui::OpenPopup("Keyboard Configuration");
    }

    ImGui::SameLine();

    char remove_label[256];
    snprintf(remove_label, sizeof(remove_label), "X##rk%s%d", text, player);

    if (ImGui::Button(remove_label))
    {
        *key = SDL_SCANCODE_UNKNOWN;
    }
}

static void gamepad_configuration_item(const char* text, int* button, int player)
{
    ImGui::Text("%s", text);
    ImGui::SameLine(130);

    const char* button_name = "";

    if (*button == SDL_GAMEPAD_BUTTON_INVALID)
    {
        button_name = "";
    }
    else if (*button >= 0 && *button < SDL_GAMEPAD_BUTTON_COUNT)
    {
        static const char* gamepad_names[21] = {"A", "B", "X" ,"Y", "BACK", "GUIDE", "START", "L3", "R3", "L1", "R1", "UP", "DOWN", "LEFT", "RIGHT", "MISC", "PAD1", "PAD2", "PAD3", "PAD4", "TOUCH"};
        button_name = gamepad_names[*button];
    }
    else if (*button >= GAMEPAD_VBTN_AXIS_BASE)
    {
        int axis = *button - GAMEPAD_VBTN_AXIS_BASE;
        if (axis == SDL_GAMEPAD_AXIS_LEFT_TRIGGER)
            button_name = "L2";
        else if (axis == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER)
            button_name = "R2";
        else
            button_name = "??";
    }

    char button_label[256];
    snprintf(button_label, sizeof(button_label), "%s##%s%d", button_name, text, player);

    if (ImGui::Button(button_label, ImVec2(70,0)))
    {
        gui_configured_button = button;
        ImGui::OpenPopup("Gamepad Configuration");
    }

    ImGui::SameLine();

    char remove_label[256];
    snprintf(remove_label, sizeof(remove_label), "X##rg%s%d", text, player);

    if (ImGui::Button(remove_label))
    {
        *button = SDL_GAMEPAD_BUTTON_INVALID;
    }
}

static void hotkey_configuration_item(const char* text, config_Hotkey* hotkey)
{
    ImGui::Text("%s", text);
    ImGui::SameLine(150);

    char button_label[256];
    snprintf(button_label, sizeof(button_label), "%s##%s", hotkey->str[0] != '\0' ? hotkey->str : "<None>", text);

    if (ImGui::Button(button_label, ImVec2(150,0)))
    {
        gui_configured_hotkey = hotkey;
        ImGui::OpenPopup("Hotkey Configuration");
    }

    ImGui::SameLine();

    char remove_label[256];
    snprintf(remove_label, sizeof(remove_label), "X##rh%s", text);

    if (ImGui::Button(remove_label))
    {
        hotkey->key = SDL_SCANCODE_UNKNOWN;
        hotkey->mod = SDL_KMOD_NONE;
        config_update_hotkey_string(hotkey);
    }
}

static void gamepad_device_selector(int player)
{
    if (player < 0 || player >= GG_MAX_GAMEPADS)
        return;

    const int max_detected_gamepads = 32;
    SDL_JoystickID id_map[max_detected_gamepads];
    id_map[0] = 0;
    int count = 1;

    std::string items;
    items.reserve(4096);
    items.append("<None>");
    items.push_back('\0');

    Gamepad_Detected_Info detected[max_detected_gamepads];
    int num_detected = gamepad_get_detected(detected, max_detected_gamepads);

    SDL_JoystickID current_id = 0;
    if (IsValidPointer(gamepad_controller[player]))
        current_id = SDL_GetJoystickID(SDL_GetGamepadJoystick(gamepad_controller[player]));

    int selected = 0;

    for (int i = 0; i < num_detected && count < max_detected_gamepads; i++)
    {
        const char* name = detected[i].name;
        if (!IsValidPointer(name))
            name = "Unknown Gamepad";

        id_map[count] = detected[i].id;

        if (current_id == detected[i].id)
            selected = count;

        size_t len = strlen(detected[i].guid_str);
        const char* id_8 = detected[i].guid_str + (len > 8 ? len - 8 : 0);

        char label[192];
        snprintf(label, sizeof(label), "%s (ID: %s)", name, id_8);

        items.append(label);
        items.push_back('\0');
        count++;
    }

    items.push_back('\0');

    char label[32];
    snprintf(label, sizeof(label), "##device_player%d", player + 1);

    if (ImGui::Combo(label, &selected, items.c_str()))
    {
        SDL_JoystickID instance_id = id_map[selected];
        gamepad_assign(player, instance_id);
    }
}

static void draw_savestate_slot_info(int slot)
{
    if (emu_savestates[slot].rom_name[0] != 0)
    {
        if (emu_savestates[slot].version != GG_SAVESTATE_VERSION)
        {
            ImGui::TextColored(ImVec4(0.98f, 0.15f, 0.45f, 1.0f), "This savestate is from an older version and will not work" );
            if (emu_savestates[slot].emu_build[0] != 0)
                ImGui::TextColored(ImVec4(0.98f, 0.15f, 0.45f, 1.0f), "Use %s - %s", GG_TITLE, emu_savestates[slot].emu_build);
            ImGui::Separator();
        }

        ImGui::Text("%s", emu_savestates[slot].rom_name);
        char date[64];
        get_date_time_string(emu_savestates[slot].timestamp, date, sizeof(date));
        ImGui::Text("%s", date);

        if (IsValidPointer(emu_savestates_screenshots[slot].data))
        {
            float width = (float)emu_savestates_screenshots[slot].width;
            float height = (float)emu_savestates_screenshots[slot].height;
            ImGui::Image((ImTextureID)(intptr_t)ogl_renderer_emu_savestates, ImVec2((height / 3.0f) * 4.0f, height), ImVec2(0, 0), ImVec2(width / 2048.0f, height / 256.0f));
        }
    }
    else
    {
        ImGui::TextColored(ImVec4(0.50f, 0.50f, 0.50f, 1.0f), "Slot %d is empty", slot + 1);
    }
}
