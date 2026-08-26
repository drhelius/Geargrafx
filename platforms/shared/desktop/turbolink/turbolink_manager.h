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

#ifndef TURBOLINK_MANAGER_H
#define TURBOLINK_MANAGER_H

#include "turbolink.h"

#define TURBOLINK_MAX_PEERS 2
#define TURBOLINK_SHARED_EVENT_COUNT 256
#define TURBOLINK_DETACH_US 500000
#define TURBOLINK_BARRIER_SLEEP_US 100
#define TURBOLINK_MAX_LEAD_TICKS (TURBOLINK_MAX_LEAD_CYCLES * 2ULL)

enum TurboLinkMode
{
    TurboLinkModeDisabled,
    TurboLinkModeConnected,
    TurboLinkModeFault
};

struct TurboLinkStatus
{
    TurboLinkMode mode;
    bool active;
    bool cable_connected;
    bool pacing_peer;
    bool local_hardware_ready;
    bool remote_hardware_ready;
    bool remote_active;
    u8 session;
    u8 local_peer_id;
    u8 remote_peer_id;
    int peer_count;
    u32 local_generation;
    u32 remote_generation;
    u64 local_anchor_tick;
    u64 bus_anchor_tick;
    u64 local_tick;
    u64 bus_tick;
    u64 remote_tick;
    s64 lead_ticks;
    u64 remote_heartbeat_age_us;
    u64 last_sample_local_tick;
    u64 last_sample_bus_tick;
    u32 last_sample_remote_generation;
    u8 local_pull_low_mask;
    bool sample_valid;
    u8 last_sample_local_pull_low_mask;
    u8 last_sample_remote_pull_low_mask;
    u8 last_sampled_lines;
    u64 events_published;
    u64 line_samples;
    u64 sync_calls;
    u64 exact_waits;
    u64 barrier_waits;
    u64 barrier_wait_us;
    u64 barrier_wait_max_us;
    u64 barrier_wait_over_1ms;
    u64 barrier_wait_over_10ms;
    u64 barrier_wait_over_50ms;
    u64 spin_iterations;
    u64 sleep_calls;
    u64 sync_gap_max_us;
    u64 sync_gap_over_50ms;
    u64 history_overflows;
    u64 peer_detaches;
    u64 peer_detach_max_age_us;
    u64 slot_reclaims;
    u64 seqlock_retries;
    u64 attachments;
    char endpoint[128];
    char last_error[128];
};

class TurboLinkManager
{
public:
    TurboLinkManager();
    ~TurboLinkManager();
    bool Connect(u8 session, u64 local_tick);
    void Stop();
    void SetHardwareReady(bool ready, u64 local_tick, u8 drive_mask = 0, u8 value_mask = 0);
    void PublishDrive(u64 local_tick, u8 drive_mask, u8 value_mask);
    u8 SampleLines(u64 local_tick);
    void Synchronize(u64 local_tick, bool exact);
    void Pump(u64 local_tick);
    bool IsActive() const;
    bool IsCableConnected() const;
    bool IsHardwareReady() const;
    bool HasRemotePeer() const;
    bool IsPacingPeer() const;
    bool ConsumeLocalAttachmentChanged();
    bool ConsumeRemoteIdentityChanged();
    TurboLinkStatus GetStatus();
    void ResetMetrics();
    void SetNormalBarrierStallUs(u32 stall_us);

private:
    struct Shared;

    bool Map(u8 session);
    void Unmap();
    bool ClaimSlot(u64 local_tick, bool reattach);
    bool EnsureAttached(u64 local_tick);
    void ReapStalePeers(u64 now_us);
    int FindRemoteSlot(u64 now_us, u32* generation = NULL) const;
    void UpdateRemoteIdentity(u64 now_us);
    bool SharedAtomicsLockFree() const;
    bool FindDriveAt(int peer_index, u32 generation, u64 sample_tick, GG_TurboLink_Drive& drive);
    void WaitForExactHorizon(u64 tick);
    void WaitForLeadWindow(u64 tick);
    void WaitForBarrier(u64 tick, bool exact);
    u64 ToBusTick(u64 local_tick) const;
    u64 GetClockMicroseconds() const;
    void SetFault(const char* message);
    void RefreshStatus();

private:
    Shared* m_shared;
    void* m_mapping_handle;
    int m_mapping_fd;
    int m_slot;
    u32 m_generation;
    u8 m_session;
    u64 m_local_anchor_tick;
    u64 m_bus_anchor_tick;
    u64 m_last_local_tick;
    u64 m_last_sync_exit_us;
    int m_remote_slot;
    u32 m_remote_generation;
    u32 m_normal_barrier_stall_us;
    GG_TurboLink_Drive m_last_drive;
    bool m_have_last_drive;
    bool m_local_attachment_changed;
    bool m_remote_identity_changed;
    bool m_hardware_ready;
    TurboLinkStatus m_status;
};

inline u32 turbolink_normal_barrier_stall_us()
{
#if defined(_WIN32)
    return 5000;
#elif defined(__APPLE__)
    return 100;
#else
    return 250;
#endif
}

inline u64 turbolink_heartbeat_age(u64 now, u64 heartbeat)
{
    return heartbeat <= now ? now - heartbeat : 0;
}

inline bool turbolink_lease_is_unchanged_and_stale(u64 now,
    u64 observed_heartbeat, u32 observed_generation,
    u64 current_heartbeat, u32 current_generation)
{
    return current_heartbeat == observed_heartbeat &&
        current_generation == observed_generation &&
        turbolink_heartbeat_age(now, current_heartbeat) > TURBOLINK_DETACH_US;
}

#endif /* TURBOLINK_MANAGER_H */
