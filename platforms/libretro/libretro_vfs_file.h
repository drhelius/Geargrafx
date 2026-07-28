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

#ifndef LIBRETRO_VFS_FILE_H
#define LIBRETRO_VFS_FILE_H

#include "types.h"
#include "libretro.h"

class LibretroVfsFile
{
public:
    LibretroVfsFile(const retro_vfs_interface* interface = NULL);
    ~LibretroVfsFile();
    void SetInterface(const retro_vfs_interface* interface);
    bool Open(const char* path, unsigned mode, unsigned hints = RETRO_VFS_FILE_ACCESS_HINT_NONE);
    bool Close();
    bool IsOpen() const;
    bool CanSeek() const;
    s64 GetSize() const;
    s64 Tell() const;
    s64 Seek(s64 offset, int position);
    s64 Read(void* buffer, u64 size);
    s64 Write(const void* buffer, u64 size);
    bool ReadAll(void* buffer, u64 size);
    bool WriteAll(const void* buffer, u64 size);
    bool Flush();

private:
    LibretroVfsFile(const LibretroVfsFile&);
    LibretroVfsFile& operator=(const LibretroVfsFile&);
    const retro_vfs_interface* m_interface;
    retro_vfs_file_handle* m_file;
};

#endif /* LIBRETRO_VFS_FILE_H */
