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
#include "imgui/implot.h"
#include "imgui/fonts/RobotoMedium.h"
#include "imgui/fonts/MaterialIcons.h"
#include "imgui/fonts/IconsMaterialDesign.h"
#include "nfd/nfd.h"
#include "config.h"
#include "application.h"
#include "emu.h"
#include "renderer.h"
#include "utils.h"
#include "geargrafx.h"

#define GUI_IMPORT
#include "gui.h"
#include "gui_menus.h"
#include "gui_popups.h"
#include "gui_actions.h"
#include "gui_debug.h"
#include "gui_debug_memory.h"
#include "gui_debug_disassembler.h"
#include "gui_debug_psg.h"
#include "gui_debug_trace_logger.h"

static bool status_message_active = false;
static char status_message[4096] = "";
static u32 status_message_start_time = 0;
static u32 status_message_duration = 0;
static bool error_window_active = false;
static char error_message[4096] = "";
static void main_window(void);
static void push_recent_rom(std::string path);
static void show_status_message(void);
static void show_error_window(void);
static void set_style(void);
static ImVec4 lerp(const ImVec4& a, const ImVec4& b, float t);

bool gui_init(void)
{
    gui_main_window_width = 0;
    gui_main_window_height = 0;

    if (NFD_Init() != NFD_OKAY)
    {
        Log("NFD Error: %s", NFD_GetError());
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGui::StyleColorsDark();
    ImGuiIO& io = ImGui::GetIO();

    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigDockingWithShift = true;
    io.IniFilename = config_imgui_file_path;
    io.FontGlobalScale /= application_display_scale;

#if defined(__APPLE__) || defined(_WIN32)
    if (config_debug.multi_viewport)
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
#endif

    gui_roboto_font = io.Fonts->AddFontFromMemoryCompressedTTF(RobotoMedium_compressed_data, RobotoMedium_compressed_size, 17.0f * application_display_scale, NULL, io.Fonts->GetGlyphRangesCyrillic());

    float iconFontSize = 20.0f * application_display_scale;
    static const ImWchar icons_ranges[] = { ICON_MIN_MD, ICON_MAX_16_MD, 0 };
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    icons_config.GlyphMinAdvanceX = iconFontSize;
    icons_config.GlyphOffset = { 0.0f, 5.0f * application_display_scale };
    gui_material_icons_font = io.Fonts->AddFontFromMemoryCompressedTTF(MaterialIcons_compressed_data, MaterialIcons_compressed_size, iconFontSize, &icons_config, icons_ranges);

    ImFontConfig font_cfg;

    for (int i = 0; i < 4; i++)
    {
        font_cfg.SizePixels = (13.0f + (i * 3)) * application_display_scale;
        gui_default_fonts[i] = io.Fonts->AddFontDefault(&font_cfg);
    }

    gui_default_font = gui_default_fonts[config_debug.font_size];

    set_style();

    gui_audio_mute_cdrom = false;
    gui_audio_mute_psg = false;
    gui_audio_mute_adpcm = false;

    emu_audio_mute(!config_audio.enable);
    emu_audio_huc6280a(config_audio.huc6280a);
    emu_audio_psg_volume(config_audio.psg_volume);
    emu_audio_cdrom_volume(config_audio.cdrom_volume);
    emu_audio_adpcm_volume(config_audio.adpcm_volume);
    emu_set_composite_palette(config_video.composite_palette);
    emu_video_no_sprite_limit(config_video.sprite_limit);
    emu_set_overscan(config_debug.debug ? 0 : config_video.overscan);
    emu_set_scanline_start_end(
                config_debug.debug ? 0 : config_video.scanline_start,
                config_debug.debug ? 241 : config_video.scanline_end);
    emu_set_memory_reset_values(
                get_reset_value(config_debug.reset_mpr),
                get_reset_value(config_debug.reset_ram),
                get_reset_value(config_debug.reset_card_ram),
                get_reset_value(config_debug.reset_arcade_card));
    emu_set_huc6260_color_table_reset_value(get_reset_value(config_debug.reset_color_table));
    emu_set_huc6280_registers_reset_value(get_reset_value(config_debug.reset_registers));
    emu_set_console_type((GG_Console_Type)config_emulator.console_type);
    emu_set_cdrom_type((GG_CDROM_Type)config_emulator.cdrom_type);
    emu_set_preload_cdrom(config_emulator.preload_cdrom);
    emu_set_backup_ram(config_emulator.backup_ram);
    emu_set_composite_palette(config_video.composite_palette);
    emu_set_turbo_tap(config_input.turbo_tap);
    for (int i = 0; i < GG_MAX_GAMEPADS; i++)
    {
        emu_set_pad_type((GG_Controllers)i, (GG_Controller_Type)config_input.controller_type[i]);
        GG_Keys key = GG_KEY_NONE;
        if (config_input.avenue_pad_3_button[i] == 1)
            key = GG_KEY_SELECT;
        else if (config_input.avenue_pad_3_button[i] == 2)
            key = GG_KEY_RUN;
        emu_set_avenue_pad_3_button((GG_Controllers)i, key);

        emu_set_turbo((GG_Controllers)i, GG_KEY_I, config_input.turbo_enabled[i][0]);
        emu_set_turbo_speed((GG_Controllers)i, GG_KEY_I, config_input.turbo_speed[i][0]);
        emu_set_turbo((GG_Controllers)i, GG_KEY_II, config_input.turbo_enabled[i][1]);
        emu_set_turbo_speed((GG_Controllers)i, GG_KEY_II, config_input.turbo_speed[i][1]);
    }
    emu_debug_set_callback(gui_debug_callback);

    strcpy(gui_savefiles_path, config_emulator.savefiles_path.c_str());
    strcpy(gui_savestates_path, config_emulator.savestates_path.c_str());
    strcpy(gui_screenshots_path, config_emulator.screenshots_path.c_str());
    strcpy(gui_backup_ram_path, config_emulator.backup_ram_path.c_str());
    strcpy(gui_syscard_bios_path, config_emulator.syscard_bios_path.c_str());
    strcpy(gui_gameexpress_bios_path, config_emulator.gameexpress_bios_path.c_str());
    if (strlen(gui_syscard_bios_path) > 0)
        gui_load_bios(gui_syscard_bios_path, true);
    if (strlen(gui_gameexpress_bios_path) > 0)
        gui_load_bios(gui_gameexpress_bios_path, false);

    gui_debug_init();
    gui_init_menus();

    return true;
}

void gui_destroy(void)
{
    gui_debug_destroy();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    NFD_Quit();
}

void gui_render(void)
{
    ImGui::NewFrame();

    if (config_debug.debug)
        ImGui::DockSpaceOverViewport();

    gui_in_use = gui_dialog_in_use;

    gui_main_menu();

    gui_main_window_hovered = false;

    if((!config_debug.debug && !emu_is_empty()) || (config_debug.debug && config_debug.show_screen))
        main_window();

    gui_debug_windows();

    if (config_emulator.show_info)
        gui_show_info();

    show_status_message();
    show_error_window();

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
    case gui_ShortcutDebugStepOver:
        if (config_debug.debug)
            emu_debug_step_over();
        break;
    case gui_ShortcutDebugStepInto:
        if (config_debug.debug)
            emu_debug_step_into();
        break;
    case gui_ShortcutDebugStepOut:
        if (config_debug.debug)
            emu_debug_step_out();
        break;
    case gui_ShortcutDebugStepFrame:
        if (config_debug.debug)
        {
            emu_debug_step_frame();
            gui_debug_memory_step_frame();
        }
        break;
    case gui_ShortcutDebugBreak:
        if (config_debug.debug)
            emu_debug_break();
        break;
    case gui_ShortcutDebugContinue:
        if (config_debug.debug)
            emu_debug_continue();
        break;
    case gui_ShortcutDebugRuntocursor:
        if (config_debug.debug)
            gui_debug_runtocursor();
        break;
    case gui_ShortcutDebugGoBack:
        if (config_debug.debug)
            gui_debug_go_back();
        break;
    case gui_ShortcutDebugBreakpoint:
        if (config_debug.debug)
            gui_debug_toggle_breakpoint();
        break;
    case gui_ShortcutDebugCopy:
        gui_debug_memory_copy();
        break;
    case gui_ShortcutDebugPaste:
        gui_debug_memory_paste();
        break;
    case gui_ShortcutDebugSelectAll:
        gui_debug_memory_select_all();
        break;
    case gui_ShortcutShowMainMenu:
        config_emulator.show_menu = !config_emulator.show_menu;
        break;
    default:
        break;
    }
}

