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
#include "cartridge.h"
#include "input.h"
#include "audio.h"
#include "sf2_mapper.h"
#include "game_db.h"
#include "crc.h"

Memory::Memory(HuC6260* huc6260, HuC6202* huc6202, HuC6280* huc6280, Cartridge* cartridge, Input* input, Audio* audio)
{
    m_huc6260 = huc6260;
    m_huc6202 = huc6202;
    m_huc6280 = huc6280;
    m_cartridge = cartridge;
    m_input = input;
    m_audio = audio;
    InitPointer(m_wram);
    InitPointer(m_card_ram);
    InitPointer(m_card_ram_map);
    InitPointer(m_backup_ram);
    InitPointer(m_syscard_bios);
    InitPointer(m_gameexpress_bios);
    InitPointer(m_bios_map);
    InitPointer(m_disassembler);
    InitPointer(m_test_memory);
    InitPointer(m_current_mapper);
    InitPointer(m_sf2_mapper);
    m_mpr_reset_value = -1;
    m_wram_reset_value = 0;
    m_card_ram_reset_value = 0;
    m_backup_ram_enabled = true;
    m_card_ram_size = 0;
    m_card_ram_start = 0;
    m_card_ram_end = 0;
    m_right_syscard_bios = false;
    m_right_gameexpress_bios = false;
    m_syscard_bios_crc = 0;
    m_gameexpress_bios_crc = 0;
}

Memory::~Memory()
{
    SafeDeleteArray(m_wram);
    SafeDeleteArray(m_card_ram);
    SafeDeleteArray(m_card_ram_map);
    SafeDeleteArray(m_backup_ram);
    SafeDeleteArray(m_syscard_bios);
    SafeDeleteArray(m_gameexpress_bios);
    SafeDeleteArray(m_bios_map);
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
}

void Memory::Init()
{
    m_wram = new u8[0x8000];
    m_card_ram = new u8[0x8000];
    m_card_ram_map = new u8*[0x20];
    m_backup_ram = new u8[0x800];
    m_syscard_bios = new u8[GG_BIOS_SYSCARD_SIZE];
    m_gameexpress_bios = new u8[GG_BIOS_GAME_EXPRESS_SIZE];
    m_bios_map = new u8*[0x200];

#if !defined(GG_DISABLE_DISASSEMBLER)
    m_disassembler = new GG_Disassembler_Record*[0x200000];
    for (int i = 0; i < 0x200000; i++)
    {
        InitPointer(m_disassembler[i]);
    }
#endif

#if defined(GG_TESTING)
    m_test_memory = new u8[0x10000];
#endif

    m_current_mapper = NULL;
    m_sf2_mapper = new SF2Mapper(m_cartridge);

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

    for (int i = 0; i < 4; i++)
        m_wram_map[i] = &m_wram[i * (m_cartridge->IsSGX() ? 0x2000 : 0)];

#if defined(GG_TESTING)
    for (int i = 0; i < 0x10000; i++)
        m_test_memory[i] = rand() & 0xFF;
#endif

    if (m_cartridge->GetMapper() == Cartridge::SF2_MAPPER)
    {
        m_sf2_mapper->Reset();
        m_current_mapper = m_sf2_mapper;
    }
    else
        m_current_mapper = NULL;

    m_card_ram_size = MIN(m_cartridge->GetCardRAMSize(), 0x8000);

    if (m_card_ram_size == 0x8000)
    {
        m_card_ram_start = 0x40;
        m_card_ram_end = 0x5F;
    }
    else
    {
        m_card_ram_start = 0x00;
        m_card_ram_end = 0x00;
    }

    if (m_card_ram_size > 0)
    {
        for (int i = 0; i < m_card_ram_size; i++)
        {
            if (m_card_ram_reset_value < 0)
                m_card_ram[i] = rand() & 0xFF;
            else
                m_card_ram[i] = m_card_ram_reset_value & 0xFF;
        }

        int ram_card_banks = MIN(m_card_ram_end - m_card_ram_start + 1, 0x20);

        for (int i = 0; i < ram_card_banks; i++)
        {
            m_card_ram_map[i] = &m_card_ram[(i * 0x2000) % m_card_ram_size];
        }
    }

    memset(m_backup_ram, 0x00, 0x800);
    memcpy(m_backup_ram, k_backup_ram_init_string, 8);
}

