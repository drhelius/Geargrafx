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

inline bool HuC6270::Clock(u8* frame_buffer)
{
    m_frame_buffer = frame_buffer;
    bool frame_ready = false;

    if (m_vpos < HUC6270_ACTIVE_DISPLAY_START)
    {
        //Debug("HuC6270 during top blanking");
    }
    else if (m_vpos < HUC6270_BOTTOM_BLANKING_START)
    {
        //Debug("HuC6270 during active display");
    }
    else if (m_vpos < HUC6270_SYNC_START)
    {
        //Debug("HuC6270 during bottom blanking");
    }
    else
    {
        //Debug("HuC6270 during sync");
    }

    if (m_hpos > 256)
    {
        //Debug("HuC6270 during horizontal blanking");
    }

    m_hpos++;

    if (m_hpos > 341)
    {
        m_hpos = 0;
        m_vpos++;

        if (m_vpos < GG_MAX_RESOLUTION_HEIGHT)
        {
            RenderLine(m_vpos - 1);

            if (m_register[HUC6270_REG_CR] & HUC6270_CONTROL_SCANLINE)
            {
                if (m_vpos == m_register[HUC6270_REG_RCR])
                {
                    m_status_register |= HUC6270_STATUS_SCANLINE;
                    m_huc6280->AssertIRQ1(true);
                }
            }
        }

        if (m_vpos > HUC6270_LINES)
        {
            frame_ready = true;
            m_vpos = 0;
            if (m_register[HUC6270_REG_CR] & HUC6270_CONTROL_VBLANK)
            {
                m_status_register |= HUC6270_STATUS_VBLANK;
                m_huc6280->AssertIRQ1(true);
            }

            if (m_trigger_sat_transfer || m_auto_sat_transfer)
            {
                m_trigger_sat_transfer = false;
                m_auto_sat_transfer = m_register[HUC6270_REG_DCR] & 0x10;

                u16 satb = m_register[HUC6270_REG_DVSSR] & 0x7FFF;

                for (int i = 0; i < HUC6270_SAT_SIZE; i++)
                {
                    m_sat[i] = m_vram[satb + i] & 0x7FFF;
                }

                m_status_register |= HUC6270_STATUS_SAT_END;
                if (m_register[HUC6270_REG_DCR] & 0x01)
                {
                    m_huc6280->AssertIRQ1(true);
                }
            }
        }
    }

    return frame_ready;
}

inline u8 HuC6270::ReadRegister(u32 address)
{
    switch (address & 0x03)
    {
        // Status register
        case 0:
        {
            // Debug("HuC6270 read status register");
            u8 ret = m_status_register & 0x7F;
            m_huc6280->AssertIRQ1(false);
            m_status_register &= 0x40;
            return ret;
            break;
        }
        // Data register (LSB)
        case 2:
            // 0x02
            if (m_address_register == HUC6270_REG_VRR)
            {
                return m_read_buffer & 0xFF;
            }
            else
            {
                Debug("HuC6270 invalid read data register: %02X", m_address_register);
                return m_register[HUC6270_REG_VWR] & 0xFF;
            }
            break;
        // Data register (MSB)
        case 3:
            // 0x02
            if (m_address_register == HUC6270_REG_VRR)
            {
                u8 ret = m_read_buffer >> 8;
                int increment = k_read_write_increment[(m_register[HUC6270_REG_CR] >> 11) & 0x03];
                m_register[HUC6270_REG_MARR] = m_register[HUC6270_REG_MARR] + increment;
                m_read_buffer = m_vram[m_register[HUC6270_REG_MARR] & 0x7FFF];
                // Debug("HuC6270 MARR inncremented %02X to %04X", increment, m_register[HUC6270_REG_MARR]);
                return ret;
            }
            else
            {
                Debug("HuC6270 invalid read data register: %02X", m_address_register);
                return m_register[HUC6270_REG_VWR] & 0xFF;
            }
            break;
        default:
            Debug("HuC6270 invalid read at %06X", address);
            return 0x00;
    }
}

