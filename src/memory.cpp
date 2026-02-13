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

#include <stdlib.h>
#include "memory.h"
#include "huc6260.h"
#include "huc6202.h"
#include "huc6280.h"
#include "media.h"
#include "input.h"
#include "audio.h"
#include "cdrom.h"
#include "sf2_mapper.h"
#include "arcade_card_mapper.h"

Memory::Memory(HuC6260* huc6260, HuC6202* huc6202, HuC6280* huc6280, Media* media, Input* input, Audio* audio, CdRom* cdrom)
{
    m_huc6260 = huc6260;
    m_huc6202 = huc6202;
    m_huc6280 = huc6280;
    m_media = media;
    m_input = input;
    m_audio = audio;
    m_cdrom = cdrom;
    InitPointer(m_disassembler);
    InitPointer(m_test_memory);
    InitPointer(m_current_mapper);
    InitPointer(m_sf2_mapper);
    InitPointer(m_arcade_card_mapper);
    m_mpr_reset_value = -1;
    m_wram_reset_value = 0;
    m_card_ram_reset_value = 0;
    m_arcade_card_reset_value = 0;
    m_backup_ram_enabled = true;
    m_cdrom_ram_size = 0;
    m_card_ram_size = 0;
    m_card_ram_start = 0;
    m_card_ram_end = 0;
}

Memory::~Memory()
{
    SafeDeleteArray(m_test_memory);
    if (IsValidPointer(m_disassembler))
    {
        for (int i = 0; i < 0x200000; i++)
        {
            SafeDelete(m_disassembler[i]);
        }
        SafeDeleteArray(m_disassembler);
    }
    SafeDelete(m_sf2_mapper);
    SafeDelete(m_arcade_card_mapper);
}

void Memory::Init()
{
    for (int i = 0; i < 0x100; i++)
        InitPointer(m_memory_map[i]);

#if !defined(GG_DISABLE_DISASSEMBLER)
    m_disassembler = new GG_Disassembler_Record*[0x200000];
    for (int i = 0; i < 0x200000; i++)
        InitPointer(m_disassembler[i]);
#endif

#if defined(GG_TESTING)
    m_test_memory = new u8[0x10000];
#endif

    m_current_mapper = NULL;
    m_sf2_mapper = new SF2Mapper(m_media, this);
    m_arcade_card_mapper = new ArcadeCardMapper(m_media, this);

    Reset();
}

void Memory::Reset()
{
    m_io_buffer = 0xFF;
    m_mpr_buffer = 0x00;
    m_mpr[7] = 0x00;

    for (int i = 0; i < 7; i++)
    {
        if (m_mpr_reset_value < 0)
        {
            do
            {
                m_mpr[i] = rand() & 0xFF;
            }
            while (m_mpr[i] == 0x00);
        }
        else
            m_mpr[i] = m_mpr_reset_value & 0xFF;
    }

    for (int i = 0; i < 0x8000; i++)
    {
        if (m_wram_reset_value < 0)
            m_wram[i] = rand() & 0xFF;
        else
            m_wram[i] = m_wram_reset_value & 0xFF;
    }

#if defined(GG_TESTING)
    for (int i = 0; i < 0x10000; i++)
        m_test_memory[i] = rand() & 0xFF;
#endif

    if (m_media->GetMapper() == Media::SF2_MAPPER)
    {
        m_sf2_mapper->Reset();
        m_current_mapper = m_sf2_mapper;
    }
    else if (m_media->GetMapper() == Media::ARCADE_CARD_MAPPER)
    {
        m_arcade_card_mapper->Reset();
        m_current_mapper = m_arcade_card_mapper;
    }
    else
        m_current_mapper = NULL;

    m_cdrom_ram_size = m_media->IsCDROM() ? 0x10000 : 0;

    for (u32 i = 0; i < m_cdrom_ram_size; i++)
    {
        if (m_wram_reset_value < 0)
            m_cdrom_ram[i] = rand() & 0xFF;
        else
            m_cdrom_ram[i] = m_wram_reset_value & 0xFF;
    }

    m_card_ram_size = m_media->GetCardRAMSize();

    if (m_card_ram_size == 0x8000)
    {
        m_card_ram_start = 0x40;
        m_card_ram_end = 0x5F;
    }
    else if (m_card_ram_size == 0x30000)
    {
        m_card_ram_start = 0x68;
        m_card_ram_end = 0x7F;
    }
    else
    {
        m_card_ram_start = 0x00;
        m_card_ram_end = 0x00;
    }

    for (u32 i = 0; i < m_card_ram_size; i++)
    {
        if (m_card_ram_reset_value < 0)
            m_card_ram[i] = rand() & 0xFF;
        else
            m_card_ram[i] = m_card_ram_reset_value & 0xFF;
    }

    u8* arcade_ram = m_arcade_card_mapper->GetRAM();

    for (u32 i = 0; i < 0x200000; i++)
    {
        if (m_arcade_card_reset_value < 0)
            arcade_ram[i] = rand() & 0xFF;
        else
            arcade_ram[i] = m_arcade_card_reset_value & 0xFF;
    }

    memset(m_backup_ram, 0xFF, 0x2000);
    memset(m_backup_ram, 0x00, 0x800);
    memcpy(m_backup_ram, k_backup_ram_init_string, 8);

    memset(m_unused_memory, 0xFF, 0x2000);

    ReloadMemoryMap();
}

