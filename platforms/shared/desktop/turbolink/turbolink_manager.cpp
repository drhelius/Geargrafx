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

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <new>
#include <thread>
#if defined(_WIN32)
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#include "turbolink_manager.h"
#include "turbolink_signal.h"
#include "common.h"
#include "log.h"

#define TURBOLINK_SHM_MAGIC 0x544C4E4B
#define TURBOLINK_SHM_VERSION 2

#define TURBOLINK_PEER_FREE 0
#define TURBOLINK_PEER_ACTIVE 1
#define TURBOLINK_PEER_CLAIMING 2
#define TURBOLINK_PEER_READY 3

struct TurboLinkManager::Shared
{
    struct Peer
    {
        std::atomic<u32> state;
        std::atomic<u32> generation;
        std::atomic<u64> heartbeat_us;
        std::atomic<u64> commit_tick;
        std::atomic<u64> write_index;
        TurboLinkSharedEvent events[TURBOLINK_SHARED_EVENT_COUNT];

        Peer() : state(0), generation(0), heartbeat_us(0), commit_tick(0), write_index(0)
        {
        }
    };

    std::atomic<u32> magic;
    u32 version;
    u8 session;
    u8 reserved[3];
    Peer peers[TURBOLINK_MAX_PEERS];

    Shared() : magic(0), version(TURBOLINK_SHM_VERSION), session(0)
    {
        memset(reserved, 0, sizeof(reserved));
    }
};

static void StoreMax(std::atomic<u64>& value, u64 desired)
{
    u64 current = value.load(std::memory_order_relaxed);
    while (current < desired && !value.compare_exchange_weak(current, desired,
        std::memory_order_release, std::memory_order_relaxed))
    {
    }
}

static bool IsPeerActive(u32 state)
{
    return state == TURBOLINK_PEER_ACTIVE || state == TURBOLINK_PEER_READY;
}

static bool IsPeerReady(u32 state)
{
    return state == TURBOLINK_PEER_READY;
}

TurboLinkManager::TurboLinkManager()
{
    m_shared = NULL;
    m_mapping_handle = NULL;
    m_mapping_fd = -1;
    m_slot = -1;
    m_generation = 0;
    m_session = 0;
    m_local_anchor_tick = 0;
    m_bus_anchor_tick = 0;
    m_last_local_tick = 0;
    m_last_sync_exit_us = 0;
    m_remote_slot = -1;
    m_remote_generation = 0;
    m_normal_barrier_stall_us = turbolink_normal_barrier_stall_us();
    m_last_drive.drive_mask = 0;
    m_last_drive.value_mask = 0;
    m_have_last_drive = false;
    m_local_attachment_changed = false;
    m_remote_identity_changed = false;
    m_hardware_ready = false;
    memset(&m_status, 0, sizeof(m_status));
    m_status.mode = TurboLinkModeDisabled;
    m_status.last_sampled_lines = GG_TURBOLINK_LINE_MASK;
}

TurboLinkManager::~TurboLinkManager()
{
    Stop();
}

bool TurboLinkManager::Connect(u8 session, u64 local_tick)
{
    Stop();

    if (session == 0)
    {
        SetFault("TurboLink session must be between 1 and 255");
        return false;
    }

    if (!Map(session))
        return false;

    m_session = session;

    if (!ClaimSlot(local_tick, false))
    {
        Unmap();
        SetFault("TurboLink session is full");
        return false;
    }

    memset(&m_status, 0, sizeof(m_status));
    m_status.mode = TurboLinkModeConnected;
    m_status.active = true;
    m_status.session = session;
    m_status.attachments = 1;
    m_status.last_sampled_lines = GG_TURBOLINK_LINE_MASK;
    snprintf(m_status.endpoint, sizeof(m_status.endpoint), "Shared session %u", session);

    u64 now = GetClockMicroseconds();
    ReapStalePeers(now);
    UpdateRemoteIdentity(now);
    RefreshStatus();
    Log("TurboLink: connected to shared session %u as peer %u", session, m_slot + 1);

    return true;
}

void TurboLinkManager::Stop()
{
    if (m_shared && m_slot >= 0)
    {
        Shared::Peer& peer = m_shared->peers[m_slot];
        if (peer.generation.load(std::memory_order_acquire) == m_generation)
            peer.state.store(0, std::memory_order_release);
    }

    Unmap();

    m_slot = -1;
    m_generation = 0;
    m_session = 0;
    m_local_anchor_tick = 0;
    m_bus_anchor_tick = 0;
    m_last_local_tick = 0;
    m_last_sync_exit_us = 0;
    m_remote_slot = -1;
    m_remote_generation = 0;
    m_have_last_drive = false;
    m_last_drive.drive_mask = 0;
    m_last_drive.value_mask = 0;
    m_local_attachment_changed = false;
    m_remote_identity_changed = false;
    m_hardware_ready = false;

    memset(&m_status, 0, sizeof(m_status));
    m_status.mode = TurboLinkModeDisabled;
    m_status.last_sampled_lines = GG_TURBOLINK_LINE_MASK;
}

