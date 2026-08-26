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

#ifndef TURBOLINK_SIGNAL_H
#define TURBOLINK_SIGNAL_H

#include <atomic>
#include "turbolink.h"

struct TurboLinkLocalEvent
{
    u64 tick;
    u8 drive_mask;
    u8 value_mask;
};

struct TurboLinkSharedEvent
{
    std::atomic<u32> sequence;
    std::atomic<u32> generation;
    std::atomic<u64> tick;
    std::atomic<u32> drive;

    TurboLinkSharedEvent() : sequence(0), generation(0), tick(0), drive(0)
    {
    }
};

static_assert(alignof(TurboLinkSharedEvent) >= alignof(std::atomic<u64>),
    "TurboLink shared events require aligned 64-bit atomics");

enum TurboLinkSharedEventRead
{
    TurboLinkSharedEventRetry,
    TurboLinkSharedEventValid,
    TurboLinkSharedEventInvalid
};

inline bool turbolink_shared_event_atomics_lock_free( const TurboLinkSharedEvent& event)
{
    return event.sequence.is_lock_free() && event.generation.is_lock_free() &&
        event.tick.is_lock_free() && event.drive.is_lock_free();
}

inline bool turbolink_local_event_is_valid(const TurboLinkLocalEvent& event)
{
    return (event.drive_mask & ~GG_TURBOLINK_LINE_MASK) == 0 && event.value_mask == 0;
}

inline void turbolink_publish_shared_event(TurboLinkSharedEvent& event,
    u32 generation, u64 tick, u8 drive_mask, u8 value_mask)
{
    u32 sequence = event.sequence.load(std::memory_order_relaxed);
    u32 busy_sequence = (sequence + 1) | 1u;
    u32 packed = (u32)(drive_mask & GG_TURBOLINK_LINE_MASK) | ((u32)(value_mask & GG_TURBOLINK_LINE_MASK) << 8);

    event.sequence.exchange(busy_sequence, std::memory_order_acq_rel);
    event.generation.store(generation, std::memory_order_relaxed);
    event.tick.store(tick, std::memory_order_relaxed);
    event.drive.store(packed, std::memory_order_relaxed);
    event.sequence.store(busy_sequence + 1, std::memory_order_release);
}

inline TurboLinkSharedEventRead turbolink_read_shared_event(
    const TurboLinkSharedEvent& source, u32 expected_generation, TurboLinkLocalEvent& event)
{
    u32 before = source.sequence.load(std::memory_order_acquire);

    if ((before & 1) != 0)
        return TurboLinkSharedEventRetry;

    u32 generation = source.generation.load(std::memory_order_relaxed);
    event.tick = source.tick.load(std::memory_order_relaxed);
    u32 packed = source.drive.load(std::memory_order_relaxed);
    event.drive_mask = (u8)(packed & 0xFF);
    event.value_mask = (u8)((packed >> 8) & 0xFF);

    std::atomic_thread_fence(std::memory_order_acquire);
    u32 after = source.sequence.load(std::memory_order_relaxed);

    if (before != after || (after & 1) != 0 || generation != expected_generation)
    {
        return TurboLinkSharedEventRetry;
    }

    return turbolink_local_event_is_valid(event) ?
        TurboLinkSharedEventValid : TurboLinkSharedEventInvalid;
}

#endif /* TURBOLINK_SIGNAL_H */