void gui_load_bios(const char* path, bool syscard)
{
    using namespace std;
    string fullpath(path);
    string filename;

    size_t pos = fullpath.find_last_of("/\\");
    if (pos != string::npos)
        filename = fullpath.substr(pos + 1);
    else
        filename = fullpath;

    if (!emu_load_bios(path, syscard))
    {
        std::string message("Error loading BIOS:\n");
        message += filename;
        gui_set_error_message(message.c_str());
        gui_action_reset();
        return;
    }

    if (!emu_get_core()->GetMedia()->IsValidBios(syscard))
    {
        std::string message("Invalid BIOS file:\n");
        message += filename;
        message += "\n\nMake sure the file is a valid BIOS file.";
        gui_set_error_message(message.c_str());
        gui_action_reset();
        return;
    }

    gui_action_reset();
}

void gui_load_rom(const char* path)
{
    using namespace std;

    string message("Loading ROM ");
    message += path;
    gui_set_status_message(message.c_str(), 3000);

    push_recent_rom(path);
    emu_resume();

    if (!emu_load_media(path))
    {
        string message("Error loading ROM:\n");
        message += path;
        gui_set_error_message(message.c_str());

        emu_get_core()->GetMedia()->Reset();
        gui_action_reset();
        return;
    }

    if (emu_get_core()->GetMedia()->IsCDROM() && !emu_get_core()->GetMedia()->IsLoadedBios())
    {
        bool is_gameexpress = emu_get_core()->GetMedia()->IsGameExpress();
        string bios_name = is_gameexpress ? "Game Express BIOS" : "System Card BIOS";

        std::string message;
        message += bios_name;
        message += " is required to run this ROM!!\n";
        message += "Make sure you have a valid BIOS file in 'Menu->Emulator->BIOS'.";
        gui_set_error_message(message.c_str());

        emu_get_core()->GetMedia()->Reset();
        gui_action_reset();
        return;
    }

    gui_debug_reset();

    std::string str(path);
    str = str.substr(0, str.find_last_of("."));
    str += ".sym";
    gui_debug_load_symbols_file(str.c_str());

    if (config_emulator.start_paused)
    {
        emu_pause();

        for (int i=0; i < (HUC6270_MAX_RESOLUTION_WIDTH * HUC6270_MAX_RESOLUTION_HEIGHT); i++)
        {
            emu_frame_buffer[i] = 0;
        }
    }

    if (!emu_is_empty())
        application_update_title_with_rom(emu_get_core()->GetMedia()->GetFileName());
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

void gui_set_error_message(const char* message)
{
    strcpy(error_message, message);
    error_window_active = true;
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
        case 3:
            ratio = 16.0f / 10.0f;
            break;
        case 4:
            ratio = 6.0f / 5.0f;
            break;
        default:
            ratio = ((float)runtime.screen_width / (float)runtime.width_scale) / (float)runtime.screen_height;
    }

    if (!config_debug.debug && config_video.scale == 3)
    {
        ratio = (float)w / (float)h;
    }

    int base_width = (int)(runtime.screen_width / runtime.width_scale);
    int base_height = (int)(runtime.screen_height);

    int w_corrected, h_corrected;
    int scale_multiplier = 0;

    if (config_debug.debug)
    {
        if ((config_video.scale != 0))
            scale_multiplier = config_video.scale_manual;
        else
            scale_multiplier = 1;

        w_corrected = base_width;
        h_corrected = base_height;
    }
    else
    {
        if (selected_ratio == 0)
        {
            w_corrected = base_width;
            h_corrected = base_height;
        }
        else
        {
            w_corrected = (int)round(base_height * ratio);
            h_corrected = base_height;
        }

        switch (config_video.scale)
        {
        case 0:
        {
            int factor_w = w / w_corrected;
            int factor_h = h / h_corrected;
            scale_multiplier = (factor_w < factor_h) ? factor_w : factor_h;
            break;
        }
        case 1:
            scale_multiplier = config_video.scale_manual;
            break;
        case 2:
            scale_multiplier = 1;
            h_corrected = h;
            w_corrected = (int)round(h * ratio);
            break;
        case 3:
            scale_multiplier = 1;
            w_corrected = w;
            h_corrected = h;
            break;
        default:
            scale_multiplier = 1;
            break;
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

        ImGui::SetNextWindowPos(ImVec2(631, 26), ImGuiCond_FirstUseEver);

        ImGui::Begin("Output###debug_output", &config_debug.show_screen, flags);
        gui_main_window_hovered = ImGui::IsWindowHovered();
    }
    else
    {
        int window_x = (w - (w_corrected * scale_multiplier)) / 2;
        int window_y = ((h - (h_corrected * scale_multiplier)) / 2) + (config_emulator.show_menu ? gui_main_menu_height : 0);

        ImGui::SetNextWindowSize(ImVec2((float)gui_main_window_width, (float)gui_main_window_height));
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos + ImVec2((float)window_x, (float)window_y));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::Begin(GG_TITLE, 0, flags);
        gui_main_window_hovered = ImGui::IsWindowHovered();
    }

    float tex_h = (float)runtime.screen_width / (float)(SYSTEM_TEXTURE_WIDTH);
    float tex_v = (float)runtime.screen_height / (float)(SYSTEM_TEXTURE_HEIGHT);

    ImGui::Image((ImTextureID)(intptr_t)renderer_emu_texture, ImVec2((float)gui_main_window_width, (float)gui_main_window_height), ImVec2(0, 0), ImVec2(tex_h, tex_v));

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

    slot = MIN(slot, config_max_recent_roms - 1);

    for (int i = slot; i > 0; i--)
    {
        config_emulator.recent_roms[i] = config_emulator.recent_roms[i - 1];
    }

    config_emulator.recent_roms[0] = path;
}

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

