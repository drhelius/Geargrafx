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

#include <limits.h>
#include <string.h>
#define OV_EXCLUDE_STATIC_CALLBACKS
#include <minivorbis.h>
#include "ogg_vorbis_decoder.h"
#include "media_file.h"
#include "common.h"

struct OggVorbisDecoder::DecoderState
{
    OggVorbis_File vorbis_file;
    bool open;
};

OggVorbisDecoder::OggVorbisDecoder()
{
    m_state = NULL;
    m_file = NULL;
    m_frame_count = 0;
    m_current_frame = -1;
}

OggVorbisDecoder::~OggVorbisDecoder()
{
    Close();
}

bool OggVorbisDecoder::Open(MediaFile* file, const char* path)
{
    Close();

    if (!IsValidPointer(file) || !IsValidPointer(path))
    {
        Error("Unable to open Ogg Vorbis stream: invalid file or path");
        return false;
    }

    DecoderState* state = new DecoderState;
    memset(state, 0, sizeof(DecoderState));

    ov_callbacks callbacks;
    callbacks.read_func = VorbisReadCallback;
    callbacks.seek_func = VorbisSeekCallback;
    callbacks.close_func = VorbisCloseCallback;
    callbacks.tell_func = VorbisTellCallback;

    int result = ov_open_callbacks(file, &state->vorbis_file, NULL, 0, callbacks);

    if (result != 0)
    {
        Error("Invalid Ogg Vorbis file %s: %s (%d)", path, VorbisErrorName(result), result);
        SafeDelete(state);
        return false;
    }

    state->open = true;
    m_state = state;
    m_file = file;

    if (ov_seekable(&state->vorbis_file) == 0)
    {
        Error("Ogg Vorbis file %s is not seekable", path);
        Close();
        return false;
    }

    long stream_count = ov_streams(&state->vorbis_file);

    if (stream_count != 1)
    {
        Error("Ogg Vorbis file %s contains %ld logical streams; chained streams are not supported", path, stream_count);
        Close();
        return false;
    }

    vorbis_info* info = ov_info(&state->vorbis_file, 0);

    if (info == NULL)
    {
        Error("Unable to read Ogg Vorbis stream information from %s", path);
        Close();
        return false;
    }

    if ((info->rate != 44100) || (info->channels != 2))
    {
        Error("Ogg Vorbis file %s has incorrect format. Required: 44100Hz, stereo. Found: %ldHz, %d channel(s)", path, info->rate, info->channels);
        Close();
        return false;
    }

    ogg_int64_t frame_count = ov_pcm_total(&state->vorbis_file, -1);

    if (frame_count <= 0)
    {
        Error("Ogg Vorbis file %s has no decoded audio frames", path);
        Close();
        return false;
    }

    if ((u64)frame_count > (0xFFFFFFFFFFFFFFFFULL / 4))
    {
        Error("Ogg Vorbis file %s is too large", path);
        Close();
        return false;
    }

    result = ov_pcm_seek(&state->vorbis_file, 0);

    if (result != 0)
    {
        Error("Unable to seek Ogg Vorbis file %s to its first PCM frame: %s (%d)", path, VorbisErrorName(result), result);
        Close();
        return false;
    }

    m_frame_count = (u64)frame_count;
    m_current_frame = 0;

    Debug("Ogg Vorbis format verified: 44100Hz, 16-bit output, 2 channels, %llu frames", (unsigned long long)m_frame_count);

    return true;
}

void OggVorbisDecoder::Close()
{
    if (m_state != NULL)
    {
        if (m_state->open)
            ov_clear(&m_state->vorbis_file);

        SafeDelete(m_state);
    }

    m_file = NULL;
    m_frame_count = 0;
    m_current_frame = -1;
}

