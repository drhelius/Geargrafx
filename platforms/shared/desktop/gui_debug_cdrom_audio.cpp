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
    ImGui::SetNextWindowSize(ImVec2(222, 196), ImGuiCond_FirstUseEver);
    ImGui::Begin("CD-ROM Audio", &config_debug.show_cdrom_audio);

    ImGui::PushFont(gui_default_font);

    GeargrafxCore* core = emu_get_core();
    CdRomAudio* cdrom_audio = core->GetCDROMAudio();
    CdRomAudio::CdRomAudio_State* cdrom_audio_state = cdrom_audio->GetState();

    ImGui::BeginTable("##table", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadOuterX);

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

    if (ImPlot::BeginPlot("Left Channel", ImVec2(80, 50), ImPlotFlags_CanvasOnly))
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

    if (ImPlot::BeginPlot("Right Channel", ImVec2(80, 50), ImPlotFlags_CanvasOnly))
    {
        ImPlot::SetupAxes("x", "y", flags, flags);
        ImPlot::SetupAxesLimits(x_min_right, x_max_right, -1.0f, 1.0f, ImPlotCond_Always);
        ImPlot::SetNextLineStyle(white, 1.0f);
        ImPlot::PlotLine("R", wave_buffer_right, data_size);
        ImPlot::EndPlot();
    }

    ImPlot::PopStyleVar();

    ImGui::EndTable();

    ImGui::NewLine();

    const char* k_cdrom_state_names[] = { "PLAYING", "IDLE   ", "PAUSED ", "STOPPED" };
    ImGui::TextColored(violet, "STATE"); ImGui::SameLine();
    ImGui::TextColored(blue, "%s", k_cdrom_state_names[*cdrom_audio_state->CURRENT_STATE]);

    ImGui::SameLine();
    const char* k_stop_event_names[] = { "STOP", "LOOP", "IRQ " };
    ImGui::TextColored(violet, " END EVENT"); ImGui::SameLine();
    ImGui::TextColored(blue, "%s", k_stop_event_names[*cdrom_audio_state->STOP_EVENT]);

    ImGui::TextColored(violet, "START LBA   "); ImGui::SameLine();
    ImGui::TextColored(white, "%d", *cdrom_audio_state->START_LBA);

    ImGui::TextColored(violet, "END LBA     "); ImGui::SameLine();
    ImGui::TextColored(white, "%d", *cdrom_audio_state->STOP_LBA);

    ImGui::TextColored(violet, "CURRENT LBA "); ImGui::SameLine();
    ImGui::TextColored(white, "%d", *cdrom_audio_state->CURRENT_LBA);

    ImGui::TextColored(violet, "SEEK CYCLES "); ImGui::SameLine();
    ImGui::TextColored((*cdrom_audio_state->SEEK_CYCLES <= 0) ? gray : white, "%d", *cdrom_audio_state->SEEK_CYCLES);

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}