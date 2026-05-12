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

#ifndef HUC6270_INLINE_H
#define HUC6270_INLINE_H

#include <assert.h>
#include "huc6270.h"
#include "huc6260.h"
#include "huc6280.h"
#include "trace_logger.h"

INLINE u16 HuC6270::Clock()
{
    if (m_sat_transfer_pending > 0)
        SATTransfer();
    else if (m_vram_transfer_pending > 0)
        VRAMTransfer();

    m_hpos++;

    m_clocks_to_next_h_state--;
    while (m_clocks_to_next_h_state == 0)
        NextHorizontalState();

    if (m_clocks_to_next_event > 0)
        LineEvents();

    u16 pixel = m_burst_mode ? HUC6270_PIXEL_BLACK : 0x100;

    if (m_active_line && (m_h_state == HuC6270_HORIZONTAL_STATE_HDW))
    {
        assert(m_line_buffer_index < HUC6270_MAX_BACKGROUND_WIDTH);
        if (m_burst_mode)
            pixel = HUC6270_PIXEL_BLACK;
        else
        {
            pixel = m_line_buffer[m_line_buffer_index];
            if ((pixel & 0x0F) == 0)
                pixel = 0;
        }
        m_line_buffer_index++;
    }

    return pixel;
}

INLINE HuC6270::HuC6270_State* HuC6270::GetState()
{
    return &m_state;
}

INLINE u16* HuC6270::GetVRAM()
{
    return m_vram;
}

INLINE u16* HuC6270::GetSAT()
{
    return m_sat;
}

INLINE void HuC6270::ProcessCpuVramAccesses(u32 cycles)
{
    while (HasPendingCpuVramAccess() && (cycles >= 3))
    {
        if (m_transfer_delay > 0)
        {
            m_transfer_delay -= 3;
            if (m_transfer_delay > 0)
            {
                cycles -= 3;
                continue;
            }
        }

        m_transfer_delay = 0;

        if (!IsCpuVramSlotAvailable(3))
        {
            cycles -= 3;
            continue;
        }

        if (m_pending_memory_read)
            ProcessVramRead();
        else if (m_pending_memory_write)
            ProcessVramWrite();

        cycles -= 3;
    }
}

INLINE bool HuC6270::HasPendingCpuVramAccess()
{
    return m_pending_memory_read || m_pending_memory_write;
}

INLINE void HuC6270::LatchScrollY()
{
    if (m_raster_line == 0)
    {
        m_bg_counter_y = m_register[HUC6270_REG_BYR];
    }
    else
    {
        if (m_bg_scroll_y_update_pending)
        {
            m_bg_counter_y = m_register[HUC6270_REG_BYR];
            m_bg_scroll_y_update_pending = false;
        }

        m_bg_counter_y++;
    }

    m_bg_offset_y = m_bg_counter_y;
    m_increment_bg_counter_y = false;
}

INLINE bool HuC6270::CheckUpdateLatchTiming(s32 clock)
{
    if (clock < 0)
        return false;

    s32 divider = m_huc6260->GetClockDivider();
    s32 start = (clock / divider) * divider;
    s32 end = start + ((divider + 1) / 2);
    s32 hclock = CurrentHClock();
    return (hclock > start) && (hclock < end);
}

INLINE s32 HuC6270::ClocksSinceHSyncStart(s32 elapsed_cycles)
{
    s32 hclock = CurrentHClock() + elapsed_cycles;
    if (hclock >= HUC6260_LINE_LENGTH)
        hclock -= HUC6260_LINE_LENGTH;

    s32 elapsed = hclock - m_hsync_start_clock;
    if (elapsed < 0)
        elapsed += HUC6260_LINE_LENGTH;

    return elapsed;
}

INLINE bool HuC6270::IsInBgFetchWindow(s32 hclock)
{
    return !m_burst_mode && (hclock >= m_load_bg_start_clock) && (hclock < m_load_bg_end_clock) &&
        (m_vpos >= HUC6270_ACTIVE_DISPLAY_START) && (m_vpos < HUC6270_BOTTOM_BLANKING_START);
}