bool OggVorbisDecoder::ReadPcm(u64 offset, u8* buffer, u32 size)
{
    if ((m_state == NULL) || !m_state->open || (m_file == NULL) || !IsValidPointer(buffer))
    {
        Error("Unable to decode Ogg Vorbis PCM: decoder is not open or buffer is invalid");
        return false;
    }

    if (((offset & 3) != 0) || ((size & 3) != 0))
    {
        Error("Unable to decode Ogg Vorbis PCM: offset %llu and size %u must be aligned to 4 bytes", (unsigned long long)offset, size);
        return false;
    }

    u64 pcm_size = m_frame_count * 4;

    if ((offset > pcm_size) || ((u64)size > (pcm_size - offset)))
    {
        Error("Unable to decode Ogg Vorbis PCM: offset %llu + size %u exceeds decoded size %llu", (unsigned long long)offset, size, (unsigned long long)pcm_size);
        return false;
    }

    if (size == 0)
        return true;

    s64 requested_frame = (s64)(offset / 4);

    if (m_current_frame != requested_frame)
    {
        int result = ov_pcm_seek(&m_state->vorbis_file, (ogg_int64_t)requested_frame);

        if (result != 0)
        {
            Error("Unable to seek Ogg Vorbis PCM to frame %lld: %s (%d)", (long long)requested_frame, VorbisErrorName(result), result);
            m_current_frame = -1;
            return false;
        }

        m_current_frame = requested_frame;
    }

    u32 total_read = 0;

    while (total_read < size)
    {
        u32 remaining = size - total_read;
        int request_size = (remaining > (u32)(INT_MAX & ~3)) ? (INT_MAX & ~3) : (int)remaining;
        int bitstream = -1;

        long read = ov_read(&m_state->vorbis_file, (char*)buffer + total_read, request_size, 0, 2, 1, &bitstream);

        if (read < 0)
        {
            Error("Ogg Vorbis PCM decode failed at frame %lld: %s (%ld)", (long long)m_current_frame, VorbisErrorName((int)read), read);
            m_current_frame = -1;
            return false;
        }

        if (read == 0)
        {
            Error("Ogg Vorbis PCM decode ended early at frame %lld", (long long)m_current_frame);
            m_current_frame = -1;
            return false;
        }

        if ((bitstream != 0) || ((read & 3) != 0))
        {
            Error("Ogg Vorbis PCM decoder returned invalid stream %d or byte count %ld", bitstream, read);
            m_current_frame = -1;
            return false;
        }

        total_read += (u32)read;
        m_current_frame += read / 4;
    }

    return true;
}

u64 OggVorbisDecoder::GetFrameCount() const
{
    return m_frame_count;
}

const char* OggVorbisDecoder::VorbisErrorName(int error)
{
    switch (error)
    {
        case OV_FALSE:      return "no data";
        case OV_EOF:        return "end of file";
        case OV_HOLE:       return "corrupt stream hole";
        case OV_EREAD:      return "read error";
        case OV_EFAULT:     return "internal decoder fault";
        case OV_EIMPL:      return "unsupported feature";
        case OV_EINVAL:     return "invalid argument or stream state";
        case OV_ENOTVORBIS: return "not a Vorbis stream";
        case OV_EBADHEADER: return "invalid Vorbis header";
        case OV_EVERSION:   return "unsupported Vorbis version";
        case OV_ENOTAUDIO:  return "not Vorbis audio";
        case OV_EBADPACKET: return "invalid Vorbis packet";
        case OV_EBADLINK:   return "invalid logical stream";
        case OV_ENOSEEK:    return "stream is not seekable";
        default:            return "unknown decoder error";
    }
}

size_t OggVorbisDecoder::VorbisReadCallback(void* ptr, size_t size, size_t count, void* data_source)
{
    if ((ptr == NULL) || (data_source == NULL) || (size == 0) || (count == 0))
        return 0;

    if (count > (SIZE_MAX / size))
        return 0;

    size_t byte_count = size * count;

    if ((u64)byte_count > 0x7FFFFFFFFFFFFFFFULL)
        return 0;

    MediaFile* file = static_cast<MediaFile*>(data_source);

    s64 read = file->Read(ptr, (u64)byte_count);

    if ((read <= 0) || ((u64)read > (u64)byte_count))
        return 0;

    return (size_t)read / size;
}

int OggVorbisDecoder::VorbisSeekCallback(void* data_source, s64 offset, int origin)
{
    if (data_source == NULL)
        return -1;

    MediaFile* file = static_cast<MediaFile*>(data_source);
    s64 base = 0;

    switch (origin)
    {
        case SEEK_SET:
            base = 0;
            break;
        case SEEK_CUR:
            base = file->Tell();
            break;
        case SEEK_END:
            base = file->GetSize();
            break;
        default:
            return -1;
    }

    if (base < 0)
        return -1;

    s64 target = 0;

    if (offset < 0)
    {
        u64 magnitude = (u64)(-(offset + 1)) + 1;

        if ((u64)base < magnitude)
            return -1;

        target = base - (s64)magnitude;
    }
    else
    {
        if (base > (s64)(0x7FFFFFFFFFFFFFFFULL - (u64)offset))
            return -1;

        target = base + (s64)offset;
    }

    return file->Seek(target) ? 0 : -1;
}

int OggVorbisDecoder::VorbisCloseCallback(void* data_source)
{
    UNUSED(data_source);
    return 0;
}

long OggVorbisDecoder::VorbisTellCallback(void* data_source)
{
    if (data_source == NULL)
        return -1;

    MediaFile* file = static_cast<MediaFile*>(data_source);

    s64 position = file->Tell();

    if ((position < 0) || ((u64)position > (u64)LONG_MAX))
        return -1;

    return (long)position;
}
