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
#include "../../../src/geargrafx.h"

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
static int get_reset_value(int option);

void gui_init_menus(void)
{
    strcpy(gui_savefiles_path, config_emulator.savefiles_path.c_str());
    strcpy(gui_savestates_path, config_emulator.savestates_path.c_str());
    strcpy(gui_screenshots_path, config_emulator.screenshots_path.c_str());
    strcpy(gui_backup_ram_path, config_emulator.backup_ram_path.c_str());
    gui_shortcut_open_rom = false;

    emu_get_core()->GetMemory()->SetResetValues(get_reset_value(config_debug.reset_mpr), get_reset_value(config_debug.reset_ram), get_reset_value(config_debug.reset_card_ram));
    emu_get_core()->GetHuC6260()->SetResetValue(get_reset_value(config_debug.reset_color_table));
    emu_get_core()->GetHuC6280()->SetResetValue(get_reset_value(config_debug.reset_registers));
    emu_get_core()->GetMemory()->EnableBackupRam(config_emulator.backup_ram);
    emu_get_core()->GetInput()->EnableCDROM(config_emulator.backup_ram);
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

        if (ImGui::MenuItem("Open ROM...", "Ctrl+O"))
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

        if (ImGui::MenuItem("Quit", "ESC"))
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
                    if (emu_get_core()->GetCartridge()->IsReady())
                        ImGui::Text("%s", emu_get_core()->GetCartridge()->GetFileDirectory());
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
                    if (emu_get_core()->GetCartridge()->IsReady())
                        ImGui::Text("%s", emu_get_core()->GetCartridge()->GetFileDirectory());
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
                    if (emu_get_core()->GetCartridge()->IsReady())
                        ImGui::Text("%s", emu_get_core()->GetCartridge()->GetFileDirectory());
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

        if (ImGui::MenuItem("Force Japanese PC Engine", "", &config_emulator.pce_jap))
        {
            emu_get_core()->GetInput()->EnablePCEJap(config_emulator.pce_jap);
        }

        if (ImGui::MenuItem("Enable Backup RAM", "", &config_emulator.backup_ram))
        {
            emu_get_core()->GetMemory()->EnableBackupRam(config_emulator.backup_ram);
            emu_get_core()->GetInput()->EnableCDROM(config_emulator.backup_ram);
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
            ImGui::PushItemWidth(160.0f);
            ImGui::Combo("##ratio", &config_video.ratio, "Square Pixels (1:1 PAR)\0Standard (4:3 DAR)\0Wide (16:9 DAR)\0Wide (16:10 DAR)\0\0");
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

        if (ImGui::BeginMenu("Scanline Start/End"))
        {
            if (ImGui::SliderInt("##scanline_start", &config_video.scanline_start, 0, 239, "Start = %d"))
            {
                emu_set_scanline_start_end(
                config_debug.debug ? 0 : config_video.scanline_start,
                config_debug.debug ? 239 : config_video.scanline_end);
            }
            if (ImGui::SliderInt("##scanline_end", &config_video.scanline_end, 0, 239, "End = %d"))
            {
                emu_set_scanline_start_end(
                config_debug.debug ? 0 : config_video.scanline_start,
                config_debug.debug ? 239 : config_video.scanline_end);
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

        ImGui::MenuItem("Bilinear Filtering", "", &config_video.bilinear);
        if (ImGui::MenuItem("Disable Sprite Limit", "", &config_video.sprite_limit))
        {
            emu_video_no_sprite_limit(config_video.sprite_limit);
        }

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

        if (ImGui::BeginMenu("Keyboard"))
        {
            if (ImGui::BeginMenu("Player 1"))
            {
                keyboard_configuration_item("Left:", &config_input[0].key_left, 0);
                keyboard_configuration_item("Right:", &config_input[0].key_right, 0);
                keyboard_configuration_item("Up:", &config_input[0].key_up, 0);
                keyboard_configuration_item("Down:", &config_input[0].key_down, 0);
                keyboard_configuration_item("Select:", &config_input[0].key_select, 0);
                keyboard_configuration_item("Run:", &config_input[0].key_run, 0);
                keyboard_configuration_item("1:", &config_input[0].key_1, 0);
                keyboard_configuration_item("2:", &config_input[0].key_2, 0);

                gui_popup_modal_keyboard();

                ImGui::EndMenu();
            }

            // if (ImGui::BeginMenu("Player 2"))
            // {
            //     keyboard_configuration_item("Left:", &config_input[1].key_left, 1);
            //     keyboard_configuration_item("Right:", &config_input[1].key_right, 1);
            //     keyboard_configuration_item("Up:", &config_input[1].key_up, 1);
            //     keyboard_configuration_item("Down:", &config_input[1].key_down, 1);
            //     keyboard_configuration_item("Select:", &config_input[1].key_select, 1);
            //     keyboard_configuration_item("Run:", &config_input[1].key_run, 1);
            //     keyboard_configuration_item("1:", &config_input[1].key_1, 1);
            //     keyboard_configuration_item("2:", &config_input[1].key_2, 1);

            //     gui_popup_modal_keyboard();

            //     ImGui::EndMenu();
            // }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Gamepads"))
        {
            if (ImGui::BeginMenu("Player 1"))
            {
                ImGui::MenuItem("Enable Gamepad P1", "", &config_input[0].gamepad);

                if (ImGui::BeginMenu("Directional Controls"))
                {
                    ImGui::PushItemWidth(150.0f);
                    ImGui::Combo("##directional", &config_input[0].gamepad_directional, "D-pad\0Left Analog Stick\0\0");
                    ImGui::PopItemWidth();
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Button Configuration"))
                {
                    gamepad_configuration_item("Select:", &config_input[0].gamepad_select, 0);
                    gamepad_configuration_item("Run:", &config_input[0].gamepad_run, 0);
                    gamepad_configuration_item("1:", &config_input[0].gamepad_1, 0);
                    gamepad_configuration_item("2:", &config_input[0].gamepad_2, 0);

                    gui_popup_modal_gamepad(0);

                    ImGui::EndMenu();
                }

                ImGui::EndMenu();
            }

            // if (ImGui::BeginMenu("Player 2"))
            // {
            //     ImGui::MenuItem("Enable Gamepad P2", "", &config_input[1].gamepad);

            //     if (ImGui::BeginMenu("Directional Controls"))
            //     {
            //         ImGui::PushItemWidth(150.0f);
            //         ImGui::Combo("##directional", &config_input[1].gamepad_directional, "D-pad\0Left Analog Stick\0\0");
            //         ImGui::PopItemWidth();
            //         ImGui::EndMenu();
            //     }

            //     if (ImGui::BeginMenu("Button Configuration"))
            //     {
            //         gamepad_configuration_item("Select:", &config_input[1].gamepad_select, 1);
            //         gamepad_configuration_item("Run:", &config_input[1].gamepad_run, 1);
            //         gamepad_configuration_item("1:", &config_input[1].gamepad_1, 1);
            //         gamepad_configuration_item("2:", &config_input[1].gamepad_2, 1);


            //         gui_popup_modal_gamepad(1);

            //         ImGui::EndMenu();
            //     }

            //     ImGui::EndMenu();
            // }

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
                config_debug.debug ? 239 : config_video.scanline_end);
        }

        ImGui::Separator();

        if (ImGui::BeginMenu("Reset Values", config_debug.debug))
        {
            if (ImGui::BeginMenu("RAM"))
            {
                ImGui::PushItemWidth(100.0f);
                if (ImGui::Combo("##init_ram", &config_debug.reset_ram, "Random\0 0x00\0 0xFF\0\0"))
                {
                    emu_get_core()->GetMemory()->SetResetValues(get_reset_value(config_debug.reset_mpr), get_reset_value(config_debug.reset_ram), get_reset_value(config_debug.reset_card_ram));
                }
                ImGui::PopItemWidth();
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Card RAM"))
            {
                ImGui::PushItemWidth(100.0f);
                if (ImGui::Combo("##init_card_ram", &config_debug.reset_card_ram, "Random\0 0x00\0 0xFF\0\0"))
                {
                    emu_get_core()->GetMemory()->SetResetValues(get_reset_value(config_debug.reset_mpr), get_reset_value(config_debug.reset_ram), get_reset_value(config_debug.reset_card_ram));
                }
                ImGui::PopItemWidth();
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Registers"))
            {
                ImGui::PushItemWidth(100.0f);
                if (ImGui::Combo("##init_registers", &config_debug.reset_registers, "Random\0 0x00\0 0xFF\0\0"))
                {
                    emu_get_core()->GetHuC6280()->SetResetValue(get_reset_value(config_debug.reset_registers));
                }
                ImGui::PopItemWidth();
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Palettes"))
            {
                ImGui::PushItemWidth(100.0f);
                if (ImGui::Combo("##init_color_table", &config_debug.reset_color_table, "Random\0 0x0000\0 0x01FF\0\0"))
                {
                    emu_get_core()->GetHuC6260()->SetResetValue(get_reset_value(config_debug.reset_color_table));
                }
                ImGui::PopItemWidth();
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("MPRs"))
            {
                ImGui::PushItemWidth(100.0f);
                if (ImGui::Combo("##init_mpr", &config_debug.reset_mpr, "Random\0 0x00\0 0xFF\0\0"))
                {
                    emu_get_core()->GetMemory()->SetResetValues(get_reset_value(config_debug.reset_mpr), get_reset_value(config_debug.reset_ram), get_reset_value(config_debug.reset_card_ram));
                }
                ImGui::PopItemWidth();
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        ImGui::Separator();

        ImGui::MenuItem("Show Output Screen", "", &config_debug.show_screen, config_debug.debug);
        ImGui::MenuItem("Show Disassembler", "", &config_debug.show_disassembler, config_debug.debug);
        ImGui::MenuItem("Show HuC6280 Status", "", &config_debug.show_processor, config_debug.debug);
        ImGui::MenuItem("Show HuC6280 Call Stack", "", &config_debug.show_call_stack, config_debug.debug);
        ImGui::MenuItem("Show Memory Editor", "", &config_debug.show_memory, config_debug.debug);
        ImGui::MenuItem("Show HuC6260 Info", "", &config_debug.show_huc6260_info, config_debug.debug);
        ImGui::MenuItem("Show HuC6260 Palettes", "", &config_debug.show_huc6260_palettes, config_debug.debug);
        ImGui::MenuItem("Show HuC6270 Info", "", &config_debug.show_huc6270_info, config_debug.debug);
        ImGui::MenuItem("Show HuC6270 Registers", "", &config_debug.show_huc6270_registers, config_debug.debug);
        ImGui::MenuItem("Show HuC6270 Background", "", &config_debug.show_huc6270_background, config_debug.debug);
        ImGui::MenuItem("Show HuC6270 Sprites", "", &config_debug.show_huc6270_sprites, config_debug.debug);
        ImGui::MenuItem("Show PSG", "", &config_debug.show_psg, config_debug.debug);
        ImGui::MenuItem("Show Trace Logger", "", &config_debug.show_trace_logger, config_debug.debug);

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
    ImGui::SameLine(100);

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
        ImGui::Text("%s", emu_savestates[slot].rom_name);
        char date[64];
        GetDateTimeString(emu_savestates[slot].timestamp, date, sizeof(date));
        ImGui::Text("%s", date);

        if (IsValidPointer(emu_savestates_screenshots[slot].data))
        {
            float width = (float)emu_savestates_screenshots[slot].width;
            float height = (float)emu_savestates_screenshots[slot].height;
            ImGui::Image((ImTextureID)(intptr_t)renderer_emu_savestates[slot], ImVec2((height / 3.0f) * 4.0f, height), ImVec2(0, 0), ImVec2(width / 512.0f, height / 512.0f));
        }
    }
    else
    {
        ImGui::TextColored(ImVec4(0.50f, 0.50f, 0.50f, 1.0f), "Slot %d is empty", slot + 1);
    }
}

static int get_reset_value(int option)
{
    switch (option)
    {
        case 0:
            return -1;
        case 1:
            return 0x0000;
        case 2:
            return 0xFFFF;
        default:
            return -1;
    }
}
