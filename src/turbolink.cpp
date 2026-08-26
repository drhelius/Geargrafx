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

#include <istream>
#include <ostream>
#include "turbolink.h"

static const GG_TurboLink_Truth_Entry k_turbolink_truth_table[4] =
{
    { 0x03, 0x00, 0x00, 0x00 },
    { 0x02, 0x00, 0x00, 0x00 },
    { 0x01, 0x00, 0x03, 0x00 },
    { 0x00, 0x00, 0x03, 0x00 }
};

const GG_TurboLink_Truth_Entry& turbolink_get_truth_entry(bool sel, bool clr)
{
    int index = (clr ? 2 : 0) | (sel ? 1 : 0);
    return k_turbolink_truth_table[index];
}

GG_TurboLink_Drive turbolink_get_control_drive(bool sel, bool clr)
{
    const GG_TurboLink_Truth_Entry& entry = turbolink_get_truth_entry(sel, clr);
    GG_TurboLink_Drive drive;
    drive.drive_mask = entry.drive_mask;
    drive.value_mask = entry.value_mask;
    return drive;
}

u8 turbolink_apply_input_mux(u8 input, bool sel, bool clr, u8 lines)
{
    const GG_TurboLink_Truth_Entry& entry = turbolink_get_truth_entry(sel, clr);

    if (entry.input_select_mask == 0)
        return input;

    lines &= GG_TURBOLINK_LINE_MASK;
    u8 link_inputs = ((lines & GG_TURBOLINK_LINE_2) >> 1) | ((lines & GG_TURBOLINK_LINE_1) << 1);

    return (input & 0xF0) | (link_inputs & entry.input_select_mask) | entry.fixed_input_value;
}

u8 turbolink_map_remote_lines(u8 remote_lines)
{
    return remote_lines & GG_TURBOLINK_LINE_MASK;
}

GG_TurboLink_Drive turbolink_map_remote_drive(const GG_TurboLink_Drive& drive)
{
    GG_TurboLink_Drive mapped;
    mapped.drive_mask = turbolink_map_remote_lines(drive.drive_mask);
    mapped.value_mask = turbolink_map_remote_lines(drive.value_mask);
    return mapped;
}

u8 turbolink_resolve_lines(const GG_TurboLink_Drive& first,
    const GG_TurboLink_Drive& second)
{
    u8 pull_low = (first.drive_mask | second.drive_mask) & GG_TURBOLINK_LINE_MASK;
    return GG_TURBOLINK_LINE_MASK & (u8)~pull_low;
}

TurboLink::TurboLink()
{
    m_cable_connected = false;
    m_sel = true;
    m_clr = true;
    m_drive = turbolink_get_control_drive(m_sel, m_clr);
    m_sample_valid = false;
    m_last_sampled_lines = GG_TURBOLINK_LINE_MASK;
    m_last_port_result = 0;
    m_last_sample_pull_low_mask = 0;
    m_last_sample_sel = true;
    m_last_sample_clr = true;
    m_last_sample_tick = 0;
    m_last_control_tick = 0;
    m_last_drive_tick = 1;
    m_publish_callback = NULL;
    m_sample_callback = NULL;
    m_sync_callback = NULL;
    m_user_data = NULL;
}

void TurboLink::Reset(u64 cycles)
{
    m_sel = true;
    m_clr = true;
    m_drive = turbolink_get_control_drive(m_sel, m_clr);
    m_sample_valid = false;
    m_last_sampled_lines = GG_TURBOLINK_LINE_MASK;
    m_last_port_result = 0;
    m_last_sample_pull_low_mask = 0;
    m_last_sample_sel = true;
    m_last_sample_clr = true;
    m_last_sample_tick = 0;
    m_last_control_tick = turbolink_make_tick(cycles, GG_TURBOLINK_TICK_BEFORE_PORT_ACCESS);
    m_last_drive_tick = turbolink_make_tick(cycles, GG_TURBOLINK_TICK_AFTER_PORT_ACCESS);

    if (m_cable_connected)
        Publish(cycles);
}

void TurboLink::SetCallbacks(GG_TurboLink_Publish_Callback publish_callback,
    GG_TurboLink_Sample_Callback sample_callback, GG_TurboLink_Sync_Callback sync_callback, void* user_data)
{
    m_publish_callback = publish_callback;
    m_sample_callback = sample_callback;
    m_sync_callback = sync_callback;
    m_user_data = user_data;
}

void TurboLink::SetCableConnected(bool connected, u64 cycles)
{
    bool changed = m_cable_connected != connected;
    m_cable_connected = connected;

    if (changed)
    {
        InvalidateSample();

        if (connected)
        {
            m_last_drive_tick = turbolink_make_tick(cycles,
                GG_TURBOLINK_TICK_AFTER_PORT_ACCESS);
            Publish(cycles);
        }
    }
}

void TurboLink::InvalidateSample()
{
    m_sample_valid = false;
    m_last_sampled_lines = GG_TURBOLINK_LINE_MASK;
    m_last_port_result = 0;
    m_last_sample_pull_low_mask = 0;
    m_last_sample_sel = m_sel;
    m_last_sample_clr = m_clr;
    m_last_sample_tick = 0;
}