void TurboLinkManager::SetHardwareReady(bool ready, u64 local_tick, u8 drive_mask, u8 value_mask)
{
    if ((drive_mask & ~GG_TURBOLINK_LINE_MASK) != 0 || value_mask != 0)
    {
        SetFault("Invalid TurboLink shared event");
        return;
    }

    if (!EnsureAttached(local_tick) || ready == m_hardware_ready)
        return;

    Shared::Peer& local = m_shared->peers[m_slot];

    if (!ready)
    {
        local.state.store(TURBOLINK_PEER_ACTIVE, std::memory_order_release);
        m_hardware_ready = false;
        m_have_last_drive = false;
        m_local_attachment_changed = true;
        m_status.local_pull_low_mask = 0;
        m_status.sample_valid = false;
        m_status.last_sample_remote_generation = 0;
        m_status.last_sample_local_pull_low_mask = 0;
        m_status.last_sample_remote_pull_low_mask = 0;
        m_status.last_sampled_lines = GG_TURBOLINK_LINE_MASK;
        UpdateRemoteIdentity(GetClockMicroseconds());
        return;
    }

    u64 now = GetClockMicroseconds();
    ReapStalePeers(now);

    u64 bus_anchor = 0;
    for (int i = 0; i < TURBOLINK_MAX_PEERS; i++)
    {
        Shared::Peer& peer = m_shared->peers[i];
        if (IsPeerActive(peer.state.load(std::memory_order_acquire)))
        {
            bus_anchor = MAX(bus_anchor, peer.commit_tick.load(std::memory_order_acquire));
        }
    }

    m_generation = local.generation.fetch_add(1, std::memory_order_acq_rel) + 1;
    m_local_anchor_tick = local_tick;
    m_bus_anchor_tick = bus_anchor;
    m_last_local_tick = local_tick;
    m_last_sync_exit_us = 0;
    m_remote_slot = -1;
    m_remote_generation = 0;
    m_last_drive.drive_mask = drive_mask;
    m_last_drive.value_mask = value_mask;
    m_have_last_drive = true;
    m_local_attachment_changed = true;
    m_remote_identity_changed = true;
    m_status.sample_valid = false;
    m_status.last_sample_remote_generation = 0;
    m_status.last_sample_local_pull_low_mask = 0;
    m_status.last_sample_remote_pull_low_mask = 0;
    m_status.last_sampled_lines = GG_TURBOLINK_LINE_MASK;

    local.write_index.store(0, std::memory_order_relaxed);
    turbolink_publish_shared_event(local.events[0], m_generation, bus_anchor, drive_mask, value_mask);
    local.write_index.store(1, std::memory_order_relaxed);
    local.commit_tick.store(bus_anchor, std::memory_order_relaxed);
    local.heartbeat_us.store(now, std::memory_order_relaxed);
    local.state.store(TURBOLINK_PEER_READY, std::memory_order_release);
    m_hardware_ready = true;

    m_status.events_published++;
    m_status.local_pull_low_mask = drive_mask;
    UpdateRemoteIdentity(now);
}

void TurboLinkManager::PublishDrive(u64 local_tick, u8 drive_mask, u8 value_mask)
{
    if ((drive_mask & ~GG_TURBOLINK_LINE_MASK) != 0 || value_mask != 0)
    {
        SetFault("Invalid TurboLink shared event");
        return;
    }

    if (!EnsureAttached(local_tick) || !m_hardware_ready)
        return;

    Shared::Peer& peer = m_shared->peers[m_slot];
    u64 now = GetClockMicroseconds();
    u64 tick = ToBusTick(local_tick);

    peer.heartbeat_us.store(now, std::memory_order_release);

    bool duplicate = m_have_last_drive && m_last_drive.drive_mask == drive_mask && m_last_drive.value_mask == value_mask;

    if (!duplicate)
    {
        u64 index = peer.write_index.load(std::memory_order_relaxed);
        TurboLinkSharedEvent& event = peer.events[index % TURBOLINK_SHARED_EVENT_COUNT];

        turbolink_publish_shared_event(event, m_generation, tick, drive_mask, value_mask);
        peer.write_index.store(index + 1, std::memory_order_release);
        m_status.events_published++;
    }

    StoreMax(peer.commit_tick, tick);

    m_last_drive.drive_mask = drive_mask;
    m_last_drive.value_mask = value_mask;
    m_have_last_drive = true;
    m_status.local_tick = local_tick;
    m_last_local_tick = local_tick;
    m_status.bus_tick = tick;
    m_status.local_pull_low_mask = drive_mask;
}

