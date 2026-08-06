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

#include "media_file_libretro.h"

#if defined(__LIBRETRO__)

#include "common.h"

const retro_vfs_interface* MediaFileLibretro::s_vfs_interface = NULL;

MediaFileLibretro::MediaFileLibretro()
{
    m_path = NULL;
    m_position = 0;
}

MediaFileLibretro::~MediaFileLibretro()
{
    Close();
}

void MediaFileLibretro::SetVfsInterface(const retro_vfs_interface* iface)
{
    s_vfs_interface = iface;
}

bool MediaFileLibretro::HasVfsInterface()
{
    return s_vfs_interface && s_vfs_interface->open;
}

bool MediaFileLibretro::IsCdRomUriPath(const char* path)
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

unsigned MediaFileLibretro::GetOpenHints(const char* path)
{
    return IsCdRomUriPath(path) ?
        RETRO_VFS_FILE_ACCESS_HINT_NONE : RETRO_VFS_FILE_ACCESS_HINT_FREQUENT_ACCESS;
}

bool MediaFileLibretro::Open(const char* path)
{
    Close();

    if (!IsValidPointer(path))
        return false;

    if (!s_vfs_interface || !s_vfs_interface->open || !s_vfs_interface->close || !s_vfs_interface->size || !s_vfs_interface->read)
        return false;

    size_t path_size = strlen(path) + 1;
    m_path = new char[path_size];
    memcpy(m_path, path, path_size);
    m_file.SetInterface(s_vfs_interface);

    unsigned hints = GetOpenHints(path);

    if (!m_file.Open(path, RETRO_VFS_FILE_ACCESS_READ, hints))
    {
        SafeDeleteArray(m_path);
        m_position = 0;
        return false;
    }

    s64 position = m_file.Tell();
    m_position = (position >= 0) ? position : 0;

    return true;
}

void MediaFileLibretro::Close()
{
    m_file.SetInterface(NULL);
    SafeDeleteArray(m_path);
    m_position = 0;
}

bool MediaFileLibretro::IsOpen() const
{
    return m_file.IsOpen();
}

bool MediaFileLibretro::IsValid() const
{
    return m_file.IsOpen();
}

s64 MediaFileLibretro::GetSize()
{
    return m_file.GetSize();
}

s64 MediaFileLibretro::Tell()
{
    if (!m_file.IsOpen())
        return -1;

    s64 position = m_file.Tell();
    if (position >= 0)
        m_position = position;

    return m_position;
}

bool MediaFileLibretro::Seek(s64 offset)
{
    if (offset < 0)
        return false;

    if (!m_file.IsOpen())
        return false;

    s64 before = Tell();

    if (m_file.CanSeek())
    {
        if (before == offset)
            return true;

        s64 result = m_file.Seek(offset, RETRO_VFS_SEEK_POSITION_START);
        s64 after = m_file.Tell();

        if (result >= 0)
            m_position = (after >= 0) ? after : offset;

        return result >= 0;
    }

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

s64 MediaFileLibretro::Read(void* buffer, u64 size)
{
    if (!IsValidPointer(buffer))
        return -1;

    if (!m_file.IsOpen())
        return -1;

    s64 before = Tell();
    s64 read = m_file.Read(buffer, size);

    if (read > 0)
        m_position = before + read;

    return read;
}

bool MediaFileLibretro::Reopen()
{
    if (!m_path)
        return false;

    unsigned hints = GetOpenHints(m_path);

    if (!m_file.Open(m_path, RETRO_VFS_FILE_ACCESS_READ, hints))
    {
        m_position = 0;
        return false;
    }

    m_position = 0;
    s64 position = m_file.Tell();
    if (position >= 0)
        m_position = position;

    return true;
}

bool MediaFileLibretro::SkipBytes(s64 bytes)
{
    if (bytes < 0)
        return false;

    if (bytes == 0)
        return true;

    if (!m_file.IsOpen())
        return false;

    u8 scratch[4096];
    s64 remaining = bytes;

    while (remaining > 0)
    {
        u64 to_read = (remaining > (s64)sizeof(scratch)) ? (u64)sizeof(scratch) : (u64)remaining;
        s64 read = m_file.Read(scratch, to_read);

        if (read <= 0)
            return false;

        m_position += read;
        remaining -= read;
    }

    return true;
}

#endif