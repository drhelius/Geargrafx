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

#include "cdrom_file.h"
#include "cdrom_file_native.h"
#include "common.h"

#if defined(__LIBRETRO__)
#include "cdrom_file_libretro.h"
#endif

CdRomFile::~CdRomFile()
{

}

CdRomFile* CdRomFile::OpenFile(const char* path)
{
#if defined(__LIBRETRO__)
    if (CdRomFileLibretro::HasVfsInterface())
    {
        CdRomFile* file = new CdRomFileLibretro;
        if (file->Open(path))
            return file;

        SafeDelete(file);

        if (CdRomFileLibretro::IsCdRomUriPath(path))
            return NULL;
    }
#endif

    CdRomFile* file = new CdRomFileNative;
    if (file->Open(path))
        return file;

    SafeDelete(file);
    return NULL;
}

void CdRomFile::SetVfsInterface(const retro_vfs_interface* iface)
{
#if defined(__LIBRETRO__)
    CdRomFileLibretro::SetVfsInterface(iface);
#else
    (void)iface;
#endif
}

bool CdRomFile::HasVfsInterface()
{
#if defined(__LIBRETRO__)
    return CdRomFileLibretro::HasVfsInterface();
#else
    return false;
#endif
}