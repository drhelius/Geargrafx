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
#include "emu.h"
#include "renderer.h"
#include "utils.h"
#include "geargrafx.h"

static bool open_rom = false;
static bool open_ram = false;
static bool save_ram = false;
static bool open_state = false;
static bool save_state = false;
static bool open_about = false;
static bool save_screenshot = false;
static bool choose_savestates_path = false;
static bool choose_screenshots_path = false;
static bool choose_backup_ram_path = false;
static bool open_syscard_bios = false;
static bool open_gameexpress_bios = false;

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
    choose_savestates_path = false;
    choose_screenshots_path = false;
    gui_main_menu_hovered = false;
    choose_backup_ram_path = false;
    open_syscard_bios = false;
    open_gameexpress_bios = false;

    if (config_emulator.show_menu && ImGui::BeginMainMenuBar())
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

        if (ImGui::MenuItem("Open ROM/CD...", "Ctrl+O"))
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
        
        if (ImGui::MenuItem("Reset", "Ctrl+R"))
        {
            gui_action_reset();
        }

        if (ImGui::MenuItem("Pause", "Ctrl+P", &config_emulator.paused))
        {
            gui_action_pause();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Fast Forward", "Ctrl+F", &config_emulator.ffwd))
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

        if (ImGui::MenuItem("Save RAM As..."))
        {
            save_ram = true;
        }

        if (ImGui::MenuItem("Load RAM From..."))
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

        if (ImGui::MenuItem("Save State", "Ctrl+S")) 
        {
            std::string message("Saving state to slot ");
            message += std::to_string(config_emulator.save_slot + 1);
            gui_set_status_message(message.c_str(), 3000);
            emu_save_state_slot(config_emulator.save_slot + 1);
        }

        if (ImGui::MenuItem("Load State", "Ctrl+L"))
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

        if (ImGui::MenuItem("Save Screenshot", "Ctrl+X"))
        {
            gui_action_save_screenshot(NULL);
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Quit", "Ctrl+Q"))
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

        if (ImGui::BeginMenu("Console"))
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
                ImGui::EndTooltip();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("CD-ROM"))
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
                ImGui::EndTooltip();
            }

            ImGui::EndMenu();
        }

        if (ImGui::MenuItem("Force Backup RAM", "", &config_emulator.backup_ram))
        {
            emu_set_backup_ram(config_emulator.backup_ram);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("This otion will add backup RAM support to HuCard games.");
            ImGui::Text("It is recommended to leave this option enabled.");
            ImGui::EndTooltip();
        }

        ImGui::Separator();

        ImGui::MenuItem("Show ROM info", "", &config_emulator.show_info);
        ImGui::MenuItem("Status Messages", "", &config_emulator.status_messages);

        ImGui::Separator();

        ImGui::MenuItem("Start Paused", "", &config_emulator.start_paused);

        ImGui::EndMenu();
    }
}

