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

#ifndef TURBOLINK_H
#define TURBOLINK_H

#include <iosfwd>
#include "types.h"

#define TURBOLINK_MAX_SYNC_CYCLES 64
#define TURBOLINK_MAX_LEAD_CYCLES 256

#define GG_TURBOLINK_LINE_1 0x01
#define GG_TURBOLINK_LINE_2 0x02
#define GG_TURBOLINK_LINE_MASK 0x03

struct GG_TurboLink_Drive
{
    u8 drive_mask;
    u8 value_mask;
};

struct GG_TurboLink_Truth_Entry
{
    u8 drive_mask;
    u8 value_mask;
    u8 input_select_mask;
    u8 fixed_input_value;
};

enum GG_TurboLink_Tick_Phase
{
    GG_TURBOLINK_TICK_BEFORE_PORT_ACCESS = 0,
    GG_TURBOLINK_TICK_AFTER_PORT_ACCESS = 1
};

inline u64 turbolink_make_tick(u64 cycles, GG_TurboLink_Tick_Phase phase)
{
    return (cycles << 1) | (u64)phase;
}

typedef void (*GG_TurboLink_Publish_Callback)(u64 tick, u8 drive_mask,
    u8 value_mask, void* user_data);
typedef u8 (*GG_TurboLink_Sample_Callback)(u64 tick, void* user_data);
typedef void (*GG_TurboLink_Sync_Callback)(u64 tick, bool exact,
    void* user_data);

const GG_TurboLink_Truth_Entry& turbolink_get_truth_entry(bool sel, bool clr);
GG_TurboLink_Drive turbolink_get_control_drive(bool sel, bool clr);
u8 turbolink_apply_input_mux(u8 input, bool sel, bool clr, u8 lines);
u8 turbolink_map_remote_lines(u8 remote_lines);
GG_TurboLink_Drive turbolink_map_remote_drive(const GG_TurboLink_Drive& drive);
u8 turbolink_resolve_lines(const GG_TurboLink_Drive& first,
    const GG_TurboLink_Drive& second);

class TurboLink
{
public:
    TurboLink();
    void Reset(u64 cycles);
    void SetCallbacks(GG_TurboLink_Publish_Callback publish_callback,
        GG_TurboLink_Sample_Callback sample_callback, GG_TurboLink_Sync_Callback sync_callback, void* user_data);
    void SetCableConnected(bool connected, u64 cycles);
    void InvalidateSample();
    bool IsCableConnected() const { return m_cable_connected; }
    bool WriteControl(bool sel, bool clr, u64 cycles);
    u8 ReadPort(u8 input, u64 cycles);
    void Synchronize(u64 cycles);
    GG_TurboLink_Drive GetDrive() const;
    bool HasLastSample() const;
    u8 GetLastSampledLines() const;
    u8 GetLastPortResult() const;
    u8 GetLastSamplePullLowMask() const;
    bool GetLastSampleSel() const;
    bool GetLastSampleClr() const;
    u64 GetLastSampleTick() const;
    u64 GetLastControlTick() const;
    u64 GetLastDriveTick() const;
    bool GetSel() const;
    bool GetClr() const;
    void SaveState(std::ostream& stream) const;
    void LoadState(std::istream& stream);
    void RestoreControl(bool sel, bool clr);

private:
    void Publish(u64 cycles);

private:
    bool m_cable_connected;
    bool m_sel;
    bool m_clr;
    GG_TurboLink_Drive m_drive;
    bool m_sample_valid;
    u8 m_last_sampled_lines;
    u8 m_last_port_result;
    u8 m_last_sample_pull_low_mask;
    bool m_last_sample_sel;
    bool m_last_sample_clr;
    u64 m_last_sample_tick;
    u64 m_last_control_tick;
    u64 m_last_drive_tick;
    GG_TurboLink_Publish_Callback m_publish_callback;
    GG_TurboLink_Sample_Callback m_sample_callback;
    GG_TurboLink_Sync_Callback m_sync_callback;
    void* m_user_data;
};

#endif /* TURBOLINK_H */
