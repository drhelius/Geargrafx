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
#define MEMORY_INLINE_H

#include <assert.h>
#include "memory.h"
#include "media.h"
#include "huc6260.h"
#include "huc6270.h"
#include "huc6280.h"
#include "input.h"
#include "audio.h"
#include "cdrom.h"
#include "mapper.h"
#include "sf2_mapper.h"
#include "arcade_card_mapper.h"

INLINE u8 Memory::Read(u16 address, bool block_transfer)
{
#if defined(GG_TESTING)
    return m_test_memory[address];
#endif

#if !defined(GG_DISABLE_DISASSEMBLER)
    m_huc6280->CheckMemoryBreakpoints(HuC6280::HuC6280_BREAKPOINT_TYPE_CPU_ADDRESS, address, true);
#endif

    u8 mpr_index = address >> 13;
    u8 bank = m_mpr[mpr_index];
    u16 offset = address & 0x1FFF;

#if !defined(GG_DISABLE_DISASSEMBLER)
    if (m_huc6280->HasPhysicalMemoryBreakpoints(true))
        CheckPhysicalMemoryBreakpoints(bank, offset, true);
#endif

    if (bank != 0xFF)
    {
        if (IsValidPointer(m_current_mapper) && (bank < 0x80))
            return m_current_mapper->Read(bank, offset);
        else 
            return m_memory_map[bank][offset];
    }
    else
    {
        // Hardware Page
        switch (offset & 0x1C00)
        {
            case 0x0000:
                // HuC6270
                m_huc6280->InjectCycles(block_transfer ? 2 : 1);
                return m_huc6202->ReadRegister(offset);
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
                // CDROM
                if (m_media->IsCDROM())
                {
                    if (IsValidPointer(m_current_mapper) && (offset >= 0x1A00))
                        return m_current_mapper->ReadHardware(offset);
                    else
                        return m_cdrom->ReadRegister(offset);
                }
                else
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

    Debug("Invalid memory read at %04X, bank: %02X", address, bank);
    return 0xFF;
}

INLINE bool Memory::TryPeek(u16 address, u8* value)
{
#if defined(GG_TESTING)
    if (!IsValidPointer(value))
        return false;

    *value = m_test_memory[address];
    return true;
#else
    return TryPeek(address, m_mpr[address >> 13], value);
#endif
}

INLINE bool Memory::TryPeek(u16 address, u8 bank, u8* value)
{
    if (!IsValidPointer(value))
        return false;

#if defined(GG_TESTING)
    *value = m_test_memory[address];
    return true;
#else
    u16 offset = address & 0x1FFF;

    if (bank == 0xFF)
        return false;

    if (IsValidPointer(m_current_mapper) && (bank < 0x80))
    {
        if (m_current_mapper == m_sf2_mapper)
        {
            *value = m_sf2_mapper->Peek(bank, offset);
            return true;
        }

        if (m_current_mapper == m_arcade_card_mapper)
        {
            if (bank >= 0x40 && bank <= 0x43)
                *value = m_arcade_card_mapper->PeekPortData(bank - 0x40);
            else if (IsValidPointer(m_memory_map[bank]))
                *value = m_memory_map[bank][offset];
            else
                return false;

            return true;
        }

        return false;
    }

    if (!IsValidPointer(m_memory_map[bank]))
        return false;

    *value = m_memory_map[bank][offset];
    return true;
#endif
}

INLINE void Memory::Write(u16 address, u8 value, bool block_transfer)
{
#if defined(GG_TESTING)
    m_test_memory[address] = value;
    return;
#endif

#if !defined(GG_DISABLE_DISASSEMBLER)
    m_huc6280->CheckMemoryBreakpoints(HuC6280::HuC6280_BREAKPOINT_TYPE_CPU_ADDRESS, address, false);
#endif

    u8 mpr_index = address >> 13;
    u8 bank = m_mpr[mpr_index];
    u16 offset = address & 0x1FFF;

#if !defined(GG_DISABLE_DISASSEMBLER)
    if (m_huc6280->HasPhysicalMemoryBreakpoints(false))
        CheckPhysicalMemoryBreakpoints(bank, offset, false);
#endif

    if (IsValidPointer(m_current_mapper) && bank < 0x80)
    {
        m_current_mapper->Write(bank, offset, value);
    }
    else if (bank == 0xF7)
    {
        if (m_memory_map_write[bank] && (offset < 0x800))
            m_memory_map[bank][offset] = value;
    }
    else if (bank != 0xFF)
    {
        if (m_memory_map_write[bank])
            m_memory_map[bank][offset] = value;
    }
    else
    {
        // Hardware Page
        switch (offset & 0x1C00)
        {
            case 0x0000:
                // HuC6270
                m_huc6202->WriteRegister(offset, value);
                m_huc6280->InjectCycles(block_transfer ? 2 : 1);
                break;
            case 0x0400:
                // HuC6260
                m_huc6260->WriteRegister(offset, value);
                m_huc6280->InjectCycles(1);
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
                // CDROM
                if (m_media->IsCDROM())
                {
                    if (IsValidPointer(m_current_mapper) && (offset >= 0x1A00))
                        m_current_mapper->WriteHardware(offset, value);
                    else
                        m_cdrom->WriteRegister(offset, value);
                }
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

INLINE bool Memory::GetROMPhysicalAddress(u16 cpu_address, u32& rom_address)
{
    u8 bank = GetBank(cpu_address);
    u16 bank_offset = cpu_address & 0x1FFF;

    return GetROMPhysicalAddress(bank, bank_offset, rom_address);
}

INLINE bool Memory::GetROMPhysicalAddress(u8 bank, u16 bank_offset, u32& rom_address)
{
    if (GetBankType(bank) != MEMORY_BANK_TYPE_ROM)
        return false;

    if (bank >= 128)
        return false;

    if (IsValidPointer(m_current_mapper) && (m_current_mapper == m_sf2_mapper))
        return m_sf2_mapper->GetROMPhysicalAddress(bank, bank_offset, rom_address);

    u32* rom_bank_offset = m_media->GetROMBankOffset();

    if (!IsValidPointer(rom_bank_offset))
        return false;

    int rom_size = m_media->GetROMSize();

    if (rom_size <= 0)
        return false;

    u32 phys = rom_bank_offset[bank] + bank_offset;

    if (phys >= (u32)rom_size)
        return false;

    rom_address = phys;
    return true;
}

INLINE u8 Memory::GetBank(u16 address)
{
    return m_mpr[(address >> 13) & 0x07];
}

INLINE GG_Disassembler_Record* Memory::GetDisassemblerRecord(u16 address)
{
    u32 physical_address = GetPhysicalAddress(address);
    if (physical_address >= 0x200000)
        return NULL;
    return m_disassembler[physical_address];
}

INLINE GG_Disassembler_Record* Memory::GetDisassemblerRecord(u16 address, u8 bank)
{
    u32 physical_address = ((u32)bank << 13) | (address & 0x1FFF);
    if (physical_address >= 0x200000)
        return NULL;
    return m_disassembler[physical_address];
}

INLINE u8* Memory::GetWorkingRAM()
{
    return m_wram;
}

INLINE u8* Memory::GetCardRAM()
{
    return m_card_ram;
}

INLINE u8* Memory::GetBackupRAM()
{
    return m_backup_ram;
}

INLINE u8* Memory::GetCDROMRAM()
{
    return m_cdrom_ram;
}

INLINE u8* Memory::GetArcadeRAM()
{
    return m_arcade_card_mapper->GetRAM();
}

INLINE int Memory::GetWorkingRAMSize()
{
    return m_media->IsSGX() ? 0x8000 : 0x2000;
}

INLINE int Memory::GetROMSize()
{
    return m_media->GetROMSize();
}

INLINE int Memory::GetCardRAMSize()
{
    return m_card_ram_size;
}

INLINE int Memory::GetCardRAMStart()
{
    return m_card_ram_start;
}

INLINE int Memory::GetCardRAMEnd()
{
    return m_card_ram_end;
}

INLINE int Memory::GetBackupRAMSize()
{
    return 0x800;
}

INLINE int Memory::GetCDROMRAMSize()
{
    return m_cdrom_ram_size;
}

INLINE int Memory::GetArcadeCardRAMSize()
{
    if (m_media->IsArcadeCard())
        return 0x200000;
    else
        return 0;
}

INLINE ArcadeCardMapper* Memory::GetArcadeCardMapper()
{
    return m_arcade_card_mapper;
}

INLINE u8** Memory::GetMemoryMap()
{
    return m_memory_map;
}

INLINE bool* Memory::GetMemoryMapWrite()
{
    return m_memory_map_write;
}

INLINE GG_Disassembler_Record** Memory::GetAllDisassemblerRecords()
{
    return m_disassembler;
}

INLINE void Memory::EnableBackupRam(bool enable)
{
    m_backup_ram_enabled = enable;
}

INLINE bool Memory::IsBackupRamEnabled()
{
    return m_backup_ram_enabled;
}

INLINE bool Memory::IsBackupRamUsed()
{
    if(!m_backup_ram_enabled)
        return false;

    for(int i = 8; i < 0x800; i++)
        if(m_backup_ram[i])
        return true;

    return false;
}

INLINE void Memory::UpdateBackupRam(bool enable)
{
    if (m_backup_ram_enabled && enable)
    {
        //Debug("Backup RAM enabled");
        m_memory_map_write[0xF7] = true;
        m_memory_map[0xF7] = m_backup_ram;
    }
    else
    {
        //Debug("Backup RAM disabled");
        m_memory_map_write[0xF7] = false;
        m_memory_map[0xF7] = m_unused_memory;
    }
}

#if !defined(GG_DISABLE_DISASSEMBLER)
INLINE void Memory::CheckPhysicalMemoryBreakpoints(u8 bank, u32 offset, bool read)
{
    switch (GetBankType(bank))
    {
        case MEMORY_BANK_TYPE_WRAM:
        {
            if (m_huc6280->HasPhysicalMemoryBreakpoints(HuC6280::HuC6280_BREAKPOINT_TYPE_ZERO_PAGE, read))
            {
                if (bank == 0xF8 && offset < 0x100)
                {
                    m_huc6280->CheckMemoryBreakpoints(
                        HuC6280::HuC6280_BREAKPOINT_TYPE_ZERO_PAGE,
                        offset,
                        read);
                }
            }

            if (!m_huc6280->HasPhysicalMemoryBreakpoints(HuC6280::HuC6280_BREAKPOINT_TYPE_WRAM, read))
                return;

            u32 addr = 0;
            if (m_media->IsSGX())
                addr = ((bank - 0xF8) * 0x2000) + offset;
            else
                addr = offset;

            m_huc6280->CheckMemoryBreakpoints(HuC6280::HuC6280_BREAKPOINT_TYPE_WRAM, addr, read);

            break;
        }

        case MEMORY_BANK_TYPE_ROM:
        {
            if (!read)
                return;

            if (!m_huc6280->HasPhysicalMemoryBreakpoints(HuC6280::HuC6280_BREAKPOINT_TYPE_ROM, read))
                return;

            u32 rom_addr = 0;
            if (GetROMPhysicalAddress(bank, offset, rom_addr))
                m_huc6280->CheckMemoryBreakpoints(HuC6280::HuC6280_BREAKPOINT_TYPE_ROM, rom_addr, true);

            break;
        }

        case MEMORY_BANK_TYPE_CDROM_RAM:
        {
            if (!m_huc6280->HasPhysicalMemoryBreakpoints(HuC6280::HuC6280_BREAKPOINT_TYPE_CDROM_RAM, read))
                return;

            u32 addr = ((bank - 0x80) * 0x2000) + offset;
            m_huc6280->CheckMemoryBreakpoints(HuC6280::HuC6280_BREAKPOINT_TYPE_CDROM_RAM, addr, read);

            break;
        }


        case MEMORY_BANK_TYPE_CARD_RAM:
        {
            if (!m_huc6280->HasPhysicalMemoryBreakpoints(HuC6280::HuC6280_BREAKPOINT_TYPE_CARD_RAM, read))
                return;

            if (m_card_ram_size == 0)
                return;

            u32 addr = ((bank - m_card_ram_start) * 0x2000) + offset;
            addr %= m_card_ram_size;

            m_huc6280->CheckMemoryBreakpoints(HuC6280::HuC6280_BREAKPOINT_TYPE_CARD_RAM, addr, read);

            break;
        }


        case MEMORY_BANK_TYPE_BACKUP_RAM:
        {
            if (!m_huc6280->HasPhysicalMemoryBreakpoints(HuC6280::HuC6280_BREAKPOINT_TYPE_BACKUP_RAM, read))
                return;

            if (offset >= 0x800)
                return;

            m_huc6280->CheckMemoryBreakpoints(HuC6280::HuC6280_BREAKPOINT_TYPE_BACKUP_RAM, offset, read);

            break;
        }

        default:
            break;
    }
}
#endif

#endif /* MEMORY_INLINE_H */