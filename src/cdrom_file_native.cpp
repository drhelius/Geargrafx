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

#include "cdrom_file_native.h"
#include "common.h"

CdRomFileNative::CdRomFileNative()
{

}

CdRomFileNative::~CdRomFileNative()
{
    Close();
}

bool CdRomFileNative::Open(const char* path)
{
    Close();

    if (!IsValidPointer(path))
        return false;

    open_ifstream_utf8(m_file, path, std::ios::in | std::ios::binary);
    return IsValid();
}

void CdRomFileNative::Close()
{
    if (m_file.is_open())
        m_file.close();
}

bool CdRomFileNative::IsOpen() const
{
    return m_file.is_open();
}

bool CdRomFileNative::IsValid() const
{
    return m_file.is_open() && !m_file.bad() && !m_file.fail() && m_file.good() && !m_file.eof();
}

s64 CdRomFileNative::GetSize()
{
    if (!m_file.is_open())
        return -1;

    if (m_file.bad())
        return -1;

    std::streampos position = m_file.tellg();
    if (position < 0)
        position = 0;

    m_file.clear();
    m_file.seekg(0, std::ios::end);
    std::streampos size = m_file.tellg();
    m_file.clear();
    m_file.seekg(position, std::ios::beg);

    if ((size < 0) || m_file.bad() || m_file.fail())
        return -1;

    return (s64)size;
}

s64 CdRomFileNative::Tell()
{
    if (!m_file.is_open())
        return -1;

    std::streampos position = m_file.tellg();
    if (position < 0)
        return -1;

    return (s64)position;
}

bool CdRomFileNative::Seek(s64 offset)
{
    if (offset < 0)
        return false;

    if (!m_file.is_open())
        return false;

    m_file.clear();
    m_file.seekg(offset, std::ios::beg);
    return !m_file.fail();
}

s64 CdRomFileNative::Read(void* buffer, u64 size)
{
    if (!IsValidPointer(buffer))
        return -1;

    if (!m_file.is_open())
        return -1;

    m_file.read(reinterpret_cast<char*>(buffer), (std::streamsize)size);
    return (s64)m_file.gcount();
}