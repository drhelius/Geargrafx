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


#ifndef MEDIA_FILE_SLICE_H
#define MEDIA_FILE_SLICE_H

#include "media_file.h"

class MediaFileSlice : public MediaFile
{
public:
    MediaFileSlice();
    virtual ~MediaFileSlice();

    bool OpenSlice(const char* path, u64 base_offset, u64 length);

    virtual bool Open(const char* path) override;
    virtual void Close() override;
    virtual bool IsOpen() const override;
    virtual bool IsValid() const override;
    virtual bool CanSeek() const override;
    virtual s64 GetSize() override;
    virtual s64 Tell() override;
    virtual bool Seek(s64 offset) override;
    virtual s64 Read(void* buffer, u64 size) override;

private:
    MediaFile* m_file;
    u64 m_base_offset;
    u64 m_length;
    u64 m_position;
};

#endif /* MEDIA_FILE_SLICE_H */
