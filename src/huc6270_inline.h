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

inline u16 HuC6270::Clock()
{
    SATTransfer();

    u16 pixel = 0x100;

    if (m_active_line &&
        (m_v_state == HuC6270_VERTICAL_STATE_VDW) &&
        (m_h_state == HuC6270_HORIZONTAL_STATE_HDW_1 || m_h_state == HuC6270_HORIZONTAL_STATE_HDW_2))
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

    return pixel;
}

inline u8 HuC6270::ReadRegister(u16 address)
{
    switch (address & 0x03)
    {
        // Status register
        case 0:
        {
            u8 ret = m_status_register & 0x7F;
            m_huc6280->AssertIRQ1(false);
            m_status_register &= 0x40;
            return ret;
        }
        // Data register (LSB)
        case 2:
        {
            if (m_address_register != HUC6270_REG_VRR)
            {
                Debug("[PC=%04X] HuC6270 invalid data register (LSB) read: %02X", m_huc6280->GetState()->PC->GetValue(), m_address_register);
            }
            return m_read_buffer & 0xFF;
        }
        // Data register (MSB)
        case 3:
        {
#if !defined(GG_DISABLE_DISASSEMBLER)
            m_huc6280->CheckMemoryBreakpoints(HuC6280::HuC6280_BREAKPOINT_TYPE_HUC6270_REGISTER, m_address_register, true);
#endif
            u8 ret = m_read_buffer >> 8;

            if (m_address_register == HUC6270_REG_VRR)
            {
#if !defined(GG_DISABLE_DISASSEMBLER)
                m_huc6280->CheckMemoryBreakpoints(HuC6280::HuC6280_BREAKPOINT_TYPE_VRAM, m_register[HUC6270_REG_MARR], true);
#endif
                m_read_buffer = m_vram[m_register[HUC6270_REG_MARR] & 0x7FFF];
                m_register[HUC6270_REG_MARR] += k_huc6270_read_write_increment[(m_register[HUC6270_REG_CR] >> 11) & 0x03];
            }
            else
            {
                Debug("[PC=%04X] HuC6270 invalid data register (MSB) read: %02X", m_huc6280->GetState()->PC->GetValue(), m_address_register);
            }

            return ret;
        }
        default:
        {
            Debug("[PC=%04X] HuC6270 invalid register read at %06X, reg=%d", m_huc6280->GetState()->PC->GetValue(), address, address & 0x03);
            return 0x00;
        }
    }
}