u8 TurboLinkManager::SampleLines(u64 local_tick)
{
    if (!EnsureAttached(local_tick) || !m_hardware_ready)
        return GG_TURBOLINK_LINE_MASK;

    u64 now = GetClockMicroseconds();
    u64 tick = ToBusTick(local_tick);
    Shared::Peer& local_peer = m_shared->peers[m_slot];
    local_peer.heartbeat_us.store(now, std::memory_order_release);

    GG_TurboLink_Drive local_drive = { 0, 0 };
    GG_TurboLink_Drive remote_drive = { 0, 0 };

    if (!FindDriveAt(m_slot, m_generation, tick, local_drive))
    {
        if (m_status.mode == TurboLinkModeFault)
            return GG_TURBOLINK_LINE_MASK;
    }

    u32 remote_generation = 0;
    int remote_slot = FindRemoteSlot(now, &remote_generation);

    if (remote_slot >= 0)
    {
        GG_TurboLink_Drive drive = { 0, 0 };
        if (!FindDriveAt(remote_slot, remote_generation, tick, drive))
        {
            if (m_status.mode == TurboLinkModeFault)
                return GG_TURBOLINK_LINE_MASK;
        }
        else if (IsPeerReady(m_shared->peers[remote_slot].state.load(
            std::memory_order_acquire)) &&
            m_shared->peers[remote_slot].generation.load(std::memory_order_acquire) == remote_generation)
        {
            remote_drive = turbolink_map_remote_drive(drive);
        }
    }

    u8 lines = turbolink_resolve_lines(local_drive, remote_drive);

    m_status.line_samples++;
    m_status.local_tick = local_tick;
    m_last_local_tick = local_tick;
    m_status.bus_tick = tick;
    m_status.sample_valid = true;
    m_status.last_sample_local_tick = local_tick;
    m_status.last_sample_bus_tick = tick;
    m_status.last_sample_remote_generation = remote_generation;
    m_status.last_sample_local_pull_low_mask = local_drive.drive_mask;
    m_status.last_sample_remote_pull_low_mask = remote_drive.drive_mask;
    m_status.last_sampled_lines = lines;

    return lines;
}

void TurboLinkManager::Synchronize(u64 local_tick, bool exact)
{
    if (!EnsureAttached(local_tick) || !m_hardware_ready)
        return;

    m_status.sync_calls++;
    u64 now = GetClockMicroseconds();

    if (m_last_sync_exit_us != 0)
    {
        u64 gap = now - m_last_sync_exit_us;
        m_status.sync_gap_max_us = MAX(m_status.sync_gap_max_us, gap);
        if (gap >= 50000)
            m_status.sync_gap_over_50ms++;
    }

    ReapStalePeers(now);

    u64 tick = ToBusTick(local_tick);
    Shared::Peer& local = m_shared->peers[m_slot];
    local.heartbeat_us.store(now, std::memory_order_release);

    StoreMax(local.commit_tick, tick);

    m_last_local_tick = local_tick;
    m_status.local_tick = local_tick;
    m_status.bus_tick = tick;

    UpdateRemoteIdentity(now);

    if (!IsCableConnected())
    {
        m_last_sync_exit_us = now;
        return;
    }

    if (exact)
        WaitForExactHorizon(tick);
    else
        WaitForLeadWindow(tick);

    m_last_sync_exit_us = GetClockMicroseconds();
}

void TurboLinkManager::Pump(u64 local_tick)
{
    if (!EnsureAttached(local_tick))
        return;

    u64 now = GetClockMicroseconds();
    Shared::Peer& local = m_shared->peers[m_slot];
    local.heartbeat_us.store(now, std::memory_order_release);
    m_last_local_tick = local_tick;
    m_status.local_tick = local_tick;
    m_status.bus_tick = ToBusTick(local_tick);

    if (m_hardware_ready)
        StoreMax(local.commit_tick, m_status.bus_tick);

    ReapStalePeers(now);
    UpdateRemoteIdentity(now);
    RefreshStatus();
}

