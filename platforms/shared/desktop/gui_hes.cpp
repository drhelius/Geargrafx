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

#define GUI_HES_IMPORT
#include "gui_hes.h"

#include "application.h"
#include "imgui.h"
#include "implot.h"
#include "geargrafx.h"
#include "gui.h"
#include "gui_debug_constants.h"
#include "config.h"
#include "emu.h"
#include "utils.h"

static float* wave_buffer_left[6];
static float* wave_buffer_right[6];

void gui_hes_init(void)
{
    for (int ch = 0; ch < 6; ch++)
    {
        wave_buffer_left[ch] = new float[GG_AUDIO_BUFFER_SIZE];
        wave_buffer_right[ch] = new float[GG_AUDIO_BUFFER_SIZE];
    }
}

void gui_hes_destroy(void)
{
    for (int ch = 0; ch < 6; ch++)
    {
        SafeDeleteArray(wave_buffer_left[ch]);
        SafeDeleteArray(wave_buffer_right[ch]);
    }
}

void gui_draw_hes_visualization(void)
{
    bool is_debug = config_debug.debug;
    ImVec2 window_pos, window_size;
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar;
    float menu_height = application_show_menu ? gui_main_menu_height : 0.0f;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);


    if (is_debug)
    {
        flags |= ImGuiWindowFlags_AlwaysAutoResize;

        window_pos = ImVec2(631, 26);
        window_size = ImVec2(300, 300);

        ImGui::SetNextWindowPos(window_pos, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(window_size, ImGuiCond_FirstUseEver);

        flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar;

        ImGui::Begin("HES Visualization##hes_debug", &config_debug.show_screen, flags);
        gui_main_window_hovered = ImGui::IsWindowHovered();

        // window_pos = ImVec2(631, 26 + menu_height);
        // window_size = ImVec2(444, 400 - menu_height);
        // ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
        // ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
        // ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        // ImGui::SetNextWindowPos(window_pos, ImGuiCond_FirstUseEver);
        // ImGui::SetNextWindowSize(window_size, ImGuiCond_FirstUseEver);
    }
    else
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        ImVec2 viewport_pos = ImGui::GetMainViewport()->Pos;
        ImVec2 viewport_size = ImGui::GetMainViewport()->Size;
        window_pos = viewport_pos + ImVec2(0.0f, menu_height);
        window_size = ImVec2(viewport_size.x, viewport_size.y - menu_height);

        ImGui::SetNextWindowPos(window_pos);
        ImGui::SetNextWindowSize(window_size);

        flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::Begin("HES Visualization", 0, flags);
        gui_main_window_hovered = ImGui::IsWindowHovered();
    }

    GeargrafxCore* core = emu_get_core();
    HuC6280PSG* psg = core->GetAudio()->GetPSG();
    HuC6280PSG::HuC6280PSG_State* psg_state = psg->GetState();

    ImVec4 channel_colors[6] = { green, yellow, cyan, violet, magenta, red };

    int data_size = (*psg_state->FRAME_SAMPLES) / 2;
    int half_window_size = 100;
    float title_bar_height = (is_debug ? 24.0f : 0.0f);
    float available_height = window_size.y - title_bar_height;
    float plot_height = available_height / 6;
    float plot_width = window_size.x;
    float line_thickness = is_debug ? 1.0f : 3.0f;
    float plot_size_height = plot_height;
    float plot_size_width = plot_height * 3.0f;
    ImVec2 plot_size = ImVec2(plot_size_width, plot_size_height);
    float x_center = (plot_width - 2 * plot_size_width) / 2.0f;
    ImPlotFlags plot_flags = ImPlotFlags_CanvasOnly | ImPlotFlags_NoFrame;
    ImPlotAxisFlags axes_flags = ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoHighlight | ImPlotAxisFlags_Lock | ImPlotAxisFlags_NoTickMarks;

    for (int ch = 0; ch < 6; ch++)
    {
        float y_offset = title_bar_height + ch * plot_height;
        ImGui::SetCursorPos(ImVec2(x_center, y_offset));
        HuC6280PSG::HuC6280PSG_Channel* psg_channel = &psg_state->CHANNELS[ch];

        for (int i = 0; i < data_size; i++)
        {
            wave_buffer_left[ch][i] = (float)(psg_channel->output[i * 2]) / 32768.0f * 8.0f;
            wave_buffer_right[ch][i] = (float)(psg_channel->output[(i * 2) + 1]) / 32768.0f * 8.0f;
        }

        int trigger_left = 0;
        int trigger_right = 0;

        for (int i = 100; i < data_size; ++i)
        {
            if (wave_buffer_left[ch][i - 1] < 0.0f && wave_buffer_left[ch][i] >= 0.0f)
            {
                trigger_left = i;
                break;
            }
        }

        for (int i = 100; i < data_size; ++i)
        {
            if (wave_buffer_right[ch][i - 1] < 0.0f && wave_buffer_right[ch][i] >= 0.0f)
            {
                trigger_right = i;
                break;
            }
        }

        int x_min_left = MAX(0, trigger_left - half_window_size);
        int x_max_left = MIN(data_size, trigger_left + half_window_size);
        int x_min_right = MAX(0, trigger_right - half_window_size);
        int x_max_right = MIN(data_size, trigger_right + half_window_size);

        if (ImPlot::BeginPlot((std::string("L") + std::to_string(ch)).c_str(), plot_size, plot_flags))
        {
            ImPlot::SetupAxes("x", "y", axes_flags, axes_flags);
            ImPlot::SetupAxesLimits(x_min_left, x_max_left, -1.0f, 1.0f, ImPlotCond_Always);
            ImPlot::SetNextLineStyle(channel_colors[ch], line_thickness);
            ImPlot::PlotLine("WaveL", wave_buffer_left[ch], data_size);
            ImPlot::EndPlot();
        }

        ImGui::SameLine();

        if (ImPlot::BeginPlot((std::string("R") + std::to_string(ch)).c_str(), plot_size, plot_flags))
        {
            ImPlot::SetupAxes("x", "y", axes_flags, axes_flags);
            ImPlot::SetupAxesLimits(x_min_right, x_max_right, -1.0f, 1.0f, ImPlotCond_Always);
            ImPlot::SetNextLineStyle(channel_colors[ch], line_thickness);
            ImPlot::PlotLine("WaveR", wave_buffer_right[ch], data_size);
            ImPlot::EndPlot();
        }
    }

    ImGui::End();

    ImGui::PopStyleVar();
    ImGui::PopStyleVar();

    if (!is_debug)
        ImGui::PopStyleVar();
}