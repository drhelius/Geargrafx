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
#include "keyboard.h"

static char build_info[4096] = "";
static int info_pos = 0;

static void add_build_info(const char* fmt, ...);

void gui_popup_modal_keyboard()
{
    if (ImGui::BeginPopupModal("Keyboard Configuration", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Press any key to assign...\n\n");
        ImGui::Separator();

        for (ImGuiKey i = ImGuiKey_NamedKey_BEGIN; i < ImGuiKey_NamedKey_END; i = (ImGuiKey)(i + 1))
        {
            if (ImGui::IsKeyDown(i))
            {
                SDL_Keycode key_code = ImGuiKeyToSDLKeycode(i);
                SDL_Scancode key = SDL_GetScancodeFromKey(key_code);

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

        for (int a = SDL_CONTROLLER_AXIS_LEFTX; a < SDL_CONTROLLER_AXIS_MAX; a++)
        {
            if (a != SDL_CONTROLLER_AXIS_TRIGGERLEFT && a != SDL_CONTROLLER_AXIS_TRIGGERRIGHT)
                continue;

            Sint16 value = SDL_GameControllerGetAxis(application_gamepad[pad], (SDL_GameControllerAxis)a);

            if (value > GAMEPAD_VBTN_AXIS_THRESHOLD)
            {
                *gui_configured_button = GAMEPAD_VBTN_AXIS_BASE + a;
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
                add_build_info("SDL %d.%d.%d (build)\n", application_sdl_build_version.major, application_sdl_build_version.minor, application_sdl_build_version.patch);
                add_build_info("SDL %d.%d.%d (link)\n", application_sdl_link_version.major, application_sdl_link_version.minor, application_sdl_link_version.patch);
                add_build_info("OpenGL %s\n", renderer_opengl_version);
                add_build_info("Dear ImGui %s (%d)\n", IMGUI_VERSION, IMGUI_VERSION_NUM);

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

        if (application_added_gamepad_mappings || application_updated_gamepad_mappings)
        {
            ImGui::Text("%d game controller mappings added from gamecontrollerdb.txt", application_added_gamepad_mappings);
            ImGui::Text("%d game controller mappings updated from gamecontrollerdb.txt", application_updated_gamepad_mappings);
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