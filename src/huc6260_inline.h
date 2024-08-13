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

#ifndef HUC6260_INLINE_H
#define HUC6260_INLINE_H

#include "huc6260.h"
#include "huc6270.h"

inline u8 HuC6260::ReadRegister(u32 address)
{
    u8 ret = 0xFF;

    switch (address & 0x07)
    {
        case 4:
            // Color table data LSB
            ret = m_color_table[m_color_table_address] & 0xFF;
            break;
        case 5:
            // Color table data MSB
            ret = 0xFE | ((m_color_table[m_color_table_address] >> 8) & 0x01);
            m_color_table_address = (m_color_table_address + 1) & 0x01FF;
            break;
    }

    return ret;
}

inline void HuC6260::WriteRegister(u32 address, u8 value)
{
    switch (address & 0x07)
    {
        case 0:
            // Control register
            m_control_register = value;
            m_speed = m_control_register & 0x03;
            m_blur = (m_control_register >> 2) & 0x01;
            m_black_and_white = IsSetBit(m_control_register, 7);

            switch (m_speed)
            {
                case 0:
                    Debug("HuC6260 Speed: 5.36 MHz");
                    m_clock_divider = 4;
                    break;
                case 1:
                    Debug("HuC6260 Speed: 7.16 MHz");
                    m_clock_divider = 3;
                    break;
                default:
                    Debug("HuC6260 Speed: 10.8 MHz");
                    m_clock_divider = 2;
                    break;
            }
            break;
        case 2:
            // Color table address LSB
            m_color_table_address = (m_color_table_address & 0x0100) | value;
            break;
        case 3:
            // Color table address MSB
            m_color_table_address = (m_color_table_address & 0x00FF) | ((value & 0x01) << 8);
            break;
        case 4:
            // Color table data LSB
            m_color_table[m_color_table_address] = (m_color_table[m_color_table_address] & 0x0100) | value;
            break;
        case 5:
            // Color table data MSB
            m_color_table[m_color_table_address] = (m_color_table[m_color_table_address] & 0x00FF) | ((value & 0x01) << 8);
            m_color_table_address = (m_color_table_address + 1) & 0x01FF;
            break;
        default:
            // Not used
            Debug("HuC6260 Write unused register");
            break;
    }
}

#endif /* HUC6260_INLINE_H */