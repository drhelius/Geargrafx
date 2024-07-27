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
#include "huc6260.h"
#include "huc6270.h"
#include "huc6280.h"
#include "input.h"
#include "audio.h"

inline u8 Memory::Read(u16 address, bool block_transfer)
{
    m_huc6280->CheckMemoryBreakpoints(address, true);

    u8 mpr_index = (address >> 13) & 0x07;
    u8 mpr_value = m_mpr[mpr_index];
    u32 physical_address = (mpr_value << 13) | (address & 0x1FFF);

    // 0x00 - 0xF6
    if (mpr_value < 0xF7)
    {
        // HuCard ROM
        u8* rom = m_cartridge->GetROM();
        int rom_size = m_cartridge->GetROMSize();
        if ((int)physical_address >= rom_size)
        {
            Debug("Attempted read out of ROM bounds at %04X (%06X), MPR(%02X,%02X)", address, physical_address, mpr_index, mpr_value);
            return rom[physical_address & (rom_size - 1)];
        }
        else
            return rom[physical_address];
    }
    // 0xF7
    else if (mpr_value < 0xF8)
    {
        // Backup RAM
        Debug("Backup RAM read at %06X", physical_address);
        return 0xFF;
    }
    // 0xF8 - 0xFB
    else if (mpr_value < 0xFC)
    {
        // RAM
        if (mpr_value > 0xF8)
        {
            Debug("SGX RAM read at %06X", physical_address);
        }
        return m_wram[physical_address & 0x1FFF];
    }
    // 0xFC - 0xFE
    else if (mpr_value < 0xFF)
    {
        // Unused
        Debug("Unused read at %06X", physical_address);
        return 0xFF;
    }
    // 0xFF
    else
    {
        // Hardware Page
        switch (physical_address & 0x001C00)
        {
            case 0x0000:
                // HuC6270
                m_huc6280->InjectCycles(1);
                return m_huc6270->ReadRegister(physical_address);
            case 0x0400:
                // HuC6260
                m_huc6280->InjectCycles(1);
                return m_huc6260->ReadRegister(physical_address);
            case 0x0800:
                // PSG
                //Debug("PSG read at %06X", physical_address);
                return block_transfer ? 0x00 : m_io_buffer;
            case 0x0C00:
                // Timer Counter
                if (block_transfer)
                    return 0x00;
                else
                {
                    m_io_buffer = (m_huc6280->ReadTimerRegister() & 0x7F) | (m_io_buffer & 0x80);
                    return m_io_buffer;
                }
            case 0x1000:
                // I/O
                if (block_transfer)
                    return 0x00;
                else
                {
                    m_io_buffer = m_input->ReadK();
                    return m_io_buffer;
                }
            case 0x1400:
            {
                // Interrupt registers
                if (block_transfer)
                    return 0x00;
                else
                {
                    int reg = physical_address & 0x03;
                    if (reg < 0x02)
                        return m_io_buffer;
                    else
                    {
                        m_io_buffer = (m_huc6280->ReadInterruptRegister(physical_address) & 0x07) | (m_io_buffer & 0xF8);
                        return m_io_buffer;
                    }
                }
            }
            case 0x1800:
                // Unused
                Debug("Unused hardware read at %06X", physical_address);
                return 0xFF;
            case 0x1C00:
                // Unused
                Debug("Unused hardware read at %06X", physical_address);
                return 0xFF;
            default:
                return 0xFF;
        }
    }
}

inline void Memory::Write(u16 address, u8 value)
{
    m_huc6280->CheckMemoryBreakpoints(address, false);

    u8 mpr_index = (address >> 13) & 0x07;
    u8 mpr_value = m_mpr[mpr_index];
    u32 physical_address = (mpr_value << 13) | (address & 0x1FFF);

    // 0x00 - 0xF6
    if (mpr_value < 0xF7)
    {
        // HuCard ROM
        Debug("Attempted write to HuCard ROM at %06X, value=%02X", physical_address, value);
    }
    // 0xF7
    else if (mpr_value < 0xF8)
    {
        // Savegame RAM
        Debug("Savegame RAM write at %06X, value=%02X", physical_address, value);
    }
    // 0xF8 - 0xFB
    else if (mpr_value < 0xFC)
    {
        // RAM
        if (mpr_value > 0xF8)
        {
            Debug("SGX RAM write at %06X, value=%02X", physical_address, value);
        }
        m_wram[physical_address & 0x1FFF] = value;
    }
    // 0xFC - 0xFE
    else if (mpr_value < 0xFF)
    {
        // Unused
        Debug("Unused write at %06X, value=%02X", physical_address, value);
    }
    else
    {
        // Hardware Page
        switch (physical_address & 0x001C00)
        {
            case 0x0000:
                // HuC6270
                m_huc6280->InjectCycles(1);
                m_huc6270->WriteRegister(physical_address, value);
                break;
            case 0x0400:
                // HuC6260
                m_huc6280->InjectCycles(1);
                m_huc6260->WriteRegister(physical_address, value);
                break;
            case 0x0800:
                // PSG
                //Debug("PSG write at %06X, value=%02X", physical_address, value);
                m_audio->WritePSG(physical_address, value);
                m_io_buffer = value;
                break;
            case 0x0C00:
                // Timer
                m_huc6280->WriteTimerRegister(physical_address, value);
                m_io_buffer = value;
                break;
            case 0x1000:
                // I/O
                m_input->WriteO(value);
                m_io_buffer = value;
                break;
            case 0x1400:
            {
                int reg = physical_address & 0x03;
                if (reg >= 0x02)
                    m_huc6280->WriteInterruptRegister(physical_address, value);
                m_io_buffer = value;
                break;
            }
            case 0x1800:
                // Unused
                Debug("Unused hardware write at %06X, value=%02X", physical_address, value);
                break;
            case 0x1C00:
                // Unused
                Debug("Unused hardware write at %06X, value=%02X", physical_address, value);
                break;
            default:
                break;
        }
    }
}

inline void Memory::SetMpr(u8 index, u8 value)
{
    m_mpr[index & 0x07] = value;
}

inline u8 Memory::GetMpr(u8 index)
{
    return m_mpr[index & 0x07];
}

inline u32 Memory::GetPhysicalAddress(u16 address)
{
    return (GetBank(address) << 13) | (address & 0x1FFF);
}

inline u8 Memory::GetBank(u16 address)
{
    return m_mpr[(address >> 13) & 0x07];
}

#endif /* MEMORY_INLINE_H */