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


#define GUI_DEBUG_LASERACTIVE_IMPORT
#include "gui_debug_laseractive.h"

#include "imgui.h"
#include "geargrafx.h"
#include "mmi_archive.h"
#include "gui_debug_constants.h"
#include "gui.h"
#include "config.h"
#include "emu.h"

static const char* const input_names[32] = {
    "Control", "Mixing mode", "Mechanical", "Playback",
    "Unused", "TOC request", "Seek mode", "Chapter",
    "Hour / frame", "Min / frame", "Sec / frame", "Frame",
    "Video control", "Audio routing", "Analog routing", "Digital gain",
    "Unused", "Unused", "Unused", "Unused",
    "Unused", "Unused", "Unused", "Unused",
    "Unknown", "Transparency", "Sprite fader", "BG fader",
    "Backdrop fader", "Blanking fader", "Analog command", "Analog gain"
};

static const char* const output_names[32] = {
    "Control", "Player status", "Disc type", "Disc side",
    "Audio status", "Buttons", "Drive status", "Playback",
    "Ready status", "Errors", "Seek mode", "Chapter",
    "Hour / frame", "Min / frame", "Sec / frame", "Frame",
    "TOC request", "TOC flags", "TOC minute", "TOC second",
    "TOC frame", "Track", "Hour / frame", "Min / frame",
    "Sec / frame", "Frame", "Stop chapter", "Stop frame",
    "Stop sec / low", "Stop min / mid", "Stop hr / high", "Stop status"
};

static void draw_value(const char* label, const char* value, const GuiDebugColor& color, bool active)
{
    ImGui::TextColored(violet, "%s", label); ImGui::SameLine();
    ImGui::TextColored(active ? color : gray, "%s", value);
}

static void draw_number(const char* label, s32 value, bool active, bool hex = false)
{
    char text[24];
    if (hex)
        snprintf(text, sizeof(text), "$%02X", (u32)value);
    else
        snprintf(text, sizeof(text), "%d", value);
    draw_value(label, text, white, active);
}

static void draw_flag(const char* label, bool value, bool active)
{
    draw_value(label, value ? "ON" : "OFF", value ? green : gray, active);
}

static void draw_registers(const LaserActive::Status& state, int first, bool available)
{
    ImGui::PushID(first);
    float unit = ImGui::CalcTextSize("M").x;
    ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_NoPadOuterX;
    if (ImGui::BeginTable("Registers", 5, flags))
    {
        ImGui::TableSetupColumn("REG", ImGuiTableColumnFlags_WidthFixed, 4 * unit);
        ImGui::TableSetupColumn("INPUT", ImGuiTableColumnFlags_WidthFixed, 15 * unit);
        ImGui::TableSetupColumn("HEX", ImGuiTableColumnFlags_WidthFixed, 4 * unit);
        ImGui::TableSetupColumn("OUTPUT", ImGuiTableColumnFlags_WidthFixed, 15 * unit);
        ImGui::TableSetupColumn("HEX", ImGuiTableColumnFlags_WidthFixed, 4 * unit);
        ImGui::TableHeadersRow();
        for (int reg = first; reg < first + 16; reg++)
        {
            bool input_used = reg != 4 && (reg < 0x10 || reg >= 0x1A);
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextColored(violet, "$%02X", reg);
            ImGui::TableNextColumn();
            ImGui::TextColored(input_used ? violet : gray, "%s", input_names[reg]);
            ImGui::TableNextColumn();
            ImGui::TextColored(available && input_used ? white : gray, "$%02X", state.input_registers[reg]);
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Input $%02X / CPU $%04X\nVisible: $%02X   Applied: $%02X\n%s",
                    reg, 0x1920 + reg, state.input_registers[reg], state.live_input_registers[reg],
                    input_used ? "Read-only snapshot; frozen input may contain pending writes." :
                    "Retained register; no implemented hardware effect.");
            }
            ImGui::TableNextColumn();
            ImGui::TextColored(violet, "%s", output_names[reg]);
            ImGui::TableNextColumn();
            ImGui::TextColored(available ? white : gray, "$%02X", state.output_registers[reg]);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Output $%02X / CPU $%04X\nPassive peek; does not consume busy or cooldown reads.",
                    reg, 0x1940 + reg);
        }
        ImGui::EndTable();
    }
    ImGui::PopID();
}

static bool begin_window(const char* title, bool* open, ImVec2 position, float columns, float rows)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
    // Font-scaled, but independent of media, transport state, and register values.
    ImGui::PushFont(gui_default_font);
    float width = columns * ImGui::CalcTextSize("M").x;
    float height = rows * ImGui::GetTextLineHeightWithSpacing();
    ImGui::PopFont();
    ImGui::SetNextWindowSize(ImVec2(width, height + ImGui::GetFrameHeight()), ImGuiCond_Always);
    bool visible = ImGui::Begin(title, open, ImGuiWindowFlags_NoResize);
    ImGui::PushFont(gui_default_font);
    return visible;
}