INLINE bool HuC6270::IsCpuVramBgSlotAllowed(s32 hclock)
{
    s32 bg_clock = hclock - m_load_bg_start_clock;
    if (bg_clock < 0)
        return false;

    s32 bg_dot = (bg_clock / m_huc6260->GetClockDivider()) - 1;
    if (bg_dot < 0)
        return false;

    switch (m_latched_mwr & 0x03)
    {
        case 0:
            return (bg_dot & 0x01) == 0;
        case 1:
        case 2:
            return ((bg_dot & 0x07) == 2) || ((bg_dot & 0x07) == 3);
        default:
            return false;
    }
}

INLINE void HuC6270::WaitForVramAccess()
{
    while (HasPendingCpuVramAccess())
        m_huc6280->StallFastCycle();
}

INLINE void HuC6270::ProcessVramRead()
{
#if !defined(GG_DISABLE_DISASSEMBLER)
    m_huc6280->CheckMemoryBreakpoints(HuC6280::HuC6280_BREAKPOINT_TYPE_VRAM, m_register[HUC6270_REG_MARR], true);
#endif
    m_read_buffer = ReadVRAM(m_register[HUC6270_REG_MARR]);
    m_register[HUC6270_REG_MARR] += k_huc6270_read_write_increment[(m_register[HUC6270_REG_CR] >> 11) & 0x03];
    m_pending_memory_read = false;
    UpdateCpuVramBusyStatus();
}

INLINE void HuC6270::ProcessVramWrite()
{
    if (m_register[HUC6270_REG_MAWR] >= 0x8000)
    {
        Debug("[PC=%04X] HuC6270 ignoring write VWR out of bounds %04X: %04X", m_huc6280->GetState()->PC->GetValue(), m_register[HUC6270_REG_MAWR], m_register[HUC6270_REG_VWR]);
    }
    else
    {
#if !defined(GG_DISABLE_DISASSEMBLER)
        m_huc6280->CheckMemoryBreakpoints(HuC6280::HuC6280_BREAKPOINT_TYPE_VRAM, m_register[HUC6270_REG_MAWR], false);
#endif
        m_vram[m_register[HUC6270_REG_MAWR] & 0x7FFF] = m_register[HUC6270_REG_VWR];
    }

    m_register[HUC6270_REG_MAWR] += k_huc6270_read_write_increment[(m_register[HUC6270_REG_CR] >> 11) & 0x03];
    m_pending_memory_write = false;
    UpdateCpuVramBusyStatus();
}

INLINE void HuC6270::UpdateCpuVramBusyStatus()
{
    if (HasPendingCpuVramAccess())
        m_status_register |= HUC6270_STATUS_BUSY;
    else
        m_status_register &= ~HUC6270_STATUS_BUSY;
}

INLINE void HuC6270::SetNoSpriteLimit(bool no_sprite_limit)
{
    m_no_sprite_limit = no_sprite_limit;
}

INLINE void HuC6270::SetSafeDefaults(bool safe_defaults)
{
    m_safe_defaults = safe_defaults;
}

INLINE int HuC6270::GetCpuVramReadDelay()
{
    int speed = m_huc6260->GetSpeed();
    assert((speed >= HuC6260::HuC6260_SPEED_5_36_MHZ) && (speed <= HuC6260::HuC6260_SPEED_10_8_MHZ));
    return k_huc6270_vram_read_delay[speed];
}

INLINE int HuC6270::GetCpuVramWriteDelay()
{
    int speed = m_huc6260->GetSpeed();
    assert((speed >= HuC6260::HuC6260_SPEED_5_36_MHZ) && (speed <= HuC6260::HuC6260_SPEED_10_8_MHZ));
    return k_huc6270_vram_write_delay[speed];
}

