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
#include "gui_debug_cdrom_audio.h"
#include "gui_debug_constants.h"
#include "gui.h"
#include "config.h"
#include "emu.h"

static const char* const input_register_names[32] =
{
    "Control", "Mixing mode", "Mechanical", "Playback",
    "Unused", "TOC request", "Seek mode", "Chapter",
    "Hour / frame", "Min / frame", "Sec / frame", "Frame",
    "Video control", "Audio routing", "Analog routing", "Digital gain",
    "Unused", "Unused", "Unused", "Unused",
    "Unused", "Unused", "Unused", "Unused",
    "Unknown", "Transparency", "Sprite fader", "BG fader",
    "Backdrop fader", "Blanking fader", "Analog command", "Analog gain"
};

static const char* const output_register_names[32] =
{
    "Control", "Player status", "Disc type", "Disc side",
    "Audio status", "Buttons", "Drive status", "Playback",
    "Ready status", "Errors", "Seek mode", "Chapter",
    "Hour / frame", "Min / frame", "Sec / frame", "Frame",
    "TOC request", "TOC flags", "TOC minute", "TOC second",
    "TOC frame", "Track", "Hour / frame", "Min / frame",
    "Sec / frame", "Frame", "Stop chapter", "Stop frame",
    "Stop sec / low", "Stop min / mid", "Stop hr / high", "Stop status"
};

static void draw_laseractive_value(const char* label, const char* value, const GuiDebugColor& color, bool active)
{
    ImGui::TextColored(violet, "%s", label); ImGui::SameLine();
    ImGui::TextColored(active ? color : gray, "%s", value);
}

static void draw_laseractive_number(const char* label, s32 value, bool active, bool as_hex = false)
{
    char text[24];

    if (as_hex)
        snprintf(text, sizeof(text), "$%02X", (u32)value);
    else
        snprintf(text, sizeof(text), "%d", value);

    draw_laseractive_value(label, text, white, active);
}

static void draw_laseractive_flag(const char* label, bool value, bool active)
{
    draw_laseractive_value(label, value ? "ON" : "OFF", value ? green : gray, active);
}

static void draw_laseractive_registers(const LaserActive::Status& status, int first_reg, bool available)
{
    ImGui::PushID(first_reg);

    float char_width = ImGui::CalcTextSize("M").x;
    ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_NoPadOuterX;

    if (ImGui::BeginTable("Registers", 5, flags))
    {
        ImGui::TableSetupColumn("REG", ImGuiTableColumnFlags_WidthFixed, 4 * char_width);
        ImGui::TableSetupColumn("INPUT", ImGuiTableColumnFlags_WidthFixed, 15 * char_width);
        ImGui::TableSetupColumn("HEX", ImGuiTableColumnFlags_WidthFixed, 4 * char_width);
        ImGui::TableSetupColumn("OUTPUT", ImGuiTableColumnFlags_WidthFixed, 15 * char_width);
        ImGui::TableSetupColumn("HEX", ImGuiTableColumnFlags_WidthFixed, 4 * char_width);
        ImGui::TableHeadersRow();

        for (int reg = first_reg; reg < first_reg + 16; reg++)
        {
            bool input_used = reg != 4 && (reg < 0x10 || reg >= 0x1A);

            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextColored(violet, "$%02X", reg);
            ImGui::TableNextColumn();
            ImGui::TextColored(input_used ? violet : gray, "%s", input_register_names[reg]);
            ImGui::TableNextColumn();
            ImGui::TextColored(available && input_used ? white : gray, "$%02X", status.input_registers[reg]);

            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Input $%02X / CPU $%04X\nVisible: $%02X   Applied: $%02X\n%s",
                    reg, 0x1920 + reg, status.input_registers[reg], status.live_input_registers[reg],
                    input_used ? "Read-only snapshot; frozen input may contain pending writes." :
                    "Retained register; no implemented hardware effect.");
            }

            ImGui::TableNextColumn();
            ImGui::TextColored(violet, "%s", output_register_names[reg]);
            ImGui::TableNextColumn();
            ImGui::TextColored(available ? white : gray, "$%02X", status.output_registers[reg]);

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Output $%02X / CPU $%04X\nPassive peek; does not consume busy or cooldown reads.",
                    reg, 0x1940 + reg);
        }

        ImGui::EndTable();
    }

    ImGui::PopID();
}