static void end_window(void)
{
    ImGui::PopFont();
    ImGui::End();
    ImGui::PopStyleVar();
}

static bool get_status(LaserActive::Status& state)
{
    bool available = !emu_is_media_loading() && !emu_turbolink_is_core_suspended() && !emu_is_empty();
    available = available && emu_get_core()->GetMedia()->IsLaserActive();
    if (available)
        emu_get_core()->GetLaserActive()->GetStatus(state);
    return available;
}

void gui_debug_window_laseractive_general(void)
{
    if (begin_window("LaserActive General", &config_debug.show_laseractive_general, ImVec2(85, 80), 32, 19))
    {
        LaserActive::Status state = {};
        bool available = get_status(state);
        bool inserted = available && !state.ejected;
        const GG_MmiMediaInfo* medium = available ? emu_get_core()->GetCDROMMedia()->GetSelectedMmiMedia() : NULL;
        bool clv = medium && medium->format.size() >= 3 &&
            medium->format.compare(medium->format.size() - 3, 3, "CLV") == 0;
        const u8* input = state.live_input_registers;
        char text[48];
        ImGui::BeginDisabled(!available);

        static const char* const modes[] = { "INACTIVE", "SEEKING", "READING", "PLAYING", "PAUSED", "STOPPED" };
        draw_value("STATE   ", modes[CLAMP((int)state.drive_mode, 0, 5)], blue, available);
        draw_value("TRAY    ", state.ejected ? "OPEN" : "CLOSED", state.ejected ? yellow : green, available);
        draw_value("PAC SRAM", state.sram_enabled ? "UNLOCKED" : "LOCKED", state.sram_enabled ? green : red, available);

        ImGui::NewLine(); ImGui::TextColored(cyan, "TRANSPORT"); ImGui::Separator();
        draw_number("HEAD LBA    ", state.head_lba, inserted);
        draw_number("TRACK       ", state.current_track, inserted);
        draw_number("MECHANICAL  ", state.current_drive_state, available, true);
        draw_number("SEEK TICKS  ", state.seek_latency, inserted && state.drive_mode == LaserActive::DRIVE_SEEKING);
        draw_number("SEARCH PHASE", state.search_sectors, inserted && (input[3] >> 4) == 3 && (input[3] & 7) >= 6);

        ImGui::NewLine(); ImGui::TextColored(cyan, "MEDIA"); ImGui::Separator();
        draw_value("FORMAT     ", clv ? "NTSC-CLV" : "NTSC-CAV", white, medium != NULL);
        snprintf(text, sizeof(text), "%lld / %lld", medium ? (long long)medium->volume_number : 0,
            medium ? (long long)medium->side_number : 0);
        draw_value("DISC / SIDE", text, white, medium != NULL);
        ImGui::EndDisabled();
    }
    end_window();
}

void gui_debug_window_laseractive_video(void)
{
    if (begin_window("LaserActive Video", &config_debug.show_laseractive_video, ImVec2(330, 80), 34, 23))
    {
        LaserActive::Status state = {};
        bool available = get_status(state);
        bool inserted = available && !state.ejected;
        const GG_MmiMediaInfo* medium = available ? emu_get_core()->GetCDROMMedia()->GetSelectedMmiMedia() : NULL;
        bool clv = medium && medium->format.size() >= 3 &&
            medium->format.compare(medium->format.size() - 3, 3, "CLV") == 0;
        u32 width = available ? emu_get_core()->GetLaserActive()->GetVideoWidth() : 0;
        u32 height = available ? emu_get_core()->GetLaserActive()->GetVideoHeight() : 0;
        const u8* input = state.live_input_registers;
        u8 mix = input[1] >> 6;
        u8 video = input[0x0C];
        char text[48];
        ImGui::BeginDisabled(!available);

        draw_number("MIX MODE", mix, available);

        ImGui::NewLine(); ImGui::TextColored(cyan, "VIDEO CONTROL"); ImGui::Separator();
        draw_flag("OUTPUT      ", !(video & 4), inserted);
        draw_flag("HOLD        ", (video & 0x20) != 0, inserted);
        bool field_selected = mix >= 2 && (video & 0x0A);
        draw_value("FIELD       ", field_selected ? ((video & 1) ? "EVEN" : "ODD") : "AUTO", white, inserted);
        draw_flag("PICTURE STOP", !(video & 0x80), inserted && !clv);

        ImGui::NewLine(); ImGui::TextColored(cyan, "POSITION"); ImGui::Separator();
        draw_number("FRAME      ", state.video_frame, inserted && state.video_frame >= 0);
        snprintf(text, sizeof(text), "%u x %u", width, height);
        draw_value("SOURCE SIZE", text, white, inserted && width != 0);

        ImGui::NewLine(); ImGui::TextColored(cyan, "GRAPHICS FADERS"); ImGui::Separator();
        static const char* const faders[] = { "SPRITES   ", "BACKGROUND", "BACKDROP  ", "BLANKING  " };
        for (int i = 0; i < 4; i++)
        {
            snprintf(text, sizeof(text), "%02u / 63", input[0x1A + i] >> 2);
            draw_value(faders[i], text, white, inserted && mix >= 2);
        }
        ImGui::EndDisabled();
    }
    end_window();
}

