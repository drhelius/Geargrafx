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
#include "media.h"
#include "game_db.h"
#include "crc.h"
#include "cdrom_media.h"

Media::Media(CdRomMedia* cdrom_media)
{
    m_cdrom_media = cdrom_media;
    InitPointer(m_rom);
    m_rom_size = 0;
    m_card_ram_size = 0;
    m_ready = false;
    m_file_path[0] = 0;
    m_file_name[0] = 0;
    m_file_extension[0] = 0;
    m_temp_path[0] = 0;
    m_bios_name_syscard[0] = 0;
    m_bios_name_gameexpress[0] = 0;
    m_crc = 0;
    m_bios_crc_syscard = 0;
    m_bios_crc_gameexpress = 0;
    m_is_hes = false;
    m_is_gameexpress = false;
    m_is_sgx = false;
    m_is_cdrom = false;
    m_is_valid_bios_syscard = false;
    m_is_valid_bios_gameexpress = false;
    m_is_loaded_bios_syscard = false;
    m_is_loaded_bios_gameexpress = false;
    m_mapper = STANDARD_MAPPER;
    m_avenue_pad_3_button = GG_KEY_SELECT;
    m_console_type = GG_CONSOLE_AUTO;
    m_cdrom_type = GG_CDROM_AUTO;
    m_force_backup_ram = false;
    m_preload_cdrom = false;

    m_rom_map = new u8*[128];
    for (int i = 0; i < 128; i++)
        InitPointer(m_rom_map[i]);
}

Media::~Media()
{
    SafeDeleteArray(m_rom);
    SafeDeleteArray(m_rom_map);
}

void Media::Init()
{
    Reset();
}

void Media::Reset()
{
    SafeDeleteArray(m_rom);
    m_rom_size = 0;
    m_card_ram_size = 0;
    m_ready = false;
    m_file_path[0] = 0;
    m_file_name[0] = 0;
    m_file_extension[0] = 0;
    m_crc = 0;
    m_is_hes = false;
    m_is_gameexpress = false;
    m_is_sgx = false;
    m_is_cdrom = false;
    m_is_mb128 = false;
    m_mapper = STANDARD_MAPPER;
    m_avenue_pad_3_button = GG_KEY_SELECT;

    for (int i = 0; i < 128; i++)
        InitPointer(m_rom_map[i]);

    m_cdrom_media->Reset();
}

void Media::SetTempPath(const char* path)
{
    if (IsValidPointer(path))
    {
        strncpy_fit(m_temp_path, path, sizeof(m_temp_path));
    }
    else
    {
        Error("Invalid temp path %s", path);
    }
}

bool Media::LoadMedia(const char* path)
{
    using namespace std;

    Log("Loading %s...", path);

    if (!IsValidFile(path))
        return false;

    Reset();
    GatherDataFromPath(path);

    if (strcmp(m_file_extension, "zip") == 0)
    {
        m_ready = LoadMediaFromZipFile(path);
    }
    else if (strcmp(m_file_extension, "cue") == 0)
    {
        m_is_cdrom = true;
        m_ready = LoadCueFromFile(path);
    }
    else if (strcmp(m_file_extension, "chd") == 0)
    {
        m_is_cdrom = true;
        m_ready = LoadChdFromFile(path);
    }
    else if (strcmp(m_file_extension, "iso") == 0)
    {
        Log("ISO files are not supported. Please use CUE files.");
        m_ready = false;
    }
    else
    {
        ifstream file;
        open_ifstream_utf8(file, path, ios::in | ios::binary | ios::ate);
        int size = (int)(file.tellg());

        if (file.is_open())
        {
            char* buffer = new char[size];
            file.seekg(0, ios::beg);
            file.read(buffer, size);
            file.close();

            bool is_empty = false;

            for (int i = 0; i < size; i++)
            {
                if (buffer[i] != 0)
                    break;

                if (i == size - 1)
                {
                    Error("File %s is empty!", path);
                    is_empty = true;
                    m_ready = false;
                }
            }

            if (!is_empty)
            {
                m_is_cdrom = false;
                m_ready = LoadHuCardFromBuffer((u8*)(buffer), size, path);
            }

            SafeDeleteArray(buffer);
        }
        else
        {
            Error("There was a problem loading the file %s...", path);
            m_ready = false;
        }
    }

    if (!m_ready)
        Reset();

    return m_ready;
}