static bool begin_laseractive_window(const char* title, bool* open, ImVec2 position, float columns, float rows)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
    ImGui::PushFont(gui_default_font);

    float width = columns * ImGui::CalcTextSize("M").x;
    float height = rows * ImGui::GetTextLineHeightWithSpacing();

    ImGui::PopFont();
    ImGui::SetNextWindowSize(ImVec2(width, height + ImGui::GetFrameHeight()), ImGuiCond_FirstUseEver);

    bool visible = ImGui::Begin(title, open);

    ImGui::PushFont(gui_default_font);

    return visible;
}

static void end_laseractive_window(void)
{
    ImGui::PopFont();
    ImGui::End();
    ImGui::PopStyleVar();
}

static bool get_laseractive_status(LaserActive::Status& status)
{
    bool available = !emu_is_media_loading() && !emu_turbolink_is_core_suspended() && !emu_is_empty();

    available = available && emu_get_core()->GetMedia()->IsLaserActive();

    if (available)
        emu_get_core()->GetLaserActive()->GetStatus(status);

    return available;
}

void gui_debug_window_laseractive_general(void)
{
    if (begin_laseractive_window("LaserActive General", &config_debug.show_laseractive_general, ImVec2(85, 80), 32, 19))
    {
        LaserActive::Status status = {};
        bool available = get_laseractive_status(status);
        bool media_inserted = available && !status.ejected;
        const GG_MmiMediaInfo* media_info = available ? emu_get_core()->GetCDROMMedia()->GetSelectedMmiMedia() : NULL;
        bool clv = media_info && media_info->format.size() >= 3 &&
            media_info->format.compare(media_info->format.size() - 3, 3, "CLV") == 0;
        const u8* input = status.live_input_registers;
        char text[48];

        ImGui::BeginDisabled(!available);

        static const char* const drive_modes[] = { "INACTIVE", "SEEKING", "READING", "PLAYING", "PAUSED", "STOPPED" };

        draw_laseractive_value("STATE   ", drive_modes[CLAMP((int)status.drive_mode, 0, 5)], blue, available);
        draw_laseractive_value("TRAY    ", status.ejected ? "OPEN" : "CLOSED",
            status.ejected ? yellow : green, available);
        draw_laseractive_value("PAC SRAM", status.sram_enabled ? "UNLOCKED" : "LOCKED",
            status.sram_enabled ? green : red, available);

        ImGui::NewLine(); ImGui::TextColored(cyan, "TRANSPORT"); ImGui::Separator();
        draw_laseractive_number("HEAD LBA    ", status.head_lba, media_inserted);
        draw_laseractive_number("TRACK       ", status.current_track, media_inserted);
        draw_laseractive_number("MECHANICAL  ", status.current_drive_state, available, true);
        draw_laseractive_number("SEEK TICKS  ", status.seek_latency,
            media_inserted && status.drive_mode == LaserActive::DRIVE_SEEKING);
        draw_laseractive_number("SEARCH PHASE", status.search_sectors,
            media_inserted && (input[3] >> 4) == 3 && (input[3] & 7) >= 6);

        ImGui::NewLine(); ImGui::TextColored(cyan, "MEDIA"); ImGui::Separator();
        draw_laseractive_value("FORMAT     ", clv ? "NTSC-CLV" : "NTSC-CAV", white, media_info != NULL);
        snprintf(text, sizeof(text), "%lld / %lld", media_info ? (long long)media_info->volume_number : 0,
            media_info ? (long long)media_info->side_number : 0);
        draw_laseractive_value("DISC / SIDE", text, white, media_info != NULL);
        ImGui::EndDisabled();
    }

    end_laseractive_window();
}

