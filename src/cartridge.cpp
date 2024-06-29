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

#include "cartridge.h"
#include "miniz/miniz.c"
#include "game_db.h"
#include <string>
#include <fstream>
#include <algorithm>

Cartridge::Cartridge()
{
    InitPointer(m_rom);
    m_rom_size = 0;
    m_ready = false;
    m_file_path[0] = 0;
    m_file_name[0] = 0;
    m_file_extension[0] = 0;
    m_rom_bank_count = 0;
    m_crc = 0;
}

Cartridge::~Cartridge()
{
    SafeDeleteArray(m_rom);
}

void Cartridge::Init()
{
    Reset();
}

void Cartridge::Reset()
{
    SafeDeleteArray(m_rom);
    m_rom_size = 0;
    m_ready = false;
    m_file_path[0] = 0;
    m_file_name[0] = 0;
    m_file_extension[0] = 0;
    m_rom_bank_count = 0;
    m_crc = 0;
}

u32 Cartridge::GetCRC()
{
    return m_crc;
}

bool Cartridge::IsReady()
{
    return m_ready;
}

int Cartridge::GetROMSize()
{
    return m_rom_size;
}

int Cartridge::GetROMBankCount()
{
    return m_rom_bank_count;
}

const char* Cartridge::GetFilePath()
{
    return m_file_path;
}

const char* Cartridge::GetFileName()
{
    return m_file_name;
}

u8* Cartridge::GetROM()
{
    return m_rom;
}

bool Cartridge::LoadFromFile(const char* path)
{
    using namespace std;

    Debug("Loading %s...", path);

    Reset();

    string pathstr(path);
    string filename;
    string extension;

    size_t pos = pathstr.find_last_of("/\\");
    if (pos != string::npos)
        filename = pathstr.substr(pos + 1);
    else
        filename = pathstr;

    extension = pathstr.substr(pathstr.find_last_of(".") + 1);
    transform(extension.begin(), extension.end(), extension.begin(), (int(*)(int)) tolower);

    strncpy(m_file_path, path, 512);
    strncpy(m_file_name, filename.c_str(), 512);
    strncpy(m_file_extension, extension.c_str(), 512);

    ifstream file(path, ios::in | ios::binary | ios::ate);

    if (file.is_open())
    {
        int size = static_cast<int> (file.tellg());
        char* memblock = new char[size];
        file.seekg(0, ios::beg);
        file.read(memblock, size);
        file.close();

        if (extension == "zip")
            m_ready = LoadFromZipFile(reinterpret_cast<u8*> (memblock), size);
        else
            m_ready = LoadFromBuffer(reinterpret_cast<u8*> (memblock), size);

        SafeDeleteArray(memblock);
    }
    else
    {
        Log("There was a problem loading the file %s...", path);
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
        Debug("Loading ROM from buffer... Size: %d", size);

        if ((size % 0x2000) != 0)
        {
            Log("Invalid size found: %d bytes", size);
            return false;
        }

        m_rom_size = size;
        m_rom = new u8[m_rom_size];
        memcpy(m_rom, buffer, m_rom_size);

        GatherROMInfo();
        m_ready = true;

        Debug("ROM loaded from buffer. Size: %d bytes", m_rom_size);

        return true;
    }
    else
    {
        Log("Unable to load ROM from buffer: Buffer invalid %p. Size: %d", buffer, size);
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
        Log("mz_zip_reader_init_mem() failed!");
        return false;
    }

    for (unsigned int i = 0; i < mz_zip_reader_get_num_files(&zip_archive); i++)
    {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat))
        {
            Log("mz_zip_reader_file_stat() failed!");
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
                Log("mz_zip_reader_extract_file_to_heap() failed!");
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

    Debug("ROM Size: %d KB, %d bytes", m_rom_size / 1024, m_rom_size);
    Debug("ROM Bank Count: %d", m_rom_bank_count);
    Debug("ROM CRC32: %08X", m_crc);

    GatherInfoFromDB();
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
        }
        else
            i++;
    }

    if (!found)
    {
        Debug("ROM not found in database. CRC: %08X", m_crc);
    }
}