bool Media::LoadHuCardFromBuffer(const u8* buffer, int size, const char* path)
{
    Log("Loading HuCard from buffer... Size: %d", size);
    Reset();

    if (!IsValidPointer(buffer) || size <= 0)
    {
        Error("Unable to load HuCard from buffer: Buffer invalid %p. Size: %d", buffer, size);
        return false;
    }

    if (IsValidPointer(path))
        GatherDataFromPath(path);

    if(size & 512)
    {
        Debug("Removing 512 bytes header from HuCard...");
        size &= ~512;
        buffer += 512;
    }

    assert((size % 0x2000) == 0);
    if ((size % 0x2000) != 0)
    {
        Error("Invalid size found: %d (0x%X) bytes", size, size);
    }

    m_rom_size = size;
    m_rom = new u8[m_rom_size];
    memcpy(m_rom, buffer, m_rom_size);
    m_ready = true;

    Debug("HuCard loaded from buffer. Size: %d bytes", m_rom_size);

    return m_ready;
}

bool Media::LoadCueFromFile(const char* path)
{
    m_ready = m_cdrom_media->LoadCueFromFile(path, m_preload_cdrom);
    return m_ready;
}

bool Media::LoadChdFromFile(const char* path)
{
    m_ready = m_cdrom_media->LoadChdFromFile(path, m_preload_cdrom);
    return m_ready;
}

bool Media::LoadBios(const char* file_path, bool syscard)
{
    using namespace std;
    int expected_size = 0;
    u8* bios = NULL;
    u32* bios_crc = NULL;
    bool* loaded_bios;

    if (syscard)
    {
        expected_size = GG_BIOS_SYSCARD_SIZE;
        bios = m_syscard_bios;
        bios_crc = &m_bios_crc_syscard;
        m_is_valid_bios_syscard = false;
        loaded_bios = &m_is_loaded_bios_syscard;
    }
    else
    {
        expected_size = GG_BIOS_GAME_EXPRESS_SIZE;
        bios = m_gameexpress_bios;
        bios_crc = &m_bios_crc_gameexpress;
        m_is_valid_bios_gameexpress = false;
        loaded_bios = &m_is_loaded_bios_gameexpress;
    }

    bool ret = true;

    ifstream file(file_path, ios::in | ios::binary | ios::ate);

    if (file.is_open())
    {
        int size = static_cast<int> (file.tellg());

        if (size != expected_size)
        {
            Log("Incorrect BIOS size %d: expected: %d. %s", size, expected_size, file_path);
        }

        memset(bios, 0x00, expected_size);

        file.seekg(0, ios::beg);

        if(size & 512)
        {
            Log("Removing 512 bytes header from BIOS...");
            size &= ~512;
            file.seekg(512, ios::beg);
        }

        file.read(reinterpret_cast<char*>(bios), MIN(size, expected_size));
        file.close();

        *bios_crc = CalculateCRC32(0, bios, expected_size);
        GatherBIOSInfoFromDB(syscard);

        Log("BIOS loaded (%d bytes): %s", size, file_path);
    }
    else
    {
        Log("There was a problem opening the file %s", file_path);
        ret = false;
    }

    *loaded_bios = ret;

    return ret;
}

