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

class CdRomMedia;

class Cartridge
{
public:
    enum CartridgeMapper
    {
        STANDARD_MAPPER,
        SF2_MAPPER
    };

public:
    Cartridge(CdRomMedia* cdrom_media);
    ~Cartridge();
    void Init();
    void Reset();
    u32 GetCRC();
    bool IsReady();
    bool IsSGX();
    bool IsCDROM();
    bool IsBios();
    bool IsValidBios();
    void ForceSGX(bool enable);
    CartridgeMapper GetMapper();
    int GetROMSize();
    int GetROMBankCount();
    int GetCardRAMSize();
    GG_Keys GetAvenuePad3Button();
    const char* GetFilePath();
    const char* GetFileDirectory();
    const char* GetFileName();
    const char* GetFileExtension();
    u8* GetROM();
    u8** GetROMMap();
    bool LoadFromFile(const char* path);
    bool LoadFromBuffer(const u8* buffer, int size, const char* path);
    bool LoadBios(u8* buffer, int size);
    void SetTempPath(const char* path);

private:
    bool LoadFromZipFile(const char* path);
    void GatherROMInfo();
    void GatherInfoFromDB();
    void GatherDataFromPath(const char* path);
    void InitRomMAP();

private:
    CdRomMedia* m_cdrom_media;
    u8* m_rom;
    u8** m_rom_map;
    int m_rom_size;
    int m_rom_bank_count;
    int m_card_ram_size;
    bool m_ready;
    char m_file_path[512];
    char m_file_directory[512];
    char m_file_name[512];
    char m_file_extension[512];
    char m_temp_path[512];
    u32 m_crc;
    bool m_is_sgx;
    bool m_force_sgx;
    bool m_is_cdrom;
    bool m_is_bios;
    bool m_is_valid_bios;
    CartridgeMapper m_mapper;
    GG_Keys m_avenue_pad_3_button;
};

#endif /* CARTRIDGE_H */