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

#ifndef CDROM_CHD_FILE_ADAPTER_H
#define CDROM_CHD_FILE_ADAPTER_H

#include <libchdr/chd.h>
#include "types.h"

class CdRomFile;

class CdRomChdFileAdapter
{
public:
    CdRomChdFileAdapter();
    ~CdRomChdFileAdapter();
    bool Open(const char* path);
    core_file* GetCoreFile();

private:
    u64 GetSize();
    size_t Read(void* buffer, size_t size, size_t count);
    int Close();
    int Seek(s64 offset, int whence);
    static u64 SizeCallback(core_file* file);
    static size_t ReadCallback(void* buffer, size_t size, size_t count, core_file* file);
    static int CloseCallback(core_file* file);
    static int SeekCallback(core_file* file, s64 offset, int whence);

private:
    CdRomChdFileAdapter(const CdRomChdFileAdapter&);
    CdRomChdFileAdapter& operator=(const CdRomChdFileAdapter&);
    core_file m_core_file;
    CdRomFile* m_file;
};

#endif /* CDROM_CHD_FILE_ADAPTER_H */
