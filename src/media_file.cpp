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

#include "media_file.h"
#include "media_file_native.h"
#include "common.h"

#if defined(__LIBRETRO__)
#include "media_file_libretro.h"
#endif

MediaFile::~MediaFile()
{

}

MediaFile* MediaFile::OpenFile(const char* path)
{
#if defined(__LIBRETRO__)
    if (MediaFileLibretro::HasVfsInterface())
    {
        MediaFile* file = new MediaFileLibretro;
        if (file->Open(path))
            return file;

        SafeDelete(file);
        return NULL;
    }

    if (MediaFileLibretro::IsCdRomUriPath(path))
        return NULL;
#endif

    MediaFile* file = new MediaFileNative;
    if (file->Open(path))
        return file;

    SafeDelete(file);
    return NULL;
}

void MediaFile::SetVfsInterface(const retro_vfs_interface* iface)
{
#if defined(__LIBRETRO__)
    MediaFileLibretro::SetVfsInterface(iface);
#else
    (void)iface;
#endif
}

bool MediaFile::HasVfsInterface()
{
#if defined(__LIBRETRO__)
    return MediaFileLibretro::HasVfsInterface();
#else
    return false;
#endif
}