inline void HuC6270::WriteRegister(u32 address, u8 value)
{
    switch (address & 0x03)
    {
        // Address register
        case 0:
            m_address_register = value & 0x1F;
            // Debug("HuC6270 write address register: %02X", m_address_register);
            break;
        // Data register (LSB)
        case 2:
        // Data register (MSB)
        case 3:
        {
            bool msb = address & 0x01;

            if (msb)
                m_register[m_address_register] = (m_register[m_address_register] & 0x00FF) | (value << 8);
            else
                m_register[m_address_register] = (m_register[m_address_register] & 0xFF00) | value;

            m_register[m_address_register] &= k_register_mask[m_address_register];

            switch (m_address_register)
            {
                // 0x01
                case HUC6270_REG_MARR:
                    // Debug("HuC6270 write MARR (%s) %02X: %04X", msb ? "MSB" : "LSB", value, m_register[m_address_register]);
                    if (msb)
                        m_read_buffer = m_vram[m_register[HUC6270_REG_MARR] & 0x7FFF];
                    break;
                // 0x02
                case HUC6270_REG_VWR:
                    // Debug("HuC6270 write VWR (%s) %02X: %04X", msb ? "MSB" : "LSB", value, m_register[m_address_register]);
                    if (msb)
                    {
                        if (m_register[HUC6270_REG_MAWR] >= 0x8000)
                        {
                            Debug("HuC6270 ignoring write VWR out of bounds (%s) %04X: %02X", msb ? "MSB" : "LSB", m_register[HUC6270_REG_MAWR], value);
                        }
                        else
                        {
                            m_vram[m_register[HUC6270_REG_MAWR] & 0x7FFF] = m_register[HUC6270_REG_VWR];
                        }

                        u16 increment = k_read_write_increment[(m_register[HUC6270_REG_CR] >> 11) & 0x03];
                        m_register[HUC6270_REG_MAWR] = m_register[HUC6270_REG_MAWR] + increment;
                    }
                    break;
                // 0x12
                case HUC6270_REG_LENR:
                    // Debug("HuC6270 write LENR (%s) %02X: %04X", msb ? "MSB" : "LSB", value, m_register[m_address_register]);
                    if (msb)
                    {
                        s16 src_increment = m_register[HUC6270_REG_DCR] & 0x02 ? -1 : 1;
                        s16 dest_increment = m_register[HUC6270_REG_DCR] & 0x04 ? -1 : 1;

                        do
                        {
                            if (m_register[HUC6270_REG_DESR] >= 0x8000)
                            {
                                Debug("HuC6270 ignoring write VRAM-DMA out of bounds: %04X", m_register[HUC6270_REG_DESR], value);
                            }
                            else
                            {
                                m_vram[m_register[HUC6270_REG_DESR] & 0x7FFF] = m_vram[m_register[HUC6270_REG_SOUR] & 0x7FFF];
                            }

                            m_register[HUC6270_REG_SOUR] += src_increment;
                            m_register[HUC6270_REG_DESR] += dest_increment;
                            m_register[HUC6270_REG_LENR]--;
                        }
                        while (m_register[HUC6270_REG_LENR]);

                        m_status_register |= HUC6270_STATUS_VRAM_END;
                        if (m_register[HUC6270_REG_DCR] & 0x02)
                        {
                            m_huc6280->AssertIRQ1(true);
                        }
                    }
                    break;
                // 0x13
                case HUC6270_REG_DVSSR:
                    // Debug("HuC6270 write DVSSR (%s) %02X: %04X", msb ? "MSB" : "LSB", value, m_register[m_address_register]);
                    m_trigger_sat_transfer = true;
                    break;
            }
            break;
        }
        default:
            Debug("HuC6270 invalid write at %06X, value=%02X", address, value);
            break;
    }
}

#endif /* HUC6270_INLINE_H */