bool TurboLink::WriteControl(bool sel, bool clr, u64 cycles)
{
    GG_TurboLink_Drive drive = turbolink_get_control_drive(sel, clr);
    bool changed = drive.drive_mask != m_drive.drive_mask || drive.value_mask != m_drive.value_mask;

    m_last_control_tick = turbolink_make_tick(cycles, GG_TURBOLINK_TICK_BEFORE_PORT_ACCESS);
    m_sel = sel;
    m_clr = clr;
    m_drive = drive;

    if (changed)
    {
        m_last_drive_tick = turbolink_make_tick(cycles, GG_TURBOLINK_TICK_AFTER_PORT_ACCESS);

        if (m_cable_connected)
            Publish(cycles);
    }

    return changed;
}

u8 TurboLink::ReadPort(u8 input, u64 cycles)
{
    if (!m_cable_connected || !m_clr)
        return input;

    u64 tick = turbolink_make_tick(cycles, GG_TURBOLINK_TICK_BEFORE_PORT_ACCESS);

    if (m_sync_callback)
        m_sync_callback(tick, true, m_user_data);

    u8 lines = GG_TURBOLINK_LINE_MASK;

    if (m_sample_callback)
        lines = m_sample_callback(tick, m_user_data);

    u8 result = turbolink_apply_input_mux(input, m_sel, m_clr, lines);
    m_sample_valid = true;
    m_last_sampled_lines = lines & GG_TURBOLINK_LINE_MASK;
    m_last_port_result = result;
    m_last_sample_pull_low_mask = m_drive.drive_mask;
    m_last_sample_sel = m_sel;
    m_last_sample_clr = m_clr;
    m_last_sample_tick = tick;

    return result;
}

void TurboLink::Synchronize(u64 cycles)
{
    if (m_cable_connected && m_sync_callback)
    {
        u64 tick = turbolink_make_tick(cycles, GG_TURBOLINK_TICK_BEFORE_PORT_ACCESS);
        m_sync_callback(tick, false, m_user_data);
    }
}

GG_TurboLink_Drive TurboLink::GetDrive() const
{
    return m_drive;
}

bool TurboLink::HasLastSample() const
{
    return m_sample_valid;
}

u8 TurboLink::GetLastSampledLines() const
{
    return m_last_sampled_lines;
}

u8 TurboLink::GetLastPortResult() const
{
    return m_last_port_result;
}

u8 TurboLink::GetLastSamplePullLowMask() const
{
    return m_last_sample_pull_low_mask;
}

bool TurboLink::GetLastSampleSel() const
{
    return m_last_sample_sel;
}

bool TurboLink::GetLastSampleClr() const
{
    return m_last_sample_clr;
}

u64 TurboLink::GetLastSampleTick() const
{
    return m_last_sample_tick;
}

u64 TurboLink::GetLastControlTick() const
{
    return m_last_control_tick;
}

u64 TurboLink::GetLastDriveTick() const
{
    return m_last_drive_tick;
}

bool TurboLink::GetSel() const
{
    return m_sel;
}

bool TurboLink::GetClr() const
{
    return m_clr;
}

void TurboLink::SaveState(std::ostream& stream) const
{
    stream.write(reinterpret_cast<const char*>(&m_sel), sizeof(m_sel));
    stream.write(reinterpret_cast<const char*>(&m_clr), sizeof(m_clr));
    stream.write(reinterpret_cast<const char*>(&m_drive.drive_mask), sizeof(m_drive.drive_mask));
    stream.write(reinterpret_cast<const char*>(&m_drive.value_mask), sizeof(m_drive.value_mask));
}

void TurboLink::LoadState(std::istream& stream)
{
    bool sel;
    bool clr;
    u8 drive_mask;
    u8 value_mask;

    stream.read(reinterpret_cast<char*>(&sel), sizeof(sel));
    stream.read(reinterpret_cast<char*>(&clr), sizeof(clr));
    stream.read(reinterpret_cast<char*>(&drive_mask), sizeof(drive_mask));
    stream.read(reinterpret_cast<char*>(&value_mask), sizeof(value_mask));

    (void)drive_mask;
    (void)value_mask;
    RestoreControl(sel, clr);
}

void TurboLink::RestoreControl(bool sel, bool clr)
{
    m_sel = sel;
    m_clr = clr;
    m_drive = turbolink_get_control_drive(sel, clr);
    m_sample_valid = false;
    m_last_sampled_lines = GG_TURBOLINK_LINE_MASK;
    m_last_port_result = 0;
    m_last_sample_pull_low_mask = 0;
    m_last_sample_sel = sel;
    m_last_sample_clr = clr;
    m_last_sample_tick = 0;
    m_last_control_tick = 0;
    m_last_drive_tick = 0;
}

void TurboLink::Publish(u64 cycles)
{
    if (!m_publish_callback)
        return;

    u64 tick = turbolink_make_tick(cycles, GG_TURBOLINK_TICK_AFTER_PORT_ACCESS);
    m_publish_callback(tick, m_drive.drive_mask, m_drive.value_mask, m_user_data);
}