bool Media::LoadMediaFromZipFile(const char* path)
{
    Debug("Loading Media from ZIP file: %s", path);

    using namespace std;

    mz_zip_archive zip_archive;
    mz_bool status;
    memset(&zip_archive, 0, sizeof (zip_archive));

    status = mz_zip_reader_init_file(&zip_archive, path, 0);

    if (!status)
    {
        Error("mz_zip_reader_init_mem() failed!");
        return false;
    }

    for (unsigned int i = 0; i < mz_zip_reader_get_num_files(&zip_archive); i++)
    {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat))
        {
            Error("mz_zip_reader_file_stat() failed!");
            mz_zip_reader_end(&zip_archive);
            return false;
        }

        Debug("ZIP Content - Filename: \"%s\", Comment: \"%s\", Uncompressed size: %u, Compressed size: %u", file_stat.m_filename, file_stat.m_comment, (unsigned int) file_stat.m_uncomp_size, (unsigned int) file_stat.m_comp_size);

        string fn((const char*) file_stat.m_filename);
        string extension = fn.substr(fn.find_last_of(".") + 1);
        transform(extension.begin(), extension.end(), extension.begin(), (int(*)(int)) tolower);

        if ((extension == "pce") || (extension == "sgx") || (extension == "hes") || (extension == "rom"))
        {
            void *p;
            size_t uncomp_size;

            p = mz_zip_reader_extract_file_to_heap(&zip_archive, file_stat.m_filename, &uncomp_size, 0);
            if (!p)
            {
                Error("mz_zip_reader_extract_file_to_heap() failed!");
                mz_zip_reader_end(&zip_archive);
                return false;
            }

            bool ok = LoadHuCardFromBuffer((const u8*) p, (int)uncomp_size, fn.c_str());

            free(p);
            mz_zip_reader_end(&zip_archive);

            return ok;
        }
        else if (extension == "cue")
        {
            mz_zip_reader_end(&zip_archive);

            m_is_cdrom = true;
            const char* temp_path = m_temp_path[0] ? m_temp_path : m_file_directory;

            string temppath(temp_path);
            string fullpath(path);
            string filename;

            size_t pos = fullpath.find_last_of("/\\");
            if (pos != string::npos)
                filename = fullpath.substr(pos + 1);
            else
                filename = fullpath;

            temppath += "/" + filename + "_tmp";

            Debug("Loading CD-ROM Media from ZIP file: %s", path);
            Debug("Temporary path: %s", temppath.c_str());

            if (extract_zip_to_folder(path, temppath.c_str()))
            {
                string cue_path = temppath + "/" + fn;
                return LoadCueFromFile(cue_path.c_str());
            }
            else
            {
                Error("Failed to extract ZIP file %s to %s", path, temppath.c_str());
                return false;
            }
        }
    }

    Error("No valid ROM or CUE file found in ZIP archive %s", path);

    return false;
}

void Media::GatherMediaInfo()
{
    if (m_is_cdrom)
    {
        m_crc = m_cdrom_media->GetCRC();
        Log("CD-ROM CRC32: 0x%08X", m_crc);
    }
    else
    {
        m_crc = CalculateCRC32(0, m_rom, m_rom_size);
        Log("HuCard Size: %d KB, %d bytes (0x%0X)", m_rom_size / 1024, m_rom_size, m_rom_size);
        Log("HuCard CRC32: 0x%08X", m_crc);
    }

    GatherMediaInfoFromDB();

    if (m_console_type == GG_CONSOLE_SGX)
    {
        m_is_sgx = true;
        Log("Forcing SuperGrafx (SGX) because of user request");
    }

    if (strcmp(m_file_extension, "hes") == 0)
    {
        m_is_hes = true;
        Log("Media is a HES music rom");
    }

    if (!m_is_sgx && (strcmp(m_file_extension, "sgx") == 0))
    {
        m_is_sgx = true;
        Log("Forcing SuperGrafx (SGX) because of extension");
    }

    if ((m_mapper == STANDARD_MAPPER) && (m_rom_size > 0x100000))
    {
        m_mapper = SF2_MAPPER;
        Log("ROM is bigger than 1MB. Forcing SF2 Mapper.");
    }

    if (m_is_cdrom && (m_cdrom_type != GG_CDROM_STANDARD))
    {
        m_card_ram_size = 0x30000;
        Log("Enabling Super CD-ROM Card RAM");
    }

    switch (m_console_type)
    {
        case GG_CONSOLE_PCE:
            Log("Console Type: PC Engine");
            break;
        case GG_CONSOLE_SGX:
            Log("Console Type: SuperGrafx");
            break;
        case GG_CONSOLE_TG16:
            Log("Console Type: TurboGrafx-16");
            break;
        default:
            Log("Console Type: Auto");
            break;
    }

    switch (m_cdrom_type)
    {
        case GG_CDROM_STANDARD:
            Log("CD-ROM Type: Standard");
            if (m_mapper == ARCADE_CARD_MAPPER)
                m_mapper = STANDARD_MAPPER;
            break;
        case GG_CDROM_SUPER_CDROM:
            Log("CD-ROM Type: Super CD-ROM");
            if (m_mapper == ARCADE_CARD_MAPPER)
                m_mapper = STANDARD_MAPPER;
            break;
        case GG_CDROM_ARCADE_CARD:
            m_mapper = ARCADE_CARD_MAPPER;
            Log("CD-ROM Type: Arcade Card");
            break;
        default:
            Log("CD-ROM Type: Auto");
            break;
    }

    InitRomMAP();
}