void gui_debug_window_laseractive_video(void)
{
    if (begin_laseractive_window("LaserActive Video", &config_debug.show_laseractive_video, ImVec2(330, 80), 34, 23))
    {
        LaserActive::Status status = {};
        bool available = get_laseractive_status(status);
        bool media_inserted = available && !status.ejected;
        const GG_MmiMediaInfo* media_info = available ? emu_get_core()->GetCDROMMedia()->GetSelectedMmiMedia() : NULL;
        bool clv = media_info && media_info->format.size() >= 3 &&
            media_info->format.compare(media_info->format.size() - 3, 3, "CLV") == 0;
        u32 width = available ? emu_get_core()->GetLaserActive()->GetVideoWidth() : 0;
        u32 height = available ? emu_get_core()->GetLaserActive()->GetVideoHeight() : 0;
        const u8* input = status.live_input_registers;
        u8 mixing_mode = input[1] >> 6;
        u8 video_control = input[0x0C];
        char text[48];

        ImGui::BeginDisabled(!available);

        draw_laseractive_number("MIX MODE", mixing_mode, available);

        ImGui::NewLine(); ImGui::TextColored(cyan, "VIDEO CONTROL"); ImGui::Separator();
        draw_laseractive_flag("OUTPUT      ", !(video_control & 4), media_inserted);
        draw_laseractive_flag("HOLD        ", (video_control & 0x20) != 0, media_inserted);

        bool field_selected = mixing_mode >= 2 && (video_control & 0x0A);

        draw_laseractive_value("FIELD       ", field_selected ? ((video_control & 1) ? "EVEN" : "ODD") : "AUTO",
            white, media_inserted);
        draw_laseractive_flag("PICTURE STOP", !(video_control & 0x80), media_inserted && !clv);

        ImGui::NewLine(); ImGui::TextColored(cyan, "POSITION"); ImGui::Separator();
        draw_laseractive_number("FRAME      ", status.video_frame, media_inserted && status.video_frame >= 0);
        snprintf(text, sizeof(text), "%u x %u", width, height);
        draw_laseractive_value("SOURCE SIZE", text, white, media_inserted && width != 0);

        ImGui::NewLine(); ImGui::TextColored(cyan, "GRAPHICS FADERS"); ImGui::Separator();

        static const char* const fader_names[] = { "SPRITES   ", "BACKGROUND", "BACKDROP  ", "BLANKING  " };

        for (int i = 0; i < 4; i++)
        {
            snprintf(text, sizeof(text), "%02u / 63", input[0x1A + i] >> 2);
            draw_laseractive_value(fader_names[i], text, white, media_inserted && mixing_mode >= 2);
        }

        ImGui::EndDisabled();
    }

    end_laseractive_window();
}

