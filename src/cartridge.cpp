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

#include <string>
#include <fstream>
#include <algorithm>
#include <assert.h>
#include "cartridge.h"
#include "miniz/miniz.h"
#include "game_db.h"

Cartridge::Cartridge()
{
    InitPointer(m_rom);
    m_rom_size = 0;
    m_card_ram_size = 0;
    m_ready = false;
    m_file_path[0] = 0;
    m_file_name[0] = 0;
    m_file_extension[0] = 0;
    m_rom_bank_count = 0;
    m_crc = 0;
    m_is_sgx = false;
    m_mapper = STANDARD_MAPPER;

    m_rom_map = new u8*[128];
    for (int i = 0; i < 128; i++)
        InitPointer(m_rom_map[i]);
}

Cartridge::~Cartridge()
{
    SafeDeleteArray(m_rom);
    SafeDeleteArray(m_rom_map);
}

void Cartridge::Init()
{
    Reset();
}

void Cartridge::Reset()
{
    SafeDeleteArray(m_rom);
    m_rom_size = 0;
    m_card_ram_size = 0;
    m_ready = false;
    m_file_path[0] = 0;
    m_file_name[0] = 0;
    m_file_extension[0] = 0;
    m_rom_bank_count = 0;
    m_crc = 0;
    m_is_sgx = false;
    m_mapper = STANDARD_MAPPER;

    for (int i = 0; i < 128; i++)
        InitPointer(m_rom_map[i]);
}

u32 Cartridge::GetCRC()
{
    return m_crc;
}

bool Cartridge::IsReady()
{
    return m_ready;
}

bool Cartridge::IsSGX()
{
    return m_is_sgx;
}

Cartridge::CartridgeMapper Cartridge::GetMapper()
{
    return m_mapper;
}

int Cartridge::GetROMSize()
{
    return m_rom_size;
}

int Cartridge::GetROMBankCount()
{
    return m_rom_bank_count;
}

int Cartridge::GetCardRAMSize()
{
    return m_card_ram_size;
}

const char* Cartridge::GetFilePath()
{
    return m_file_path;
}

const char* Cartridge::GetFileDirectory()
{
    return m_file_directory;
}

const char* Cartridge::GetFileName()
{
    return m_file_name;
}

const char* Cartridge::GetFileExtension()
{
    return m_file_extension;
}

u8* Cartridge::GetROM()
{
    return m_rom;
}

u8** Cartridge::GetROMMap()
{
    return m_rom_map;
}

bool Cartridge::LoadFromFile(const char* path)
{
    using namespace std;

    Log("Loading %s...", path);

    Reset();

    string fullpath(path);
    string directory;
    string filename;
    string extension;

    size_t pos = fullpath.find_last_of("/\\");
    if (pos != string::npos)
    {
        filename = fullpath.substr(pos + 1);
        directory = fullpath.substr(0, pos);
    }
    else
    {
        filename = fullpath;
        directory = "";
    }

    extension = fullpath.substr(fullpath.find_last_of(".") + 1);
    transform(extension.begin(), extension.end(), extension.begin(), (int(*)(int)) tolower);

    snprintf(m_file_path, sizeof(m_file_path), "%s", path);
    snprintf(m_file_directory, sizeof(m_file_directory), "%s", directory.c_str());
    snprintf(m_file_name, sizeof(m_file_name), "%s", filename.c_str());
    snprintf(m_file_extension, sizeof(m_file_extension), "%s", extension.c_str());

    ifstream file(path, ios::in | ios::binary | ios::ate);

    if (file.is_open())
    {
        int size = static_cast<int> (file.tellg());

        if (size <= 0)
        {
            Log("ERROR: Unable to open file %s. Size: %d", path, size);
            file.close();
            return false;
        }

        if (file.bad() || file.fail() || !file.good() || file.eof())
        {
            Log("ERROR: Unable to open file %s. Bad file!", path);
            file.close();
            return false;
        }

        char* memblock = new char[size];
        file.seekg(0, ios::beg);
        file.read(memblock, size);
        file.close();

        for (int i = 0; i < size; i++)
        {
            if (memblock[i] != 0)
                break;

            if (i == size - 1)
            {
                Log("ERROR: File %s is empty!", path);
                file.close();
                SafeDeleteArray(memblock);
                return false;
            }
        }

        if (extension == "zip")
            m_ready = LoadFromZipFile(reinterpret_cast<u8*> (memblock), size);
        else
            m_ready = LoadFromBuffer(reinterpret_cast<u8*> (memblock), size);

        SafeDeleteArray(memblock);
    }
    else
    {
        Log("ERROR: There was a problem loading the file %s...", path);
        m_ready = false;
    }

    if (!m_ready)
        Reset();

    return m_ready;
}