inline void HuC6270::WriteRegister(u16 address, u8 value)
{
    switch (address & 0x03)
    {
        // Address register
        case 0:
            m_address_register = value & 0x1F;
            break;
        // Data register (LSB)
        case 2:
        // Data register (MSB)
        case 3:
        {
#if !defined(GG_DISABLE_DISASSEMBLER)
            m_huc6280->CheckMemoryBreakpoints(HuC6280::HuC6280_BREAKPOINT_TYPE_HUC6270_REGISTER, m_address_register, false);
#endif

            bool msb = address & 0x01;

            if (m_address_register > 0x13)
            {
                Debug("[PC=%04X] HuC6270 INVALID write to data register (%s) %02X: %04X", m_huc6280->GetState()->PC->GetValue(), msb ? "MSB" : "LSB", value, m_address_register);
                return;
            }

            if (msb)
                m_register[m_address_register] = (m_register[m_address_register] & 0x00FF) | (value << 8);
            else
                m_register[m_address_register] = (m_register[m_address_register] & 0xFF00) | value;

            m_register[m_address_register] &= k_register_mask[m_address_register];

            switch (m_address_register)
            {
                // 0x01
                case HUC6270_REG_MARR:
                    if (msb)
                    {
                        m_read_buffer = m_vram[m_register[HUC6270_REG_MARR] & 0x7FFF];
                        m_register[HUC6270_REG_MARR] += k_huc6270_read_write_increment[(m_register[HUC6270_REG_CR] >> 11) & 0x03];
                    }
                    break;
                // 0x02
                case HUC6270_REG_VWR:
                    if (msb)
                    {
                        if (m_register[HUC6270_REG_MAWR] >= 0x8000)
                        {
                            Debug("[PC=%04X] HuC6270 ignoring write VWR out of bounds (%s) %04X: %02X", m_huc6280->GetState()->PC->GetValue(), msb ? "MSB" : "LSB", m_register[HUC6270_REG_MAWR], value);
                        }
                        else
                        {
#if !defined(GG_DISABLE_DISASSEMBLER)
                            m_huc6280->CheckMemoryBreakpoints(HuC6280::HuC6280_BREAKPOINT_TYPE_VRAM, m_register[HUC6270_REG_MAWR], false);
#endif
                            m_vram[m_register[HUC6270_REG_MAWR] & 0x7FFF] = m_register[HUC6270_REG_VWR];
                        }

                        m_register[HUC6270_REG_MAWR] += k_huc6270_read_write_increment[(m_register[HUC6270_REG_CR] >> 11) & 0x03];
                    }
                    break;
                // 0x07
                case HUC6270_REG_BXR:
                    HUC6270_DEBUG("**** BXR Set");
                    break;
                // 0x08
                case HUC6270_REG_BYR:
                    m_bg_counter_y = m_register[HUC6270_REG_BYR];
                    HUC6270_DEBUG("**** BYR Set");
                    break;
                // 0x12
                case HUC6270_REG_LENR:
                    if (msb)
                    {
                        s16 src_increment = m_register[HUC6270_REG_DCR] & 0x02 ? -1 : 1;
                        s16 dest_increment = m_register[HUC6270_REG_DCR] & 0x04 ? -1 : 1;

                        do
                        {
                            if (m_register[HUC6270_REG_DESR] >= 0x8000)
                            {
                                Debug("[PC=%04X] HuC6270 ignoring write VRAM-DMA out of bounds: %04X", m_huc6280->GetState()->PC->GetValue(), m_register[HUC6270_REG_DESR], value);
                            }
                            else
                            {
                                m_vram[m_register[HUC6270_REG_DESR] & 0x7FFF] = m_vram[m_register[HUC6270_REG_SOUR] & 0x7FFF];
                            }

                            m_register[HUC6270_REG_SOUR] += src_increment;
                            m_register[HUC6270_REG_DESR] += dest_increment;
                            m_register[HUC6270_REG_LENR]--;
                        }
                        while (m_register[HUC6270_REG_LENR] != 0xFFFF);

                        m_status_register |= HUC6270_STATUS_VRAM_END;
                        if (m_register[HUC6270_REG_DCR] & 0x02)
                        {
                            m_huc6280->AssertIRQ1(true);
                        }
                    }
                    break;
                // 0x13
                case HUC6270_REG_DVSSR:
                    m_trigger_sat_transfer = true;
                    break;
            }
            break;
        }
        default:
            Debug("[PC=%04X] HuC6270 invalid write at %06X, value=%02X", m_huc6280->GetState()->PC->GetValue(), address, value);
            break;
    }
}

inline void HuC6270::SetHSync(bool active)
{
    // High to low
    if (!active)
    {
        HUC6270_DEBUG("HSYNC H to L");
    }
    // Low to high
    else
    {
        m_h_state = HuC6270_HORIZONTAL_STATE_HSW;
        m_clocks_to_next_h_state = 8;

        HUC6270_DEBUG("HSYNC L to H");
    }
}

inline void HuC6270::SetVSync(bool active)
{
    // High to low
    if (!active)
    {
        m_latched_mwr = m_register[HUC6270_REG_MWR];
        m_latched_vds = HUC6270_VAR_VDS;
        m_latched_vdw = HUC6270_VAR_VDW;
        m_latched_vcr = HUC6270_VAR_VCR;
        m_latched_vsw = HUC6270_VAR_VSW;

        m_v_state = HuC6270_VERTICAL_STATE_VSW;
        m_lines_to_next_v_state = m_latched_vsw + 1;

        m_increment_bg_counter_y = false;

        HUC6270_DEBUG("+++ VSYNC H to L");
    }
    // Low to high
    else
    {
        HUC6270_DEBUG("+++ VSYNC L to H");
    }
}