static void show_error_window(void)
{
    if (error_window_active)
    {
        error_window_active = false;
        ImGui::OpenPopup("Error");
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Error", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("%s\n\n", error_message);
        ImGui::Separator();
        if (ImGui::Button("OK"))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

static void set_style(void)
{
    ImGuiStyle& style = ImGui::GetStyle();

    style.Alpha = 1.0f;
    style.DisabledAlpha = 0.6000000238418579f;
    style.WindowPadding = ImVec2(8.0f, 8.0f);
    style.WindowRounding = 4.0f;
    style.WindowBorderSize = 1.0f;
    style.WindowMinSize = ImVec2(32.0f, 32.0f);
    style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
    style.WindowMenuButtonPosition = ImGuiDir_Left;
    style.ChildRounding = 0.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupRounding = 4.0f;
    style.PopupBorderSize = 1.0f;
    style.FramePadding = ImVec2(4.0f, 3.0f);
    style.FrameRounding = 2.5f;
    style.FrameBorderSize = 0.0f;
    style.ItemSpacing = ImVec2(8.0f, 4.0f);
    style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
    style.CellPadding = ImVec2(4.0f, 2.0f);
    style.IndentSpacing = 21.0f;
    style.ColumnsMinSpacing = 6.0f;
    style.ScrollbarSize = 11.0f;
    style.ScrollbarRounding = 2.5f;
    style.GrabMinSize = 10.0f;
    style.GrabRounding = 2.0f;
    style.TabRounding = 3.5f;
    style.TabBorderSize = 0.0f;
    style.TabMinWidthForCloseButton = 0.0f;
    style.ColorButtonPosition = ImGuiDir_Right;
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

    style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.5921568870544434f, 0.5921568870544434f, 0.5921568870544434f, 1.0f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.060085229575634f, 0.060085229575634f, 0.06008583307266235f, 1.0f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.05882352963089943f, 0.05882352963089943f, 0.05882352963089943f, 1.0f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.1176470592617989f, 0.1176470592617989f, 0.1176470592617989f, 1.0f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.1802574992179871f, 0.1802556961774826f, 0.1802556961774826f, 1.0f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.3058823645114899f, 0.3058823645114899f, 0.3058823645114899f, 1.0f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.1843137294054031f, 0.1843137294054031f, 0.1843137294054031f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.270386278629303f, 0.2703835666179657f, 0.2703848779201508f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.8745098114013672f, 0.007843137718737125f, 0.3882353007793427f, 1.0f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.8745098114013672f, 0.007843137718737125f, 0.3882353007793427f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.1176470592617989f, 0.1176470592617989f, 0.1176470592617989f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.1176470592617989f, 0.1176470592617989f, 0.1176470592617989f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.6266094446182251f, 0.6266031861305237f, 0.6266063451766968f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.9999899864196777f, 0.9999899864196777f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.9999899864196777f, 0.9999899864196777f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.8745098114013672f, 0.007843137718737125f, 0.3882353007793427f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.8745098114013672f, 0.007843137718737125f, 0.3882353007793427f, 1.0f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.8745098114013672f, 0.007843137718737125f, 0.3882353007793427f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.184547483921051f, 0.184547483921051f, 0.1845493316650391f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.8745098114013672f, 0.007843137718737125f, 0.3882353007793427f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.8745098114013672f, 0.007843137718737125f, 0.3882353007793427f, 1.0f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.1843137294054031f, 0.1843137294054031f, 0.1843137294054031f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.8745098114013672f, 0.007843137718737125f, 0.3882353007793427f, 1.0f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.8745098114013672f, 0.007843137718737125f, 0.3882353007793427f, 1.0f);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.1803921610116959f, 0.1803921610116959f, 0.1803921610116959f, 1.0f);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.1803921610116959f, 0.1803921610116959f, 0.1803921610116959f, 1.0f);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.1803921610116959f, 0.1803921610116959f, 0.1803921610116959f, 1.0f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.2489270567893982f, 0.2489245682954788f, 0.2489245682954788f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.0f, 0.9999899864196777f, 0.9999899864196777f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(1.0f, 0.9999899864196777f, 0.9999899864196777f, 1.0f);
    style.Colors[ImGuiCol_Tab] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.8745098114013672f, 0.007843137718737125f, 0.3882353007793427f, 1.0f);
    style.Colors[ImGuiCol_TabActive] = ImVec4(0.8755365014076233f, 0.00751531170681119f, 0.3875076174736023f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.8745098114013672f, 0.007843137718737125f, 0.3882353007793427f, 1.0f);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.8745098114013672f, 0.007843137718737125f, 0.3882353007793427f, 1.0f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.8745098114013672f, 0.007843137718737125f, 0.3882353007793427f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.8745098114013672f, 0.007843137718737125f, 0.3882353007793427f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.8745098114013672f, 0.007843137718737125f, 0.3882353007793427f, 1.0f);
    style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882352977991104f, 0.1882352977991104f, 0.2000000029802322f, 1.0f);
    style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3490196168422699f, 1.0f);
    style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.2274509817361832f, 0.2274509817361832f, 0.2470588237047195f, 1.0f);
    style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.05999999865889549f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.8745098114013672f, 0.007843137718737125f, 0.3882353007793427f, 1.0f);
    style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.699999988079071f);
    style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.2000000029802322f);
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 0.7f);

    style.Colors[ImGuiCol_DockingPreview] = style.Colors[ImGuiCol_HeaderActive] * ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
    style.Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_TabHovered] = style.Colors[ImGuiCol_HeaderHovered];
    //style.Colors[ImGuiCol_Tab] = lerp(style.Colors[ImGuiCol_Header], style.Colors[ImGuiCol_TitleBgActive], 0.80f);
    style.Colors[ImGuiCol_TabSelected] = lerp(style.Colors[ImGuiCol_HeaderActive], style.Colors[ImGuiCol_TitleBgActive], 0.60f);
    style.Colors[ImGuiCol_TabSelectedOverline] = style.Colors[ImGuiCol_HeaderActive];
    style.Colors[ImGuiCol_TabDimmed] = lerp(style.Colors[ImGuiCol_Tab], style.Colors[ImGuiCol_TitleBg], 0.60f);
    style.Colors[ImGuiCol_TabDimmedSelected] = lerp(style.Colors[ImGuiCol_TabSelected], style.Colors[ImGuiCol_TitleBg], 0.40f);
    style.Colors[ImGuiCol_TabDimmedSelectedOverline] = lerp(style.Colors[ImGuiCol_TabSelected], style.Colors[ImGuiCol_TitleBg], 0.20f);
}

static ImVec4 lerp(const ImVec4& a, const ImVec4& b, float t)
{
    return ImVec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t);
}
