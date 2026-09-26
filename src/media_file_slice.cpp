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

#include "media_file_slice.h"
#include "common.h"

MediaFileSlice::MediaFileSlice()
{
    InitPointer(m_file);
    m_base_offset = 0;
    m_size = 0;
    m_position = 0;
}

MediaFileSlice::~MediaFileSlice()
{
    Close();
}

bool MediaFileSlice::OpenSlice(const char* path, u64 base_offset, u64 size)
{
    Close();

    if (!IsValidPointer(path) || (base_offset > (u64)INT64_MAX) || (size > (u64)INT64_MAX))
        return false;

    u64 end_offset;

    if (!checked_add_u64(base_offset, size, &end_offset) || (end_offset > (u64)INT64_MAX))
        return false;

    m_file = MediaFile::OpenFile(path);

    if (!IsValidPointer(m_file) || !m_file->CanSeek())
    {
        Close();
        return false;
    }

    s64 file_size = m_file->GetSize();

    if ((file_size < 0) || (end_offset > (u64)file_size) || !m_file->Seek((s64)base_offset))
    {
        Close();
        return false;
    }

    m_base_offset = base_offset;
    m_size = size;
    m_position = 0;

    return true;
}

bool MediaFileSlice::Open(const char* path)
{
    UNUSED(path);

    return false;
}

void MediaFileSlice::Close()
{
    SafeDelete(m_file);
    m_base_offset = 0;
    m_size = 0;
    m_position = 0;
}

bool MediaFileSlice::IsOpen() const
{
    return IsValidPointer(m_file) && m_file->IsOpen();
}

bool MediaFileSlice::IsValid() const
{
    return IsValidPointer(m_file) && m_file->IsValid() && (m_position <= m_size);
}

bool MediaFileSlice::CanSeek() const
{
    return IsValidPointer(m_file) && m_file->CanSeek();
}

s64 MediaFileSlice::GetSize()
{
    return IsOpen() ? (s64)m_size : -1;
}

s64 MediaFileSlice::Tell()
{
    return IsOpen() ? (s64)m_position : -1;
}

bool MediaFileSlice::Seek(s64 offset)
{
    if (!IsOpen() || (offset < 0) || ((u64)offset > m_size))
        return false;

    u64 absolute_offset;

    if (!checked_add_u64(m_base_offset, (u64)offset, &absolute_offset) || (absolute_offset > (u64)INT64_MAX))
        return false;

    if (!m_file->Seek((s64)absolute_offset))
        return false;

    m_position = (u64)offset;

    return true;
}

s64 MediaFileSlice::Read(void* buffer, u64 size)
{
    if (!IsOpen() || (!IsValidPointer(buffer) && (size != 0)))
        return -1;

    u64 available_size = m_size - m_position;
    u64 read_size = MIN(size, available_size);

    if (read_size == 0)
        return 0;

    s64 bytes_read = m_file->Read(buffer, read_size);

    if ((bytes_read < 0) || ((u64)bytes_read > read_size))
        return -1;

    m_position += (u64)bytes_read;

    return bytes_read;
}