void Media::GatherMediaInfoFromDB()
{
    m_card_ram_size = 0;
    m_is_sgx = false;

    int i = 0;
    bool found = false;

    while(!found && (k_game_database[i].title != 0))
    {
        u32 db_crc = k_game_database[i].crc;

        if (db_crc == m_crc)
        {
            found = true;
            Log("Media found in database: %s. CRC: %08X", k_game_database[i].title, m_crc);

            if (k_game_database[i].flags & GG_GAMEDB_CARD_RAM_8000)
            {
                m_card_ram_size = 0x8000;
                Log("Media has 32KB of cartridge RAM");
            }

            if (k_game_database[i].flags & GG_GAMEDB_SGX_REQUIRED)
            {
                m_is_sgx = true;
                Log("Media is a SuperGrafx (SGX) game.");
            }

            if (k_game_database[i].flags & GG_GAMEDB_SGX_OPTIONAL)
            {
                m_is_sgx = true;
                Log("Media is a SuperGrafx (SGX) optional game.");
            }

            if (k_game_database[i].flags & GG_GAMEDB_SF2_MAPPER)
            {
                m_mapper = SF2_MAPPER;
                Log("Media uses Street Fighter II mapper.");
            }
            else if (k_game_database[i].flags & GG_GAMEDB_ARCADE_CARD)
            {
                m_mapper = ARCADE_CARD_MAPPER;
                Log("Media is an Arcade Card game.");
            }
            else
            {
                m_mapper = STANDARD_MAPPER;
                Log("Media uses standard mapper.");
            }

            if (k_game_database[i].flags & GG_GAMEDB_GAME_EXPRESS)
            {
                m_is_gameexpress = true;
                Log("Media is a Game Express game.");
            }

            if (k_game_database[i].flags & GG_GAMEDB_AVENUE_PAD_3_SELECT)
            {
                m_avenue_pad_3_button = GG_KEY_SELECT;
                Log("Media uses Avenue Pad 3 select button.");
            }
            else if (k_game_database[i].flags & GG_GAMEDB_AVENUE_PAD_3_RUN)
            {
                m_avenue_pad_3_button = GG_KEY_RUN;
                Log("Media uses Avenue Pad 3 run button.");
            }

            if (k_game_database[i].flags & GG_GAMEDB_MB128)
            {
                m_is_mb128 = true;
                Log("Media supports MB128 backup memory.");
            }
        }
        else
            i++;
    }

    if (!found)
    {
        Debug("Media not found in database. CRC: %08X", m_crc);
    }
}

