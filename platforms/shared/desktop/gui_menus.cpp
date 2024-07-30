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
#include "../../../src/geargrafx.h"

static char savefiles_path[4096] = "";
static char savestates_path[4096] = "";
static bool open_rom = false;
static bool open_ram = false;
static bool save_ram = false;
static bool open_state = false;
static bool save_state = false;
static bool open_about = false;
static bool open_symbols = false;
static bool save_screenshot = false;
static bool choose_savestates_path = false;

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

void gui_init_menus(void)
{
    strcpy(savefiles_path, config_emulator.savefiles_path.c_str());
    strcpy(savestates_path, config_emulator.savestates_path.c_str());
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
    open_symbols = false;
    save_screenshot = false;
    choose_savestates_path = false;
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
    if (ImGui::BeginMenu(GEARGRAFX_TITLE))
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

        ImGui::MenuItem("Start Paused", "", &config_emulator.start_paused);
        
        ImGui::Separator();

        if (ImGui::BeginMenu("Save State Location"))
        {
            ImGui::PushItemWidth(220.0f);
            if (ImGui::Combo("##savestate_option", &config_emulator.savestates_dir_option, "Savestates In Custom Folder\0Savestates In ROM Folder\0\0"))
            {
                emu_savestates_dir_option = config_emulator.savestates_dir_option;
            }

            if (config_emulator.savestates_dir_option == 0)
            {
                if (ImGui::MenuItem("Choose Savestate Folder..."))
                {
                    choose_savestates_path = true;
                }

                ImGui::PushItemWidth(350);
                if (ImGui::InputText("##savestate_path", savestates_path, IM_ARRAYSIZE(savestates_path), ImGuiInputTextFlags_AutoSelectAll))
                {
                    config_emulator.savestates_path.assign(savestates_path);
                    strcpy(emu_savestates_path, savestates_path);
                }
                ImGui::PopItemWidth();
            }

            ImGui::EndMenu();
        }

        ImGui::Separator();

        ImGui::MenuItem("Show ROM info", "", &config_emulator.show_info);
        ImGui::MenuItem("Status Messages", "", &config_emulator.status_messages);

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
            if (!config_debug.debug && (config_video.ratio != 3))
            {
                application_trigger_fit_to_content(gui_main_window_width, gui_main_window_height + gui_main_menu_height);
            }
        }

        ImGui::Separator();

        if (ImGui::BeginMenu("Scale"))
        {
            ImGui::PushItemWidth(250.0f);
            ImGui::Combo("##scale", &config_video.scale, "Integer Scale (Auto)\0Integer Scale (X1)\0Integer Scale (X2)\0Integer Scale (X3)\0Scale to Window Height\0Scale to Window Width & Height\0\0");
            ImGui::PopItemWidth();
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Aspect Ratio"))
        {
            ImGui::PushItemWidth(160.0f);
            ImGui::Combo("##ratio", &config_video.ratio, "Square Pixels (1:1 PAR)\0Standard (4:3 DAR)\0Wide (16:9 DAR)\0\0");
            ImGui::PopItemWidth();
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Overscan"))
        {
            ImGui::PushItemWidth(150.0f);
            if (ImGui::Combo("##overscan", &config_video.overscan, "Disabled\0Top+Bottom\0Full (284 width)\0Full (320 width)\0\0"))
            {
                // emu_set_overscan(config_debug.debug ? 0 : config_video.overscan);
            }
            ImGui::PopItemWidth();
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

            if (ImGui::BeginMenu("Player 2"))
            {
                keyboard_configuration_item("Left:", &config_input[1].key_left, 1);
                keyboard_configuration_item("Right:", &config_input[1].key_right, 1);
                keyboard_configuration_item("Up:", &config_input[1].key_up, 1);
                keyboard_configuration_item("Down:", &config_input[1].key_down, 1);
                keyboard_configuration_item("Select:", &config_input[1].key_select, 1);
                keyboard_configuration_item("Run:", &config_input[1].key_run, 1);
                keyboard_configuration_item("1:", &config_input[1].key_1, 1);
                keyboard_configuration_item("2:", &config_input[1].key_2, 1);

                gui_popup_modal_keyboard();

                ImGui::EndMenu();
            }

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

            if (ImGui::BeginMenu("Player 2"))
            {
                ImGui::MenuItem("Enable Gamepad P2", "", &config_input[1].gamepad);

                if (ImGui::BeginMenu("Directional Controls"))
                {
                    ImGui::PushItemWidth(150.0f);
                    ImGui::Combo("##directional", &config_input[1].gamepad_directional, "D-pad\0Left Analog Stick\0\0");
                    ImGui::PopItemWidth();
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Button Configuration"))
                {
                    gamepad_configuration_item("Select:", &config_input[1].gamepad_select, 1);
                    gamepad_configuration_item("Run:", &config_input[1].gamepad_run, 1);
                    gamepad_configuration_item("1:", &config_input[1].gamepad_1, 1);
                    gamepad_configuration_item("2:", &config_input[1].gamepad_2, 1);


                    gui_popup_modal_gamepad(1);

                    ImGui::EndMenu();
                }

                ImGui::EndMenu();
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
    if (ImGui::BeginMenu("Debug"))
    {
        gui_in_use = true;

        if (ImGui::MenuItem("Enable", "", &config_debug.debug))
        {
            emu_set_overscan(config_debug.debug ? 0 : config_video.overscan);
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Step Over", "F10", (void*)0, config_debug.debug))
        {
            emu_debug_step_over();
        }

        if (ImGui::MenuItem("Step Into", "F11", (void*)0, config_debug.debug))
        {
            emu_debug_step_into();
        }

        if (ImGui::MenuItem("Step Out", "SHIFT + F11", (void*)0, config_debug.debug))
        {
            emu_debug_step_out();
        }

        if (ImGui::MenuItem("Step Frame", "F6", (void*)0, config_debug.debug))
        {
            emu_debug_step_frame();
        }

        if (ImGui::MenuItem("Break", "F7", (void*)0, config_debug.debug))
        {
            emu_debug_break();
        }

        if (ImGui::MenuItem("Continue", "F5", (void*)0, config_debug.debug))
        {
            emu_debug_continue();
        }

        if (ImGui::MenuItem("Run To Cursor", "F8", (void*)0, config_debug.debug))
        {
            gui_debug_runtocursor();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Go Back", "CTRL + BACKSPACE", (void*)0, config_debug.debug))
        {
            gui_debug_go_back();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Toggle Breakpoint", "F9", (void*)0, config_debug.debug))
        {
            gui_debug_toggle_breakpoint();
        }

        if (ImGui::MenuItem("Clear All Breakpoints", 0, (void*)0, config_debug.debug))
        {
            gui_debug_reset_breakpoints();
        }

        ImGui::MenuItem("Disable All Breakpoints", 0, &emu_debug_disable_breakpoints, config_debug.debug);

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

        ImGui::Separator();

        ImGui::MenuItem("Show Output Screen", "", &config_debug.show_screen, config_debug.debug);
        ImGui::MenuItem("Show Disassembler", "", &config_debug.show_disassembler, config_debug.debug);
        ImGui::MenuItem("Show HuC6280 Status", "", &config_debug.show_processor, config_debug.debug);
        ImGui::MenuItem("Show Memory Editor", "", &config_debug.show_memory, config_debug.debug);
        ImGui::MenuItem("Show HuC6260 Info", "", &config_debug.show_huc6260_info, config_debug.debug);
        ImGui::MenuItem("Show HuC6260 Palettes", "", &config_debug.show_huc6260_palettes, config_debug.debug);
        ImGui::MenuItem("Show HuC6270 Info", "", &config_debug.show_huc6270_info, config_debug.debug);
        ImGui::MenuItem("Show HuC6270 Registers", "", &config_debug.show_huc6270_registers, config_debug.debug);
        ImGui::MenuItem("Show HuC6270 Background", "", &config_debug.show_huc6270_background, config_debug.debug);
        ImGui::MenuItem("Show HuC6270 Sprites", "", &config_debug.show_huc6270_sprites, config_debug.debug);

        ImGui::Separator();

#if defined(__APPLE__) || defined(_WIN32)
        ImGui::MenuItem("Multi-Viewport (Restart required)", "", &config_debug.multi_viewport, config_debug.debug);
        ImGui::Separator();
#endif

        if (ImGui::MenuItem("Load Symbols...", "", (void*)0, config_debug.debug))
        {
            open_symbols = true;
        }

        if (ImGui::MenuItem("Clear Symbols", "", (void*)0, config_debug.debug))
        {
            gui_debug_reset_symbols();
        }

        ImGui::EndMenu();
    }
}

static void menu_about(void)
{
    if (ImGui::BeginMenu("About"))
    {
        gui_in_use = true;

        if (ImGui::MenuItem("About " GEARGRAFX_TITLE " " GEARGRAFX_VERSION " ..."))
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
    if (open_symbols)
        gui_file_dialog_load_symbols();
    if (open_about)
    {
        gui_dialog_in_use = true;
        ImGui::OpenPopup("About " GEARGRAFX_TITLE);
    }

    gui_popup_modal_about();
}

static void keyboard_configuration_item(const char* text, SDL_Scancode* key, int player)
{
    ImGui::Text("%s", text);
    ImGui::SameLine(100);

    char button_label[256];
    sprintf(button_label, "%s##%s%d", SDL_GetScancodeName(*key), text, player);

    if (ImGui::Button(button_label, ImVec2(90,0)))
    {
        gui_configured_key = key;
        ImGui::OpenPopup("Keyboard Configuration");
    }
}

static void gamepad_configuration_item(const char* text, int* button, int player)
{
    ImGui::Text("%s", text);
    ImGui::SameLine(100);

    static const char* gamepad_names[16] = {"A", "B", "X" ,"Y", "BACK", "GUID", "START", "L3", "R3", "L1", "R1", "UP", "DOWN", "LEFT", "RIGHT", "15"};

    char button_label[256];
    sprintf(button_label, "%s##%s%d", gamepad_names[*button], text, player);

    if (ImGui::Button(button_label, ImVec2(70,0)))
    {
        gui_configured_button = button;
        ImGui::OpenPopup("Gamepad Configuration");
    }
}
