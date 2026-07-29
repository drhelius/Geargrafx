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

#include "libretro_vfs_file.h"

LibretroVfsFile::LibretroVfsFile(const retro_vfs_interface* iface)
{
    m_interface = iface;
    m_file = NULL;
}

LibretroVfsFile::~LibretroVfsFile()
{
    Close();
}

void LibretroVfsFile::SetInterface(const retro_vfs_interface* iface)
{
    Close();
    m_interface = iface;
}

bool LibretroVfsFile::Open(const char* path, unsigned mode, unsigned hints)
{
    Close();

    if (!m_interface || !m_interface->open || !m_interface->close)
        return false;

    m_file = m_interface->open(path, mode, hints);
    return m_file != NULL;
}

bool LibretroVfsFile::Close()
{
    if (!m_file)
        return true;

    if (!m_interface || !m_interface->close)
    {
        m_file = NULL;
        return false;
    }

    int result = m_interface->close(m_file);
    m_file = NULL;
    return result == 0;
}

bool LibretroVfsFile::IsOpen() const
{
    return m_file != NULL;
}

bool LibretroVfsFile::CanSeek() const
{
    return m_interface && m_interface->seek;
}

s64 LibretroVfsFile::GetSize() const
{
    if (!m_file || !m_interface || !m_interface->size)
        return -1;

    return (s64)m_interface->size(m_file);
}

s64 LibretroVfsFile::Tell() const
{
    if (!m_file || !m_interface || !m_interface->tell)
        return -1;

    return (s64)m_interface->tell(m_file);
}

s64 LibretroVfsFile::Seek(s64 offset, int position)
{
    if (!m_file || !m_interface || !m_interface->seek)
        return -1;

    return (s64)m_interface->seek(m_file, offset, position);
}

s64 LibretroVfsFile::Read(void* buffer, u64 size)
{
    if (!m_file || !m_interface || !m_interface->read)
        return -1;

    return (s64)m_interface->read(m_file, buffer, size);
}

s64 LibretroVfsFile::Write(const void* buffer, u64 size)
{
    if (!m_file || !m_interface || !m_interface->write)
        return -1;

    return (s64)m_interface->write(m_file, buffer, size);
}

bool LibretroVfsFile::ReadAll(void* buffer, u64 size)
{
    u8* destination = reinterpret_cast<u8*>(buffer);
    u64 total = 0;

    while (total < size)
    {
        s64 read = Read(destination + total, size - total);
        if (read <= 0 || (u64)read > size - total)
            return false;

        total += read;
    }

    return true;
}

bool LibretroVfsFile::WriteAll(const void* buffer, u64 size)
{
    const u8* source = reinterpret_cast<const u8*>(buffer);
    u64 total = 0;

    while (total < size)
    {
        s64 written = Write(source + total, size - total);
        if (written <= 0 || (u64)written > size - total)
            return false;

        total += written;
    }

    return true;
}

bool LibretroVfsFile::Flush()
{
    return m_file && m_interface && (!m_interface->flush || (m_interface->flush(m_file) == 0));
}
