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

#define GUI_DEBUG_PSG_IMPORT
#include "gui_debug_psg.h"

#include "imgui/imgui.h"
#include "imgui/implot.h"
#include "../../../src/geargrafx.h"
#include "gui.h"
#include "gui_debug_constants.h"
#include "gui_debug_memeditor.h"
#include "config.h"
#include "emu.h"
#include "utils.h"

static MemEditor mem_edit[6];
static float plot_x[32];
static float plot_y[32];

void gui_debug_window_psg(void)
{
    for (int i = 0; i < 6; i++)
    {
        mem_edit[i].SetGuiFont(gui_roboto_font);
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(180, 45), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(438, 482), ImGuiCond_FirstUseEver);
    ImGui::Begin("PSG", &config_debug.show_psg);

    ImGui::PushFont(gui_default_font);

    GeargrafxCore* core = emu_get_core();
    HuC6280PSG* psg = core->GetAudio()->GetPSG();
    HuC6280PSG::HuC6280PSG_State* psg_state = psg->GetState();

    ImGui::Columns(2, "psg", true);

    ImGui::TextColored(cyan, "R00 "); ImGui::SameLine();
    ImGui::TextColored(violet, "CHANNEL SEL "); ImGui::SameLine();
    ImGui::TextColored(white, "%d", *psg_state->CHANNEL_SELECT);

    ImGui::TextColored(cyan, "R01 "); ImGui::SameLine();
    ImGui::TextColored(violet, "MAIN AMPL   "); ImGui::SameLine();
    ImGui::TextColored(white, "%02X", *psg_state->MAIN_AMPLITUDE);

    ImGui::NextColumn();

    ImGui::TextColored(cyan, "R08 "); ImGui::SameLine();
    ImGui::TextColored(violet, "LFO FREQ    "); ImGui::SameLine();
    ImGui::TextColored(white, "%02X", *psg_state->LFO_FREQUENCY);

    ImGui::TextColored(cyan, "R09 "); ImGui::SameLine();
    ImGui::TextColored(violet, "LFO CTRL    "); ImGui::SameLine();
    ImGui::TextColored(white, "%02X", *psg_state->LFO_CONTROL);

    ImGui::Columns(1);

    ImGui::NewLine();

    if (ImGui::BeginTabBar("##memory_tabs", ImGuiTabBarFlags_None))
    {
        for (int channel = 0; channel < 6; channel++)
        {
            char tab_name[32];
            sprintf(tab_name, "CH %d", channel);

            if (ImGui::BeginTabItem(tab_name))
            {
                HuC6280PSG::HuC6280PSG_Channel* psg_channel = &psg_state->CHANNELS[channel];

                ImGui::Columns(2, "channels", false);

                ImGui::TextColored(cyan, "DDA "); ImGui::SameLine();
                ImGui::TextColored(violet, "            "); ImGui::SameLine();
                ImGui::TextColored(white, "%d", psg_channel->output);

                ImGui::TextColored(cyan, "R02 "); ImGui::SameLine();
                ImGui::TextColored(violet, "FREQ LOW    "); ImGui::SameLine();
                ImGui::TextColored(white, "%02X", psg_channel->frequency & 0xFF);

                ImGui::TextColored(cyan, "R03 "); ImGui::SameLine();
                ImGui::TextColored(violet, "FREQ HI     "); ImGui::SameLine();
                ImGui::TextColored(white, "%02X", psg_channel->frequency >> 8);

                ImGui::TextColored(cyan, "R04 "); ImGui::SameLine();
                ImGui::TextColored(violet, "CONTROL     "); ImGui::SameLine();
                ImGui::TextColored(white, "%02X", psg_channel->control);

                ImGui::TextColored(cyan, "R05 "); ImGui::SameLine();
                ImGui::TextColored(violet, "AMPLITUDE   "); ImGui::SameLine();
                ImGui::TextColored(white, "%02X", psg_channel->amplitude);

                ImGui::TextColored(cyan, "R06 "); ImGui::SameLine();
                ImGui::TextColored(violet, "WAVE        "); ImGui::SameLine();
                ImGui::TextColored(white, "%02X", psg_channel->wave);
                ImGui::TextColored(cyan, "    "); ImGui::SameLine();
                ImGui::TextColored(violet, "WAVE INDEX  "); ImGui::SameLine();
                ImGui::TextColored(white, "%02X", psg_channel->wave_index);

                if (channel > 3)
                {
                    ImGui::TextColored(cyan, "R07 "); ImGui::SameLine();
                    ImGui::TextColored(violet, "NOISE CTRL  "); ImGui::SameLine();
                    ImGui::TextColored(white, "%02X", psg_channel->noise_control);

                    ImGui::TextColored(cyan, "    "); ImGui::SameLine();
                    ImGui::TextColored(violet, "NOISE FREQ  "); ImGui::SameLine();
                    ImGui::TextColored(white, "%04X", psg_channel->noise_frequency);

                    ImGui::TextColored(cyan, "    "); ImGui::SameLine();
                    ImGui::TextColored(violet, "NOISE SEED  "); ImGui::SameLine();
                    ImGui::TextColored(white, "%04X", psg_channel->noise_seed);
                }

                ImGui::NewLine();

                ImGui::NextColumn();


                for (int i = 0; i < 32; i++)
                {
                    plot_x[i] = i;
                    plot_y[i] = psg_channel->wave_data[i];
                }

                ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(1,1));

                if (ImPlot::BeginPlot("Line Plots", ImVec2(200, 200), ImPlotFlags_CanvasOnly))
                {
                    ImPlotAxisFlags flags = ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoHighlight | ImPlotAxisFlags_Lock | ImPlotAxisFlags_NoTickMarks;
                    ImPlot::SetupAxes("x", "y", flags, flags);
                    ImPlot::SetupAxesLimits(-1,32, -1,32);
                    ImPlot::SetupAxisTicks(ImAxis_X1, 0, 32, 33, nullptr, false);
                    ImPlot::SetupAxisTicks(ImAxis_Y1, 0, 32, 33, nullptr, false);
                    //ImPlot::SetNextMarkerStyle(0, 2.0f, IMPLOT_AUTO_COL, 1.0f);
                    ImPlot::SetNextLineStyle(orange, 3.0f);
                    ImPlot::PlotLine("f(x)", plot_x, plot_y, 32);
                    ImPlot::EndPlot();
                }

                ImPlot::PopStyleVar();

                ImGui::Columns(1);

                ImGui::BeginChild("##waveform", ImVec2(ImGui::GetWindowWidth() - 20, 60), true);

                mem_edit[channel].Draw(psg_channel->wave_data, 32, 0, 1, false, false, false, false);

                ImGui::EndChild();

                ImGui::EndTabItem();
            }
        }

        ImGui::EndTabBar();
    }

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}