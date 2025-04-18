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

#include <assert.h>
#include "memory.h"
#include "cartridge.h"
#include "huc6260.h"
#include "huc6270.h"
#include "huc6280.h"
#include "input.h"
#include "audio.h"
#include "mapper.h"

INLINE u8 Memory::Read(u16 address, bool block_transfer)
{
#if defined(GG_TESTING)
    return m_test_memory[address];
#endif

#if !defined(GG_DISABLE_DISASSEMBLER)
    m_huc6280->CheckMemoryBreakpoints(HuC6280::HuC6280_BREAKPOINT_TYPE_ROMRAM, address, true);
#endif

    u8 mpr_index = address >> 13;
    u8 bank = m_mpr[mpr_index];
    u16 offset = address & 0x1FFF;

    // 0x00 - 0x7F
    if (bank < 0x80)
    {
        // HuCard ROM
        if (IsValidPointer(m_current_mapper))
            return m_current_mapper->Read(bank, offset);
        else if ((m_card_ram_size > 0) && (bank >= m_card_ram_start) && (bank <= m_card_ram_end))
        {
            return m_card_ram_map[bank - m_card_ram_start][offset];
        }
        else
        {
            u8** rom_map = m_cartridge->GetROMMap();
            return rom_map[bank][offset];
        }
    }
    // 0x80 - 0xF6
    else if (bank < 0xF7)
    {
        // Unused
        Debug("Unused read at %04X, bank=%02X", address, bank);
        return 0xFF;
    }
    // 0xF7
    else if (bank < 0xF8)
    {
        // Backup RAM
        Debug("Backup RAM read at %04X, bank=%02X", address, bank);
        return 0xFF;
    }
    // 0xF8 - 0xFB
    else if (bank < 0xFC)
    {
        // RAM
        if (bank > 0xF8)
        {
            Debug("SGX RAM read at %04X, bank=%02X", address, bank);
        }
        return m_wram[offset];
    }
    // 0xFC - 0xFE
    else if (bank < 0xFF)
    {
        // Unused
        Debug("Unused read at %04X, bank=%02X", address, bank);
        return 0xFF;
    }
    // 0xFF
    else
    {
        // Hardware Page
        switch (offset & 0x001C00)
        {
            case 0x0000:
                // HuC6270
                m_huc6280->InjectCycles(1);
                return m_huc6270->ReadRegister(offset);
            case 0x0400:
                // HuC6260
                m_huc6280->InjectCycles(1);
                return m_huc6260->ReadRegister(offset);
            case 0x0800:
                // PSG
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
                    switch (address & 0x03)
                    {
                        case 0:
                        case 1:
                        {
                            Debug("Invalid interrupt register read at %04X", address);
                            break;
                        }
                        case 2:
                        case 3:
                        {
                            m_io_buffer = (m_huc6280->ReadInterruptRegister(offset) & 0x07) | (m_io_buffer & 0xF8);
                            break;
                        }
                    }
                    return m_io_buffer;
                }
            }
            case 0x1800:
                // Unused
                Debug("Unused hardware read at %04X", address);
                return 0xFF;
            case 0x1C00:
                // Unused
                Debug("Unused hardware read at %04X", address);
                return 0xFF;
            default:
                Debug("Invalid hardware read at %04X", address);
                return 0xFF;
        }
    }
}

INLINE void Memory::Write(u16 address, u8 value)
{
#if defined(GG_TESTING)
    m_test_memory[address] = value;
    return;
#endif

#if !defined(GG_DISABLE_DISASSEMBLER)
    m_huc6280->CheckMemoryBreakpoints(HuC6280::HuC6280_BREAKPOINT_TYPE_ROMRAM, address, false);
#endif

    u8 mpr_index = address >> 13;
    u8 bank = m_mpr[mpr_index];
    u16 offset = address & 0x1FFF;

    // 0x00 - 0x7F
    if (bank < 0x80)
    {
        // HuCard ROM
        if (IsValidPointer(m_current_mapper))
            m_current_mapper->Write(bank, offset, value);
        else if ((m_card_ram_size > 0) && (bank >= m_card_ram_start) && (bank <= m_card_ram_end))
        {
            m_card_ram_map[bank - m_card_ram_start][offset] = value;
        }
        else
        {
            Debug("Attempted write to HuCard ROM at %04X, value=%02X, bank=%02X", address, value, bank);
        }
    }
    // 0x80 - 0xF6
    else if (bank < 0xF7)
    {
        // Unused
        Debug("Unused write at %04X, value=%02X, bank=%02X", address, value, bank);
    }
    // 0xF7
    else if (bank < 0xF8)
    {
        // Savegame RAM
        Debug("Savegame RAM write at %04X, value=%02X, bank=%02X", address, value, bank);
    }
    // 0xF8 - 0xFB
    else if (bank < 0xFC)
    {
        // RAM
        if (bank > 0xF8)
        {
            Debug("SGX RAM write at %04X, value=%02X, bank=%02X", address, value, bank);
        }
        m_wram[offset] = value;
    }
    // 0xFC - 0xFE
    else if (bank < 0xFF)
    {
        // Unused
        Debug("Unused write at %04X, value=%02X, bank=%02X", address, value, bank);
    }
    else
    {
        // Hardware Page
        switch (offset & 0x1C00)
        {
            case 0x0000:
                // HuC6270
                m_huc6280->InjectCycles(1);
                m_huc6270->WriteRegister(offset, value);
                break;
            case 0x0400:
                // HuC6260
                m_huc6280->InjectCycles(1);
                m_huc6260->WriteRegister(offset, value);
                break;
            case 0x0800:
                // PSG
                m_audio->WritePSG(offset, value);
                m_io_buffer = value;
                break;
            case 0x0C00:
                // Timer
                m_huc6280->WriteTimerRegister(offset, value);
                m_io_buffer = value;
                break;
            case 0x1000:
                // I/O
                m_input->WriteO(value);
                m_io_buffer = value;
                break;
            case 0x1400:
            {
                switch (address & 0x03)
                {
                    case 0:
                    case 1:
                    {
                        Debug("Invalid interrupt register write at %04X, value=%02X", address, value);
                        break;
                    }
                    case 2:
                    case 3:
                    {
                        m_huc6280->WriteInterruptRegister(offset, value);
                        break;
                    }
                }
                m_io_buffer = value;
                break;
            }
            case 0x1800:
                // Unused
                Debug("Unused hardware write at %04X, value=%02X", address, value);
                break;
            case 0x1C00:
                // Unused
                Debug("Unused hardware write at %04X, value=%02X", address, value);
                break;
            default:
                Debug("Invalid hardware write at %04X, value=%02X", address, value);
                break;
        }
    }
}

INLINE void Memory::SetMpr(u8 index, u8 value)
{
    assert(index < 8);
    m_mpr[index] = value;
}

INLINE u8 Memory::GetMpr(u8 index)
{
    assert(index < 8);
    return m_mpr[index];
}

INLINE u32 Memory::GetPhysicalAddress(u16 address)
{
    return (GetBank(address) << 13) | (address & 0x1FFF);
}

INLINE u8 Memory::GetBank(u16 address)
{
    return m_mpr[(address >> 13) & 0x07];
}

INLINE GG_Disassembler_Record* Memory::GetDisassemblerRecord(u16 address)
{
    return m_disassembler[GetPhysicalAddress(address)];
}

#endif /* MEMORY_INLINE_H */