void Memory::SetResetValues(int mpr, int wram, int card_ram)
{
    m_card_ram_reset_value = card_ram;
    m_mpr_reset_value = mpr;
    m_wram_reset_value = wram;
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
    assert((bits != 0) && !(bits & (bits - 1)));

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
    assert((bits != 0) && !(bits & (bits - 1)));

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

bool Memory::LoadBios(const char* file_path, bool syscard)
{
    using namespace std;
    int expected_size = 0;
    u8* bios = NULL;
    bool* right_bios = NULL;
    u32* crc = NULL;

    if  (syscard)
    {
        right_bios = &m_right_syscard_bios;
        expected_size = GG_BIOS_SYSCARD_SIZE;
        bios = m_syscard_bios;
        crc = &m_syscard_bios_crc;
    }
    else
    {
        right_bios = &m_right_gameexpress_bios;
        expected_size = GG_BIOS_GAME_EXPRESS_SIZE;
        bios = m_gameexpress_bios;
        crc = &m_gameexpress_bios_crc;
    }

    *right_bios = false;

    ifstream file(file_path, ios::in | ios::binary | ios::ate);

    if (file.is_open())
    {
        int size = static_cast<int> (file.tellg());

        if (size != expected_size)
        {
            Log("Incorrect BIOS size %d: %s", size, file_path);
            return false;
        }

        file.seekg(0, ios::beg);
        file.read(reinterpret_cast<char*>(bios), size);
        file.close();

        *right_bios = true;

        Log("BIOS %s loaded (%d bytes)", file_path, size);
    }
    else
    {
        Log("There was a problem opening the file %s", file_path);
        return false;
    }

    int i = 0;
    *crc = CalculateCRC32(*crc, bios, expected_size);

    while(k_bios_database[i].title != 0)
    {
        u32 db_crc = k_bios_database[i].crc;

        if (db_crc == *crc)
        {
            Debug("BIOS found in database: %s. CRC: %08X", k_game_database[i].title, *crc);
        }
        else
        {
            Debug("BIOS not found in database. CRC: %08X", *crc);
        }
        i++;
    }

    return true;
}

void Memory::SaveRam(std::ostream &file)
{
    Debug("Saving backup RAM to file");

    file.write(reinterpret_cast<const char*> (m_backup_ram), sizeof(u8) * 0x800);
}

bool Memory::LoadRam(std::istream &file, s32 file_size)
{
    Debug("Loading backup RAM from file");

    if (file_size != 0x800)
    {
        Log("Invalid backup RAM size: %d", file_size);
        return false;
    }

    file.read(reinterpret_cast<char*> (m_backup_ram), sizeof(u8) * 0x800);

    return true;
}

void Memory::SaveState(std::ostream& stream)
{
    using namespace std;
    stream.write(reinterpret_cast<const char*> (m_mpr), sizeof(m_mpr));
    stream.write(reinterpret_cast<const char*> (m_wram), sizeof(u8) * 0x8000);
    stream.write(reinterpret_cast<const char*> (m_card_ram), sizeof(u8) * 0x8000);
    stream.write(reinterpret_cast<const char*> (&m_card_ram_size), sizeof(m_card_ram_size));
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
    stream.read(reinterpret_cast<char*> (m_card_ram), sizeof(u8) * 0x8000);
    stream.read(reinterpret_cast<char*> (&m_card_ram_size), sizeof(m_card_ram_size));
    stream.read(reinterpret_cast<char*> (&m_card_ram_start), sizeof(m_card_ram_start));
    stream.read(reinterpret_cast<char*> (&m_card_ram_end), sizeof(m_card_ram_end));
    stream.read(reinterpret_cast<char*> (m_backup_ram), sizeof(u8) * 0x800);
    stream.read(reinterpret_cast<char*> (&m_backup_ram_enabled), sizeof(m_backup_ram_enabled));
    stream.read(reinterpret_cast<char*> (&m_io_buffer), sizeof(m_io_buffer));
    stream.read(reinterpret_cast<char*> (&m_mpr_buffer), sizeof(m_mpr_buffer));
    if (IsValidPointer(m_current_mapper))
        m_current_mapper->LoadState(stream);
}