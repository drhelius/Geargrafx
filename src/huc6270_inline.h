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

inline u8 HuC6270::ReadRegister(u32 address)
{
    Debug("HuC6270 read at %06X", address);
    switch (address & 0x03)
    {
        case 0:
            // Status register
            Debug("HuC6270 read status register");
            break;
        case 2:
            // Data register (LSB)
            Debug("HuC6270 read data register (LSB)");
            break;
        case 3:
            // Data register (MSB)
            Debug("HuC6270 read data register (MSB)");
            break;
        default:
            Debug("HuC6270 invalid read at %06X", address);
            return 0x00;
    }
}

inline void HuC6270::WriteRegister(u32 address, u8 value)
{
    Debug("HuC6270 write at %06X, value=%02X", address, value);
    switch (address & 0x03)
    {
        case 0:
            // Address register
            Debug("HuC6270 write address register: %02X", value);
            break;
        case 2:
            // Data register (LSB)
            Debug("HuC6270 write data register (LSB): %02X", value);
            break;
        case 3:
            // Data register (MSB)
            Debug("HuC6270 write data register (MSB): %02X", value);
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

#endif /* HUC6270_INLINE_H */