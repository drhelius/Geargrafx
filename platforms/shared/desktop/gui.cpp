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

#include <math.h>
#include "imgui/imgui.h"
#include "imgui/fonts/RobotoMedium.h"
#include "nfd/nfd.h"
#include "config.h"
#include "application.h"
#include "emu.h"
#include "renderer.h"
#include "../../../src/geargrafx.h"

#define GUI_IMPORT
#include "gui.h"
#include "gui_menus.h"
#include "gui_popups.h"
#include "gui_actions.h"
#include "gui_debug.h"
#include "gui_debug_memory.h"
#include "gui_debug_disassembler.h"

static ImFont* default_font[4];
static bool status_message_active = false;
static char status_message[4096] = "";
static u32 status_message_start_time = 0;
static u32 status_message_duration = 0;
static void main_window(void);
static void push_recent_rom(std::string path);
//static Cartridge::CartridgeRegions get_region(int index);
static void show_status_message(void);

void gui_init(void)
{
    gui_main_window_width = 0;
    gui_main_window_height = 0;

    if (NFD_Init() != NFD_OKAY)
    {
        Log("NFD Error: %s", NFD_GetError());
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGuiIO& io = ImGui::GetIO();

    io.IniFilename = config_imgui_file_path;

    io.FontGlobalScale /= application_display_scale;

    gui_roboto_font = io.Fonts->AddFontFromMemoryCompressedTTF(RobotoMedium_compressed_data, RobotoMedium_compressed_size, 17.0f * application_display_scale, NULL, io.Fonts->GetGlyphRangesCyrillic());

    ImFontConfig font_cfg;

    for (int i = 0; i < 4; i++)
    {
        font_cfg.SizePixels = (13.0f + (i * 3)) * application_display_scale;
        default_font[i] = io.Fonts->AddFontDefault(&font_cfg);
    }

    gui_default_font = default_font[config_debug.font_size];

    emu_audio_mute(!config_audio.enable);
    emu_set_overscan(config_debug.debug ? 0 : config_video.overscan);

    gui_init_menus();
}

void gui_destroy(void)
{
    ImGui::DestroyContext();
    NFD_Quit();
}

void gui_render(void)
{
    ImGui::NewFrame();

    gui_in_use = gui_dialog_in_use;

    gui_main_menu();

    gui_main_window_hovered = false;

    if((!config_debug.debug && !emu_is_empty()) || (config_debug.debug && config_debug.show_screen))
        main_window();

    gui_debug_windows();

    if (config_emulator.show_info)
        gui_show_info();

    show_status_message();

    ImGui::Render();
}

void gui_shortcut(gui_ShortCutEvent event)
{
    switch (event)
    {  
    case gui_ShortcutOpenROM:
        gui_shortcut_open_rom = true;
        break;
    case gui_ShortcutReset:
        gui_action_reset();
        break;
    case gui_ShortcutPause:
        gui_action_pause();
        break;
    case gui_ShortcutFFWD:
        config_emulator.ffwd = !config_emulator.ffwd;
        gui_action_ffwd();
        break;
    case gui_ShortcutSaveState:
    {
        std::string message("Saving state to slot ");
        message += std::to_string(config_emulator.save_slot + 1);
        gui_set_status_message(message.c_str(), 3000);
        emu_save_state_slot(config_emulator.save_slot + 1);
        break;
    }
    case gui_ShortcutLoadState:
    {
        std::string message("Loading state from slot ");
        message += std::to_string(config_emulator.save_slot + 1);
        gui_set_status_message(message.c_str(), 3000);
        emu_load_state_slot(config_emulator.save_slot + 1);
        break;
    }
    case gui_ShortcutScreenshot:
        gui_action_save_screenshot(NULL);
        break;
    // case gui_ShortcutDebugStep:
    //     if (config_debug.debug)
    //         emu_debug_step();
    //     break;
    // case gui_ShortcutDebugContinue:
    //     if (config_debug.debug)
    //         emu_debug_continue();
    //     break;
    // case gui_ShortcutDebugNextFrame:
    //     if (config_debug.debug)
    //         emu_debug_next_frame();
    //     break;
    // case gui_ShortcutDebugBreakpoint:
    //     if (config_debug.debug)
    //         gui_debug_toggle_breakpoint();
    //     break;
    // case gui_ShortcutDebugRuntocursor:
    //     if (config_debug.debug)
    //         gui_debug_runtocursor();
    //     break;
    // case gui_ShortcutDebugGoBack:
    //     if (config_debug.debug)
    //         gui_debug_go_back();
    //     break;
    case gui_ShortcutDebugCopy:
        gui_debug_copy_memory();
        break;
    case gui_ShortcutDebugPaste:
        gui_debug_paste_memory();
        break;
    case gui_ShortcutShowMainMenu:
        config_emulator.show_menu = !config_emulator.show_menu;
        break;
    default:
        break;
    }
}

void gui_load_rom(const char* path)
{
    std::string message("Loading ROM ");
    message += path;
    gui_set_status_message(message.c_str(), 3000);

    push_recent_rom(path);
    emu_resume();
    emu_load_rom(path);

    gui_debug_reset();

    std::string str(path);
    str = str.substr(0, str.find_last_of("."));
    str += ".sym";
    gui_debug_load_symbols_file(str.c_str());

    if (config_emulator.start_paused)
    {
        emu_pause();
        
        for (int i=0; i < (GG_MAX_RESOLUTION_WIDTH * GG_MAX_RESOLUTION_HEIGHT); i++)
        {
            emu_frame_buffer[i] = 0;
        }
    }
}

void gui_set_status_message(const char* message, u32 milliseconds)
{
    if (config_emulator.status_messages)
    {
        strcpy(status_message, message);
        status_message_active = true;
        status_message_start_time = SDL_GetTicks();
        status_message_duration = milliseconds;
    }
}

static void main_window(void)
{
    GG_Runtime_Info runtime;
    emu_get_runtime(runtime);

    int w = (int)ImGui::GetIO().DisplaySize.x;
    int h = (int)ImGui::GetIO().DisplaySize.y - (config_emulator.show_menu ? gui_main_menu_height : 0);

    int selected_ratio = config_debug.debug ? 0 : config_video.ratio;
    float ratio = 0;

    switch (selected_ratio)
    {
        case 1:
            ratio = 4.0f / 3.0f;
            break;
        case 2:
            ratio = 16.0f / 9.0f;
            break;
        default:
            ratio = (float)runtime.screen_width / (float)runtime.screen_height;
    }

    if (!config_debug.debug && config_video.scale == 5)
    {
        ratio = (float)w / (float)h;
    }

    int w_corrected = (int)(runtime.screen_height * ratio);
    int h_corrected = (int)(runtime.screen_height);
    int scale_multiplier = 0;

    if (config_debug.debug)
    {
        if ((config_video.scale > 0) && (config_video.scale < 4))
            scale_multiplier = config_video.scale;
        else
            scale_multiplier = 1;
    }
    else
    {
        if ((config_video.scale > 0) && (config_video.scale < 4))
        {
            scale_multiplier = config_video.scale;
        }
        else if (config_video.scale == 0)
        {
            int factor_w = w / w_corrected;
            int factor_h = h / h_corrected;
            scale_multiplier = (factor_w < factor_h) ? factor_w : factor_h;
        }
        else if (config_video.scale == 4)
        {
            scale_multiplier = 1;
            h_corrected = h;
            w_corrected = h * ratio;
        }
        else if (config_video.scale == 5)
        {
            scale_multiplier = 1;
            w_corrected = w;
            h_corrected = h;
        }
    }

    gui_main_window_width = w_corrected * scale_multiplier;
    gui_main_window_height = h_corrected * scale_multiplier;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar;
    
    if (config_debug.debug)
    {
        flags |= ImGuiWindowFlags_AlwaysAutoResize;

        ImGui::SetNextWindowPos(ImVec2(568, 31), ImGuiCond_FirstUseEver);

        ImGui::Begin("Output###debug_output", &config_debug.show_screen, flags);
        gui_main_window_hovered = ImGui::IsWindowHovered();
    }
    else
    {
        int window_x = (w - (w_corrected * scale_multiplier)) / 2;
        int window_y = ((h - (h_corrected * scale_multiplier)) / 2) + (config_emulator.show_menu ? gui_main_menu_height : 0);

        ImGui::SetNextWindowSize(ImVec2((float)gui_main_window_width, (float)gui_main_window_height));
        ImGui::SetNextWindowPos(ImVec2((float)window_x, (float)window_y));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::Begin(GEARGRAFX_TITLE, 0, flags);
        gui_main_window_hovered = ImGui::IsWindowHovered();
    }

    float tex_h = (float)runtime.screen_width / (float)(GG_MAX_RESOLUTION_WIDTH);
    float tex_v = (float)runtime.screen_height / (float)(GG_MAX_RESOLUTION_HEIGHT);

    ImGui::Image((void*)(intptr_t)renderer_emu_texture, ImVec2((float)gui_main_window_width, (float)gui_main_window_height), ImVec2(0, 0), ImVec2(tex_h, tex_v));

    if (config_video.fps)
        gui_show_fps();

    ImGui::End();

    ImGui::PopStyleVar();
    ImGui::PopStyleVar();

    if (!config_debug.debug)
    {
        ImGui::PopStyleVar();
    }
}




// static GC_Color color_float_to_int(ImVec4 color)
// {
//     GC_Color ret;
//     ret.red = (u8)floor(color.x >= 1.0 ? 255.0 : color.x * 256.0);
//     ret.green = (u8)floor(color.y >= 1.0 ? 255.0 : color.y * 256.0);
//     ret.blue = (u8)floor(color.z >= 1.0 ? 255.0 : color.z * 256.0);
//     return ret;
// }

// static ImVec4 color_int_to_float(GC_Color color)
// {
//     ImVec4 ret;
//     ret.w = 0;
//     ret.x = (1.0f / 255.0f) * color.red;
//     ret.y = (1.0f / 255.0f) * color.green;
//     ret.z = (1.0f / 255.0f) * color.blue;
//     return ret;
// }

static void push_recent_rom(std::string path)
{
    int slot = 0;
    for (slot = 0; slot < config_max_recent_roms; slot++)
    {
        if (config_emulator.recent_roms[slot].compare(path) == 0)
        {
            break;
        }
    }

    slot = std::min(slot, config_max_recent_roms - 1);

    for (int i = slot; i > 0; i--)
    {
        config_emulator.recent_roms[i] = config_emulator.recent_roms[i - 1];
    }

    config_emulator.recent_roms[0] = path;
}

// static Cartridge::CartridgeRegions get_region(int index)
// {
//     switch (index)
//     {
//         case 0:
//             return Cartridge::CartridgeUnknownRegion;
//         case 1:
//             return Cartridge::CartridgeNTSC;
//         case 2:
//             return Cartridge::CartridgePAL;
//         default:
//             return Cartridge::CartridgeUnknownRegion;
//     }
// }

static void show_status_message(void)
{
    if (status_message_active)
    {
        u32 current_time = SDL_GetTicks();
        if ((current_time - status_message_start_time) > status_message_duration)
            status_message_active = false;
        else
            ImGui::OpenPopup("Status");
    }

    if (status_message_active)
    {
        ImGui::SetNextWindowPos(ImVec2(0.0f, config_emulator.show_menu ? gui_main_menu_height : 0.0f));
        ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, 0.0f));
        ImGui::SetNextWindowBgAlpha(0.9f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav;

        if (ImGui::BeginPopup("Status", flags))
        {
            ImGui::PushFont(gui_default_font);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.1f,0.9f,0.1f,1.0f));
            ImGui::TextWrapped("%s", status_message);
            ImGui::PopStyleColor();
            ImGui::PopFont();
            ImGui::EndPopup();
        }

        ImGui::PopStyleVar();
    }
}