inline void HuC6270::NextVerticalState()
{
    m_v_state = (m_v_state + 1) % HuC6270_VERTICAL_STATE_COUNT;

    switch (m_v_state)
    {
        case HuC6270_VERTICAL_STATE_VDS:
            m_lines_to_next_v_state = m_latched_vds + 2;
            HUC6270_DEBUG("+ VDS");
            break;
        case HuC6270_VERTICAL_STATE_VDW:
            m_lines_to_next_v_state = m_latched_vdw + 1;
            m_raster_line = 0;
            m_vblank_triggered = false;
            HUC6270_DEBUG("+ VDW");
            break;
        case HuC6270_VERTICAL_STATE_VCR:
            m_lines_to_next_v_state = m_latched_vcr;
            HUC6270_DEBUG("+ VCR");
            break;
        case HuC6270_VERTICAL_STATE_VSW:
            m_lines_to_next_v_state = m_latched_vsw + 1;
            m_latched_mwr = m_register[HUC6270_REG_MWR];
            m_latched_vds = HUC6270_VAR_VDS;
            m_latched_vdw = HUC6270_VAR_VDW;
            m_latched_vcr = HUC6270_VAR_VCR;
            m_latched_vsw = HUC6270_VAR_VSW;
            HUC6270_DEBUG(">>>\nVSW Start!  VSW: %d, VDS: %d, VDW: %d, VCR: %d", m_latched_vsw, m_latched_vds, m_latched_vdw, m_latched_vcr);
            break;
    }
}

inline void HuC6270::NextHorizontalState()
{
    m_h_state = (m_h_state + 1) % HuC6270_HORIZONTAL_STATE_COUNT;

    switch (m_h_state)
    {
        case HuC6270_HORIZONTAL_STATE_HDS_1:
            m_line_buffer_index = 0;
            //HUC6270_DEBUG("------ hpos reset: %d", m_hpos);
            m_hpos = 0;
            m_vpos = (m_vpos + 1) % 263;
            m_active_line = (m_raster_line < 240);
            m_latched_hds = HUC6270_VAR_HDS;
            m_latched_hdw = HUC6270_VAR_HDW;
            m_latched_hde = HUC6270_VAR_HDE;
            m_latched_hsw = HUC6270_VAR_HSW;
            m_latched_cr = HUC6270_VAR_CR;
            m_clocks_to_next_h_state = ClocksToBYRLatch();
            HUC6270_DEBUG(">>>\nHDS Start!  HSW: %d, HDW: %d, HDW: %d, HDE: %d", m_latched_hsw, m_latched_hds, m_latched_hdw, m_latched_hde);
            HUC6270_DEBUG("HDS 1");
            break;
        case HuC6270_HORIZONTAL_STATE_HDS_2:
            m_clocks_to_next_h_state = ClocksToBXRLatch();

            if (m_increment_bg_counter_y)
            {
                m_increment_bg_counter_y = false;
                if(m_raster_line == 0)
                    m_bg_counter_y = m_register[HUC6270_REG_BYR];
                else
                    m_bg_counter_y++;
            }
            m_bg_offset_y = m_bg_counter_y;

            HUC6270_DEBUG("HDS 2");
            break;
        case HuC6270_HORIZONTAL_STATE_HDS_3:
            m_clocks_to_next_h_state = ((m_latched_hds + 1) << 3) - ClocksToBYRLatch() - ClocksToBXRLatch();
            assert(m_clocks_to_next_h_state > 0);
            m_latched_bxr = m_register[HUC6270_REG_BXR];
            HUC6270_DEBUG("HDS 3");
            break;
        case HuC6270_HORIZONTAL_STATE_HDW_1:
            m_clocks_to_next_h_state = ((m_latched_hdw + 1) << 3) - HUC6270_RCR_IRQ_CYCLES_BEFORE_HDE;
            if ((m_v_state != HuC6270_VERTICAL_STATE_VDW) && !m_vblank_triggered)
            {
                m_vblank_triggered = true;
                VBlankIRQ();
            }
            if (m_v_state == HuC6270_VERTICAL_STATE_VDW)
                RenderLine();
            HUC6270_DEBUG("HDW 1");
            break;
        case HuC6270_HORIZONTAL_STATE_HDW_2:
            m_clocks_to_next_h_state = HUC6270_RCR_IRQ_CYCLES_BEFORE_HDE;

            m_raster_line++;
            m_increment_bg_counter_y = true;

            m_lines_to_next_v_state--;
            while (m_lines_to_next_v_state <= 0)
                NextVerticalState();

            if (m_v_state == HuC6270_VERTICAL_STATE_VDW)
                FetchSprites();

            RCRIRQ();

            HUC6270_DEBUG("HDW 2");
            break;
        case HuC6270_HORIZONTAL_STATE_HDE:
            m_clocks_to_next_h_state = (m_latched_hde + 1) << 3;
            HUC6270_DEBUG("HDE");
            break;
        case HuC6270_HORIZONTAL_STATE_HSW:
            m_clocks_to_next_h_state = (m_latched_hsw + 1) << 3;
            HUC6270_DEBUG("HSW");
            break;
    }
}