void Memory::ReloadMemoryMap()
{
    // 0x00 - 0x7F
    for (int i = 0x00; i <= 0x7F; i++)
    {
        // Card RAM
        if ((m_card_ram_size > 0) && (i >= m_card_ram_start) && (i <= m_card_ram_end))
        {
            m_memory_map_write[i] = true;
            m_memory_map[i] = &m_card_ram[((i - m_card_ram_start) * 0x2000) % m_card_ram_size];
        }
        // ROM
        else
        {
            m_memory_map_write[i] = false;
            m_memory_map[i] = m_media->GetROMMap()[i];
        }
    }

    // 0x80 - 0xFF
    for (int i = 0x80; i <= 0xFF; i++)
    {
        m_memory_map_write[i] = false;
        m_memory_map[i] = m_unused_memory;
    }

    // 0x80 - 0x87
    if (m_media->IsCDROM())
        for (int i = 0x80; i <= 0x87; i++)
        {
            // CDROM RAM
            m_memory_map_write[i] = true;
            m_memory_map[i] = &m_cdrom_ram[(i - 0x80) * 0x2000];
        }

    // 0xF7
    if (m_backup_ram_enabled)
    {
        // Backup RAM
        m_memory_map_write[0xF7] = true;
        m_memory_map[0xF7] = m_backup_ram;
    }

    // 0xF8 - 0xFB
    for (int i = 0xF8; i <= 0xFB; i++)
    {
        // RAM
        m_memory_map_write[i] = true;
        if (m_media->IsSGX())
            m_memory_map[i] = &m_wram[(i - 0xF8) * 0x2000];
        else
            m_memory_map[i] = &m_wram[0];
    }
}

void Memory::SetResetValues(int mpr, int wram, int card_ram, int arcade_card)
{
    m_card_ram_reset_value = card_ram;
    m_mpr_reset_value = mpr;
    m_wram_reset_value = wram;
    m_arcade_card_reset_value = arcade_card;
}

void Memory::ResetDisassemblerRecords()
{
#if !defined(GG_DISABLE_DISASSEMBLER)
    for (int i = 0; i < 0x200000; i++)
    {
        SafeDelete(m_disassembler[i]);
    }
#endif
}

void Memory::SetMprTAM(u8 bits, u8 value)
{
    //assert((bits != 0) && !(bits & (bits - 1)));

    if(bits == 0)
    {
        Debug("No TAM bit: %02X", bits);
        return;
    }

    if (bits & (bits - 1))
    {
        Debug("Invalid TAM bits: %02X", bits);
    }

    m_mpr_buffer = value;

    for (int i = 0; i < 8; i++)
    {
        if ((bits & (0x01 << i)) != 0)
        {
            m_mpr[i] = value;
        }
    }
}

u8 Memory::GetMprTMA(u8 bits)
{
    //assert((bits != 0) && !(bits & (bits - 1)));

    if(bits == 0)
    {
        Debug("No TAM bit: %02X", bits);
        return m_mpr_buffer;
    }

    if (bits & (bits - 1))
    {
        Debug("Invalid TAM bits: %02X", bits);
    }

    u8 ret = 0;

    for (int i = 0; i < 8; i++)
    {
        if ((bits & (0x01 << i)) != 0)
        {
            ret |= m_mpr[i];
        }
    }

    m_mpr_buffer = ret;
    return ret;
}

GG_Disassembler_Record* Memory::GetOrCreateDisassemblerRecord(u16 address)
{
    u32 physical_address = GetPhysicalAddress(address);

    GG_Disassembler_Record* record = m_disassembler[physical_address];

    if (!IsValidPointer(record))
    {
        record = new GG_Disassembler_Record();
        record->address = physical_address;
        record->bank = GetBank(address);
        record->segment[0] = 0;
        record->name[0] = 0;
        record->bytes[0] = 0;
        record->size = 0;
        for (int i = 0; i < 7; i++)
            record->opcodes[i] = 0;
        record->jump = false;
        record->jump_address = 0;
        record->jump_bank = 0;
        record->subroutine = false;
        record->irq = 0;
        m_disassembler[physical_address] = record;
    }

    return record;
}

