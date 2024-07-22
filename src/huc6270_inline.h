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

inline u16 HuC6270::Clock()
{
    return 0;
}

inline u8 HuC6270::ReadRegister(u32 address)
{
    switch (address & 0x03)
    {
        case 0:
            // Status register
            Debug("HuC6270 read status register");
            return m_status_register & 0x7F;
            break;
        case 2:
            // Data register (LSB)
            return ReadDataRegister(false);
            break;
        case 3:
            // Data register (MSB)
            return ReadDataRegister(true);
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
        case 0:
            // Address register
            m_address_register = value & 0x1F;
            Debug("HuC6270 write address register: %02X", m_address_register);
            break;
        case 2:
            // Data register (LSB)
            WriteDataRegister(value, false);
            break;
        case 3:
            // Data register (MSB)
            WriteDataRegister(value, true);
            break;
        default:
            Debug("HuC6270 invalid write at %06X, value=%02X", address, value);
            break;
    }
}

inline void HuC6270::DirectWrite(u32 address, u8 value)
{
    switch (address)
    {
        case 0x1FE000:
            Debug("HuC6270 direct write (ST0) at %06X, value=%02X", address, value);
            break;
        case 0x1FE002:
            Debug("HuC6270 direct write (ST1) at %06X, value=%02X", address, value);
            break;
        case 0x1FE003:
            Debug("HuC6270 direct write (ST2) at %06X, value=%02X", address, value);
            break;
        default:
            Debug("HuC6270 invalid direct write at %06X, value=%02X", address, value);
            break;
    }

    WriteRegister(address, value);
}

inline u8 HuC6270::ReadDataRegister(bool msb)
{
    if (m_address_register == 0x02)
    {
        // VRR
        Debug("HuC6270 read (%s) VRR: %02X", msb ? "MSB" : "LSB", m_register[0x02]);
        m_register[0x02] = m_vram[m_register[0x01] & 0x7FFF];
        return msb ? m_register[0x02] >> 8 : m_register[0x02] & 0xFF;
    }
    else
    {
        Debug("HuC6270 invalid read data register: %02X", m_address_register);
        return m_register[0x02] & 0xFF;
    }
}

inline void HuC6270::WriteDataRegister(u8 value, bool msb)
{
    if (m_address_register <= 0x13 && m_address_register != 0x03 && m_address_register != 0x04)
    {
        m_register[m_address_register] = msb ? (m_register[m_address_register] & 0x00FF) | (value << 8) : (m_register[m_address_register] & 0xFF00) | value;
        m_register[m_address_register] &= k_register_mask[m_address_register];
    }

    switch (m_address_register)
    {
        case 0x00:
            // MAWR
            Debug("HuC6270 write (%s) MAWR %02X: %04X", msb ? "MSB" : "LSB", value, m_register[m_address_register]);
            break;
        case 0x01:
            // MARR
            Debug("HuC6270 write (%s) MARR %02X: %04X", msb ? "MSB" : "LSB", value, m_register[m_address_register]);
            break;
        case 0x02:
            // VWR
            m_vram[m_register[0x00] & 0x7FFF] = m_register[0x02];
            Debug("HuC6270 write (%s) VWR %02X: %04X", msb ? "MSB" : "LSB", value, m_register[m_address_register]);
            break;
        case 0x05:
            // CR
            Debug("HuC6270 write (%s) CR %02X: %04X", msb ? "MSB" : "LSB", value, m_register[m_address_register]);
            break;
        case 0x06:
            // RCR
            Debug("HuC6270 write (%s) RCR %02X: %04X", msb ? "MSB" : "LSB", value, m_register[m_address_register]);
            break;
        case 0x07:
            // BXR
            Debug("HuC6270 write (%s) BXR %02X: %04X", msb ? "MSB" : "LSB", value, m_register[m_address_register]);
            break;
        case 0x08:
            // BYR
            Debug("HuC6270 write (%s) BYR %02X: %04X", msb ? "MSB" : "LSB", value, m_register[m_address_register]);
            break;
        case 0x09:
            // MWR
            Debug("HuC6270 write (%s) MWR %02X: %04X", msb ? "MSB" : "LSB", value, m_register[m_address_register]);
            break;
        case 0x0A:
            // HSR
            Debug("HuC6270 write (%s) HSR %02X: %04X", msb ? "MSB" : "LSB", value, m_register[m_address_register]);
            break;
        case 0x0B:
            // HDR
            Debug("HuC6270 write (%s) HDR %02X: %04X", msb ? "MSB" : "LSB", value, m_register[m_address_register]);
            break;
        case 0x0C:
            // VPR
            Debug("HuC6270 write (%s) VPR %02X: %04X", msb ? "MSB" : "LSB", value, m_register[m_address_register]);
            break;
        case 0x0D:
            // VDW
            Debug("HuC6270 write (%s) VDW %02X: %04X", msb ? "MSB" : "LSB", value, m_register[m_address_register]);
            break;
        case 0x0E:
            // VCR
            Debug("HuC6270 write (%s) VCR %02X: %04X", msb ? "MSB" : "LSB", value, m_register[m_address_register]);
            break;
        case 0x0F:
            // DCR
            Debug("HuC6270 write (%s) DCR %02X: %04X", msb ? "MSB" : "LSB", value, m_register[m_address_register]);
            break;
        case 0x10:
            // SOUR
            Debug("HuC6270 write (%s) SOUR %02X: %04X", msb ? "MSB" : "LSB", value, m_register[m_address_register]);
            break;
        case 0x11:
            // DESR
            Debug("HuC6270 write (%s) DESR %02X: %04X", msb ? "MSB" : "LSB", value, m_register[m_address_register]);
            break;
        case 0x12:
            // LENR
            Debug("HuC6270 write (%s) LENR %02X: %04X", msb ? "MSB" : "LSB", value, m_register[m_address_register]);
            break;
        case 0x13:
            // DVSSR
            Debug("HuC6270 write (%s) DVSSR %02X: %04X", msb ? "MSB" : "LSB", value, m_register[m_address_register]);
            break;
        default:
            Debug("HuC6270 invalid write data register %02X: %02X", m_address_register, value);
            break;
    }
}

#endif /* HUC6270_INLINE_H */