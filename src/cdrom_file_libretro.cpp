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

#include "cdrom_file_libretro.h"

#if defined(__LIBRETRO__)

#include "common.h"

const retro_vfs_interface* CdRomFileLibretro::s_vfs_interface = NULL;

CdRomFileLibretro::CdRomFileLibretro()
{
    m_vfs_interface = NULL;
    m_vfs_file = NULL;
    m_path[0] = 0;
    m_position = 0;
}

CdRomFileLibretro::~CdRomFileLibretro()
{
    Close();
}

void CdRomFileLibretro::SetVfsInterface(const retro_vfs_interface* iface)
{
    s_vfs_interface = iface;
}

bool CdRomFileLibretro::HasVfsInterface()
{
    return s_vfs_interface && s_vfs_interface->open;
}

bool CdRomFileLibretro::IsCdRomUriPath(const char* path)
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

unsigned CdRomFileLibretro::GetOpenHints(const char* path)
{
    return IsCdRomUriPath(path) ?
        RETRO_VFS_FILE_ACCESS_HINT_NONE : RETRO_VFS_FILE_ACCESS_HINT_FREQUENT_ACCESS;
}

bool CdRomFileLibretro::Open(const char* path)
{
    Close();

    if (!IsValidPointer(path))
        return false;

    if (!s_vfs_interface || !s_vfs_interface->open || !s_vfs_interface->close || !s_vfs_interface->size || !s_vfs_interface->read)
        return false;

    strncpy_fit(m_path, path, sizeof(m_path));
    m_vfs_interface = s_vfs_interface;

    unsigned hints = GetOpenHints(path);

    m_vfs_file = m_vfs_interface->open(path, RETRO_VFS_FILE_ACCESS_READ, hints);
    if (!m_vfs_file)
    {
        m_vfs_interface = NULL;
        m_path[0] = 0;
        m_position = 0;
        return false;
    }

    s64 position = m_vfs_interface->tell ? (s64)m_vfs_interface->tell(m_vfs_file) : -1;
    m_position = (position >= 0) ? position : 0;

    return true;
}

void CdRomFileLibretro::Close()
{
    if (m_vfs_file)
    {
        if (m_vfs_interface && m_vfs_interface->close)
            m_vfs_interface->close(m_vfs_file);
    }

    m_vfs_file = NULL;
    m_vfs_interface = NULL;
    m_path[0] = 0;
    m_position = 0;
}

bool CdRomFileLibretro::IsOpen() const
{
    return m_vfs_file != NULL;
}

bool CdRomFileLibretro::IsValid() const
{
    return m_vfs_file != NULL;
}

s64 CdRomFileLibretro::GetSize()
{
    if (!m_vfs_file)
        return -1;

    if (m_vfs_interface && m_vfs_interface->size)
        return (s64)m_vfs_interface->size(m_vfs_file);

    return -1;
}

s64 CdRomFileLibretro::Tell()
{
    if (!m_vfs_file)
        return -1;

    if (m_vfs_interface && m_vfs_interface->tell)
    {
        s64 position = (s64)m_vfs_interface->tell(m_vfs_file);
        if (position >= 0)
            m_position = position;
    }

    return m_position;
}

bool CdRomFileLibretro::Seek(s64 offset)
{
    if (offset < 0)
        return false;

    if (!m_vfs_file || !m_vfs_interface)
        return false;

    s64 before = Tell();

    if (m_vfs_interface->seek)
    {
        if (before == offset)
            return true;

        s64 result = (s64)m_vfs_interface->seek(m_vfs_file, offset, RETRO_VFS_SEEK_POSITION_START);
        s64 after = m_vfs_interface->tell ? (s64)m_vfs_interface->tell(m_vfs_file) : -1;

        if (result >= 0)
            m_position = (after >= 0) ? after : offset;

        return result >= 0;
    }

    if (!m_vfs_interface->read)
        return false;

    if (offset < before)
    {
        if (!Reopen())
            return false;
    }

    if (offset < m_position)
        return false;

    s64 bytes_to_skip = offset - m_position;
    bool skipped = SkipBytes(bytes_to_skip);

    return skipped && (m_position == offset);
}

s64 CdRomFileLibretro::Read(void* buffer, u64 size)
{
    if (!IsValidPointer(buffer))
        return -1;

    if (!m_vfs_file || !m_vfs_interface || !m_vfs_interface->read)
        return -1;

    s64 before = Tell();
    s64 read = (s64)m_vfs_interface->read(m_vfs_file, buffer, size);

    if (read > 0)
        m_position = before + read;

    return read;
}

bool CdRomFileLibretro::Reopen()
{
    if (!m_vfs_interface || !m_vfs_interface->open || (m_path[0] == 0))
        return false;

    if (m_vfs_file)
    {
        if (!m_vfs_interface->close)
            return false;

        m_vfs_interface->close(m_vfs_file);
        m_vfs_file = NULL;
    }

    unsigned hints = GetOpenHints(m_path);

    m_vfs_file = m_vfs_interface->open(m_path, RETRO_VFS_FILE_ACCESS_READ, hints);
    if (!m_vfs_file)
    {
        m_position = 0;
        return false;
    }

    m_position = 0;
    if (m_vfs_interface->tell)
    {
        s64 position = (s64)m_vfs_interface->tell(m_vfs_file);
        if (position >= 0)
            m_position = position;
    }

    return true;
}

bool CdRomFileLibretro::SkipBytes(s64 bytes)
{
    if (bytes < 0)
        return false;

    if (bytes == 0)
        return true;

    if (!m_vfs_file || !m_vfs_interface || !m_vfs_interface->read)
        return false;

    u8 scratch[4096];
    s64 remaining = bytes;

    while (remaining > 0)
    {
        u64 to_read = (remaining > (s64)sizeof(scratch)) ? (u64)sizeof(scratch) : (u64)remaining;
        s64 read = (s64)m_vfs_interface->read(m_vfs_file, scratch, to_read);

        if (read <= 0)
            return false;

        m_position += read;
        remaining -= read;
    }

    return true;
}

#endif