inline bool HuC6270::IsCpuVramSlotAvailable(s32 elapsed_cycles)
{
    s32 hclock = CurrentHClock();
    int divider = m_huc6260->GetClockDivider();
    bool in_bg_fetch = IsInBgFetchWindow(hclock);
    bool sprites_enabled = (m_latched_cr & 0x0040) != 0;
    bool access_blocked;

    if ((m_sat_transfer_pending > 0) || (m_vram_transfer_pending > 0))
    {
        m_allow_vram_access = false;
        return false;
    }

    if ((m_v_state != HuC6270_VERTICAL_STATE_VDW) || m_burst_mode ||
        (((!sprites_enabled || (m_sprite_count == 0)) && !in_bg_fetch && (m_v_state == HuC6270_VERTICAL_STATE_VDW))))
    {
        m_allow_vram_access = false;
        access_blocked = ((hclock / divider) & 0x01) != 0;
    }
    else
    {
        m_allow_vram_access = in_bg_fetch && IsCpuVramBgSlotAllowed(hclock);
        access_blocked = in_bg_fetch && !m_allow_vram_access;

        if (!access_blocked && !in_bg_fetch && sprites_enabled)
        {
            s32 clock_count = (hclock > m_load_bg_end_clock) ? (hclock - m_load_bg_end_clock) : (HUC6260_LINE_LENGTH - m_load_bg_end_clock + hclock);
            s32 dot_count = clock_count / divider;
            int dots_per_sprite;

            switch ((m_latched_mwr >> 2) & 0x03)
            {
                default:
                case 0:
                case 1:
                    dots_per_sprite = 4;
                    break;
                case 2:
                    dots_per_sprite = 8;
                    break;
                case 3:
                    dots_per_sprite = 16;
                    break;
            }

            if (dot_count < (m_sprite_count * dots_per_sprite))
                access_blocked = true;
            else
                access_blocked = ((hclock / divider) & 0x01) != 0;
        }
    }

    if (access_blocked)
        return false;

    if ((m_h_state == HuC6270_HORIZONTAL_STATE_HSW) && (ClocksSinceHSyncStart(elapsed_cycles) < DotsToClocks(8)))
        return false;

    return true;
}

INLINE u16 HuC6270::ReadVRAM(u16 address)
{
    if (address < HUC6270_VRAM_SIZE)
    {
        m_vram_openbus = m_vram[address];
        return m_vram_openbus;
    }
    else
    {
        Debug("HuC6270 VRAM read out of bounds %04X", address);
        return m_vram_openbus;
    }
}

INLINE void HuC6270::RCRIRQ()
{
    HUC6270_DEBUG("  [!] RCR IRQ\t");
    if (m_register[HUC6270_REG_CR] & HUC6270_CONTROL_SCANLINE)
    {
        if (((int)m_register[HUC6270_REG_RCR] - 64) == m_raster_line)
        {
            m_status_register |= HUC6270_STATUS_SCANLINE;
            m_huc6202->AssertIRQ1(this, true);

#if !defined(GG_DISABLE_DISASSEMBLER)
            if (m_trace_logger->IsEnabled(TRACE_VDC))
            {
                GG_Trace_Entry e = {};
                e.type = TRACE_VDC;
                e.vdc.event = TRACE_VDC_SCANLINE_IRQ;
                e.vdc.value = m_register[HUC6270_REG_RCR];
                e.vdc.chip = m_chip_id;
                m_trace_logger->TraceLog(e);
            }
#endif
        }
    }
}

INLINE void HuC6270::OverflowIRQ()
{
    if (m_register[HUC6270_REG_CR] & HUC6270_CONTROL_OVERFLOW)
    {
        m_status_register |= HUC6270_STATUS_OVERFLOW;
        m_huc6202->AssertIRQ1(this, true);

#if !defined(GG_DISABLE_DISASSEMBLER)
        if (m_trace_logger->IsEnabled(TRACE_VDC))
        {
            GG_Trace_Entry e = {};
            e.type = TRACE_VDC;
            e.vdc.event = TRACE_VDC_OVERFLOW_IRQ;
            e.vdc.chip = m_chip_id;
            m_trace_logger->TraceLog(e);
        }
#endif
    }
}

INLINE void HuC6270::SpriteCollisionIRQ()
{
    HUC6270_DEBUG("  [!] Sprite COL IRQ");
    if (m_register[HUC6270_REG_CR] & HUC6270_CONTROL_COLLISION)
    {
        m_status_register |= HUC6270_STATUS_COLLISION;
        m_huc6202->AssertIRQ1(this, true);

#if !defined(GG_DISABLE_DISASSEMBLER)
        if (m_trace_logger->IsEnabled(TRACE_VDC))
        {
            GG_Trace_Entry e = {};
            e.type = TRACE_VDC;
            e.vdc.event = TRACE_VDC_SPRITE_COLLISION_IRQ;
            e.vdc.chip = m_chip_id;
            m_trace_logger->TraceLog(e);
        }
#endif
    }
}

#endif /* HUC6270_INLINE_H */