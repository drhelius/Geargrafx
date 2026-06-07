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

#include <SDL3/SDL.h>
#include <vector>

#define GUI_POPUPS_IMPORT
#include "gui_popups.h"
#include "gui.h"
#include "gui_debug_constants.h"
#include "config.h"
#include "application.h"
#include "gamepad.h"
#include "emu.h"
#include "license.h"
#include "backers.h"
#include "ogl_renderer.h"
#include "keyboard.h"
#include "imgui.h"
#include "implot.h"
#if defined(GG_ENABLE_PHYSICAL_CDROM)
#include "cdrom_drive.h"
#endif

static char build_info[4096] = "";
static int info_pos = 0;
#if defined(GG_ENABLE_PHYSICAL_CDROM)
static const char* physical_cdrom_popup_title = "Select Physical CD-ROM...";
static std::vector<CdRomDriveInfo> physical_cdrom_drives;
static int physical_cdrom_selected = -1;
static bool physical_cdrom_refresh = false;
#endif

static void add_build_info(const char* fmt, ...);
static void check_hotkey_duplicates_popup(config_Hotkey* current_hotkey);
#if defined(GG_ENABLE_PHYSICAL_CDROM)
static bool open_selected_physical_cdrom_drive(void);
#endif
static void refresh_physical_cdrom_drives(void);

void gui_popup_open_physical_cdrom(void)
{
    #if defined(GG_ENABLE_PHYSICAL_CDROM)
    Debug("Opening physical CD-ROM popup");
    physical_cdrom_refresh = true;
    physical_cdrom_selected = -1;
    gui_dialog_in_use = true;
    ImGui::OpenPopup(physical_cdrom_popup_title);
    #endif
}

void gui_popup_modal_physical_cdrom(void)
{
    #if defined(GG_ENABLE_PHYSICAL_CDROM)
    if (physical_cdrom_refresh)
    {
        refresh_physical_cdrom_drives();
        physical_cdrom_refresh = false;
    }

    if (ImGui::BeginPopupModal(physical_cdrom_popup_title, NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        gui_dialog_in_use = true;

        ImGuiTableFlags table_flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings;
        if (ImGui::BeginTable("##physical_cdrom_drives", 2, table_flags, ImVec2(350.0f, 100.0f)))
        {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Drive", ImGuiTableColumnFlags_WidthFixed, 270.0f);
            ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableHeadersRow();

            if (physical_cdrom_drives.empty())
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextDisabled("No physical CD-ROM found");
                ImGui::TableNextColumn();
                ImGui::TextDisabled("-");
            }
            else
            {
                for (int i = 0; i < (int)physical_cdrom_drives.size(); i++)
                {
                    CdRomDriveInfo& drive = physical_cdrom_drives[i];
                    bool selected = physical_cdrom_selected == i;
                    char label[640];
                    snprintf(label, sizeof(label), "##physical_cdrom_%d", i);

                    ImGui::TableNextRow();
                    if (selected)
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, ImGui::GetColorU32(red));

                    ImGui::TableNextColumn();
                    if (ImGui::Selectable(label, false, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick))
                    {
                        bool double_clicked = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

                        if (double_clicked)
                        {
                            physical_cdrom_selected = i;
                            open_selected_physical_cdrom_drive();
                        }
                        else if (selected)
                            physical_cdrom_selected = -1;
                        else
                            physical_cdrom_selected = i;
                    }

                    ImGui::SameLine(0, 0);
                    ImGui::TextUnformatted(drive.name);

                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(drive.has_disc ? "Ready" : "No Disc");
                }
            }

            ImGui::EndTable();
        }

        bool can_open = (physical_cdrom_selected >= 0) && (physical_cdrom_selected < (int)physical_cdrom_drives.size()) && physical_cdrom_drives[physical_cdrom_selected].has_disc;

        if (!can_open)
            ImGui::BeginDisabled();

        if (ImGui::Button("Open", ImVec2(110.0f, 0.0f)) && can_open)
            open_selected_physical_cdrom_drive();

        if (!can_open)
            ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(110.0f, 0.0f)))
        {
            Debug("Physical CD-ROM popup canceled");
            gui_dialog_in_use = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Refresh", ImVec2(110.0f, 0.0f)))
        {
            Debug("Physical CD-ROM popup refresh button pressed");
            refresh_physical_cdrom_drives();
        }

        ImGui::EndPopup();
    }
    #endif
}

