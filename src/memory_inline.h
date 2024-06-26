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

#ifndef MEMORY_INLINE_H
#define	MEMORY_INLINE_H

#include "memory.h"
#include "cartridge.h"

inline u8 Memory::Read(u16 address)
{
    u8 mpr_index = (address >> 13) & 0x07;
    u8 mpr_value = m_mpr[mpr_index];
    u32 physical_address = (mpr_value << 13) | (address & 0x1FFF);

    if (mpr_value < 0x80)
    {
        // HuCard ROM
        u8* rom = m_cartridge->GetROM();
        return rom[physical_address];
    }
    else if (mpr_value < 0xF8)
    {
        // Unused
        Debug("Unused read at %06X", physical_address);
        return 0xFF;
    }
    else if (mpr_value < 0xFC)
    {
        // Work RAM
        return m_wram[physical_address & 0x1FFF];
    }
    else if (mpr_value < 0xFF)
    {
        // Unused
        Debug("Unused read at %06X", physical_address);
        return 0xFF;
    }
    else
    {
        // Hardware Page
        switch (physical_address & 0x001C00)
        {
            case 0x0000:
                // HuC6270
                Debug("HuC6270 read at %06X", physical_address);
                return 0xFF;
            case 0x0400:
                // HuC6260
                Debug("HuC6260 read at %06X", physical_address);
                return 0xFF;
            case 0x0800:
                // PSG
                Debug("PSG read at %06X", physical_address);
                return 0xFF;
            case 0x0C00:
                // Timer
                Debug("Timer read at %06X", physical_address);
                return 0xFF;
            case 0x1000:
                // I/O
                Debug("I/O read at %06X", physical_address);
                return 0xFF;
            case 0x1400:
                // CD-ROM
                Debug("CD-ROM read at %06X", physical_address);
                return 0xFF;
            case 0x1800:
                // Unused
                Debug("Unused read at %06X", physical_address);
                return 0xFF;
            case 0x1C00:
                // Unused
                Debug("Unused read at %06X", physical_address);
                return 0xFF;
            default:
                return 0xFF;
        }
    }
}

inline void Memory::Write(u16 address, u8 value)
{
    u8 mpr_index = (address >> 13) & 0x07;
    u8 mpr_value = m_mpr[mpr_index];
    u32 physical_address = (mpr_value << 13) | (address & 0x1FFF);

    if (mpr_value < 0x80)
    {
        // HuCard ROM
        Debug("Attempted write to HuCard ROM at %06X", physical_address);
    }
    else if (mpr_value < 0xF8)
    {
        // Unused
        Debug("Unused write at %06X", physical_address);
    }
    else if (mpr_value < 0xFC)
    {
        // Work RAM
        m_wram[physical_address & 0x1FFF] = value;
    }
    else if (mpr_value < 0xFF)
    {
        // Unused
        Debug("Unused write at %06X", physical_address);
    }
    else
    {
        // Hardware Page
        switch (physical_address & 0x001C00)
        {
            case 0x0000:
                // HuC6270
                Debug("HuC6270 write at %06X", physical_address);
                break;
            case 0x0400:
                // HuC6260
                Debug("HuC6260 write at %06X", physical_address);
                break;
            case 0x0800:
                // PSG
                Debug("PSG write at %06X", physical_address);
                break;
            case 0x0C00:
                // Timer
                Debug("Timer write at %06X", physical_address);
                break;
            case 0x1000:
                // I/O
                Debug("I/O write at %06X", physical_address);
                break;
            case 0x1400:
                // CD-ROM
                Debug("CD-ROM write at %06X", physical_address);
                break;
            case 0x1800:
                // Unused
                Debug("Unused write at %06X", physical_address);
                break;
            case 0x1C00:
                // Unused
                Debug("Unused write at %06X", physical_address);
                break;
            default:
                break;
        }
    }
}

inline void Memory::SetMpr(u8 index, u8 value)
{
    m_mpr[index] = value;
}

inline u8 Memory::GetMpr(u8 index)
{
    return m_mpr[index];
}

inline u32 Memory::GetPhysicalAddress(u16 address)
{
    u8 mpr_index = (address >> 13) & 0x07;
    u8 mpr_value = m_mpr[mpr_index];
    return (mpr_value << 13) | (address & 0x1FFF);
}

#endif /* MEMORY_INLINE_H */