bool TurboLinkManager::IsActive() const
{
    if (!m_shared || m_slot < 0)
        return false;

    const Shared::Peer& local = m_shared->peers[m_slot];
    return IsPeerActive(local.state.load(std::memory_order_acquire)) &&
        local.generation.load(std::memory_order_acquire) == m_generation;
}

bool TurboLinkManager::IsCableConnected() const
{
    if (!IsActive() || m_remote_slot < 0 || !m_hardware_ready)
        return false;

    const Shared::Peer& peer = m_shared->peers[m_remote_slot];
    u64 heartbeat = peer.heartbeat_us.load(std::memory_order_acquire);

    return IsPeerReady(peer.state.load(std::memory_order_acquire)) &&
        peer.generation.load(std::memory_order_acquire) == m_remote_generation &&
        turbolink_heartbeat_age(GetClockMicroseconds(), heartbeat) <= TURBOLINK_DETACH_US;
}

bool TurboLinkManager::IsHardwareReady() const
{
    return IsActive() && m_hardware_ready;
}

bool TurboLinkManager::HasRemotePeer() const
{
    if (!IsActive())
        return false;

    u64 now = GetClockMicroseconds();
    for (int i = 0; i < TURBOLINK_MAX_PEERS; i++)
    {
        if (i == m_slot)
            continue;

        const Shared::Peer& peer = m_shared->peers[i];
        u32 state = peer.state.load(std::memory_order_acquire);
        u64 heartbeat = peer.heartbeat_us.load(std::memory_order_acquire);

        if (IsPeerActive(state) && turbolink_heartbeat_age(now, heartbeat) <= TURBOLINK_DETACH_US)
        {
            return true;
        }
    }

    return false;
}

bool TurboLinkManager::IsPacingPeer() const
{
    if (!IsCableConnected())
        return false;

    for (int i = 0; i < m_slot; i++)
    {
        if (IsPeerReady(m_shared->peers[i].state.load(std::memory_order_acquire)))
            return false;
    }

    return true;
}

bool TurboLinkManager::ConsumeLocalAttachmentChanged()
{
    bool changed = m_local_attachment_changed;
    m_local_attachment_changed = false;
    return changed;
}

bool TurboLinkManager::ConsumeRemoteIdentityChanged()
{
    bool changed = m_remote_identity_changed;
    m_remote_identity_changed = false;
    return changed;
}

TurboLinkStatus TurboLinkManager::GetStatus()
{
    if (m_shared)
    {
        u64 now = GetClockMicroseconds();
        ReapStalePeers(now);
        UpdateRemoteIdentity(now);
    }
    RefreshStatus();
    return m_status;
}

void TurboLinkManager::ResetMetrics()
{
    m_status.events_published = 0;
    m_status.line_samples = 0;
    m_status.sync_calls = 0;
    m_status.exact_waits = 0;
    m_status.barrier_waits = 0;
    m_status.barrier_wait_us = 0;
    m_status.barrier_wait_max_us = 0;
    m_status.barrier_wait_over_1ms = 0;
    m_status.barrier_wait_over_10ms = 0;
    m_status.barrier_wait_over_50ms = 0;
    m_status.spin_iterations = 0;
    m_status.sleep_calls = 0;
    m_status.sync_gap_max_us = 0;
    m_status.sync_gap_over_50ms = 0;
    m_status.history_overflows = 0;
    m_status.peer_detaches = 0;
    m_status.peer_detach_max_age_us = 0;
    m_status.slot_reclaims = 0;
    m_status.seqlock_retries = 0;
    m_status.attachments = 0;
    m_last_sync_exit_us = GetClockMicroseconds();
}

void TurboLinkManager::SetNormalBarrierStallUs(u32 stall_us)
{
    m_normal_barrier_stall_us = stall_us;
}

bool TurboLinkManager::Map(u8 session)
{
    char name[64];
    bool created;

#if defined(_WIN32)
    snprintf(name, sizeof(name), "Local\\geargrafx-turbolink-v%u-%u", TURBOLINK_SHM_VERSION, session);

    HANDLE mapping = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL,
        PAGE_READWRITE, 0, (DWORD)sizeof(Shared), name);

    if (!mapping)
    {
        SetFault("Failed to create TurboLink shared memory");
        return false;
    }

    created = GetLastError() != ERROR_ALREADY_EXISTS;
    void* address = MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(Shared));

    if (!address)
    {
        CloseHandle(mapping);
        SetFault("Failed to map TurboLink shared memory");
        return false;
    }

    m_mapping_handle = mapping;
    m_shared = (Shared*)address;
