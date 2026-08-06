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

#ifndef MEDIA_FILE_H
#define MEDIA_FILE_H

#include "types.h"

struct retro_vfs_interface;

class MediaFile
{
public:
    virtual ~MediaFile();

    static MediaFile* OpenFile(const char* path);
    static void SetVfsInterface(const retro_vfs_interface* iface);
    static bool HasVfsInterface();

    virtual bool Open(const char* path) = 0;
    virtual void Close() = 0;
    virtual bool IsOpen() const = 0;
    virtual bool IsValid() const = 0;
    virtual s64 GetSize() = 0;
    virtual s64 Tell() = 0;
    virtual bool Seek(s64 offset) = 0;
    virtual s64 Read(void* buffer, u64 size) = 0;
};

#endif /* MEDIA_FILE_H */
