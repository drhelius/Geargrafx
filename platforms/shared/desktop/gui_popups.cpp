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

#include <SDL.h>

#define GUI_POPUPS_IMPORT
#include "gui_popups.h"
#include "gui.h"
#include "gui_debug_constants.h"
#include "config.h"
#include "application.h"
#include "emu.h"
#include "license.h"
#include "backers.h"
#include "renderer.h"

void gui_popup_modal_keyboard()
{
    if (ImGui::BeginPopupModal("Keyboard Configuration", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Press any key to assign...\n\n");
        ImGui::Separator();

        for ( int i = 0; i < ImGuiKey_NamedKey_END; ++i )
        {
            if (ImGui::IsKeyDown((ImGuiKey)i))
            {
                SDL_Scancode key = (SDL_Scancode)i;

                if ((key != SDL_SCANCODE_LCTRL) && (key != SDL_SCANCODE_RCTRL) && (key != SDL_SCANCODE_CAPSLOCK))
                {
                    *gui_configured_key = key;
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

void gui_popup_modal_gamepad(int pad)
{
    if (ImGui::BeginPopupModal("Gamepad Configuration", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Press any button in your gamepad...\n\n");
        ImGui::Separator();

        for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; i++)
        {
            if (SDL_GameControllerGetButton(application_gamepad[pad], (SDL_GameControllerButton)i))
            {
                *gui_configured_button = i;
                ImGui::CloseCurrentPopup();
                break;
            }
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

        ImGui::TextColored(orange, "  By Ignacio Sánchez (DrHelius)");
        ImGui::Text(" "); ImGui::SameLine();
        ImGui::TextLink("https://github.com/drhelius/Geargrafx");
        ImGui::Text(" "); ImGui::SameLine();
        ImGui::TextLink("https://x.com/drhelius");
        ImGui::NewLine();

        ImGui::PopFont();

        ImGui::Separator();
        ImGui::Text("%s is licensed under the GPL-3.0 License, see LICENSE for more information.", GG_TITLE);
        ImGui::Separator();
        ImGui::NewLine();

        if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None))
        {
            if (ImGui::BeginTabItem("Build Info"))
            {
                ImGui::BeginChild("build", ImVec2(0, 150), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);

                ImGui::Text("Build: %s", GG_VERSION);

                #if defined(__DATE__) && defined(__TIME__)
                ImGui::Text("Built on: %s - %s", __DATE__, __TIME__);
                #endif
                #if defined(_M_ARM64)
                ImGui::Text("Windows ARM64 build");
                #endif
                #if defined(_M_X64)
                ImGui::Text("Windows 64 bit build");
                #endif
                #if defined(_M_IX86)
                ImGui::Text("Windows 32 bit build");
                #endif
                #if defined(__linux__) && defined(__x86_64__)
                ImGui::Text("Linux 64 bit build");
                #endif
                #if defined(__linux__) && defined(__i386__)
                ImGui::Text("Linux 32 bit build");
                #endif
                #if defined(__linux__) && defined(__arm__)
                ImGui::Text("Linux ARM build");
                #endif
                #if defined(__linux__) && defined(__aarch64__)
                ImGui::Text("Linux ARM64 build");
                #endif
                #if defined(__APPLE__) && defined(__arm64__ )
                ImGui::Text("macOS build (Apple Silicon)");
                #endif
                #if defined(__APPLE__) && defined(__x86_64__)
                ImGui::Text("macOS build (Intel)");
                #endif
                #if defined(__ANDROID__)
                ImGui::Text("Android build");
                #endif
                #if defined(_MSC_FULL_VER)
                ImGui::Text("Microsoft C++ %d", _MSC_FULL_VER);
                #endif
                #if defined(_MSVC_LANG)
                ImGui::Text("MSVC %d", _MSVC_LANG);
                #endif
                #if defined(__CLR_VER)
                ImGui::Text("CLR version: %d", __CLR_VER);
                #endif
                #if defined(__MINGW32__)
                ImGui::Text("MinGW 32 bit (%d.%d)", __MINGW32_MAJOR_VERSION, __MINGW32_MINOR_VERSION);
                #endif
                #if defined(__MINGW64__)
                ImGui::Text("MinGW 64 bit (%d.%d)", __MINGW64_VERSION_MAJOR, __MINGW64_VERSION_MINOR);
                #endif
                #if defined(__GNUC__) && !defined(__llvm__) && !defined(__INTEL_COMPILER)
                ImGui::Text("GCC %d.%d.%d", (int)__GNUC__, (int)__GNUC_MINOR__, (int)__GNUC_PATCHLEVEL__);
                #endif
                #if defined(__clang_version__)
                ImGui::Text("Clang %s", __clang_version__);
                #endif
                ImGui::Text("SDL %d.%d.%d (build)", application_sdl_build_version.major, application_sdl_build_version.minor, application_sdl_build_version.patch);
                ImGui::Text("SDL %d.%d.%d (link) ", application_sdl_link_version.major, application_sdl_link_version.minor, application_sdl_link_version.patch);
                ImGui::Text("OpenGL %s", renderer_opengl_version);
                #if !defined(__APPLE__)
                ImGui::Text("GLEW %s", renderer_glew_version);
                #endif
                ImGui::Text("Dear ImGui %s (%d)", IMGUI_VERSION, IMGUI_VERSION_NUM);

                #if defined(DEBUG)
                ImGui::Text("define: DEBUG");
                #endif
                #if defined(GG_DEBUG)
                ImGui::Text("define: GG_DEBUG");
                #endif
                #if defined(GG_NO_OPTIMIZATIONS)
                ImGui::Text("define: GG_NO_OPTIMIZATIONS");
                #endif
                #if defined(__cplusplus)
                ImGui::Text("define: __cplusplus = %d", (int)__cplusplus);
                #endif
                #if defined(__STDC__)
                ImGui::Text("define: __STDC__ = %d", (int)__STDC__);
                #endif
                #if defined(__STDC_VERSION__)
                ImGui::Text("define: __STDC_VERSION__ = %d", (int)__STDC_VERSION__);
                #endif
                #if defined(GG_LITTLE_ENDIAN)
                ImGui::Text("define: GG_LITTLE_ENDIAN");
                #endif
                #if defined(GG_BIG_ENDIAN)
                ImGui::Text("define: GG_BIG_ENDIAN");
                #endif
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Special thanks to"))
            {
                ImGui::BeginChild("backers", ImVec2(0, 150), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
                ImGui::Text("%s", BACKERS_STR);
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("LICENSE"))
            {
                ImGui::BeginChild("license", ImVec2(0, 150), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
                ImGui::TextUnformatted(GPL_LICENSE_STR);
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::NewLine();
        ImGui::Separator();

        for (int i = 0; i < 2; i++)
        {
            if (application_gamepad[i])
                ImGui::Text("> Gamepad detected for Player %d", i+1);
            else
                ImGui::Text("> No gamepad detected for Player %d", i+1);
        }

        if (application_gamepad_mappings > 0)
            ImGui::Text("%d game controller mappings loaded from gamecontrollerdb.txt", application_gamepad_mappings);
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

void gui_show_info(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::Begin("ROM Info", &config_emulator.show_info, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize);

    static char info[512] = "";
    emu_get_info(info);

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