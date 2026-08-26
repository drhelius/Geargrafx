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

#define GUI_DEBUG_TURBOLINK_IMPORT
#include "gui_debug_turbolink.h"

#include "imgui.h"
#include "geargrafx.h"
#include "gui.h"
#include "gui_debug_constants.h"
#include "gui_debug_widgets.h"
#include "config.h"
#include "emu.h"
#include "utils.h"

static void turbolink_write_callback(u16 address, u8 value, void* user_data);
static void draw_byte_value(const char* label, u8 value);
static void draw_metric(const char* label, u64 value);
static void draw_metric_pair(const char* label, u64 first, u64 second);
static const char* get_driver_state(u8 pull_low_mask, u8 line);

void gui_debug_window_turbolink(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(106, 120), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(246, 440), ImGuiCond_FirstUseEver);
    ImGui::Begin("TurboLink Hardware", &config_debug.show_turbolink);

    bool hardware_available = !emu_is_media_loading() && !emu_turbolink_is_core_suspended();
    GeargrafxCore* core = NULL;
    Input* input = NULL;
    GG_TurboLink_Drive drive = { 0, 0 };
    u8 input_base = 0;
    u8 o = 0;
    bool sel = false;
    bool clr = false;
    bool endpoint_active = false;
    bool sample_valid = false;
    u8 lines = GG_TURBOLINK_LINE_MASK;
    u8 port_result = 0;
    u8 sample_pull_low_mask = 0;
    bool sample_sel = false;
    bool sample_clr = false;
    u64 sample_tick = 0;
    u64 cycle = 0;
    u64 control_tick = 0;
    u64 drive_tick = 0;

    if (hardware_available)
    {
        core = emu_get_core();
        input = core->GetInput();
        drive = input->GetTurboLinkDrive();
        input_base = input->GetIORegister();
        sel = input->GetSel();
        clr = input->GetClr();
        o = (sel ? 0x01 : 0x00) | (clr ? 0x02 : 0x00);
        endpoint_active = input->IsTurboLinkCableConnected();
        sample_valid = input->HasTurboLinkSample();
        lines = input->GetTurboLinkLastSampledLines();
        port_result = input->GetTurboLinkLastPortResult();
        sample_pull_low_mask = input->GetTurboLinkLastSamplePullLowMask();
        sample_sel = input->GetTurboLinkLastSampleSel();
        sample_clr = input->GetTurboLinkLastSampleClr();
        sample_tick = input->GetTurboLinkLastSampleTick();
        cycle = core->GetTurboLinkCycle();
        control_tick = input->GetTurboLinkLastControlTick();
        drive_tick = input->GetTurboLinkLastDriveTick();
    }

    bool mux_active = endpoint_active && clr;

    ImGui::PushFont(gui_default_font);

    ImGui::TextColored(magenta, "PC ENGINE I/O PORT:");
    ImGui::TextColored(violet, " CORE ACCESS    "); ImGui::SameLine();
    ImGui::TextColored(hardware_available ? green : gray, "%s", hardware_available ? "AVAILABLE" : "SUSPENDED");

    ImGui::BeginDisabled(!hardware_available);

    draw_byte_value(" K INPUT BASE   ", input_base);
    EditableRegister8("O      ", " $1000", 0x1000, o, turbolink_write_callback, input);

    ImGui::TextColored(violet, " SEL            "); ImGui::SameLine();
    ImGui::TextColored(sel ? green : gray, "%u", sel ? 1 : 0);
    ImGui::TextColored(violet, " CLR            "); ImGui::SameLine();
    ImGui::TextColored(clr ? green : gray, "%u", clr ? 1 : 0);
    ImGui::TextColored(violet, " INPUT MUX      "); ImGui::SameLine();
    ImGui::TextColored(mux_active ? green : gray, "%s", mux_active ? "LINK D0/D1" : "CONTROLLER");
    ImGui::TextColored(violet, " ENDPOINT       "); ImGui::SameLine();
    ImGui::TextColored(endpoint_active ? green : gray, "%s", endpoint_active ? "ACTIVE" : "INACTIVE");
    ImGui::Separator();
    ImGui::TextColored(magenta, "BU5782K LINK I/O:");

    ImGui::TextColored(violet, " LINK1 DRIVER   "); ImGui::SameLine();
    ImGui::TextColored((drive.drive_mask & GG_TURBOLINK_LINE_1) ? yellow : gray, "%s", get_driver_state(drive.drive_mask, GG_TURBOLINK_LINE_1));
    ImGui::TextColored(violet, " LINK2 DRIVER   "); ImGui::SameLine();
    ImGui::TextColored((drive.drive_mask & GG_TURBOLINK_LINE_2) ? yellow : gray, "%s", get_driver_state(drive.drive_mask, GG_TURBOLINK_LINE_2));
    ImGui::TextColored(violet, " PULL-LOW MASK  "); ImGui::SameLine();
    ImGui::TextColored(white, "$%02X", drive.drive_mask);

    ImGui::Separator();
    ImGui::TextColored(magenta, "LAST LINK READ:");

    ImGui::BeginDisabled(!sample_valid);
    ImGui::TextColored(violet, " LINK1 / LINK2  "); ImGui::SameLine();
    ImGui::TextColored(white, "%s / %s",
        (lines & GG_TURBOLINK_LINE_1) ? "HIGH" : "LOW",
        (lines & GG_TURBOLINK_LINE_2) ? "HIGH" : "LOW");
    ImGui::TextColored(violet, " PHYSICAL MASK  "); ImGui::SameLine();
    ImGui::TextColored(white, "$%02X", lines);
    ImGui::TextColored(violet, " SEL/CLR/PULL   "); ImGui::SameLine();
    ImGui::TextColored(white, "%u / %u / $%02X", sample_sel ? 1 : 0, sample_clr ? 1 : 0, sample_pull_low_mask);
    ImGui::TextColored(violet, " D0/D1/D2/D3    "); ImGui::SameLine();
    ImGui::TextColored(white, "%u / %u / %u / %u",
        port_result & 0x01, (port_result >> 1) & 0x01,
        (port_result >> 2) & 0x01, (port_result >> 3) & 0x01);
    draw_byte_value(" K RESULT       ", port_result);
    draw_metric(" SAMPLE TICK    ", sample_tick);
    ImGui::EndDisabled();

    ImGui::Separator();
    ImGui::TextColored(magenta, "TIMING:");

    draw_metric(" LINK CYCLE     ", cycle);
    draw_metric(" O WRITE TICK   ", control_tick);
    draw_metric(" DRIVE TICK     ", drive_tick);

    ImGui::EndDisabled();

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}

