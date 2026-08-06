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

#include "cdrom_chd_file_adapter.h"
#include "media_file.h"
#include "common.h"

CdRomChdFileAdapter::CdRomChdFileAdapter()
{
    m_core_file.argp = this;
    m_core_file.fsize = &CdRomChdFileAdapter::SizeCallback;
    m_core_file.fread = &CdRomChdFileAdapter::ReadCallback;
    m_core_file.fclose = &CdRomChdFileAdapter::CloseCallback;
    m_core_file.fseek = &CdRomChdFileAdapter::SeekCallback;
    m_file = NULL;
}

CdRomChdFileAdapter::~CdRomChdFileAdapter()
{
    Close();
}

bool CdRomChdFileAdapter::Open(const char* path)
{
    Close();
    m_file = MediaFile::OpenFile(path);
    return m_file != NULL;
}

core_file* CdRomChdFileAdapter::GetCoreFile()
{
    return &m_core_file;
}

u64 CdRomChdFileAdapter::GetSize()
{
    if (!m_file)
        return (u64)-1;

    s64 size = m_file->GetSize();
    return size >= 0 ? (u64)size : (u64)-1;
}

size_t CdRomChdFileAdapter::Read(void* buffer, size_t size, size_t count)
{
    if (!m_file || (size == 0) || (count == 0) || (count > ((size_t)-1 / size)))
        return 0;

    s64 read = m_file->Read(buffer, size * count);
    return read > 0 ? (size_t)read / size : 0;
}

int CdRomChdFileAdapter::Close()
{
    SafeDelete(m_file);
    return 0;
}

int CdRomChdFileAdapter::Seek(s64 offset, int whence)
{
    if (!m_file)
        return -1;

    s64 base = 0;
    if (whence == SEEK_CUR)
        base = m_file->Tell();
    else if (whence == SEEK_END)
        base = m_file->GetSize();
    else if (whence != SEEK_SET)
        return -1;

    if (base < 0 || offset < -base || (offset > 0 && base > INT64_MAX - offset))
        return -1;

    return m_file->Seek(base + offset) ? 0 : -1;
}

u64 CdRomChdFileAdapter::SizeCallback(core_file* file)
{
    CdRomChdFileAdapter* adapter = reinterpret_cast<CdRomChdFileAdapter*>(file->argp);
    return adapter ? adapter->GetSize() : (u64)-1;
}

size_t CdRomChdFileAdapter::ReadCallback(void* buffer, size_t size, size_t count, core_file* file)
{
    CdRomChdFileAdapter* adapter = reinterpret_cast<CdRomChdFileAdapter*>(file->argp);
    return adapter ? adapter->Read(buffer, size, count) : 0;
}

int CdRomChdFileAdapter::CloseCallback(core_file* file)
{
    CdRomChdFileAdapter* adapter = reinterpret_cast<CdRomChdFileAdapter*>(file->argp);
    return adapter ? adapter->Close() : 0;
}

int CdRomChdFileAdapter::SeekCallback(core_file* file, s64 offset, int whence)
{
    CdRomChdFileAdapter* adapter = reinterpret_cast<CdRomChdFileAdapter*>(file->argp);
    return adapter ? adapter->Seek(offset, whence) : -1;
}