#else
    snprintf(name, sizeof(name), "/geargrafx-turbolink-v%u-%u", TURBOLINK_SHM_VERSION, session);

    int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
    created = fd >= 0;

    if (!created && errno == EEXIST)
        fd = shm_open(name, O_RDWR, 0600);

    if (fd < 0)
    {
        SetFault("Failed to open TurboLink shared memory");
        return false;
    }

    if (created && ftruncate(fd, sizeof(Shared)) != 0)
    {
        close(fd);
        shm_unlink(name);
        SetFault("Failed to size TurboLink shared memory");
        return false;
    }

    if (!created)
    {
        u64 started = GetClockMicroseconds();
        struct stat status;

        while (fstat(fd, &status) != 0 || status.st_size < (off_t)sizeof(Shared))
        {
            if (GetClockMicroseconds() - started > TURBOLINK_DETACH_US)
            {
                close(fd);
                SetFault("TurboLink shared memory sizing timed out");
                return false;
            }

            std::this_thread::yield();
        }
    }

    void* address = mmap(NULL, sizeof(Shared), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (address == MAP_FAILED)
    {
        close(fd);
        SetFault("Failed to map TurboLink shared memory");
        return false;
    }

    m_mapping_fd = fd;
    m_shared = (Shared*)address;
#endif

    if (created)
    {
        new (m_shared) Shared();
        m_shared->session = session;
        m_shared->magic.store(TURBOLINK_SHM_MAGIC,
            std::memory_order_release);
    }
    else
    {
        u64 started = GetClockMicroseconds();

        while (m_shared->magic.load(std::memory_order_acquire) != TURBOLINK_SHM_MAGIC)
        {
            if (GetClockMicroseconds() - started > TURBOLINK_DETACH_US)
            {
                Unmap();
                SetFault("TurboLink shared memory initialization timed out");
                return false;
            }

            std::this_thread::yield();
        }

        if (m_shared->version != TURBOLINK_SHM_VERSION || m_shared->session != session)
        {
            Unmap();
            SetFault("Incompatible TurboLink shared memory");
            return false;
        }
    }

    if (!SharedAtomicsLockFree())
    {
        Unmap();
        SetFault("TurboLink shared event atomics are not lock-free");
        return false;
    }

    return true;
}

void TurboLinkManager::Unmap()
{
    if (!m_shared)
        return;

#if defined(_WIN32)
    UnmapViewOfFile(m_shared);

    if (m_mapping_handle)
        CloseHandle((HANDLE)m_mapping_handle);
#else
    munmap(m_shared, sizeof(Shared));

    if (m_mapping_fd >= 0)
        close(m_mapping_fd);
#endif

    m_shared = NULL;
    m_mapping_handle = NULL;
    m_mapping_fd = -1;
}

bool TurboLinkManager::ClaimSlot(u64 local_tick, bool reattach)
{
    u64 now = GetClockMicroseconds();
    ReapStalePeers(now);

    u64 bus_anchor = 0;

    for (int i = 0; i < TURBOLINK_MAX_PEERS; i++)
    {
        Shared::Peer& peer = m_shared->peers[i];
        if (IsPeerActive(peer.state.load(std::memory_order_acquire)))
        {
            bus_anchor = MAX(bus_anchor, peer.commit_tick.load(std::memory_order_acquire));
        }
    }

    for (int i = 0; i < TURBOLINK_MAX_PEERS; i++)
    {
        Shared::Peer& peer = m_shared->peers[i];
        u32 expected = TURBOLINK_PEER_FREE;

        if (!peer.state.compare_exchange_strong(expected, TURBOLINK_PEER_CLAIMING, std::memory_order_acq_rel))
        {
            continue;
        }

        m_slot = i;
        m_generation = peer.generation.fetch_add(1, std::memory_order_acq_rel) + 1;
        m_local_anchor_tick = local_tick;
        m_bus_anchor_tick = bus_anchor;
        m_last_local_tick = local_tick;
        m_last_drive.drive_mask = 0;
        m_last_drive.value_mask = 0;
        m_have_last_drive = false;
        m_remote_slot = -1;
        m_remote_generation = 0;
        m_hardware_ready = false;
        m_local_attachment_changed = true;
        m_remote_identity_changed = true;

        peer.write_index.store(0, std::memory_order_relaxed);
        peer.commit_tick.store(bus_anchor, std::memory_order_relaxed);
        peer.heartbeat_us.store(now, std::memory_order_relaxed);
        peer.state.store(TURBOLINK_PEER_ACTIVE, std::memory_order_release);

        m_status.local_tick = local_tick;
        m_status.bus_tick = bus_anchor;
        m_status.local_pull_low_mask = 0;

        if (reattach)
        {
            m_status.slot_reclaims++;
            m_status.mode = TurboLinkModeConnected;
            m_status.active = true;
            m_status.last_error[0] = '\0';
            Log("TurboLink: reattached to shared session %u", m_session);
        }

        m_status.attachments++;

        return true;
    }

    return false;
}