void gui_debug_window_laseractive_audio(void)
{
    if (begin_laseractive_window("LaserActive Audio", &config_debug.show_laseractive_audio, ImVec2(590, 80), 76, 33))
    {
        LaserActive::Status status = {};
        bool available = get_laseractive_status(status);
        bool media_inserted = available && !status.ejected;
        CdRom* cdrom = available ? emu_get_core()->GetCDROM() : NULL;
        CdRomAudio* audio = available ? emu_get_core()->GetCDROMAudio() : NULL;
        CdRomAudio::CdRomAudio_State* audio_state = audio ? audio->GetState() : NULL;
        const u8* input = status.live_input_registers;
        u8 mixing_mode = input[1] >> 6;
        u8 audio_selection = input[0x0D] >> 6;
        bool digital_enabled = mixing_mode == 0 ||
            (audio_selection != 2 && (mixing_mode != 1 || !(input[0x0D] & 0x10)));
        bool analog_enabled = mixing_mode != 0 && (mixing_mode != 1 || (input[0x0D] & 0x10));
        bool gain_bypassed = mixing_mode == 0 || audio_selection == 3;
        char text[48];

        ImGui::BeginDisabled(!available);

        gui_debug_cdrom_audio_output(audio, "Mute LaserActive Audio (analog + digital)");

        ImGui::NewLine();

        float char_width = ImGui::CalcTextSize("M").x;
        ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings |
            ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_BordersInnerV;

        if (ImGui::BeginTable("Audio", 2, flags))
        {
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 36 * char_width);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 36 * char_width);
            ImGui::TableNextColumn();

            ImGui::TextColored(cyan, "PLAYBACK"); ImGui::Separator();

            static const char* const drive_modes[] =
            {
                "INACTIVE", "SEEKING", "READING", "PLAYING", "PAUSED", "STOPPED"
            };

            draw_laseractive_value("STATE        ", drive_modes[CLAMP((int)status.drive_mode, 0, 5)], blue, available);
            snprintf(text, sizeof(text), "%u%s", status.playback_mode, status.playback_mode == 2 ? " (SILENT)" : "");
            draw_laseractive_value("PLAY MODE    ", text, white, media_inserted);
            draw_laseractive_number("HEAD LBA     ", status.head_lba, media_inserted);
            draw_laseractive_number("TRACK        ", status.current_track, media_inserted);
            snprintf(text, sizeof(text), "%u ticks / %.1f ms", status.seek_latency,
                status.seek_latency * 1000.0 / 75.0);
            draw_laseractive_value("SEEK         ", text, white,
                media_inserted && status.drive_mode == LaserActive::DRIVE_SEEKING);

            bool end_set = status.audio_end_lba != 0x00FFFFFF;

            if (end_set)
                draw_laseractive_number("END LBA      ", status.audio_end_lba, media_inserted);
            else
                draw_laseractive_value("END LBA      ", "NONE", gray, media_inserted);

            static const char* const stop_events[] = { "STOP", "LOOP", "IRQ" };
            int stop_event = audio_state ? CLAMP((int)*audio_state->STOP_EVENT, 0, 2) : 0;

            draw_laseractive_value("END EVENT    ", stop_events[stop_event], blue, media_inserted && end_set);
            draw_laseractive_flag("END PENDING  ", status.audio_end_pending, media_inserted);
            draw_laseractive_value("CDDA BUFFER  ", status.digital_sector_valid ? "VALID" : "EMPTY",
                status.digital_sector_valid ? green : gray, media_inserted);

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Digital audio sector buffer; data tracks and failed reads provide silence.");

            snprintf(text, sizeof(text), "%03u / 588", status.sample);
            draw_laseractive_value("SECTOR SAMPLE", text, white, media_inserted);

            ImGui::NewLine(); ImGui::TextColored(cyan, "ROUTING"); ImGui::Separator();
            draw_laseractive_number("MIX MODE     ", mixing_mode, available);
            draw_laseractive_flag("DIGITAL PATH ", digital_enabled, media_inserted);

            static const char* const digital_routes[] = { "STEREO", "L -> L+R", "R -> L+R", "HALF STEREO" };
            u8 digital_route = input[0x0D] & 3;

            if (mixing_mode == 0 && digital_route == 3)
                digital_route = 1;

            snprintf(text, sizeof(text), "$%02X / %s", input[0x0D], digital_routes[digital_route]);
            draw_laseractive_value("DIGITAL ROUTE", text, white, media_inserted && digital_enabled);
            draw_laseractive_flag("ANALOG PATH  ", analog_enabled, media_inserted);

            static const char* const analog_routes[] = { "STEREO", "L -> L+R", "R -> L+R", "MUTED" };

            snprintf(text, sizeof(text), "$%02X / %s", input[0x0E], analog_routes[input[0x0E] & 3]);
            draw_laseractive_value("ANALOG ROUTE ", text, white, media_inserted && analog_enabled);
            draw_laseractive_flag("INPUT FREEZE ", status.input_frozen, available);

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip(
                    "Routing and gains show applied state. Pending input writes are visible in LaserActive Registers.");

            ImGui::TableNextColumn();

            ImGui::TextColored(cyan, "GAIN / ATTENUATION"); ImGui::Separator();
            snprintf(text, sizeof(text), "$%02X / %.1f%%", input[0x0F],
                gain_bypassed ? 100.0 : input[0x0F] * 100.0 / 255.0);
            draw_laseractive_value("DIGITAL GAIN ", text, white, media_inserted && digital_enabled);
            draw_laseractive_value("GAIN MODE    ", gain_bypassed ? "BYPASSED" : "APPLIED",
                blue, media_inserted && digital_enabled);

            bool fader_enabled = cdrom && cdrom->IsFaderEnabled(false);

            if (fader_enabled)
            {
                snprintf(text, sizeof(text), "%.1f%% / %s", cdrom->GetFaderValue() * 100.0,
                    (*cdrom->GetState()->FADER & 4) ? "FAST" : "SLOW");
                draw_laseractive_value("CD FADER     ", text, yellow, media_inserted && digital_enabled);
            }
            else
                draw_laseractive_value("CD FADER     ", "OFF", gray, media_inserted);

            snprintf(text, sizeof(text), "$%02X / %.1f%%", status.analog_attenuation_left,
                (256 - status.analog_attenuation_left) * 100.0 / 256.0);
            draw_laseractive_value("ANALOG LEFT  ", text, white, media_inserted && analog_enabled);

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Latched attenuation / remaining gain before mute and fade.");

            snprintf(text, sizeof(text), "$%02X / %.1f%%", status.analog_attenuation_right,
                (256 - status.analog_attenuation_right) * 100.0 / 256.0);
            draw_laseractive_value("ANALOG RIGHT ", text, white, media_inserted && analog_enabled);

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Latched attenuation / remaining gain before mute and fade.");

            draw_laseractive_number("COMMAND $1E  ", input[0x1E], available, true);
            draw_laseractive_number("VALUE   $1F  ", input[0x1F], available, true);

            ImGui::NewLine(); ImGui::TextColored(cyan, "ANALOG MUTE / FADE"); ImGui::Separator();
            draw_laseractive_flag("MUTE         ", (input[0x0E] & 0x80) != 0, media_inserted && analog_enabled);
            draw_laseractive_value("LEFT         ",
                status.analog_muted_left ? (status.analog_fade_samples_left ? "FADING" : "MUTED") : "OFF",
                status.analog_muted_left ? yellow : gray, available);
            draw_laseractive_value("RIGHT        ",
                status.analog_muted_right ? (status.analog_fade_samples_right ? "FADING" : "MUTED") : "OFF",
                status.analog_muted_right ? yellow : gray, available);
            snprintf(text, sizeof(text), "%u / %.1f ms", status.analog_fade_samples_left,
                status.analog_fade_samples_left * 1000.0 / GG_AUDIO_SAMPLE_RATE);
            draw_laseractive_value("FADE LEFT    ", text, white,
                media_inserted && status.analog_fade_samples_left != 0);
            snprintf(text, sizeof(text), "%u / %.1f ms", status.analog_fade_samples_right,
                status.analog_fade_samples_right * 1000.0 / GG_AUDIO_SAMPLE_RATE);
            draw_laseractive_value("FADE RIGHT   ", text, white,
                media_inserted && status.analog_fade_samples_right != 0);

            ImGui::NewLine(); ImGui::TextColored(cyan, "OUTPUT / SAMPLES"); ImGui::Separator();
            draw_laseractive_number("FRAME SAMPLES", audio_state ? *audio_state->FRAME_SAMPLES / 2 : 0, media_inserted);
            snprintf(text, sizeof(text), "%+06d", audio ? audio->GetLeftSample() : 0);
            draw_laseractive_value("LEFT         ", text, white, media_inserted);
            snprintf(text, sizeof(text), "%+06d", audio ? audio->GetRightSample() : 0);
            draw_laseractive_value("RIGHT        ", text, white, media_inserted);
            ImGui::EndTable();
        }

        ImGui::EndDisabled();
    }

    end_laseractive_window();
}