#if defined(GG_ENABLE_PHYSICAL_CDROM)
static bool open_selected_physical_cdrom_drive(void)
{
    if ((physical_cdrom_selected < 0) || (physical_cdrom_selected >= (int)physical_cdrom_drives.size()))
        return false;

    CdRomDriveInfo& drive = physical_cdrom_drives[physical_cdrom_selected];
    if (!drive.has_disc)
        return false;

    Log("Opening physical CD-ROM drive %s", drive.id);
    gui_load_physical_cdrom(drive.id);
    gui_dialog_in_use = false;
    ImGui::CloseCurrentPopup();
    return true;
}
#endif

static void refresh_physical_cdrom_drives(void)
{
    #if defined(GG_ENABLE_PHYSICAL_CDROM)
    Debug("Enumerating physical CD-ROM drives");
    bool listed = CdRomDrive::ListDrives(physical_cdrom_drives);
    physical_cdrom_selected = -1;

    if (!listed)
    {
        Error("Physical CD-ROM drive enumeration failed");
        return;
    }

    Debug("Physical CD-ROM drives found: %d", (int)physical_cdrom_drives.size());

    for (int i = 0; i < (int)physical_cdrom_drives.size(); i++)
    {
        Debug("Physical CD-ROM drive %d: id=%s name=%s has_disc=%s", i, physical_cdrom_drives[i].id, physical_cdrom_drives[i].name, physical_cdrom_drives[i].has_disc ? "true" : "false");
    }
    #endif
}