inline void HuC6270::VBlankIRQ()
{
    if (m_register[HUC6270_REG_CR] & HUC6270_CONTROL_VBLANK)
    {
        HUC6270_DEBUG("VBlank IRQ");
        m_status_register |= HUC6270_STATUS_VBLANK;
        m_huc6280->AssertIRQ1(true);
    }

    if (m_trigger_sat_transfer || (m_register[HUC6270_REG_DCR] & 0x10))
    {
        m_trigger_sat_transfer = false;

        m_sat_transfer_pending = 1024;
        m_status_register |= HUC6270_STATUS_BUSY;
    }
}

inline void HuC6270::RenderLine()
{
    int width = std::min(1024, (m_latched_hdw + 1) << 3);

    if((m_latched_cr & 0x80) == 0)
    {
        u16 color = 0x100;

        for (int i = 0; i < width; i++)
        {
            m_line_buffer[i] = color;
        }
    }

    if((m_latched_cr & 0x80) != 0)
    {
        RenderBackground(width);
    }

    if((m_latched_cr & 0x40) != 0)
    {
        RenderSprites(width);
    }
}

inline void HuC6270::SATTransfer()
{
    if (m_sat_transfer_pending > 0)
    {
        m_sat_transfer_pending--;

        if ((m_sat_transfer_pending & 3) == 0)
        {
            u16 satb = m_register[HUC6270_REG_DVSSR];
            int i = 255 - (m_sat_transfer_pending >> 2);
            m_sat[i] = m_vram[(satb + i) & 0x7FFF];
        }

        if (m_sat_transfer_pending == 0)
        {
            m_status_register &= ~HUC6270_STATUS_BUSY;

            if (m_register[HUC6270_REG_DCR] & 0x01)
            {
                m_status_register |= HUC6270_STATUS_SAT_END;
                m_huc6280->AssertIRQ1(true);
            }
        }
    }
}

inline void HuC6270::RCRIRQ()
{
    if (m_register[HUC6270_REG_CR] & HUC6270_CONTROL_SCANLINE)
    {
        if (((int)m_register[HUC6270_REG_RCR] - 64) == m_raster_line)
        {
            HUC6270_DEBUG("RCR IRQ");
            m_status_register |= HUC6270_STATUS_SCANLINE;
            m_huc6280->AssertIRQ1(true);
        }
    }
}

inline void HuC6270::OverflowIRQ()
{
    if (m_register[HUC6270_REG_CR] & HUC6270_CONTROL_OVERFLOW)
    {
        m_status_register |= HUC6270_STATUS_OVERFLOW;
        m_huc6280->AssertIRQ1(true);
    }
}

inline void HuC6270::SpriteCollisionIRQ()
{
    if (m_register[HUC6270_REG_CR] & HUC6270_CONTROL_COLLISION)
    {
        m_status_register |= HUC6270_STATUS_COLLISION;
        m_huc6280->AssertIRQ1(true);
    }
}

inline int HuC6270::ClocksToBYRLatch()
{
    int ret = 1;
    if (m_latched_hds > 2)
        ret += ((m_latched_hds + 1) << 3) - 24 + 2;
    return ret;
}

inline int HuC6270::ClocksToBXRLatch()
{
    int ret = 2;
    if(m_latched_hds > 2)
        ret = 1;
    return ret;
}

inline HuC6270::HuC6270_State* HuC6270::GetState()
{
    return &m_state;
}

inline u16* HuC6270::GetVRAM()
{
    return m_vram;
}

inline u16* HuC6270::GetSAT()
{
    return m_sat;
}

inline void HuC6270::SetNoSpriteLimit(bool no_sprite_limit)
{
    m_no_sprite_limit = no_sprite_limit;
}

#endif /* HUC6270_INLINE_H */