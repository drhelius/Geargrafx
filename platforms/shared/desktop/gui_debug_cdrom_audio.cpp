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

#define GUI_DEBUG_CDROM_AUDIO_IMPORT
#include "gui_debug_cdrom_audio.h"

#include "imgui.h"
#include "implot.h"
#include "fonts/IconsMaterialDesign.h"
#include "geargrafx.h"
#include "gui_debug_constants.h"
#include "gui.h"
#include "config.h"
#include "emu.h"
#include "utils.h"

static float* wave_buffer_left = NULL;
static float* wave_buffer_right = NULL;

static void format_cdrom_audio_msf(u32 lba, char* buffer, size_t size);
static double cdrom_audio_cycles_to_ms(s32 cycles);

void gui_debug_cdrom_audio_init(void)
{
    wave_buffer_left = new float[GG_AUDIO_BUFFER_SIZE];
    wave_buffer_right = new float[GG_AUDIO_BUFFER_SIZE];
}

void gui_debug_cdrom_audio_destroy(void)
{
    SafeDeleteArray(wave_buffer_left);
    SafeDeleteArray(wave_buffer_right);
}

void gui_debug_window_cdrom_audio(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(120, 100), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(262, 464), ImGuiCond_FirstUseEver);
    ImGui::Begin("CD-ROM Audio", &config_debug.show_cdrom_audio);

    ImGui::PushFont(gui_default_font);

    GeargrafxCore* core = emu_get_core();
    CdRom* cdrom = core->GetCDROM();
    CdRomMedia* cdrom_media = core->GetCDROMMedia();
    CdRomAudio* cdrom_audio = core->GetCDROMAudio();
    CdRomAudio::CdRomAudio_State* cdrom_audio_state = cdrom_audio->GetState();

    if (ImGui::BeginTable("##table", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadOuterX))
    {
        ImGui::TableNextColumn();

        ImGui::PushStyleColor(ImGuiCol_Text, gui_audio_mute_cdrom ? mid_gray : white);
        ImGui::PushFont(gui_material_icons_font);

        char label[32];
        snprintf(label, 32, "%s##cdaudiomute", gui_audio_mute_cdrom ? ICON_MD_MUSIC_OFF : ICON_MD_MUSIC_NOTE);
        if (ImGui::Button(label))
        {
            gui_audio_mute_cdrom = !gui_audio_mute_cdrom;
            emu_audio_cdrom_volume(gui_audio_mute_cdrom ? 0.0f : config_audio.cdrom_volume);
        }
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("Mute CD Audio");
        ImGui::PopFont();

        ImGui::TableNextColumn();

        ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(1, 1));

        int data_size = (*cdrom_audio_state->FRAME_SAMPLES) / 2;
        int trigger_left = 0;
        int trigger_right = 0;

        for (int i = 0; i < data_size; i++)
        {
            wave_buffer_left[i] = (float)(cdrom_audio_state->BUFFER[i * 2]) / 32768.0f * 2.0f;
            wave_buffer_right[i] = (float)(cdrom_audio_state->BUFFER[(i * 2) + 1]) / 32768.0f * 2.0f;
        }

        for (int i = 100; i < data_size; ++i)
        {
            if (wave_buffer_left[i - 1] < 0.0f && wave_buffer_left[i] >= 0.0f)
            {
                trigger_left = i;
                break;
            }
        }

        for (int i = 100; i < data_size; ++i)
        {
            if (wave_buffer_right[i - 1] < 0.0f && wave_buffer_right[i] >= 0.0f)
            {
                trigger_right = i;
                break;
            }
        }

        int half_window_size = 100;
        int x_min_left = MAX(0, trigger_left - half_window_size);
        int x_max_left = MIN(data_size, trigger_left + half_window_size);

        ImPlotAxisFlags flags = ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoHighlight | ImPlotAxisFlags_Lock | ImPlotAxisFlags_NoTickMarks;

        if (ImPlot::BeginPlot("Left Channel", ImVec2(100, 50), ImPlotFlags_CanvasOnly))
        {
            ImPlot::SetupAxes("x", "y", flags, flags);
            ImPlot::SetupAxesLimits(x_min_left, x_max_left, -1.0f, 1.0f, ImPlotCond_Always);
            ImPlot::SetNextLineStyle(white, 1.0f);
            ImPlot::PlotLine("L", wave_buffer_left, data_size);
            ImPlot::EndPlot();
        }

        ImGui::SameLine();

        int x_min_right = MAX(0, trigger_right - half_window_size);
        int x_max_right = MIN(data_size, trigger_right + half_window_size);

        if (ImPlot::BeginPlot("Right Channel", ImVec2(100, 50), ImPlotFlags_CanvasOnly))
        {
            ImPlot::SetupAxes("x", "y", flags, flags);
            ImPlot::SetupAxesLimits(x_min_right, x_max_right, -1.0f, 1.0f, ImPlotCond_Always);
            ImPlot::SetNextLineStyle(white, 1.0f);
            ImPlot::PlotLine("R", wave_buffer_right, data_size);
            ImPlot::EndPlot();
        }

        ImPlot::PopStyleVar();

        ImGui::EndTable();
    }

    u32 current_lba = *cdrom_audio_state->CURRENT_LBA;
    s32 current_track = cdrom_media->FindTrackFromLBA(current_lba, true);
    const std::vector<CdRomImage::Track>& tracks = cdrom_media->GetTracks();
    const CdRomImage::Track* track = NULL;

    if ((current_track >= 0) && ((size_t)current_track < tracks.size()))
        track = &tracks[(size_t)current_track];

    bool audio_sector = IsValidPointer(track) && (track->type == GG_CDROM_AUDIO_TRACK) &&
        (current_lba >= track->start_lba);
    const char* output_state = "SILENT";
    const GuiDebugColor* output_color = &gray;

    if (*cdrom_audio_state->CURRENT_STATE == CdRomAudio::CD_AUDIO_STATE_PLAYING)
    {
        if (*cdrom_audio_state->SEEK_CYCLES > 0)
        {
            output_state = "SEEKING";
            output_color = &yellow;
        }
        else if (*cdrom_audio_state->PLAYBACK_DELAY_CYCLES > 0)
        {
            output_state = "DELAYED";
            output_color = &yellow;
        }
        else if (audio_sector)
        {
            output_state = "AUDIBLE";
            output_color = &green;
        }
        else
        {
            output_state = "MUTED";
            output_color = &orange;
        }
    }

    char start_msf[16];
    char stop_msf[16];
    char current_msf[16];
    format_cdrom_audio_msf(*cdrom_audio_state->START_LBA, start_msf, sizeof(start_msf));
    format_cdrom_audio_msf(*cdrom_audio_state->STOP_LBA, stop_msf, sizeof(stop_msf));
    format_cdrom_audio_msf(current_lba, current_msf, sizeof(current_msf));

    ImGui::NewLine(); ImGui::TextColored(cyan, "PLAYBACK"); ImGui::Separator();

    const char* k_cdrom_state_names[] = { "PLAYING", "IDLE   ", "PAUSED ", "STOPPED" };
    ImGui::TextColored(violet, "STATE     "); ImGui::SameLine();
    ImGui::TextColored(blue, "%s", k_cdrom_state_names[*cdrom_audio_state->CURRENT_STATE]);

    ImGui::TextColored(violet, "OUTPUT    "); ImGui::SameLine();
    ImGui::TextColored(*output_color, "%s", output_state);

    const char* k_stop_event_names[] = { "STOP", "LOOP", "IRQ " };
    ImGui::TextColored(violet, "END EVENT "); ImGui::SameLine();
    ImGui::TextColored(blue, "%s", k_stop_event_names[*cdrom_audio_state->STOP_EVENT]);

    ImGui::TextColored(violet, "TRACK     "); ImGui::SameLine();
    if (IsValidPointer(track))
    {
        ImGui::TextColored(orange, "%02d", current_track + 1); ImGui::SameLine();
        ImGui::TextColored((track->type == GG_CDROM_AUDIO_TRACK) ? green : yellow,
            " %s / %u", TrackTypeName(track->type), track->sector_size);
    }
    else
        ImGui::TextColored(gray, "--");

    u8 fader = *cdrom->GetState()->FADER;
    bool fader_enabled = IS_SET_BIT(fader, 3);
    bool fader_adpcm = IS_SET_BIT(fader, 1);

    ImGui::TextColored(violet, "FADER     "); ImGui::SameLine();
    if (cdrom->IsFaderEnabled(false))
    {
        double fader_gain = cdrom->GetFaderValue() * 100.0;
        ImGui::TextColored(yellow, "%.1f%% / %s", fader_gain,
            IS_SET_BIT(fader, 2) ? "FAST" : "SLOW");
    }
    else if (fader_enabled && fader_adpcm)
        ImGui::TextColored(gray, "ADPCM");
    else
        ImGui::TextColored(gray, "OFF");

    ImGui::NewLine(); ImGui::TextColored(cyan, "POSITION"); ImGui::Separator();

    ImGui::TextColored(violet, "START     "); ImGui::SameLine();
    ImGui::TextColored(white, "LBA %u  %s", *cdrom_audio_state->START_LBA, start_msf);

    ImGui::TextColored(violet, "STOP      "); ImGui::SameLine();
    ImGui::TextColored(white, "LBA %u  %s", *cdrom_audio_state->STOP_LBA, stop_msf);

    ImGui::TextColored(violet, "CURRENT   "); ImGui::SameLine();
    ImGui::TextColored(white, "LBA %u  %s", current_lba, current_msf);

    ImGui::TextColored(violet, "TRACK POS "); ImGui::SameLine();
    if (IsValidPointer(track))
    {
        char track_position_msf[16];
        char track_length_msf[16];

        if (current_lba >= track->start_lba)
        {
            format_cdrom_audio_msf(current_lba - track->start_lba,
                track_position_msf, sizeof(track_position_msf));
            format_cdrom_audio_msf(track->sector_count, track_length_msf, sizeof(track_length_msf));
            ImGui::TextColored(white, "%s / %s", track_position_msf, track_length_msf);
        }
        else
        {
            format_cdrom_audio_msf(track->start_lba - current_lba,
                track_position_msf, sizeof(track_position_msf));
            ImGui::TextColored(yellow, "-%s (LEAD-IN)", track_position_msf);
        }
    }
    else
        ImGui::TextColored(gray, "--");

    ImGui::TextColored(violet, "SECTOR SAMPLE "); ImGui::SameLine();
    ImGui::TextColored(white, "%03u / %03u", *cdrom_audio_state->CURRENT_SAMPLE, 2352 / 4);

    ImGui::NewLine(); ImGui::TextColored(cyan, "TIMING / SAMPLES"); ImGui::Separator();

    ImGui::TextColored(violet, "SEEK  "); ImGui::SameLine();
    ImGui::TextColored((*cdrom_audio_state->SEEK_CYCLES <= 0) ? gray : white,
        "%010d cycles  %.3f ms", *cdrom_audio_state->SEEK_CYCLES,
        cdrom_audio_cycles_to_ms(*cdrom_audio_state->SEEK_CYCLES));

    ImGui::TextColored(violet, "DELAY "); ImGui::SameLine();
    ImGui::TextColored((*cdrom_audio_state->PLAYBACK_DELAY_CYCLES <= 0) ? gray : white,
        "%010d cycles  %.3f ms", *cdrom_audio_state->PLAYBACK_DELAY_CYCLES,
        cdrom_audio_cycles_to_ms(*cdrom_audio_state->PLAYBACK_DELAY_CYCLES));

    ImGui::TextColored(violet, "FRAME "); ImGui::SameLine();
    ImGui::TextColored(white, "%04d samples", *cdrom_audio_state->FRAME_SAMPLES / 2);

    ImGui::TextColored(violet, "LEFT  "); ImGui::SameLine();
    ImGui::TextColored(white, "%+06d", cdrom_audio->GetLeftSample());

    ImGui::TextColored(violet, "RIGHT "); ImGui::SameLine();
    ImGui::TextColored(white, "%+06d", cdrom_audio->GetRightSample());

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}

static void format_cdrom_audio_msf(u32 lba, char* buffer, size_t size)
{
    GG_CdRomMSF msf;
    LbaToMsf(lba, &msf);
    snprintf(buffer, size, "%02u:%02u:%02u", msf.minutes, msf.seconds, msf.frames);
}

static double cdrom_audio_cycles_to_ms(s32 cycles)
{
    if (cycles <= 0)
        return 0.0;

    return (double)cycles * 1000.0 / (double)GG_MASTER_CLOCK_RATE;
}
