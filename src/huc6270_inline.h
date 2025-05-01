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

#include "huc6270.h"
#include "huc6260.h"
#include "huc6280.h"

INLINE u32 HuC6270::Clock()
{
    if (m_sat_transfer_pending > 0)
        SATTransfer();
    if (m_vram_transfer_pending > 0)
        VRAMTransfer();

    u32 pixel = 0x10000;

    //bool active_line = ((m_vpos >= 14) && (m_vpos < 256));

    if (m_active_line && (m_v_state == HuC6270_VERTICAL_STATE_VDW) && (m_h_state == HuC6270_HORIZONTAL_STATE_HDW))
    //if (active_line && (m_v_state == HuC6270_VERTICAL_STATE_VDW) && (m_h_state == HuC6270_HORIZONTAL_STATE_HDW))
    {
        if (m_line_buffer_index < 512)
        {
            pixel = m_line_buffer[m_line_buffer_index];
        }
        else
        {
            Debug("HuC6270 line buffer overflow %d", m_line_buffer_index);
        }
        m_line_buffer_index++;
    }

    m_hpos++;

    m_clocks_to_next_h_state--;
    while (m_clocks_to_next_h_state == 0)
        NextHorizontalState();

    if (m_clocks_to_next_event > 0)
        LineEvents();

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

INLINE void HuC6270::SetNoSpriteLimit(bool no_sprite_limit)
{
    m_no_sprite_limit = no_sprite_limit;
}

INLINE void HuC6270::RCRIRQ()
{
    HUC6270_DEBUG("  [?] RCR IRQ\t");
    if (m_register[HUC6270_REG_CR] & HUC6270_CONTROL_SCANLINE)
    {
        if (((int)m_register[HUC6270_REG_RCR] - 64) == m_raster_line)
        {
            m_status_register |= HUC6270_STATUS_SCANLINE;
            m_huc6280->AssertIRQ1(true);
        }
    }
}

INLINE void HuC6270::OverflowIRQ()
{
    HUC6270_DEBUG("  [?] SPRITE OV IRQ");
    if (m_register[HUC6270_REG_CR] & HUC6270_CONTROL_OVERFLOW)
    {
        m_status_register |= HUC6270_STATUS_OVERFLOW;
        m_huc6280->AssertIRQ1(true);
    }
}

INLINE void HuC6270::SpriteCollisionIRQ()
{
    if (m_register[HUC6270_REG_CR] & HUC6270_CONTROL_COLLISION)
    {
        m_status_register |= HUC6270_STATUS_COLLISION;
        m_huc6280->AssertIRQ1(true);
    }
}

#endif /* HUC6270_INLINE_H */