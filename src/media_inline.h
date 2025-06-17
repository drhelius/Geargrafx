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

#ifndef MEDIA_INLINE_H
#define MEDIA_INLINE_H

#include "media.h"
#include "cdrom_media.h"

inline u32 Media::GetCRC()
{
    return m_crc;
}

inline u32 Media::GetCdromCRC()
{
    return m_cdrom_media->GetCRC();
}

inline bool Media::IsReady()
{
    return m_ready;
}

inline bool Media::IsSGX()
{
    return m_is_sgx;
}

inline bool Media::IsCDROM()
{
    return m_is_cdrom;
}

inline bool Media::IsGameExpress()
{
    return m_is_gameexpress;
}

inline bool Media::IsValidBios(bool syscard)
{
    return syscard ? m_is_valid_bios_syscard : m_is_valid_bios_gameexpress;
}

inline bool Media::IsLoadedBios()
{
    return m_is_gameexpress ? m_is_loaded_bios_gameexpress : m_is_loaded_bios_syscard;
}

inline void Media::SetConsoleType(GG_Console_Type console_type)
{
    m_console_type = console_type;
}

inline GG_Console_Type Media::GetConsoleType()
{
    return m_console_type;
}

inline void Media::SetCDROMType(GG_CDROM_Type cdrom_type)
{
    m_cdrom_type = cdrom_type;
}

inline GG_CDROM_Type Media::GetCDROMType()
{
    return m_cdrom_type;
}

inline Media::HuCardMapper Media::GetMapper()
{
    return m_mapper;
}

inline void Media::ForceBackupRAM(bool force)
{
    m_force_backup_ram = force;
}

inline bool Media::IsBackupRAMForced()
{
    return m_force_backup_ram;
}

inline int Media::GetROMSize()
{
    return m_rom_size;
}

inline int Media::GetCardRAMSize()
{
    return m_card_ram_size;
}

inline GG_Keys Media::GetAvenuePad3Button()
{
    return m_avenue_pad_3_button;
}

inline const char* Media::GetFilePath()
{
    return m_file_path;
}

inline const char* Media::GetFileDirectory()
{
    return m_file_directory;
}

inline const char* Media::GetFileName()
{
    return m_file_name;
}

inline const char* Media::GetFileExtension()
{
    return m_file_extension;
}

inline const char* Media::GetBiosName(bool syscard)
{
    return syscard ? m_bios_name_syscard : m_bios_name_gameexpress;
}

inline u8* Media::GetROM()
{
    return m_rom;
}

inline u8** Media::GetROMMap()
{
    return m_rom_map;
}

#endif /* MEDIA_INLINE_H */