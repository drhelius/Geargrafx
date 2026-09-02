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

#ifndef OGG_VORBIS_DECODER_H
#define OGG_VORBIS_DECODER_H

#include <stddef.h>
#include "types.h"

class MediaFile;

class OggVorbisDecoder
{
public:
    OggVorbisDecoder();
    ~OggVorbisDecoder();

    bool Open(MediaFile* file, const char* path);
    void Close();
    bool ReadPcm(u64 offset, u8* buffer, u32 size);
    u64 GetFrameCount() const;

private:
    struct DecoderState;

    static const char* VorbisErrorName(int error);
    static size_t VorbisReadCallback(void* ptr, size_t size, size_t count, void* data_source);
    static int VorbisSeekCallback(void* data_source, s64 offset, int origin);
    static int VorbisCloseCallback(void* data_source);
    static long VorbisTellCallback(void* data_source);

private:
    OggVorbisDecoder(const OggVorbisDecoder&);
    OggVorbisDecoder& operator=(const OggVorbisDecoder&);

    DecoderState* m_state;
    MediaFile* m_file;
    u64 m_frame_count;
    s64 m_current_frame;
};

#endif /* OGG_VORBIS_DECODER_H */
