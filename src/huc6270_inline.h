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
    if (m_sat_transfer_pending > 0)
    {
        m_sat_transfer_pending--;

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

#endif /* HUC6270_INLINE_H */