void Media::GatherBIOSInfoFromDB(bool syscard)
{
    int i = 0;
    bool found = false;

    u32* bios_crc = syscard ? &m_bios_crc_syscard : &m_bios_crc_gameexpress;
    char* bios_name = syscard ? m_bios_name_syscard : m_bios_name_gameexpress;
    bool* is_valid_bios = syscard ? &m_is_valid_bios_syscard : &m_is_valid_bios_gameexpress;

    while(!found && (k_game_database[i].title != 0))
    {
        u32 db_crc = k_game_database[i].crc;

        if (db_crc == *bios_crc)
        {
            if (syscard && !(k_game_database[i].flags & GG_GAMEDB_BIOS_SYSCARD))
                continue;

            if (!syscard && !(k_game_database[i].flags & GG_GAMEDB_BIOS_GAME_EXPRESS))
                continue;

            found = true;
            *is_valid_bios = true;
            strncpy_fit(bios_name, k_game_database[i].title, 64);

            Log("BIOS found in database: %s. CRC: %08X", k_game_database[i].title, db_crc);
        }
        else
            i++;
    }

    if (!found)
    {
        Debug("BIOS not found in database. CRC: %08X", *bios_crc);
        *is_valid_bios = false;
        strncpy_fit(bios_name, "Unknown", 64);
    }
}

void Media::GatherDataFromPath(const char* path)
{
    using namespace std;

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
}

void Media::InitRomMAP()
{
    int rom_size = m_is_cdrom ? GG_BIOS_SYSCARD_SIZE : m_rom_size;
    int rom_bank_count = (rom_size / 0x2000) + (rom_size % 0x2000 ? 1 : 0);
    u8* bios_ptr = m_is_gameexpress ? m_gameexpress_bios : m_syscard_bios;
    u8* rom_ptr = m_is_cdrom ? bios_ptr : m_rom;

    if (rom_bank_count == 0x30)
    {
        Debug("Mapping 384KB ROM");

        for(int x = 0; x < 64; x++)
        {
            int bank = x & 0x1F;
            int bank_address = bank * 0x2000;
            m_rom_map[x] = &rom_ptr[bank_address];
        }

        for(int x = 64; x < 128; x++)
        {
            int bank = (x & 0x0F) + 0x20;
            int bank_address = bank * 0x2000;
            m_rom_map[x] = &rom_ptr[bank_address];
        }
    }
    else if (rom_bank_count == 0x40)
    {
        Debug("Mapping 512KB ROM");

        for(int x = 0; x < 64; x++)
        {
            int bank = x & 0x3F;
            int bank_address = bank * 0x2000;
            m_rom_map[x] = &rom_ptr[bank_address];
        }

        for(int x = 64; x < 128; x++)
        {
            int bank = (x & 0x1F) + 0x20;
            int bank_address = bank * 0x2000;
            m_rom_map[x] = &rom_ptr[bank_address];
        }
    }
    else if (rom_bank_count == 0x60)
    {
        Debug("Mapping 768KB ROM");

        for(int x = 0; x < 64; x++)
        {
            int bank = x & 0x3F;
            int bank_address = bank * 0x2000;
            m_rom_map[x] = &rom_ptr[bank_address];
        }

        for(int x = 64; x < 128; x++)
        {
            int bank = (x & 0x1F) + 0x40;
            int bank_address = bank * 0x2000;
            m_rom_map[x] = &rom_ptr[bank_address];
        }
    }
    else
    {
        Debug("Default mapping ROM");

        for(int x = 0; x < 128; x++)
        {
            int bank = x % rom_bank_count;
            int bank_address = bank * 0x2000;
            m_rom_map[x] = &rom_ptr[bank_address];
        }
    }
}

bool Media::IsValidFile(const char* path)
{
    using namespace std;

    if (!IsValidPointer(path))
    {
        Error("Invalid path %s", path);
        return false;
    }

    ifstream file;
    open_ifstream_utf8(file, path, ios::in | ios::binary | ios::ate);

    if (file.is_open())
    {
        int size = static_cast<int> (file.tellg());

        if (size <= 0)
        {
            Error("Unable to open file %s. Size: %d", path, size);
            file.close();
            return false;
        }

        if (file.bad() || file.fail() || !file.good() || file.eof())
        {
            Error("Unable to open file %s. Bad file!", path);
            file.close();
            return false;
        }

        file.close();
        return true;
    }
    else
    {
        Error("Unable to open file %s", path);
        return false;
    }
}