static void menu_video(void)
{
    if (ImGui::BeginMenu("Video"))
    {
        gui_in_use = true;

        if (ImGui::MenuItem("Full Screen", "F12", &config_emulator.fullscreen))
        {
            application_trigger_fullscreen(config_emulator.fullscreen);
        }

        ImGui::MenuItem("Show Menu", "CTRL+M", &config_emulator.show_menu);

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
            SDL_GL_SetSwapInterval(config_video.sync ? 1 : 0);

            if (config_video.sync)
            {
                config_audio.sync = true;
                emu_audio_reset();
            }
        }

        ImGui::MenuItem("Show FPS", "", &config_video.fps);

        ImGui::Separator();

        if (ImGui::MenuItem("Disable Sprite Limit", "", &config_video.sprite_limit))
        {
            emu_video_no_sprite_limit(config_video.sprite_limit);
        }

        if (ImGui::MenuItem("Composite Colors", "", &config_video.composite_palette))
        {
            emu_set_composite_palette(config_video.composite_palette);
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

        if (ImGui::BeginMenu("Controller Type"))
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
                    if (!config_input_gamepad[i].detected)
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

                    if (ImGui::BeginMenu("Directional Controls"))
                    {
                        ImGui::PushItemWidth(150.0f);
                        ImGui::Combo("##directional", &config_input_gamepad[i].gamepad_directional, "D-pad\0Left Analog Stick\0\0");
                        ImGui::PopItemWidth();
                        ImGui::EndMenu();
                    }

                    if (ImGui::BeginMenu("Button Configuration"))
                    {
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

        ImGui::Separator();

        if (ImGui::MenuItem("Mute PSG", "", &config_audio.mute_psg, config_audio.enable))
        {
            emu_audio_mute_psg(config_audio.mute_psg);
        }

        if (ImGui::MenuItem("Mute CD-ROM", "", &config_audio.mute_cd, config_audio.enable))
        {
            emu_audio_mute_cdrom(config_audio.mute_cd);
        }

        if (ImGui::MenuItem("Mute ADPCM", "", &config_audio.mute_adpcm, config_audio.enable))
        {
            emu_audio_mute_adpcm(config_audio.mute_adpcm);
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Sync With Emulator", "", &config_audio.sync))
        {
            config_emulator.ffwd = false;

            if (!config_audio.sync)
            {
                config_video.sync = false;
                SDL_GL_SetSwapInterval(0);
            }
        }

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
                    emu_set_memory_reset_values(get_reset_value(config_debug.reset_mpr), get_reset_value(config_debug.reset_ram), get_reset_value(config_debug.reset_card_ram));
                }
                ImGui::PopItemWidth();
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("RAM"))
            {
                ImGui::PushItemWidth(100.0f);
                if (ImGui::Combo("##init_ram", &config_debug.reset_ram, "Random\0 0x00\0 0xFF\0\0"))
                {
                    emu_set_memory_reset_values(get_reset_value(config_debug.reset_mpr), get_reset_value(config_debug.reset_ram), get_reset_value(config_debug.reset_card_ram));
                }
                ImGui::PopItemWidth();
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Card RAM"))
            {
                ImGui::PushItemWidth(100.0f);
                if (ImGui::Combo("##init_card_ram", &config_debug.reset_card_ram, "Random\0 0x00\0 0xFF\0\0"))
                {
                    emu_set_memory_reset_values(get_reset_value(config_debug.reset_mpr), get_reset_value(config_debug.reset_ram), get_reset_value(config_debug.reset_card_ram));
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
        ImGui::MenuItem("Show Disassembler", "", &config_debug.show_disassembler, config_debug.debug);
        ImGui::MenuItem("Show Memory Editor", "", &config_debug.show_memory, config_debug.debug);
        ImGui::MenuItem("Show Trace Logger", "", &config_debug.show_trace_logger, config_debug.debug);
        ImGui::Separator();
        ImGui::MenuItem("Show HuC6280 Status", "", &config_debug.show_processor, config_debug.debug);
        ImGui::MenuItem("Show HuC6280 Call Stack", "", &config_debug.show_call_stack, config_debug.debug);
        ImGui::Separator();
        ImGui::MenuItem("Show HuC6260 Info", "", &config_debug.show_huc6260_info, config_debug.debug);
        ImGui::MenuItem("Show HuC6260 Palettes", "", &config_debug.show_huc6260_palettes, config_debug.debug);
        ImGui::Separator();
        if (emu_get_core()->GetMedia()->IsSGX())
        {
            ImGui::MenuItem("Show HuC6202 Info", "", &config_debug.show_huc6202_info, config_debug.debug);
            ImGui::Separator();
            ImGui::MenuItem("Show HuC6270 (1) Info", "", &config_debug.show_huc6270_1_info, config_debug.debug);
            ImGui::MenuItem("Show HuC6270 (2) Info", "", &config_debug.show_huc6270_2_info, config_debug.debug);
            ImGui::MenuItem("Show HuC6270 (1) Registers", "", &config_debug.show_huc6270_1_registers, config_debug.debug);
            ImGui::MenuItem("Show HuC6270 (2) Registers", "", &config_debug.show_huc6270_2_registers, config_debug.debug);
            ImGui::MenuItem("Show HuC6270 (1) Background", "", &config_debug.show_huc6270_1_background, config_debug.debug);
            ImGui::MenuItem("Show HuC6270 (2) Background", "", &config_debug.show_huc6270_2_background, config_debug.debug);
            ImGui::MenuItem("Show HuC6270 (1) Sprites", "", &config_debug.show_huc6270_1_sprites, config_debug.debug);
            ImGui::MenuItem("Show HuC6270 (2) Sprites", "", &config_debug.show_huc6270_2_sprites, config_debug.debug);
        }
        else
        {
            ImGui::MenuItem("Show HuC6270 Info", "", &config_debug.show_huc6270_1_info, config_debug.debug);
            ImGui::MenuItem("Show HuC6270 Registers", "", &config_debug.show_huc6270_1_registers, config_debug.debug);
            ImGui::MenuItem("Show HuC6270 Background", "", &config_debug.show_huc6270_1_background, config_debug.debug);
            ImGui::MenuItem("Show HuC6270 Sprites", "", &config_debug.show_huc6270_1_sprites, config_debug.debug);
        }

        ImGui::Separator();
        ImGui::MenuItem("Show CD-ROM", "", &config_debug.show_cdrom, config_debug.debug && emu_get_core()->GetMedia()->IsCDROM());
        ImGui::Separator();
        ImGui::MenuItem("Show PSG", "", &config_debug.show_psg, config_debug.debug);
        ImGui::MenuItem("Show CD-ROM Audio", "", &config_debug.show_cdrom_audio, config_debug.debug && emu_get_core()->GetMedia()->IsCDROM());
        ImGui::MenuItem("Show CD-ROM ADPCM", "", &config_debug.show_adpcm, config_debug.debug && emu_get_core()->GetMedia()->IsCDROM());

#if defined(__APPLE__) || defined(_WIN32)
        ImGui::Separator();
        ImGui::MenuItem("Multi-Viewport (Restart required)", "", &config_debug.multi_viewport, config_debug.debug);
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
    if (choose_savestates_path)
        gui_file_dialog_choose_savestate_path();
    if (choose_screenshots_path)
        gui_file_dialog_choose_screenshot_path();
    if (choose_backup_ram_path)
        gui_file_dialog_choose_backup_ram_path();
    if (open_syscard_bios)
        gui_file_dialog_load_bios(true);
    if (open_gameexpress_bios)
        gui_file_dialog_load_bios(false);
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
    snprintf(button_label, 256, "%s##%s%d", SDL_GetKeyName(SDL_GetKeyFromScancode(*key)), text, player);

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
    ImGui::SameLine(100);

    static const char* gamepad_names[16] = {"A", "B", "X" ,"Y", "BACK", "GUIDE", "START", "L3", "R3", "L1", "R1", "UP", "DOWN", "LEFT", "RIGHT", "15"};
    const char* button_name = (*button >= 0 && *button < 16) ? gamepad_names[*button] : "";

    char button_label[256];
    snprintf(button_label, 256, "%s##%s%d", button_name, text, player);

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
        *button = SDL_CONTROLLER_BUTTON_INVALID;
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
        GetDateTimeString(emu_savestates[slot].timestamp, date, sizeof(date));
        ImGui::Text("%s", date);

        if (IsValidPointer(emu_savestates_screenshots[slot].data))
        {
            float width = (float)emu_savestates_screenshots[slot].width;
            float height = (float)emu_savestates_screenshots[slot].height;
            ImGui::Image((ImTextureID)(intptr_t)renderer_emu_savestates, ImVec2((height / 3.0f) * 4.0f, height), ImVec2(0, 0), ImVec2(width / 2048.0f, height / 256.0f));
        }
    }
    else
    {
        ImGui::TextColored(ImVec4(0.50f, 0.50f, 0.50f, 1.0f), "Slot %d is empty", slot + 1);
    }
}