bool Cartridge::LoadFromBuffer(const u8* buffer, int size)
{
    if (IsValidPointer(buffer))
    {
        Log("Loading ROM from buffer... Size: %d", size);

        if(size & 512)
        {
            Debug("Removing 512 bytes header...");
            size &= ~512;
            buffer += 512;
        }

        if(!memcmp(buffer + 0x1FD0, "MCGENJIN", 8))
        {
            Debug("MCGENJIN mapper detected.");
        }

        assert((size % 0x2000) == 0);

        if ((size % 0x2000) != 0)
        {
            Log("ERROR: Invalid size found: %d (0x%X) bytes", size, size);
        }

        m_rom_size = size;
        m_rom = new u8[m_rom_size];
        memcpy(m_rom, buffer, m_rom_size);

        GatherROMInfo();

        InitRomMAP();

        m_ready = true;

        Debug("ROM loaded from buffer. Size: %d bytes", m_rom_size);

        return true;
    }
    else
    {
        Log("ERROR: Unable to load ROM from buffer: Buffer invalid %p. Size: %d", buffer, size);
        return false;
    }
}

bool Cartridge::LoadFromZipFile(const u8* buffer, int size)
{
    Debug("Loading from ZIP file... Size: %d", size);

    using namespace std;

    mz_zip_archive zip_archive;
    mz_bool status;
    memset(&zip_archive, 0, sizeof (zip_archive));

    status = mz_zip_reader_init_mem(&zip_archive, (void*) buffer, size, 0);
    if (!status)
    {
        Log("ERROR: mz_zip_reader_init_mem() failed!");
        return false;
    }

    for (unsigned int i = 0; i < mz_zip_reader_get_num_files(&zip_archive); i++)
    {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat))
        {
            Log("ERROR: mz_zip_reader_file_stat() failed!");
            mz_zip_reader_end(&zip_archive);
            return false;
        }

        Debug("ZIP Content - Filename: \"%s\", Comment: \"%s\", Uncompressed size: %u, Compressed size: %u", file_stat.m_filename, file_stat.m_comment, (unsigned int) file_stat.m_uncomp_size, (unsigned int) file_stat.m_comp_size);

        string fn((const char*) file_stat.m_filename);
        string extension = fn.substr(fn.find_last_of(".") + 1);
        transform(extension.begin(), extension.end(), extension.begin(), (int(*)(int)) tolower);

        if ((extension == "pce") || (extension == "sgx") || (extension == "rom") || (extension == "bin"))
        {
            void *p;
            size_t uncomp_size;

            p = mz_zip_reader_extract_file_to_heap(&zip_archive, file_stat.m_filename, &uncomp_size, 0);
            if (!p)
            {
                Log("ERROR: mz_zip_reader_extract_file_to_heap() failed!");
                mz_zip_reader_end(&zip_archive);
                return false;
            }

            bool ok = LoadFromBuffer((const u8*) p, (int)uncomp_size);

            free(p);
            mz_zip_reader_end(&zip_archive);

            return ok;
        }
    }
    return false;
}