void gui_debug_window_laseractive_audio(void)
{
    if (begin_window("LaserActive Audio", &config_debug.show_laseractive_audio, ImVec2(590, 80), 34, 25))
    {
        LaserActive::Status state = {};
        bool available = get_status(state);
        bool inserted = available && !state.ejected;
        const u8* input = state.live_input_registers;
        u8 mix = input[1] >> 6;
        s16 left = available ? emu_get_core()->GetCDROMAudio()->GetLeftSample() : 0;
        s16 right = available ? emu_get_core()->GetCDROMAudio()->GetRightSample() : 0;
        char text[32];
        ImGui::BeginDisabled(!available);

        draw_number("ROUTING     ", input[0x0D], available, true);
        draw_number("ANALOG ROUTE", input[0x0E], mix != 0 && inserted, true);

        ImGui::NewLine(); ImGui::TextColored(cyan, "GAIN / ATTENUATION"); ImGui::Separator();
        draw_number("DIGITAL GAIN", input[0x0F], mix != 0 && (input[0x0D] >> 6) != 3 && inserted, true);
        draw_number("ANALOG LEFT ", state.analog_attenuation_left, mix != 0 && inserted, true);
        draw_number("ANALOG RIGHT", state.analog_attenuation_right, mix != 0 && inserted, true);

        ImGui::NewLine(); ImGui::TextColored(cyan, "ANALOG MUTE / FADE"); ImGui::Separator();
        draw_flag("MUTE  ", (input[0x0E] & 0x80) != 0, inserted && mix != 0);
        draw_value("LEFT  ", state.analog_muted_left ? (state.analog_fade_samples_left ? "FADING" : "MUTED") : "OFF",
            state.analog_muted_left ? yellow : gray, available);
        draw_value("RIGHT ", state.analog_muted_right ? (state.analog_fade_samples_right ? "FADING" : "MUTED") : "OFF",
            state.analog_muted_right ? yellow : gray, available);
        draw_number("FADE L", state.analog_fade_samples_left, state.analog_fade_samples_left != 0);
        draw_number("FADE R", state.analog_fade_samples_right, state.analog_fade_samples_right != 0);

        ImGui::NewLine(); ImGui::TextColored(cyan, "OUTPUT / SAMPLES"); ImGui::Separator();
        snprintf(text, sizeof(text), "%+06d", left);
        draw_value("LEFT         ", text, white, inserted);
        snprintf(text, sizeof(text), "%+06d", right);
        draw_value("RIGHT        ", text, white, inserted);
        snprintf(text, sizeof(text), "%03u / 588", state.sample);
        draw_value("SECTOR SAMPLE", text, white, inserted);
        ImGui::EndDisabled();
    }
    end_window();
}

void gui_debug_window_laseractive_registers(void)
{
    if (begin_window("LaserActive Registers", &config_debug.show_laseractive_registers, ImVec2(85, 540), 104, 23))
    {
        LaserActive::Status state = {};
        bool available = get_status(state);
        float unit = ImGui::CalcTextSize("M").x;
        ImGui::BeginDisabled(!available);
        draw_flag("INPUT FREEZE ", state.input_frozen, available);
        ImGui::SameLine(0, 3 * unit);
        draw_flag("OUTPUT FREEZE ", state.output_frozen, available);
        ImGui::SameLine(0, 3 * unit);
        bool error = (state.output_registers[9] & 0x13) != 0;
        draw_value("ERRORS ", error ? "SET" : "CLEAR", error ? red : gray, available);

        ImGui::NewLine(); ImGui::TextColored(cyan, "PD6103A REGISTERS"); ImGui::Separator();
        ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings |
            ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_BordersInnerV;
        if (ImGui::BeginTable("Banks", 2, flags))
        {
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 50 * unit);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 50 * unit);
            ImGui::TableNextColumn(); draw_registers(state, 0, available);
            ImGui::TableNextColumn(); draw_registers(state, 16, available);
            ImGui::EndTable();
        }
        ImGui::EndDisabled();
    }
    end_window();
}
