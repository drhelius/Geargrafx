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
#include "common.h"

#if defined(__LIBRETRO__)
#include "libretro.h"

const retro_vfs_interface* CdRomFile::s_vfs_interface = NULL;

static bool IsCdRomUriPath(const char* path)
{
    static const char* kCdRomUriPrefix = "cdrom://";

    if (!path)
        return false;

    for (int index = 0; kCdRomUriPrefix[index]; index++)
    {
        if (path[index] != kCdRomUriPrefix[index])
            return false;
    }

    return true;
}
#endif

CdRomFile::CdRomFile()
{
#if defined(__LIBRETRO__)
    m_vfs_file = NULL;
#endif
}

CdRomFile::~CdRomFile()
{
    Close();
}

#if defined(__LIBRETRO__)
void CdRomFile::SetVfsInterface(const retro_vfs_interface* iface)
{
    s_vfs_interface = iface;
}

bool CdRomFile::HasVfsInterface()
{
    return s_vfs_interface != NULL;
}
#endif

bool CdRomFile::Open(const char* path)
{
    Close();

    if (!IsValidPointer(path))
        return false;

#if defined(__LIBRETRO__)
    if (s_vfs_interface && s_vfs_interface->open)
    {
        unsigned hints = IsCdRomUriPath(path) ?
            RETRO_VFS_FILE_ACCESS_HINT_NONE : RETRO_VFS_FILE_ACCESS_HINT_FREQUENT_ACCESS;
        m_vfs_file = s_vfs_interface->open(path, RETRO_VFS_FILE_ACCESS_READ, hints);
        if (m_vfs_file)
            return true;
    }
#endif

    open_ifstream_utf8(m_file, path, std::ios::in | std::ios::binary);
    return IsValid();
}

void CdRomFile::Close()
{
#if defined(__LIBRETRO__)
    if (m_vfs_file)
    {
        if (s_vfs_interface && s_vfs_interface->close)
            s_vfs_interface->close(m_vfs_file);
        m_vfs_file = NULL;
    }
#endif

    if (m_file.is_open())
        m_file.close();
}

bool CdRomFile::IsOpen() const
{
#if defined(__LIBRETRO__)
    if (m_vfs_file)
        return true;
#endif

    return m_file.is_open();
}

bool CdRomFile::IsValid() const
{
#if defined(__LIBRETRO__)
    if (m_vfs_file)
        return true;
#endif

    return m_file.is_open() && !m_file.bad() && !m_file.fail() && m_file.good() && !m_file.eof();
}

s64 CdRomFile::GetSize()
{
#if defined(__LIBRETRO__)
    if (m_vfs_file)
    {
        if (s_vfs_interface && s_vfs_interface->size)
            return (s64)s_vfs_interface->size(m_vfs_file);
        return -1;
    }
#endif

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

s64 CdRomFile::Tell()
{
#if defined(__LIBRETRO__)
    if (m_vfs_file)
    {
        if (s_vfs_interface && s_vfs_interface->tell)
            return (s64)s_vfs_interface->tell(m_vfs_file);
        return -1;
    }
#endif

    if (!m_file.is_open())
        return -1;

    std::streampos position = m_file.tellg();
    if (position < 0)
        return -1;

    return (s64)position;
}

bool CdRomFile::Seek(s64 offset)
{
    if (offset < 0)
        return false;

#if defined(__LIBRETRO__)
    if (m_vfs_file)
    {
        if (s_vfs_interface && s_vfs_interface->seek)
            return s_vfs_interface->seek(m_vfs_file, offset, RETRO_VFS_SEEK_POSITION_START) >= 0;
        return false;
    }
#endif

    if (!m_file.is_open())
        return false;

    m_file.clear();
    m_file.seekg(offset, std::ios::beg);
    return !m_file.fail();
}

s64 CdRomFile::Read(void* buffer, u64 size)
{
    if (!IsValidPointer(buffer))
        return -1;

#if defined(__LIBRETRO__)
    if (m_vfs_file)
    {
        if (s_vfs_interface && s_vfs_interface->read)
            return (s64)s_vfs_interface->read(m_vfs_file, buffer, size);
        return -1;
    }
#endif

    if (!m_file.is_open())
        return -1;

    m_file.read(reinterpret_cast<char*>(buffer), (std::streamsize)size);
    return (s64)m_file.gcount();
}