void gui_debug_window_laseractive_registers(void)
{
    if (begin_laseractive_window("LaserActive Registers", &config_debug.show_laseractive_registers,
        ImVec2(85, 540), 104, 23))
    {
        LaserActive::Status status = {};
        bool available = get_laseractive_status(status);
        float char_width = ImGui::CalcTextSize("M").x;

        ImGui::BeginDisabled(!available);
        draw_laseractive_flag("INPUT FREEZE ", status.input_frozen, available);
        ImGui::SameLine(0, 3 * char_width);
        draw_laseractive_flag("OUTPUT FREEZE ", status.output_frozen, available);
        ImGui::SameLine(0, 3 * char_width);

        bool error = (status.output_registers[9] & 0x13) != 0;

        draw_laseractive_value("ERRORS ", error ? "SET" : "CLEAR", error ? red : gray, available);

        ImGui::NewLine(); ImGui::TextColored(cyan, "PD6103A REGISTERS"); ImGui::Separator();

        ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings |
            ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_BordersInnerV;

        if (ImGui::BeginTable("Banks", 2, flags))
        {
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 50 * char_width);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 50 * char_width);
            ImGui::TableNextColumn(); draw_laseractive_registers(status, 0, available);
            ImGui::TableNextColumn(); draw_laseractive_registers(status, 16, available);
            ImGui::EndTable();
        }

        ImGui::EndDisabled();
    }

    end_laseractive_window();
}
