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

#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include "common.h"

class Cartridge
{
public:
    enum CartridgeMapper
    {
        STANDARD_MAPPER,
        SF2_MAPPER
    };

public:
    Cartridge();
    ~Cartridge();
    void Init();
    void Reset();
    u32 GetCRC();
    bool IsReady();
    bool IsSGX();
    CartridgeMapper GetMapper();
    int GetROMSize();
    int GetROMBankCount();
    const char* GetFilePath();
    const char* GetFileDirectory();
    const char* GetFileName();
    const char* GetFileExtension();
    u8* GetROM();
    u8** GetROMMap();
    bool LoadFromFile(const char* path);
    bool LoadFromBuffer(const u8* buffer, int size);

private:
    bool LoadFromZipFile(const u8* buffer, int size);
    void GatherROMInfo();
    void GatherInfoFromDB();
    void InitRomMAP();

private:
    u8* m_rom;
    u8** m_rom_map;
    int m_rom_size;
    int m_rom_bank_count;
    bool m_ready;
    char m_file_path[512];
    char m_file_directory[512];
    char m_file_name[512];
    char m_file_extension[512];
    u32 m_crc;
    bool m_is_sgx;
    CartridgeMapper m_mapper;
};

#endif /* CARTRIDGE_H */