Memory::MemoryBankType Memory::GetBankType(u8 bank)
{
    if (m_media->IsCDROM() && bank >= 0x80 && bank <= 0x87)
        return MEMORY_BANK_TYPE_CDROM_RAM;

    if (bank == 0xF7 && m_backup_ram_enabled)
        return MEMORY_BANK_TYPE_BACKUP_RAM;

    if (bank >= 0xF8 && bank <= 0xFB)
        return MEMORY_BANK_TYPE_WRAM;

    if ((m_card_ram_size > 0) && (bank >= m_card_ram_start) && (bank <= m_card_ram_end))
        return MEMORY_BANK_TYPE_CARD_RAM;

    if (bank < 0x80)
        return m_media->IsCDROM() ? MEMORY_BANK_TYPE_BIOS : MEMORY_BANK_TYPE_ROM;

    if (bank == 0xFF)
        return MEMORY_BANK_TYPE_HARDWARE;

    return MEMORY_BANK_TYPE_UNUSED;
}

void Memory::SaveRam(std::ostream &file)
{
    Debug("Saving BRAM to file");

    file.write(reinterpret_cast<const char*> (m_backup_ram), sizeof(u8) * 0x800);
}

bool Memory::LoadRam(std::istream &file, s32 file_size)
{
    Debug("Loading BRAM from file");

    if (file_size > 0x2000)
    {
        Log("Invalid BRAM size: %d", file_size);
        return false;
    }

    file.read(reinterpret_cast<char*> (m_backup_ram), sizeof(u8) * file_size);

    return true;
}

void Memory::SaveState(std::ostream& stream)
{
    using namespace std;
    stream.write(reinterpret_cast<const char*> (m_mpr), sizeof(m_mpr));
    stream.write(reinterpret_cast<const char*> (m_wram), sizeof(u8) * 0x8000);
    stream.write(reinterpret_cast<const char*> (&m_cdrom_ram_size), sizeof(m_cdrom_ram_size));
    stream.write(reinterpret_cast<const char*> (m_cdrom_ram), sizeof(u8) * m_cdrom_ram_size);
    stream.write(reinterpret_cast<const char*> (&m_card_ram_size), sizeof(m_card_ram_size));
    stream.write(reinterpret_cast<const char*> (m_card_ram), sizeof(u8) * m_card_ram_size);
    stream.write(reinterpret_cast<const char*> (&m_card_ram_start), sizeof(m_card_ram_start));
    stream.write(reinterpret_cast<const char*> (&m_card_ram_end), sizeof(m_card_ram_end));
    stream.write(reinterpret_cast<const char*> (m_backup_ram), sizeof(u8) * 0x800);
    stream.write(reinterpret_cast<const char*> (&m_backup_ram_enabled), sizeof(m_backup_ram_enabled));
    stream.write(reinterpret_cast<const char*> (&m_io_buffer), sizeof(m_io_buffer));
    stream.write(reinterpret_cast<const char*> (&m_mpr_buffer), sizeof(m_mpr_buffer));
    if (IsValidPointer(m_current_mapper))
        m_current_mapper->SaveState(stream);
}

void Memory::LoadState(std::istream& stream)
{
    using namespace std;
    stream.read(reinterpret_cast<char*> (m_mpr), sizeof(m_mpr));
    stream.read(reinterpret_cast<char*> (m_wram), sizeof(u8) * 0x8000);
    stream.read(reinterpret_cast<char*> (&m_cdrom_ram_size), sizeof(m_cdrom_ram_size));
    stream.read(reinterpret_cast<char*> (m_cdrom_ram), sizeof(u8) * m_cdrom_ram_size);
    stream.read(reinterpret_cast<char*> (&m_card_ram_size), sizeof(m_card_ram_size));
    stream.read(reinterpret_cast<char*> (m_card_ram), sizeof(u8) * m_card_ram_size);
    stream.read(reinterpret_cast<char*> (&m_card_ram_start), sizeof(m_card_ram_start));
    stream.read(reinterpret_cast<char*> (&m_card_ram_end), sizeof(m_card_ram_end));
    stream.read(reinterpret_cast<char*> (m_backup_ram), sizeof(u8) * 0x800);
    stream.read(reinterpret_cast<char*> (&m_backup_ram_enabled), sizeof(m_backup_ram_enabled));
    stream.read(reinterpret_cast<char*> (&m_io_buffer), sizeof(m_io_buffer));
    stream.read(reinterpret_cast<char*> (&m_mpr_buffer), sizeof(m_mpr_buffer));
    if (IsValidPointer(m_current_mapper))
        m_current_mapper->LoadState(stream);
}
