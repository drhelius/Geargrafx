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

#define GUI_DEBUG_ADPCM_IMPORT
#include "gui_debug_adpcm.h"

#include "imgui.h"
#include "implot.h"
#include "fonts/IconsMaterialDesign.h"
#include "geargrafx.h"
#include "gui_debug_constants.h"
#include "gui.h"
#include "config.h"
#include "emu.h"
#include "utils.h"

static float* adpcm_wave_buffer = NULL;

void gui_debug_adpcm_init(void)
{
    adpcm_wave_buffer = new float[1024];
}

void gui_debug_adpcm_destroy(void)
{
    SafeDeleteArray(adpcm_wave_buffer);
}

void gui_debug_window_adpcm(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(200, 90), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(214, 408), ImGuiCond_FirstUseEver);
    ImGui::Begin("CD-ROM ADPCM", &config_debug.show_adpcm);

    ImGui::PushFont(gui_default_font);

    GeargrafxCore* core = emu_get_core();
    Adpcm* adpcm = core->GetAdpcm();
    Adpcm::Adpcm_State* adpcm_state = adpcm->GetState();

    ImGui::BeginTable("##table", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadOuterX);

    ImGui::TableNextColumn();

    ImGui::PushStyleColor(ImGuiCol_Text, gui_audio_mute_adpcm ? mid_gray : white);
    ImGui::PushFont(gui_material_icons_font);

    char label[32];
    snprintf(label, 32, "%s##adpcmmute", gui_audio_mute_adpcm ? ICON_MD_MUSIC_OFF : ICON_MD_MUSIC_NOTE);
    if (ImGui::Button(label))
    {
        gui_audio_mute_adpcm = !gui_audio_mute_adpcm;
        emu_audio_adpcm_volume(gui_audio_mute_adpcm ? 0.0f : config_audio.adpcm_volume);
    }
    ImGui::PopStyleColor();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip("Mute ADPCM");
    ImGui::PopFont();

    ImGui::TableNextColumn();

    ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(1, 1));

    int data_size = (*adpcm_state->FRAME_SAMPLES) / 2;
    int trigger_left = 0;

    for (int i = 0; i < data_size; i++)
    {
        adpcm_wave_buffer[i] = (float)(adpcm_state->BUFFER[i * 2]) / 32768.0f * 3.0f;
    }

    for (int i = 100; i < data_size; ++i)
    {
        if (adpcm_wave_buffer[i - 1] < 0.0f && adpcm_wave_buffer[i] >= 0.0f)
        {
            trigger_left = i;
            break;
        }
    }

    int half_window_size = 100;
    int x_min_left = MAX(0, trigger_left - half_window_size);
    int x_max_left = MIN(data_size, trigger_left + half_window_size);

    ImPlotAxisFlags flags = ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoHighlight | ImPlotAxisFlags_Lock | ImPlotAxisFlags_NoTickMarks;

    if (ImPlot::BeginPlot("Left Channel", ImVec2(160, 80), ImPlotFlags_CanvasOnly))
    {
        ImPlot::SetupAxes("x", "y", flags, flags);
        ImPlot::SetupAxesLimits(x_min_left, x_max_left, -1.0f, 1.0f, ImPlotCond_Always);
        ImPlot::SetNextLineStyle(white, 1.0f);
        ImPlot::PlotLine("L", adpcm_wave_buffer, data_size);
        ImPlot::EndPlot();
    }

    ImPlot::PopStyleVar();

    ImGui::EndTable();

    ImGui::NewLine(); ImGui::TextColored(cyan, "STATE"); ImGui::Separator();

    ImGui::TextColored(violet, "PLAYING"); ImGui::SameLine();
    ImGui::TextColored(*adpcm_state->PLAYING ? green : gray, "%s", *adpcm_state->PLAYING ? "YES " : "NO");

    ImGui::TextColored(violet, "READ ADDR "); ImGui::SameLine();
    ImGui::TextColored(white, "$%04X", *adpcm_state->READ_ADDRESS);

    ImGui::TextColored(violet, "WRITE ADDR"); ImGui::SameLine();
    ImGui::TextColored(white, "$%04X", *adpcm_state->WRITE_ADDRESS);

    ImGui::TextColored(violet, "LENGTH"); ImGui::SameLine();
    ImGui::TextColored(white, "%d", *adpcm_state->LENGTH);

    ImGui::NewLine(); ImGui::TextColored(cyan, "REGISTERS"); ImGui::Separator();

    u8 status = adpcm->Read(0x0C);
    ImGui::TextColored(violet, "STATUS " ); ImGui::SameLine();
    ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", status, BYTE_TO_BINARY(status));

    ImGui::TextColored(violet, "CONTROL"); ImGui::SameLine();
    ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", *adpcm_state->CONTROL, BYTE_TO_BINARY(*adpcm_state->CONTROL));

    ImGui::TextColored(violet, "DMA    "); ImGui::SameLine();
    ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", *adpcm_state->DMA, BYTE_TO_BINARY(*adpcm_state->DMA));
    
    ImGui::TextColored(violet, "ADDR   "); ImGui::SameLine();
    ImGui::TextColored(white, "$%04X", *adpcm_state->ADDRESS);

    ImGui::NewLine(); ImGui::TextColored(cyan, "IRQs"); ImGui::Separator();

    ImGui::TextColored(violet, "END IRQ "); ImGui::SameLine();
    ImGui::TextColored(*adpcm_state->END_IRQ ? green : gray, "%s", *adpcm_state->END_IRQ ? "ON " : "OFF");

    ImGui::TextColored(violet, "HALF IRQ"); ImGui::SameLine();
    ImGui::TextColored(*adpcm_state->HALF_IRQ ? green : gray, "%s", *adpcm_state->HALF_IRQ ? "ON " : "OFF");

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}