void gui_popup_modal_keyboard()
{
    if (ImGui::BeginPopupModal("Keyboard Configuration", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Press any key to assign...\n\n");
        ImGui::Separator();

        SDL_Scancode scancode = keyboard_get_first_pressed_scancode();
        if (scancode != SDL_SCANCODE_UNKNOWN)
        {
            *gui_configured_key = scancode;
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void gui_popup_modal_gamepad(int pad)
{
    if (ImGui::BeginPopupModal("Gamepad Configuration", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        SDL_Gamepad* controller = gamepad_controller[pad];

        if (IsValidPointer(controller))
            ImGui::Text("Press any button in your gamepad...\n\n");
        else
            ImGui::Text("No gamepad detected.\n\n");

        ImGui::Separator();

        if (IsValidPointer(controller))
        {
            for (int i = 0; i < SDL_GAMEPAD_BUTTON_COUNT; i++)
            {
                if (SDL_GetGamepadButton(controller, (SDL_GamepadButton)i))
                {
                    *gui_configured_button = i;
                    ImGui::CloseCurrentPopup();
                    break;
                }
            }

            for (int a = SDL_GAMEPAD_AXIS_LEFTX; a < SDL_GAMEPAD_AXIS_COUNT; a++)
            {
                if (a != SDL_GAMEPAD_AXIS_LEFT_TRIGGER && a != SDL_GAMEPAD_AXIS_RIGHT_TRIGGER)
                    continue;

                Sint16 value = SDL_GetGamepadAxis(controller, (SDL_GamepadAxis)a);

                if (value > GAMEPAD_VBTN_AXIS_THRESHOLD)
                {
                    *gui_configured_button = GAMEPAD_VBTN_AXIS_BASE + a;
                    ImGui::CloseCurrentPopup();
                    break;
                }
            }
        }

        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void gui_popup_modal_hotkey()
{
    if (ImGui::BeginPopupModal("Hotkey Configuration", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Press any key combination...\n");
        ImGui::Text("Hold Ctrl, Shift, or Alt before pressing the key\n\n");
        ImGui::Separator();

        SDL_Keymod mods = (SDL_Keymod)(SDL_GetModState() & (SDL_KMOD_CTRL | SDL_KMOD_SHIFT | SDL_KMOD_ALT | SDL_KMOD_GUI));
        SDL_Scancode scancode = keyboard_get_first_pressed_scancode();
        if (scancode != SDL_SCANCODE_UNKNOWN)
        {
            gui_configured_hotkey->key = scancode;
            gui_configured_hotkey->mod = mods;
            config_update_hotkey_string(gui_configured_hotkey);
            check_hotkey_duplicates_popup(gui_configured_hotkey);
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void gui_popup_modal_about(void)
{
    if (ImGui::BeginPopupModal("About " GG_TITLE, NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::PushFont(gui_default_font);
        ImGui::TextColored(cyan, "%s\n", GG_TITLE_ASCII);

        ImGui::TextColored(violet, "  By Ignacio Sánchez (DrHelius)");
        ImGui::Text(" "); ImGui::SameLine();
        ImGui::TextLinkOpenURL("https://github.com/drhelius/Geargrafx");
        ImGui::Text(" "); ImGui::SameLine();
        ImGui::TextLinkOpenURL("https://x.com/drhelius");
        ImGui::NewLine();

        ImGui::PopFont();

        if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None))
        {
            if (ImGui::BeginTabItem("Build Info"))
            {
                build_info[0] = '\0';
                info_pos = 0;

                add_build_info("Build: %s\n", GG_VERSION);
                #if defined(__DATE__) && defined(__TIME__)
                add_build_info("Built on: %s - %s\n", __DATE__, __TIME__);
                #endif
                #if defined(_M_ARM64)
                add_build_info("Windows ARM64 build\n");
                #endif
                #if defined(_M_X64)
                add_build_info("Windows 64 bit build\n");
                #endif
                #if defined(_M_IX86)
                add_build_info("Windows 32 bit build\n");
                #endif
                #if defined(__linux__) && defined(__x86_64__)
                add_build_info("Linux 64 bit build\n");
                #endif
                #if defined(__linux__) && defined(__i386__)
                add_build_info("Linux 32 bit build\n");
                #endif
                #if defined(__linux__) && defined(__arm__)
                add_build_info("Linux ARM build\n");
                #endif
                #if defined(__linux__) && defined(__aarch64__)
                add_build_info("Linux ARM64 build\n");
                #endif
                #if defined(__APPLE__) && defined(__arm64__ )
                add_build_info("macOS build (Apple Silicon)\n");
                #endif
                #if defined(__APPLE__) && defined(__x86_64__)
                add_build_info("macOS build (Intel)\n");
                #endif
                #if defined(__ANDROID__)
                add_build_info("Android build\n");
                #endif
                add_build_info("Config file: %s\n", config_emu_file_path);
                add_build_info("ImGui file: %s\n", config_imgui_file_path);
                add_build_info("Savestate version: %d\n", GG_SAVESTATE_VERSION);
                #if defined(_MSC_FULL_VER)
                add_build_info("Microsoft C++ %d\n", _MSC_FULL_VER);
                #endif
                #if defined(_MSVC_LANG)
                add_build_info("MSVC %d\n", _MSVC_LANG);
                #endif
                #if defined(__CLR_VER)
                add_build_info("CLR version: %d\n", __CLR_VER);
                #endif
                #if defined(__MINGW32__)
                add_build_info("MinGW 32 bit (%d.%d)\n", __MINGW32_MAJOR_VERSION, __MINGW32_MINOR_VERSION);
                #endif
                #if defined(__MINGW64__)
                add_build_info("MinGW 64 bit (%d.%d)\n", __MINGW64_VERSION_MAJOR, __MINGW64_VERSION_MINOR);
                #endif
                #if defined(__GNUC__) && !defined(__llvm__) && !defined(__INTEL_COMPILER)
                add_build_info("GCC %d.%d.%d\n", (int)__GNUC__, (int)__GNUC_MINOR__, (int)__GNUC_PATCHLEVEL__);
                #endif
                #if defined(__clang_version__)
                add_build_info("Clang %s\n", __clang_version__);
                #endif
                add_build_info("SDL %d.%d.%d (build)\n", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION);
                add_build_info("SDL %d.%d.%d (link)\n", application_sdl_version_major, application_sdl_version_minor, application_sdl_version_patch);
                add_build_info("OpenGL %s\n", ogl_renderer_opengl_version);
                add_build_info("Dear ImGui %s (%d)\n", IMGUI_VERSION, IMGUI_VERSION_NUM);
                add_build_info("ImPlot %s (%d)\n", IMPLOT_VERSION, IMPLOT_VERSION_NUM);

                #if defined(DEBUG)
                add_build_info("define: DEBUG\n");
                #endif
                #if defined(NDEBUG)
                add_build_info("define: NDEBUG\n");
                #endif
                #if defined(GG_DEBUG)
                add_build_info("define: GG_DEBUG\n");
                #endif
                #if defined(GG_NO_OPTIMIZATIONS)
                add_build_info("define: GG_NO_OPTIMIZATIONS\n");
                #endif
                #if defined(GG_DISABLE_DISASSEMBLER)
                add_build_info("define: GG_DISABLE_DISASSEMBLER\n");
                #endif
                #if defined(__cplusplus)
                add_build_info("define: __cplusplus = %d\n", (int)__cplusplus);
                #endif
                #if defined(__STDC__)
                add_build_info("define: __STDC__ = %d\n", (int)__STDC__);
                #endif
                #if defined(__STDC_VERSION__)
                add_build_info("define: __STDC_VERSION__ = %d\n", (int)__STDC_VERSION__);
                #endif
                #if defined(GG_LITTLE_ENDIAN)
                add_build_info("define: GG_LITTLE_ENDIAN");
                #endif
                #if defined(GG_BIG_ENDIAN)
                add_build_info("define: GG_BIG_ENDIAN");
                #endif

                ImGui::InputTextMultiline("##build_info", build_info, sizeof(build_info), ImVec2(-1.0f, 100.0f), ImGuiInputTextFlags_ReadOnly);

                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Special thanks to"))
            {
                ImGui::BeginChild("backers", ImVec2(0, 100), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
                ImGui::Text("%s", BACKERS_STR);
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("LICENSE"))
            {
                ImGui::BeginChild("license", ImVec2(0, 100), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
                ImGui::TextUnformatted(GPL_LICENSE_STR);
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::NewLine();
        ImGui::Separator();

        if (gamepad_added_mappings || gamepad_updated_mappings)
        {
            ImGui::Text("%d game controller mappings added from gamecontrollerdb.txt", gamepad_added_mappings);
            ImGui::Text("%d game controller mappings updated from gamecontrollerdb.txt", gamepad_updated_mappings);
        }
        else
            ImGui::Text("ERROR: Game controller database not found (gamecontrollerdb.txt)!!");

        ImGui::Separator();
        ImGui::NewLine();

        if (ImGui::Button("OK", ImVec2(120, 0))) 
        {
            ImGui::CloseCurrentPopup();
            gui_dialog_in_use = false;
        }
        ImGui::SetItemDefaultFocus();

        ImGui::EndPopup();
    }
}

void gui_popup_modal_load_defaults(void)
{
    if (ImGui::BeginPopupModal("Load Default Settings", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Are you sure you want to load default settings?\n\n");
        ImGui::Text("This action cannot be reverted.\n\n");
        ImGui::Separator();

        if (ImGui::Button("Yes", ImVec2(120, 0)))
        {
            config_load_defaults();
            gui_set_style();
            ImGui::CloseCurrentPopup();
            gui_dialog_in_use = false;
        }

        ImGui::SameLine();

        if (ImGui::Button("No", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
            gui_dialog_in_use = false;
        }

        ImGui::SetItemDefaultFocus();
        ImGui::EndPopup();
    }
}

void gui_show_info(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::Begin("ROM Info", &config_emulator.show_info, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize);

    static char info[512] = "";
    emu_get_info(info, 512);

    ImGui::PushFont(gui_default_font);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f,0.502f,0.957f,1.0f));
    ImGui::SetCursorPosX(5.0f);
    ImGui::Text("%s", info);
    ImGui::PopStyleColor();
    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}

void gui_show_fps(void)
{
    ImGui::PushFont(gui_default_font);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f,1.00f,0.0f,1.0f));
    ImGui::SetCursorPos(ImVec2(5.0f, config_debug.debug ? 25.0f : 5.0f));
    ImGui::Text("FPS:  %.2f\nTIME: %.2f ms", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
    ImGui::PopStyleColor();
    ImGui::PopFont();
}

static void add_build_info(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int written = vsnprintf(build_info + info_pos, sizeof(build_info) - info_pos, fmt, args);
    if (written > 0 && info_pos + written < (int)sizeof(build_info)) {
        info_pos += written;
    } else {
        info_pos = (int)sizeof(build_info) - 1;
        build_info[info_pos] = '\0';
    }
    va_end(args);
}

static void check_hotkey_duplicates_popup(config_Hotkey* current_hotkey)
{
    if (current_hotkey->key == SDL_SCANCODE_UNKNOWN)
        return;

    for (int i = 0; i < config_HotkeyIndex_COUNT; i++)
    {
        config_Hotkey* other = &config_hotkeys[i];

        if (other == current_hotkey)
            continue;

        if (other->key == current_hotkey->key &&
            other->mod == current_hotkey->mod)
        {
            other->key = SDL_SCANCODE_UNKNOWN;
            other->mod = SDL_KMOD_NONE;
            config_update_hotkey_string(other);
        }
    }
}