void Cartridge::GatherROMInfo()
{
    m_rom_bank_count = (m_rom_size / 0x2000) + (m_rom_size % 0x2000 ? 1 : 0);
    m_crc = CalculateCRC32(0, m_rom, m_rom_size);

    Log("ROM Size: %d KB, %d bytes (0x%0X)", m_rom_size / 1024, m_rom_size, m_rom_size);
    Log("ROM Bank Count: %d (0x%0X)", m_rom_bank_count, m_rom_bank_count);
    Log("ROM CRC32: %08X", m_crc);

    GatherInfoFromDB();

    if ((m_mapper == STANDARD_MAPPER) && (m_rom_size > 0x100000))
    {
        m_mapper = SF2_MAPPER;
        Log("ROM is bigger than 1MB. Forcing SF2 Mapper.");
    }
}

void Cartridge::GatherInfoFromDB()
{
    int i = 0;
    bool found = false;

    while(!found && (k_game_database[i].title != 0))
    {
        u32 db_crc = k_game_database[i].crc;

        if (db_crc == m_crc)
        {
            found = true;
            Log("ROM found in database: %s. CRC: %08X", k_game_database[i].title, m_crc);

            if (k_game_database[i].flags & GG_GAMEDB_CARD_RAM_8000)
            {
                m_card_ram_size = 0x8000;
                Log("ROM has 32KB of cartridge RAM");
            }

            if (k_game_database[i].flags & GG_GAMEDB_SGX_REQUIRED)
            {
                m_is_sgx = true;
                Log("ROM is a SuperGrafx (SGX) game.");
            }

            if (k_game_database[i].flags & GG_GAMEDB_SGX_OPTIONAL)
            {
                m_is_sgx = true;
                Log("ROM is a SuperGrafx (SGX) optional game.");
            }

            if (k_game_database[i].flags & GG_GAMEDB_SF2_MAPPER)
            {
                m_mapper = SF2_MAPPER;
                Log("ROM uses Street Fighter II mapper.");
            }
            else
            {
                m_mapper = STANDARD_MAPPER;
                Log("ROM uses standard mapper.");
            }
        }
        else
            i++;
    }

    if (!found)
    {
        Debug("ROM not found in database. CRC: %08X", m_crc);
    }
}

void Cartridge::InitRomMAP()
{
    if (m_rom_bank_count == 0x30)
    {
        Debug("Mapping 384KB ROM");

        for(int x = 0; x < 64; x++)
        {
            int bank = x & 0x1F;
            int bank_address = bank * 0x2000;
            m_rom_map[x] = &m_rom[bank_address];
        }

        for(int x = 64; x < 128; x++)
        {
            int bank = (x & 0x0F) + 0x20;
            int bank_address = bank * 0x2000;
            m_rom_map[x] = &m_rom[bank_address];
        }
    }
    else if (m_rom_bank_count == 0x40)
    {
        Debug("Mapping 512KB ROM");

        for(int x = 0; x < 64; x++)
        {
            int bank = x & 0x3F;
            int bank_address = bank * 0x2000;
            m_rom_map[x] = &m_rom[bank_address];
        }

        for(int x = 64; x < 128; x++)
        {
            int bank = (x & 0x1F) + 0x20;
            int bank_address = bank * 0x2000;
            m_rom_map[x] = &m_rom[bank_address];
        }
    }
    else if (m_rom_bank_count == 0x60)
    {
        Debug("Mapping 768KB ROM");

        for(int x = 0; x < 64; x++)
        {
            int bank = x & 0x3F;
            int bank_address = bank * 0x2000;
            m_rom_map[x] = &m_rom[bank_address];
        }

        for(int x = 64; x < 128; x++)
        {
            int bank = (x & 0x1F) + 0x40;
            int bank_address = bank * 0x2000;
            m_rom_map[x] = &m_rom[bank_address];
        }
    }
    else
    {
        Debug("Default mapping ROM");

        for(int x = 0; x < 128; x++)
        {
            int bank = x % m_rom_bank_count;
            int bank_address = bank * 0x2000;
            m_rom_map[x] = &m_rom[bank_address];
        }
    }
}
