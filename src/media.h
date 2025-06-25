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

#ifndef MEDIA_H
#define MEDIA_H

#include "common.h"

class CdRomMedia;

class Media
{
public:
    enum HuCardMapper
    {
        STANDARD_MAPPER,
        SF2_MAPPER,
        ARCADE_CARD_MAPPER
    };

public:
    Media(CdRomMedia* cdrom_media);
    ~Media();
    void Init();
    void Reset();
    u32 GetCRC();
    bool IsReady();
    bool IsSGX();
    bool IsCDROM();
    bool IsGameExpress();
    bool IsValidBios(bool syscard);
    bool IsLoadedBios();
    void SetConsoleType(GG_Console_Type console_type);
    GG_Console_Type GetConsoleType();
    void SetCDROMType(GG_CDROM_Type cdrom_type);
    GG_CDROM_Type GetCDROMType();
    HuCardMapper GetMapper();
    void ForceBackupRAM(bool force);
    bool IsBackupRAMForced();
    int GetROMSize();
    int GetCardRAMSize();
    GG_Keys GetAvenuePad3Button();
    const char* GetFilePath();
    const char* GetFileDirectory();
    const char* GetFileName();
    const char* GetFileExtension();
    const char* GetBiosName(bool syscard);
    u8* GetROM();
    u8** GetROMMap();
    bool LoadMedia(const char* path);
    bool LoadHuCardFromBuffer(const u8* buffer, int size, const char* path);
    bool LoadCueFromBuffer(const u8* buffer, int size, const char* path);
    bool LoadCueFromFile(const char* path);
    bool LoadChdFromBuffer(const u8* buffer, int size, const char* path);
    bool LoadChdFromFile(const char* path);
    bool LoadBios(const char* file_path, bool syscard);
    void SetTempPath(const char* path);
    void GatherMediaInfo();

private:
    bool LoadMediaFromZipFile(const char* path);
    void GatherMediaInfoFromDB();
    void GatherBIOSInfoFromDB(bool syscard);
    void GatherDataFromPath(const char* path);
    void InitRomMAP();
    bool IsValidFile(const char* path);

private:
    CdRomMedia* m_cdrom_media;
    u8* m_rom;
    u8** m_rom_map;
    int m_rom_size;
    int m_card_ram_size;
    bool m_ready;
    char m_file_path[512];
    char m_file_directory[512];
    char m_file_name[512];
    char m_file_extension[512];
    char m_temp_path[512];
    u32 m_crc;
    u32 m_bios_crc_syscard;
    u32 m_bios_crc_gameexpress;
    bool m_is_gameexpress;
    bool m_is_sgx;
    bool m_is_cdrom;
    bool m_is_loaded_bios_syscard;
    bool m_is_loaded_bios_gameexpress;
    bool m_is_valid_bios_syscard;
    bool m_is_valid_bios_gameexpress;
    char m_bios_name_syscard[64];
    char m_bios_name_gameexpress[64];
    HuCardMapper m_mapper;
    GG_Keys m_avenue_pad_3_button;
    GG_Console_Type m_console_type;
    GG_CDROM_Type m_cdrom_type;
    bool m_force_backup_ram;
    u8 m_syscard_bios[GG_BIOS_SYSCARD_SIZE] = {};
    u8 m_gameexpress_bios[GG_BIOS_GAME_EXPRESS_SIZE] = {};
};

#include "media_inline.h"

#endif /* MEDIA_H */