bool TurboLinkManager::EnsureAttached(u64 local_tick)
{
    if (!m_shared || m_status.mode == TurboLinkModeFault)
        return false;

    m_last_local_tick = local_tick;

    if (m_slot >= 0)
    {
        Shared::Peer& peer = m_shared->peers[m_slot];
        if (IsPeerActive(peer.state.load(std::memory_order_acquire)) &&
            peer.generation.load(std::memory_order_acquire) == m_generation)
        {
            return true;
        }
    }

    m_slot = -1;
    m_generation = 0;
    m_hardware_ready = false;
    if (ClaimSlot(local_tick, true))
        return true;

    SetFault("TurboLink session is full after reattachment");
    return false;
}

void TurboLinkManager::ReapStalePeers(u64 now_us)
{
    if (!m_shared)
        return;

    for (int i = 0; i < TURBOLINK_MAX_PEERS; i++)
    {
        if (i == m_slot)
            continue;

        Shared::Peer& peer = m_shared->peers[i];
        u64 heartbeat = peer.heartbeat_us.load(std::memory_order_acquire);
        u32 generation = peer.generation.load(std::memory_order_acquire);
        u64 age = turbolink_heartbeat_age(now_us, heartbeat);

        u32 observed_state = peer.state.load(std::memory_order_acquire);
        if (!IsPeerActive(observed_state) || age <= TURBOLINK_DETACH_US)
        {
            continue;
        }

        if (!IsPeerActive(peer.state.load(std::memory_order_acquire)))
            continue;

        u32 current_generation =
            peer.generation.load(std::memory_order_acquire);
        u64 current_heartbeat =
            peer.heartbeat_us.load(std::memory_order_acquire);

        if (!turbolink_lease_is_unchanged_and_stale(now_us, heartbeat,
            generation, current_heartbeat, current_generation))
        {
            continue;
        }

        u32 expected = observed_state;
        if (peer.state.compare_exchange_strong(expected, 0, std::memory_order_acq_rel))
        {
            m_status.peer_detaches++;
            m_status.peer_detach_max_age_us = MAX(m_status.peer_detach_max_age_us, age);
        }
    }
}

int TurboLinkManager::FindRemoteSlot(u64 now_us, u32* generation) const
{
    if (!m_shared)
        return -1;

    for (int i = 0; i < TURBOLINK_MAX_PEERS; i++)
    {
        if (i == m_slot)
            continue;

        const Shared::Peer& peer = m_shared->peers[i];
        if (!IsPeerReady(peer.state.load(std::memory_order_acquire)))
            continue;

        u64 heartbeat = peer.heartbeat_us.load(std::memory_order_acquire);
        if (turbolink_heartbeat_age(now_us, heartbeat) > TURBOLINK_DETACH_US)
        {
            continue;
        }

        if (generation)
            *generation = peer.generation.load(std::memory_order_acquire);
        return i;
    }

    return -1;
}

void TurboLinkManager::UpdateRemoteIdentity(u64 now_us)
{
    u32 generation = 0;
    int slot = m_hardware_ready ? FindRemoteSlot(now_us, &generation) : -1;

    if (slot == m_remote_slot && generation == m_remote_generation)
        return;

    m_remote_slot = slot;
    m_remote_generation = generation;
    m_remote_identity_changed = true;
    m_status.sample_valid = false;
    m_status.last_sample_remote_generation = 0;
    m_status.last_sample_remote_pull_low_mask = 0;
    m_status.last_sampled_lines = GG_TURBOLINK_LINE_MASK;
}

bool TurboLinkManager::SharedAtomicsLockFree() const
{
    if (!m_shared->magic.is_lock_free())
        return false;

    for (int i = 0; i < TURBOLINK_MAX_PEERS; i++)
    {
        const Shared::Peer& peer = m_shared->peers[i];
        if (!peer.state.is_lock_free() || !peer.generation.is_lock_free() ||
            !peer.heartbeat_us.is_lock_free() ||
            !peer.commit_tick.is_lock_free() ||
            !peer.write_index.is_lock_free())
        {
            return false;
        }

        for (int event = 0; event < TURBOLINK_SHARED_EVENT_COUNT; event++)
        {
            if (!turbolink_shared_event_atomics_lock_free(peer.events[event]))
            {
                return false;
            }
        }
    }

    return true;
}

