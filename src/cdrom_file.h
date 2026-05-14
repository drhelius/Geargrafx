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

#ifndef CDROM_FILE_H
#define CDROM_FILE_H

#include <fstream>
#include "types.h"

#if defined(__LIBRETRO__)
struct retro_vfs_interface;
struct retro_vfs_file_handle;
#endif

class CdRomFile
{
public:
    CdRomFile();
    ~CdRomFile();

#if defined(__LIBRETRO__)
    static void SetVfsInterface(const retro_vfs_interface* iface);
    static bool HasVfsInterface();
#endif

    bool Open(const char* path);
    void Close();
    bool IsOpen() const;
    bool IsValid() const;
    s64 GetSize();
    s64 Tell();
    bool Seek(s64 offset);
    s64 Read(void* buffer, u64 size);

private:
#if defined(__LIBRETRO__)
    static const retro_vfs_interface* s_vfs_interface;
    retro_vfs_file_handle* m_vfs_file;
#endif
    std::ifstream m_file;
};

#endif /* CDROM_FILE_H */