void gui_debug_window_turbolink_transport(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(90, 90), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(336, 726), ImGuiCond_FirstUseEver);
    ImGui::Begin("TurboLink Transport", &config_debug.show_turbolink_transport);

    TurboLinkStatus link = emu_turbolink_get_status();
    const char* mode = "DISABLED";

    if (link.mode == TurboLinkModeConnected)
        mode = "JOINED";
    else if (link.mode == TurboLinkModeFault)
        mode = "FAULT";

    ImGui::PushFont(gui_default_font);
    ImGui::TextColored(magenta, "SESSION:");

    ImGui::TextColored(violet, " CABLE             "); ImGui::SameLine();
    ImGui::TextColored(link.cable_connected ? green : red, "%s", link.cable_connected ? "CONNECTED" : "DISCONNECTED");
    ImGui::TextColored(violet, " LOCAL HARDWARE    "); ImGui::SameLine();
    ImGui::TextColored(link.local_hardware_ready ? green : gray, "%s", link.local_hardware_ready ? "READY" : "INACTIVE");
    ImGui::TextColored(violet, " REMOTE MEMBER     "); ImGui::SameLine();
    ImGui::TextColored(link.remote_active ? green : gray, "%s", link.remote_active ? "ACTIVE" : "ABSENT");
    ImGui::TextColored(violet, " REMOTE HARDWARE   "); ImGui::SameLine();
    ImGui::TextColored(link.remote_hardware_ready ? green : gray, "%s", link.remote_hardware_ready ? "READY" : "INACTIVE");
    ImGui::TextColored(violet, " STATUS            "); ImGui::SameLine();
    ImGui::TextColored(link.mode == TurboLinkModeFault ? red : white, "%s", mode);
    ImGui::TextColored(violet, " SESSION           "); ImGui::SameLine();
    ImGui::TextColored(white, "%u", link.session);
    ImGui::TextColored(violet, " PEER              "); ImGui::SameLine();

    if (link.mode == TurboLinkModeConnected)
        ImGui::TextColored(white, "%d / %d", link.local_peer_id,
            link.peer_count);
    else
        ImGui::TextColored(gray, "-");

    ImGui::TextColored(violet, " PACING            "); ImGui::SameLine();

    if (link.mode == TurboLinkModeConnected)
    {
        if (!link.cable_connected)
            ImGui::TextColored(white, "LOCAL AUDIO");
        else
        {
            ImGui::TextColored(link.pacing_peer ? green : cyan, "%s",
                link.pacing_peer ? "THIS PEER" : "REMOTE PEER");
        }
    }
    else
        ImGui::TextColored(gray, "-");

    ImGui::TextColored(violet, " ENDPOINT          "); ImGui::SameLine();
    ImGui::TextColored(white, "%s", link.endpoint[0] ? link.endpoint : "-");

    ImGui::Separator();
    ImGui::TextColored(magenta, "SIGNAL ACTIVITY:");

    draw_metric(" DRIVE EVENTS      ", link.events_published);
    draw_metric(" LINE SAMPLES      ", link.line_samples);
    ImGui::TextColored(violet, " LOCAL PULL-LOW    "); ImGui::SameLine();
    ImGui::TextColored(white, "$%02X", link.local_pull_low_mask);
    draw_metric(" HISTORY OVERFLOW  ", link.history_overflows);

    ImGui::BeginDisabled(!link.sample_valid);
    ImGui::TextColored(violet, " SAMPLE L/R PULL   "); ImGui::SameLine();
    ImGui::TextColored(white, "$%02X / $%02X", link.last_sample_local_pull_low_mask, link.last_sample_remote_pull_low_mask);
    ImGui::TextColored(violet, " SAMPLED LINES     "); ImGui::SameLine();
    ImGui::TextColored(white, "$%02X", link.last_sampled_lines);
    draw_metric_pair(" SAMPLE LOCAL/BUS  ", link.last_sample_local_tick, link.last_sample_bus_tick);
    draw_metric(" SAMPLE REMOTE GEN ", link.last_sample_remote_generation);
    ImGui::EndDisabled();

    ImGui::Separator();
    ImGui::TextColored(magenta, "SYNCHRONIZATION:");

    draw_metric_pair(" LOCAL / BUS TICK  ", link.local_tick, link.bus_tick);
    draw_metric_pair(" LOCAL / BUS ANCHOR", link.local_anchor_tick, link.bus_anchor_tick);
    draw_metric_pair(" LOCAL / REMOTE GEN", link.local_generation, link.remote_generation);

    ImGui::BeginDisabled(!link.remote_hardware_ready);
    draw_metric(" REMOTE COMMIT     ", link.remote_tick);
    ImGui::EndDisabled();

    ImGui::BeginDisabled(!link.cable_connected);
    ImGui::TextColored(violet, " LEAD TICKS        "); ImGui::SameLine();
    ImGui::TextColored(white, "%lld", (long long)link.lead_ticks);
    ImGui::EndDisabled();

    draw_metric(" SYNC CALLS        ", link.sync_calls);
    draw_metric(" EXACT WAITS       ", link.exact_waits);
    draw_metric_pair(" BARRIER # / us    ", link.barrier_waits, link.barrier_wait_us);
    draw_metric_pair(" MAX WAIT/GAP us   ", link.barrier_wait_max_us, link.sync_gap_max_us);

    ImGui::TextColored(violet, " WAITS 1/10/50ms   "); ImGui::SameLine();
    ImGui::TextColored(white, "%llu / %llu / %llu",
        (unsigned long long)link.barrier_wait_over_1ms,
        (unsigned long long)link.barrier_wait_over_10ms,
        (unsigned long long)link.barrier_wait_over_50ms);
    draw_metric(" GAPS OVER 50ms    ", link.sync_gap_over_50ms);
    draw_metric_pair(" SPIN / SLEEP      ", link.spin_iterations, link.sleep_calls);

    ImGui::Separator();
    ImGui::TextColored(magenta, "RECOVERY:");

    draw_metric_pair(" DETACH / RECLAIM  ", link.peer_detaches, link.slot_reclaims);
    draw_metric(" ATTACHMENTS       ", link.attachments);
    draw_metric(" SEQ RETRIES       ", link.seqlock_retries);
    draw_metric(" MAX DETACH AGE us ", link.peer_detach_max_age_us);

    ImGui::Separator();
    ImGui::BeginDisabled(link.mode != TurboLinkModeFault);
    ImGui::TextColored(violet, " LAST ERROR        "); ImGui::SameLine();
    ImGui::TextColored(link.mode == TurboLinkModeFault ? red : gray, "%s",
        link.last_error[0] ? link.last_error : "-");
    ImGui::EndDisabled();

    ImGui::Separator();

    if (ImGui::Button("RESET METRICS"))
        emu_turbolink_reset_metrics();

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}

static void turbolink_write_callback(u16 address, u8 value, void* user_data)
{
    UNUSED(address);
    Input* input = (Input*)user_data;
    input->WriteO(value);
}

static void draw_byte_value(const char* label, u8 value)
{
    ImGui::TextColored(violet, "%s", label); ImGui::SameLine();
    ImGui::Text("$%02X ", value); ImGui::SameLine(0.0f, 0.0f);
    ImGui::TextColored(gray, "(" BYTE_TO_BINARY_PATTERN_SPACED ")", BYTE_TO_BINARY(value));
}

static void draw_metric(const char* label, u64 value)
{
    ImGui::TextColored(violet, "%s", label); ImGui::SameLine();
    ImGui::TextColored(white, "%llu", (unsigned long long)value);
}

static void draw_metric_pair(const char* label, u64 first, u64 second)
{
    ImGui::TextColored(violet, "%s", label); ImGui::SameLine();
    ImGui::TextColored(white, "%llu / %llu", (unsigned long long)first, (unsigned long long)second);
}

static const char* get_driver_state(u8 pull_low_mask, u8 line)
{
    if ((pull_low_mask & line) == 0)
        return "RELEASED";

    return "LOW";
}