bool TurboLinkManager::FindDriveAt(int peer_index, u32 generation, u64 sample_tick, GG_TurboLink_Drive& drive)
{
    if (!m_shared || peer_index < 0 || peer_index >= TURBOLINK_MAX_PEERS)
        return false;

    Shared::Peer& peer = m_shared->peers[peer_index];
    u64 write_index = peer.write_index.load(std::memory_order_acquire);
    u32 count = (u32)MIN(write_index,
        (u64)TURBOLINK_SHARED_EVENT_COUNT);

    if (count == 0)
    {
        SetFault("Invalid TurboLink shared event");
        return false;
    }

    for (int attempt = 0; attempt < 4; attempt++)
    {
        bool read_valid = false;
        u64 oldest_tick = ~0ULL;

        for (u32 offset = 0; offset < count; offset++)
        {
            TurboLinkSharedEvent& source = peer.events[(write_index - 1 - offset) % TURBOLINK_SHARED_EVENT_COUNT];
            TurboLinkLocalEvent event;
            TurboLinkSharedEventRead result = turbolink_read_shared_event(source, generation, event);

            if (result == TurboLinkSharedEventInvalid)
            {
                SetFault("Invalid TurboLink shared event");
                return false;
            }

            if (result == TurboLinkSharedEventRetry)
            {
                m_status.seqlock_retries++;
                continue;
            }

            if (result != TurboLinkSharedEventValid)
                continue;

            read_valid = true;
            oldest_tick = MIN(oldest_tick, event.tick);

            if (event.tick <= sample_tick)
            {
                drive.drive_mask = event.drive_mask;
                drive.value_mask = event.value_mask;
                return true;
            }
        }

        if (read_valid)
        {
            if (write_index > TURBOLINK_SHARED_EVENT_COUNT && oldest_tick > sample_tick)
            {
                m_status.history_overflows++;
                SetFault("TurboLink shared event history overflow");
                return false;
            }

            drive.drive_mask = 0;
            drive.value_mask = 0;
            return true;
        }

        std::this_thread::yield();
        m_status.spin_iterations++;
        write_index = peer.write_index.load(std::memory_order_acquire);
        count = (u32)MIN(write_index, (u64)TURBOLINK_SHARED_EVENT_COUNT);
    }

    SetFault("Invalid TurboLink shared event");
    return false;
}

void TurboLinkManager::WaitForExactHorizon(u64 tick)
{
    WaitForBarrier(tick, true);
}

void TurboLinkManager::WaitForLeadWindow(u64 tick)
{
    WaitForBarrier(tick, false);
}

void TurboLinkManager::WaitForBarrier(u64 tick, bool exact)
{
    if (!IsCableConnected())
        return;

    Shared::Peer& local = m_shared->peers[m_slot];
    u64 wait_started = 0;
    u64 progress_time = GetClockMicroseconds();
    u64 previous_floor = ~0ULL;

    for (;;)
    {
        bool ready = true;
        int remote_count = 0;
        u64 floor = ~0ULL;

        for (int i = 0; i < TURBOLINK_MAX_PEERS; i++)
        {
            Shared::Peer& peer = m_shared->peers[i];

            if (!IsPeerReady(peer.state.load(std::memory_order_acquire)))
                continue;

            u64 commit = peer.commit_tick.load(std::memory_order_acquire);
            floor = MIN(floor, commit);

            if (i != m_slot)
            {
                remote_count++;
                if (exact && commit < tick)
                    ready = false;
            }
        }

        if (!exact)
        {
            ready = remote_count == 0 || floor == ~0ULL || tick <= floor ||
                tick - floor <= TURBOLINK_MAX_LEAD_TICKS;
        }

        if (ready)
            break;

        if (wait_started == 0)
        {
            wait_started = GetClockMicroseconds();
            progress_time = wait_started;
            m_status.barrier_waits++;
            if (exact)
                m_status.exact_waits++;
        }

        u64 now = GetClockMicroseconds();

        if (floor != previous_floor)
        {
            previous_floor = floor;
            progress_time = now;
        }

        ReapStalePeers(now);
        UpdateRemoteIdentity(now);

        if (!IsPeerReady(local.state.load(std::memory_order_acquire)) ||
            local.generation.load(std::memory_order_acquire) != m_generation ||
            !IsCableConnected())
        {
            break;
        }

        local.heartbeat_us.store(now, std::memory_order_release);

        if (now - progress_time >= m_normal_barrier_stall_us)
        {
            std::this_thread::sleep_for(std::chrono::microseconds(TURBOLINK_BARRIER_SLEEP_US));
            m_status.sleep_calls++;
        }
        else
        {
            std::this_thread::yield();
            m_status.spin_iterations++;
        }
    }

    if (wait_started != 0)
    {
        u64 wait = GetClockMicroseconds() - wait_started;
        m_status.barrier_wait_us += wait;
        m_status.barrier_wait_max_us = MAX(m_status.barrier_wait_max_us, wait);

        if (wait >= 1000)
            m_status.barrier_wait_over_1ms++;
        if (wait >= 10000)
            m_status.barrier_wait_over_10ms++;
        if (wait >= 50000)
            m_status.barrier_wait_over_50ms++;
    }
}

u64 TurboLinkManager::ToBusTick(u64 local_tick) const
{
    if (local_tick >= m_local_anchor_tick)
        return local_tick - m_local_anchor_tick + m_bus_anchor_tick;

    u64 difference = m_local_anchor_tick - local_tick;
    return difference <= m_bus_anchor_tick ? m_bus_anchor_tick - difference : 0;
}

u64 TurboLinkManager::GetClockMicroseconds() const
{
    return (u64)std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

void TurboLinkManager::SetFault(const char* message)
{
    if (m_shared && m_slot >= 0)
    {
        Shared::Peer& peer = m_shared->peers[m_slot];
        if (peer.generation.load(std::memory_order_acquire) == m_generation)
            peer.state.store(TURBOLINK_PEER_FREE, std::memory_order_release);
    }

    m_slot = -1;
    m_hardware_ready = false;
    m_remote_slot = -1;
    m_remote_generation = 0;
    m_local_attachment_changed = true;
    m_remote_identity_changed = true;
    m_status.mode = TurboLinkModeFault;
    m_status.active = false;
    m_status.cable_connected = false;
    snprintf(m_status.last_error, sizeof(m_status.last_error), "%s", message);
    Error("TurboLink: %s", message);
}

void TurboLinkManager::RefreshStatus()
{
    m_status.active = IsActive();
    m_status.session = m_session;
    m_status.local_generation = m_generation;
    m_status.remote_generation = 0;
    m_status.local_anchor_tick = m_local_anchor_tick;
    m_status.bus_anchor_tick = m_bus_anchor_tick;
    m_status.local_tick = m_last_local_tick;
    m_status.bus_tick = m_status.active ? ToBusTick(m_last_local_tick) : 0;
    m_status.local_hardware_ready = false;
    m_status.remote_hardware_ready = false;
    m_status.remote_active = false;
    m_status.local_peer_id = 0;
    m_status.remote_peer_id = 0;
    m_status.peer_count = 0;
    m_status.remote_tick = 0;
    m_status.remote_heartbeat_age_us = 0;

    if (m_shared)
    {
        u64 now = GetClockMicroseconds();

        for (int i = 0; i < TURBOLINK_MAX_PEERS; i++)
        {
            Shared::Peer& peer = m_shared->peers[i];
            u64 heartbeat = peer.heartbeat_us.load(std::memory_order_acquire);
            u64 age = turbolink_heartbeat_age(now, heartbeat);
            u32 state = peer.state.load(std::memory_order_acquire);

            if (!IsPeerActive(state) || age > TURBOLINK_DETACH_US)
                continue;

            m_status.peer_count++;

            if (i == m_slot)
            {
                m_status.local_peer_id = (u8)(i + 1);
                m_status.local_hardware_ready = IsPeerReady(state);
            }
            else
            {
                m_status.remote_peer_id = (u8)(i + 1);
                m_status.remote_active = true;
                m_status.remote_generation = peer.generation.load(std::memory_order_acquire);
                m_status.remote_heartbeat_age_us = age;

                if (IsPeerReady(state))
                {
                    m_status.remote_hardware_ready = true;
                    m_status.remote_tick = peer.commit_tick.load(std::memory_order_acquire);
                }
            }
        }
    }

    m_status.cable_connected = m_status.active &&
        m_status.local_hardware_ready && m_status.remote_hardware_ready;
    m_status.pacing_peer = m_status.cable_connected && IsPacingPeer();

    if (m_status.cable_connected)
    {
        m_status.lead_ticks = m_status.bus_tick >= m_status.remote_tick ?
            (s64)(m_status.bus_tick - m_status.remote_tick) : -(s64)(m_status.remote_tick - m_status.bus_tick);
    }
    else
        m_status.lead_ticks = 0;
}
