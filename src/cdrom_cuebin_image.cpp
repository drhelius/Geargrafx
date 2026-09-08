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

#include <fstream>
#include <sstream>
#include <algorithm>
#include "cdrom_cuebin_image.h"
#if defined(GG_ENABLE_CDROM_CUEBIN_READAHEAD)
#include <chrono>
#endif
#include "cdrom_common.h"
#include "media_file.h"
#include "ogg_vorbis_decoder.h"
#include "crc.h"

CdRomCueBinImage::CdRomCueBinImage() : CdRomImage()
{
    m_load_options = GG_CdRomCueBinDefaultLoadOptions();
    m_file_resolver = ResolveNormalFile;
    m_file_resolver_user_data = this;
#if defined(GG_ENABLE_CDROM_CUEBIN_READAHEAD)
    m_read_ahead_running.store(false);
    m_keep_alive_file = NULL;
    ResetReadAheadQueue();
#endif
}

CdRomCueBinImage::~CdRomCueBinImage()
{
#if defined(GG_ENABLE_CDROM_CUEBIN_READAHEAD)
    StopReadAheadWorker();
#endif
    DestroyImgFiles();
}

void CdRomCueBinImage::Init()
{
    CdRomImage::Init();
    Reset();
}

void CdRomCueBinImage::Reset()
{
#if defined(GG_ENABLE_CDROM_CUEBIN_READAHEAD)
    StopReadAheadWorker();
    m_keep_alive_file = NULL;
#endif
    CdRomImage::Reset();
    DestroyImgFiles();
    m_file_resolver = ResolveNormalFile;
    m_file_resolver_user_data = this;
}

bool CdRomCueBinImage::LoadFromFile(const char* path, bool preload)
{
    Log("Loading CUE from %s...", path);

    if (!IsValidPointer(path))
    {
        Error("Invalid path %s", path);
        m_ready = false;
        return m_ready;
    }

    CdRomCueBinImage::Reset();
    GatherPaths(path);

    if (strcmp(m_file_extension, "cue") != 0)
    {
        Error("Invalid file extension %s. Expected .cue", m_file_extension);
        m_ready = false;
        return m_ready;
    }

    MediaFile* file = MediaFile::OpenFile(path);

    if (file)
    {
        s64 file_size = file->GetSize();

        if ((file_size <= 0) || (file_size > 0x7FFFFFFF))
        {
            Error("Unable to open file %s. Size: %lld", path, (long long)file_size);
            SafeDelete(file);
            m_ready = false;
            return m_ready;
        }

        if (!file->IsValid())
        {
            Error("Unable to open file %s. Bad file!", path);
            SafeDelete(file);
            m_ready = false;
            return m_ready;
        }

        size_t size = (size_t)file_size;
        u8* buffer = new u8[size];
        bool read = file->ReadAt(0, buffer, size);
        SafeDelete(file);

        if (!read)
        {
            Error("Unable to read complete CUE file %s", path);
            SafeDeleteArray(buffer);
            m_ready = false;
            return m_ready;
        }

        bool empty = true;
        for (size_t i = 0; i < size; i++)
        {
            if (buffer[i] != 0)
            {
                empty = false;
                break;
            }
        }

        if (empty)
            Error("File %s is empty!", path);
        else
            m_ready = LoadFromCueData(path, buffer, size, preload, ResolveNormalFile, this);
        SafeDeleteArray(buffer);
    }
    else
    {
        Error("There was a problem loading the file %s...", path);
        m_ready = false;
    }

    if (!m_ready)
        Reset();

    return m_ready;
}

bool CdRomCueBinImage::LoadFromCueData(const char* source_path, const u8* cue_data,
    size_t cue_size, bool preload, GG_CdRomCueFileResolver resolver, void* resolver_user_data)
{
    if (!IsValidPointer(source_path) || !IsValidPointer(cue_data) || (cue_size == 0) ||
        (cue_size > 0x7FFFFFFF) || !IsValidPointer(resolver))
    {
        Error("Invalid in-memory CUE data");
        return false;
    }

    CdRomCueBinImage::Reset();
    GatherPaths(source_path);
    m_file_resolver = resolver;
    m_file_resolver_user_data = resolver_user_data;

    char* text = new char[cue_size + 1];
    memcpy(text, cue_data, cue_size);
    text[cue_size] = 0;

    if (memchr(text, 0, cue_size) != NULL)
    {
        Error("CUE data contains an embedded NUL byte");
        SafeDeleteArray(text);
        CdRomCueBinImage::Reset();
        return false;
    }

    m_ready = ParseCueFile(text);
    SafeDeleteArray(text);

    if (preload && m_ready)
        m_ready = PreloadDisc();

    if (m_ready)
        CalculateCRC();

#if defined(GG_ENABLE_CDROM_CUEBIN_READAHEAD)
    if (m_ready && m_load_options.enable_read_ahead)
        StartReadAheadWorker();
#endif

    if (!m_ready)
        CdRomCueBinImage::Reset();

    return m_ready;
}

MediaFile* CdRomCueBinImage::ResolveNormalFile(const char* reference, char* resolved_path,
    size_t resolved_path_size, void* user_data)
{
    CdRomCueBinImage* image = reinterpret_cast<CdRomCueBinImage*>(user_data);
    if (!IsValidPointer(image) || !IsValidPointer(reference) || !IsValidPointer(resolved_path) ||
        (resolved_path_size == 0))
    {
        return NULL;
    }

    std::string path = reference;
    if (!path.empty() && !image->IsUriPath(path.c_str()) && (path[0] != '/') && (path[0] != '\\') &&
        ((path.size() < 2) || (path[1] != ':')))
    {
        path = std::string(image->m_file_directory) + "/" + path;
    }

    strncpy_fit(resolved_path, path.c_str(), resolved_path_size);
    return MediaFile::OpenFile(resolved_path);
}

void CdRomCueBinImage::SetLoadOptions(const GG_CdRomCueBinLoadOptions& options)
{
    m_load_options = options;

    if (m_load_options.chunk_size == 0)
        m_load_options.chunk_size = GG_CdRomCueBinDefaultLoadOptions().chunk_size;

#if defined(GG_ENABLE_CDROM_CUEBIN_READAHEAD)
    if (m_load_options.enable_read_ahead && (m_load_options.read_ahead_chunks == 0))
        m_load_options.read_ahead_chunks = GG_CdRomCueBinStreamingLoadOptions().read_ahead_chunks;
#else
    m_load_options.enable_read_ahead = false;
    m_load_options.read_ahead_chunks = 0;
#endif
}

bool CdRomCueBinImage::ReadSector(u32 lba, u8* buffer)
{
    if (!m_ready || buffer == NULL)
    {
        Error("ReadSector failed - Media not ready or buffer is NULL");
        return false;
    }

    s32 track_index = FindTrackFromLBA(lba, false);
    if (track_index < 0)
    {
        Error("ReadSector failed - LBA %d not found in any track", lba);
        return false;
    }

    const Track& track = m_toc.tracks[(size_t)track_index];
    const TrackFile& track_file = m_track_files[(size_t)track_index];
    u32 sector_size = track.sector_size;
    u32 sector_offset = lba - track.start_lba;
    ImgFile* img_file = track_file.img_file;

    if (img_file == NULL || img_file->file_size == 0)
    {
        Error("ReadSector failed - ImgFile is NULL or file size is 0");
        return false;
    }

    u64 sector_byte_offset;
    u64 byte_offset;
    if (!checked_multiply_u64(sector_offset, sector_size, &sector_byte_offset) ||
        !checked_add_u64(track.file_offset, sector_byte_offset, &byte_offset))
    {
        Error("ReadSector failed - Byte offset overflow");
        return false;
    }

    if (sector_size == 2352)
    {
        if (!checked_add_u64(byte_offset, 16, &byte_offset))
            return false;
        sector_size = 2048;
    }

    u64 read_end;
    if (!checked_add_u64(byte_offset, sector_size, &read_end) || (read_end > img_file->file_size))
    {
        Error("ReadSector failed - Byte offset %llu + sector size %u exceeds file size %llu",
            (unsigned long long)byte_offset, sector_size, (unsigned long long)img_file->file_size);
        return false;
    }

    m_current_sector = lba + 1;
    if (m_current_sector >= m_toc.sector_count)
        m_current_sector = m_toc.sector_count - 1;

    Debug("Reading sector %d from track %d (offset: %llu)", lba, track_index,
        (unsigned long long)byte_offset);

    return ReadFromImgFile(img_file, byte_offset, buffer, sector_size);
}

bool CdRomCueBinImage::ReadSamples(u32 lba, u32 offset, s16* buffer, u32 count)
{
    if (!m_ready || buffer == NULL)
    {
        Error("ReadBytes failed - Media not ready or buffer is NULL");
        return false;
    }

    if (lba >= m_toc.sector_count)
    {
        Error("ReadBytes failed - LBA %d out of bounds (max: %d)", lba, m_toc.sector_count - 1);
        return false;
    }

    s32 track_index = FindTrackFromLBA(lba, false);
    if (track_index < 0)
    {
        Error("ReadBytes failed - LBA %d not found in any track", lba);
        return false;
    }

    const Track& track = m_toc.tracks[(size_t)track_index];
    const TrackFile& track_file = m_track_files[(size_t)track_index];
    u32 sector_size = track.sector_size;
    u32 sector_offset = lba - track.start_lba;
    ImgFile* img_file = track_file.img_file;

    if (img_file == NULL || img_file->file_size == 0)
    {
        Error("ReadBytes failed - ImgFile is NULL or file size is 0");
        return false;
    }

    u64 sector_byte_offset;
    u64 byte_offset;
    u64 sample_size;
    if (!checked_multiply_u64(sector_offset, sector_size, &sector_byte_offset) ||
        !checked_add_u64(track.file_offset, sector_byte_offset, &byte_offset) ||
        !checked_add_u64(byte_offset, offset, &byte_offset) ||
        !checked_multiply_u64(count, 2, &sample_size) || (sample_size > UINT32_MAX))
    {
        Error("ReadBytes failed - Byte offset overflow");
        return false;
    }
    u32 size = (u32)sample_size;

    u64 read_end;
    if (!checked_add_u64(byte_offset, size, &read_end) || (read_end > img_file->file_size))
    {
        Error("ReadBytes failed - Byte offset %llu + size %u exceeds file size %llu",
            (unsigned long long)byte_offset, size, (unsigned long long)img_file->file_size);
        return false;
    }

    m_current_sector = lba;

    bool ret = ReadFromImgFile(img_file, byte_offset, (u8*)buffer, size);

#ifdef GG_BIG_ENDIAN
    for (u32 i = 0; i < count; i++)
    {
        u16 u = (u16)buffer[i];
        buffer[i] = (s16)((u >> 8) | (u << 8));
    }
#endif

    return ret;
}

bool CdRomCueBinImage::PreloadDisc()
{
    if (!m_load_options.allow_disc_preload)
    {
        Debug("Skipping full-disc preload for CUE/BIN media");
        return true;
    }

    Debug("Preloading all tracks...");

    size_t files_count = m_img_files.size();

    if (files_count == 0)
    {
        Error("No image files found to preload");
        return false;
    }

    for (size_t i = 0; i < files_count; i++)
    {
        ImgFile* img_file = m_img_files[i];

        if (!IsValidPointer(img_file))
        {
            Error("Invalid ImgFile pointer at index %u when preloading", i);
            return false;
        }

        if (!PreloadChunks(img_file, 0, img_file->chunk_count))
        {
            Error("Failed to preload chunks for ImgFile %s", img_file->file_path);
            return false;
        }
    }

    return true;
}

bool CdRomCueBinImage::PreloadTrack(u32 track_number)
{
    if (track_number >= m_toc.tracks.size())
    {
        Error("PreloadTrackChunks failed - Track number %d out of bounds (max: %d)", track_number, m_toc.tracks.size() - 1);
        return false;
    }

    const Track& track = m_toc.tracks[track_number];
    const TrackFile& track_file = m_track_files[track_number];

    u32 sector_size = track.sector_size;
    u64 start_offset = track.file_offset;
    u64 total_bytes;
    if (!checked_multiply_u64(track.sector_count, sector_size, &total_bytes))
        return false;

    if (total_bytes == 0)
        return true;

    u32 chunk_size = track_file.img_file->chunk_size;
    u64 final_offset;
    if (!checked_add_u64(start_offset, total_bytes - 1, &final_offset))
        return false;
    u64 start_chunk = start_offset / chunk_size;
    u64 end_chunk = final_offset / chunk_size;
    u64 chunks_needed = end_chunk - start_chunk + 1;

#if defined(GG_ENABLE_CDROM_CUEBIN_READAHEAD)
    if (m_load_options.enable_read_ahead)
    {
        QueueReadAhead(track_file.img_file, start_chunk);
        return true;
    }
#endif

    if (m_load_options.max_preload_chunks != GG_CDROM_CUEBIN_PRELOAD_FULL_TRACK)
    {
        chunks_needed = MIN(chunks_needed, m_load_options.max_preload_chunks);
        Debug("Preloading %llu chunk(s) for track %u", (unsigned long long)chunks_needed, track_number);
    }
    else
    {
        Debug("Preloading all sectors for track %u (sectors: %u, bytes: %llu)", track_number,
            track.sector_count, (unsigned long long)total_bytes);
    }

    return PreloadChunks(track_file.img_file, start_chunk, chunks_needed);
}

void CdRomCueBinImage::InitImgFile(ImgFile* img_file)
{
    img_file->file_name[0] = 0;
    img_file->file_path[0] = 0;
    img_file->file_size = 0;
    img_file->chunk_size = 0;
    img_file->chunk_count = 0;
    img_file->chunk_cache_count = 0;
    img_file->chunks = NULL;
    img_file->cached_chunk_indices = NULL;
    InitPointer(img_file->file);
    InitPointer(img_file->ogg_decoder);
    img_file->decoded_pcm_size = 0;
    img_file->is_ogg = false;
    img_file->is_wav = false;
    img_file->wav_data_offset = 0;
}

void CdRomCueBinImage::InitParsedCueTrack(ParsedCueTrack& track)
{
    track.number = 0;
    track.type = GG_CDROM_AUDIO_TRACK;
    track.has_index0 = false;
    track.index0_lba = 0;
    track.has_pregap = false;
    track.pregap_length = 0;
    track.index1_lba = 0;
}

void CdRomCueBinImage::InitParsedCueFile(ParsedCueFile& cue_file)
{
    cue_file.img_file = NULL;
    cue_file.tracks.clear();
}

void CdRomCueBinImage::InitTrackFile(TrackFile& track_file)
{
    track_file.img_file = NULL;
}

void CdRomCueBinImage::DestroyImgFile(ImgFile* img_file)
{
    if (!IsValidPointer(img_file))
        return;

    SafeDelete(img_file->ogg_decoder);
    SafeDelete(img_file->file);

    if (IsValidPointer(img_file->chunks))
    {
        for (u64 i = 0; i < img_file->chunk_cache_count; i++)
            SafeDeleteArray(img_file->chunks[i]);
        SafeDeleteArray(img_file->chunks);
    }

    SafeDeleteArray(img_file->cached_chunk_indices);
    SafeDelete(img_file);
}

void CdRomCueBinImage::DestroyImgFiles()
{
    int img_file_count = (int)(m_img_files.size());
    for (int i = 0; i < img_file_count; i++)
        DestroyImgFile(m_img_files[i]);

    m_img_files.clear();
    m_track_files.clear();
}

bool CdRomCueBinImage::GatherImgInfo(ImgFile* img_file)
{
    if (!IsValidPointer(img_file))
    {
        Error("Invalid ImgFile pointer");
        return false;
    }

    if (!IsValidPointer(img_file->file_path))
    {
        Error("Invalid file path in ImgFile");
        return false;
    }

    if (!OpenImgFile(img_file))
        return false;

    if (!ProcessFileFormat(img_file))
        return false;

    if (!SetupFileChunks(img_file))
        return false;

    Debug("Gathered ImgFile info: %s", img_file->file_path);
    Debug("ImgFile info Size: %llu, Chunk size: %u, Chunk count: %llu",
        (unsigned long long)img_file->file_size, img_file->chunk_size,
        (unsigned long long)img_file->chunk_count);

    return true;
}

bool CdRomCueBinImage::OpenImgFile(ImgFile* img_file)
{
    if (!IsValidPointer(img_file) || !IsValidPointer(img_file->file_path))
        return false;

    SafeDelete(img_file->ogg_decoder);
    SafeDelete(img_file->file);
    img_file->decoded_pcm_size = 0;
    img_file->is_ogg = false;
    img_file->is_wav = false;
    img_file->wav_data_offset = 0;
    if (!IsValidPointer(m_file_resolver))
        return false;

    img_file->file = m_file_resolver(img_file->file_name, img_file->file_path,
        sizeof(img_file->file_path), m_file_resolver_user_data);

    if (img_file->file)
    {
        s64 size = img_file->file->GetSize();

        if (size <= 0)
        {
            Error("Unable to open file %s. Size: %lld", img_file->file_path, (long long)size);
            SafeDelete(img_file->file);
            return false;
        }

        if (!img_file->file->IsValid())
        {
            Error("Unable to open file %s. Bad file!", img_file->file_path);
            SafeDelete(img_file->file);
            return false;
        }

        img_file->file_size = (u64)size;

        return true;
    }

    Error("Unable to open file %s", img_file->file_path);
    SafeDelete(img_file->file);
    return false;
}

bool CdRomCueBinImage::ProcessFileFormat(ImgFile* img_file)
{
    using namespace std;

    string file_path(img_file->file_path);
    string extension = file_path.substr(file_path.find_last_of(".") + 1);
    transform(extension.begin(), extension.end(), extension.begin(), (int(*)(int)) tolower);

    if (!IsValidPointer(img_file->file))
    {
        Error("Invalid open file for %s", img_file->file_path);
        return false;
    }

    if ((extension == "ogg") || (extension == "oga"))
        return ProcessOggFormat(img_file);
    else if (extension == "wav")
        return ProcessWavFormat(img_file);

    return true;
}

bool CdRomCueBinImage::ProcessOggFormat(ImgFile* img_file)
{
    Debug("Ogg Vorbis file detected: %s", img_file->file_path);

    if (!IsValidPointer(img_file->file))
        return false;

    OggVorbisDecoder* decoder = new OggVorbisDecoder;

    if (!decoder->Open(img_file->file, img_file->file_path))
    {
        SafeDelete(decoder);
        return false;
    }

    u64 frame_count = decoder->GetFrameCount();
    u64 decoded_pcm_size = frame_count * 4;
    u64 sector_count = decoded_pcm_size / 2352;

    if ((decoded_pcm_size % 2352) != 0)
        sector_count++;

    if ((sector_count == 0) || (sector_count > (0xFFFFFFFFULL / 2352)))
    {
        Error("Decoded Ogg Vorbis file %s is too large", img_file->file_path);
        SafeDelete(decoder);
        return false;
    }

    img_file->ogg_decoder = decoder;
    img_file->decoded_pcm_size = decoded_pcm_size;
    img_file->file_size = (u32)(sector_count * 2352);
    img_file->is_ogg = true;

    Debug("Ogg Vorbis virtual PCM size: %llu bytes, %llu sector(s)",
        (unsigned long long)decoded_pcm_size, (unsigned long long)sector_count);

    return true;
}

bool CdRomCueBinImage::ProcessWavFormat(ImgFile* img_file)
{
    Debug("WAV file detected: %s", img_file->file_path);

    if (!IsValidPointer(img_file->file))
        return false;

    char header[44];

    if (!img_file->file->Seek(0))
    {
        Error("Failed to seek to WAV header in %s", img_file->file_path);
        return false;
    }

    if (img_file->file->Read(header, 44) != 44)
    {
        Error("Failed to read WAV header from %s", img_file->file_path);
        return false;
    }

    if (strncmp(header, "RIFF", 4) != 0 || strncmp(header + 8, "WAVE", 4) != 0)
    {
        Error("Invalid WAV format in %s", img_file->file_path);
        return false;
    }

    u16 channels = read_u16_le((u8*)(header + 22));
    u32 sample_rate = read_u32_le((u8*)(header + 24));
    u16 bits_per_sample = read_u16_le((u8*)(header + 34));

    if (sample_rate != 44100 || bits_per_sample != 16 || channels != 2)
    {
        Error("WAV file %s has incorrect format. Required: 44100Hz, 16-bit, stereo. Found: %dHz, %d-bit, %d channel(s)", img_file->file_path, sample_rate, bits_per_sample, channels);
        return false;
    }

    Debug("WAV format verified: %dHz, %d-bit, %d channels", sample_rate, bits_per_sample, channels);

    return FindWavDataChunk(img_file, *img_file->file);
}

bool CdRomCueBinImage::FindWavDataChunk(ImgFile* img_file, MediaFile& file)
{
    if (!file.Seek(12))
    {
        Error("Failed to seek to WAV chunks in %s", img_file->file_path);
        return false;
    }

    uint32_t data_size = 0;
    u64 data_offset = 0;
    bool found_data = false;

    while (!found_data)
    {
        char chunk_id[4];
        u8 chunk_size_bytes[4];

        if ((file.Read(chunk_id, 4) != 4) || (file.Read(chunk_size_bytes, 4) != 4))
            break;

        u32 chunk_size = read_u32_le(chunk_size_bytes);

        if (strncmp(chunk_id, "data", 4) == 0)
        {
            data_size = chunk_size;
            s64 position = file.Tell();
            if (position < 0)
                return false;
            data_offset = (u64)position;
            found_data = true;
            break;
        }

        s64 position = file.Tell();
        if ((position < 0) || !file.Seek(position + chunk_size))
            break;
    }
    
    if (!found_data)
    {
        Error("Failed to find 'data' chunk in WAV file %s", img_file->file_path);
        return false;
    }
    
    Debug("WAV data chunk found at offset %llu with size %u",
        (unsigned long long)data_offset, data_size);

    img_file->is_wav = true;
    img_file->wav_data_offset = data_offset;
    img_file->file_size = data_size;

    return true;
}

bool CdRomCueBinImage::SetupFileChunks(ImgFile* img_file)
{
    if (!IsValidPointer(img_file))
    {
        Error("Invalid ImgFile pointer");
        return false;
    }

    img_file->chunk_size = m_load_options.chunk_size;

    if (img_file->chunk_size == 0)
    {
        Error("Invalid chunk size for %s", img_file->file_path);
        return false;
    }

    if (img_file->is_ogg && ((img_file->chunk_size & 3) != 0))
    {
        Error("Ogg Vorbis chunk size must be aligned to 4 bytes: %u", img_file->chunk_size);
        return false;
    }

    img_file->chunk_count = img_file->file_size / img_file->chunk_size;

    if (img_file->file_size % img_file->chunk_size != 0)
        img_file->chunk_count++;

    img_file->chunk_cache_count = img_file->chunk_count;
    if ((m_load_options.max_cached_chunks != 0) &&
        (img_file->chunk_cache_count > m_load_options.max_cached_chunks))
    {
        img_file->chunk_cache_count = MAX(2U, m_load_options.max_cached_chunks);
    }

    const u64 max_chunk_count = (u64)SIZE_MAX / MAX(sizeof(u8*), sizeof(u64));
    if ((img_file->chunk_cache_count == 0) || (img_file->chunk_cache_count > max_chunk_count))
    {
        Error("Invalid chunk cache size for %s: %llu", img_file->file_path,
            (unsigned long long)img_file->chunk_cache_count);
        return false;
    }

    img_file->chunks = new u8*[(size_t)img_file->chunk_cache_count];
    img_file->cached_chunk_indices = new u64[(size_t)img_file->chunk_cache_count];

    for (u64 i = 0; i < img_file->chunk_cache_count; i++)
    {
        InitPointer(img_file->chunks[i]);
        img_file->cached_chunk_indices[i] = UINT64_MAX;
    }

    return true;
}

u64 CdRomCueBinImage::CalculateFileOffset(ImgFile* img_file, u64 chunk_index)
{
    u64 offset;
    if (!checked_multiply_u64(chunk_index, img_file->chunk_size, &offset))
        return UINT64_MAX;

    if (img_file->is_wav)
    {
        if (!checked_add_u64(offset, img_file->wav_data_offset, &offset))
            return UINT64_MAX;
    }

    return offset;
}

u32 CdRomCueBinImage::CalculateReadSize(ImgFile* img_file, u64 file_offset)
{
    u32 to_read = img_file->chunk_size;
    u64 effective_offset = file_offset;

    if (img_file->is_wav)
        effective_offset -= img_file->wav_data_offset;

    if (effective_offset > img_file->file_size)
        return 0;

    if ((u64)to_read > (img_file->file_size - effective_offset))
        to_read = (u32)(img_file->file_size - effective_offset);

    return to_read;
}

bool CdRomCueBinImage::IsUriPath(const char* path)
{
    return IsValidPointer(path) && (strstr(path, "://") != NULL);
}

bool CdRomCueBinImage::ParseCueFile(const char* cue_content)
{
    using namespace std;

    if (!IsValidPointer(cue_content))
    {
        Error("Invalid CUE content pointer");
        return false;
    }

    istringstream stream(cue_content);
    string line;
    vector<ParsedCueFile> parsed_files;
    ParsedCueTrack current_parsed_track;
    InitParsedCueTrack(current_parsed_track);
    bool in_track = false;

    while (getline(stream, line))
    {
        line.erase(0, line.find_first_not_of(" \t"));

        if (line.empty() || line[0] == '#')
            continue;

        string lowercase_line = line;
        transform(lowercase_line.begin(), lowercase_line.end(), lowercase_line.begin(), [](unsigned char c) { return std::tolower(c); });

        if (lowercase_line.find("file") == 0)
        {
            if (in_track)
            {
                in_track = false;
                parsed_files.back().tracks.push_back(current_parsed_track);
            }

            string file_name;

            size_t first_quote = line.find_first_of("\"");
            size_t last_quote = line.find_last_of("\"");

            if (first_quote != string::npos && last_quote != string::npos && first_quote != last_quote)
            {
                file_name = line.substr(first_quote + 1, last_quote - first_quote - 1);
            }
            else
            {
                istringstream file_stream(line.substr(4));
                file_stream >> file_name;

                if (file_name.empty())
                {
                    Error("Invalid FILE format in CUE: %s", line.c_str());
                    return false;
                }
            }

            Debug("Found FILE: %s", file_name.c_str());

            ImgFile* img_file = new ImgFile;
            InitImgFile(img_file);

            strncpy_fit(img_file->file_path, file_name.c_str(), sizeof(img_file->file_path));
            strncpy_fit(img_file->file_name, file_name.c_str(), sizeof(img_file->file_name));

            if (!GatherImgInfo(img_file))
            {
                Error("Failed to gather ImgFile info for %s", file_name.c_str());
                DestroyImgFile(img_file);
                return false;
            }

            m_img_files.push_back(img_file);

            ParsedCueFile parsed_file;
            InitParsedCueFile(parsed_file);
            parsed_file.img_file = img_file;
            parsed_files.push_back(parsed_file);
        }
        else if (lowercase_line.find("track") == 0)
        {
            if (in_track)
                parsed_files.back().tracks.push_back(current_parsed_track);

            in_track = true;
            current_parsed_track = ParsedCueTrack();
            InitParsedCueTrack(current_parsed_track);

            if (parsed_files.empty())
            {
                Error("TRACK found without FILE in CUE");
                return false;
            }

            istringstream track_stream(line.substr(5));
            track_stream >> current_parsed_track.number;

            string type_str;
            track_stream >> type_str;
            transform(type_str.begin(), type_str.end(), type_str.begin(), [](unsigned char c) { return std::tolower(c); });

            if (type_str == "audio")
            {
                current_parsed_track.type = GG_CDROM_AUDIO_TRACK;
                Debug("Found TRACK %d: AUDIO", current_parsed_track.number);
            }
            else if (type_str == "mode1/2048")
            {
                current_parsed_track.type = GG_CDROM_DATA_TRACK_MODE1_2048;
                Debug("Found TRACK %d: DATA (MODE1/2048)", current_parsed_track.number);
            }
            else if (type_str == "mode1/2352")
            {
                current_parsed_track.type = GG_CDROM_DATA_TRACK_MODE1_2352;
                Debug("Found TRACK %d: DATA (MODE1/2352)", current_parsed_track.number);
            }
            else if (type_str.find("mode2/") != string::npos)
            {
                Error("Unsupported track type MODE2: %s", type_str.c_str());
                return false;
            }
            else
            {
                Log("WARNING: Unknown track type: %s", type_str.c_str());
                return false;
            }
        }
        else if (lowercase_line.find("pregap") == 0)
        {
            int m = 0, s = 0, f = 0;
            char colon1, colon2;
            istringstream pregap_stream(line.substr(6));
            if (!(pregap_stream >> m >> colon1 >> s >> colon2 >> f) ||
                colon1 != ':' || colon2 != ':' ||
                m < 0 || s < 0 || f < 0 || s >= 60 || f >= 75 || m > 99)
            {
                Error("Invalid time format in PREGAP entry");
                continue;
            }

            GG_CdRomMSF pregap_msf;
            pregap_msf.minutes = (u8)m;
            pregap_msf.seconds = (u8)s;
            pregap_msf.frames = (u8)f;
            current_parsed_track.pregap_length = MsfToLba(&pregap_msf);
            current_parsed_track.has_pregap = true;

            Debug("Track %d pregap length %02d:%02d:%02d", current_parsed_track.number, m, s, f);
        }
        else if (lowercase_line.find("index") == 0)
        {
            if (!in_track)
            {
                Error("INDEX found outside of TRACK in CUE file");
                return false;
            }

            int index_number;
            istringstream index_stream(line.substr(5));
            index_stream >> index_number;

            int m = 0, s = 0, f = 0;
            char colon1, colon2;

            if (!(index_stream >> m >> colon1 >> s >> colon2 >> f) ||
                colon1 != ':' || colon2 != ':' ||
                m < 0 || s < 0 || f < 0 || s >= 60 || f >= 75 || m > 99)
            {
                Error("Invalid time format in INDEX entry");
                continue;
            }

            GG_CdRomMSF msf;
            msf.minutes = (u8)m;
            msf.seconds = (u8)s;
            msf.frames = (u8)f;

            if (index_number == 0)
            {
                current_parsed_track.index0_lba = MsfToLba(&msf);
                current_parsed_track.has_index0 = true;
                Debug("Track %d lead-in (INDEX 00) at %02d:%02d:%02d", current_parsed_track.number, m, s, f);
            }
            else if (index_number == 1)
            {
                current_parsed_track.index1_lba = MsfToLba(&msf);
                Debug("Track %d starts (INDEX 01) at %02d:%02d:%02d", current_parsed_track.number, m, s, f);
            }
        }
    }

    if (in_track)
        parsed_files.back().tracks.push_back(current_parsed_track);

    if (parsed_files.empty())
    {
        Error("No valid files found in CUE file");
        return false;
    }

    for (size_t i = 0; i < parsed_files.size(); i++)
    {
        ParsedCueFile& f = parsed_files[i];

        if (f.tracks.empty())
        {
            Error("No tracks found for file %s", f.img_file->file_path);
            continue;
        }

        if (!IsValidPointer(f.img_file))
        {
            Error("Invalid ImgFile pointer for file %s", f.img_file->file_path);
            continue;
        }

        if (f.img_file->is_ogg)
        {
            for (size_t j = 0; j < f.tracks.size(); j++)
            {
                if (f.tracks[j].type != GG_CDROM_AUDIO_TRACK)
                {
                    Error("Ogg Vorbis file %s cannot be used by data track %u", f.img_file->file_path, f.tracks[j].number);
                    return false;
                }
            }
        }

        u32 start_sector = (m_toc.tracks.empty() ? 0 : m_toc.tracks.back().end_lba + 1);
        u32 total_pregap_length = 0;

        for (size_t j = 0; j < f.tracks.size(); j++)
        {
            ParsedCueTrack& p = f.tracks[j];
            Track track;
            TrackFile track_file;
            InitTrack(track);
            InitTrackFile(track_file);
            track.type = p.type;
            track.sector_size = TrackTypeSectorSize(p.type);
            track_file.img_file = f.img_file;

            bool file_starts_at_index1 = m_load_options.track_files_start_at_index1 &&
                (f.tracks.size() == 1);

            if (p.has_pregap)
                total_pregap_length += p.pregap_length;

            u32 index1_lba = p.index1_lba;

            if (file_starts_at_index1 && (p.type == GG_CDROM_AUDIO_TRACK))
            {
                bool previous_track_is_data = !m_toc.tracks.empty() &&
                    (m_toc.tracks.back().type != GG_CDROM_AUDIO_TRACK);

                if (!previous_track_is_data)
                    index1_lba = 0;
            }

            track.start_lba = index1_lba + total_pregap_length + start_sector;

            LbaToMsf(track.start_lba, &track.start_msf);

            if (p.has_pregap)
            {
                track.has_lead_in = true;
                track.lead_in_lba = p.index1_lba + total_pregap_length - p.pregap_length + start_sector;
            }
            else if (p.has_index0)
            {
                track.has_lead_in = true;
                track.lead_in_lba = p.index0_lba + start_sector;
            }

            u64 current_file_offset = 0;

            if(j != 0)
            {
                Track& prev = m_toc.tracks.back();
                prev.end_lba = track.has_lead_in ? track.lead_in_lba - 1 : track.start_lba - 1;
                LbaToMsf(prev.end_lba, &prev.end_msf);

                prev.sector_count = prev.end_lba - prev.start_lba + 1;
                u64 previous_size;
                if (!checked_multiply_u64(prev.sector_count, prev.sector_size, &previous_size) ||
                    !checked_add_u64(prev.file_offset, previous_size, &current_file_offset))
                {
                    Error("CUE file offset overflow in %s", f.img_file->file_path);
                    return false;
                }
            }

            track.file_offset = current_file_offset;

            if (track.has_lead_in && !p.has_pregap && !file_starts_at_index1)
            {
                u64 lead_in_size;
                if (!checked_multiply_u64(track.start_lba - track.lead_in_lba,
                    track.sector_size, &lead_in_size) ||
                    !checked_add_u64(track.file_offset, lead_in_size, &track.file_offset))
                {
                    Error("CUE lead-in offset overflow in %s", f.img_file->file_path);
                    return false;
                }
            }

            m_toc.tracks.push_back(track);
            m_track_files.push_back(track_file);
        }

        Track& last = m_toc.tracks.back();
        if (last.file_offset > f.img_file->file_size)
        {
            Error("CUE track offset exceeds file size in %s", f.img_file->file_path);
            return false;
        }

        u64 last_size = f.img_file->file_size - last.file_offset;
        u64 last_sector_count = last_size / last.sector_size;

        if (last_size % last.sector_size != 0)
        {
            Log("WARNING: Last track has remaining bytes that do not fit into a full sector:");
            Log("File size: %llu, File offset: %llu, Sector size: %u",
                (unsigned long long)f.img_file->file_size,
                (unsigned long long)last.file_offset, last.sector_size);
            last_sector_count++;
        }

        if ((last_sector_count == 0) || (last_sector_count > UINT32_MAX) ||
            (last_sector_count > (u64)UINT32_MAX - last.start_lba + 1))
        {
            Error("Invalid CUE track sector count in %s", f.img_file->file_path);
            return false;
        }

        last.sector_count = (u32)last_sector_count;

        last.end_lba = last.start_lba + last.sector_count - 1;
        LbaToMsf(last.end_lba, &last.end_msf);
    }

    for (size_t i = 0; i < m_toc.tracks.size(); ++i)
    {
        Track& track = m_toc.tracks[i];

        Log("Track %2d (%s): Start LBA: %6u, End LBA: %6u, Sectors: %6u, File Offset: %8llu",
            (int)(i + 1),
            TrackTypeName(track.type),
            track.start_lba,
            track.end_lba,
            track.sector_count,
            (unsigned long long)track.file_offset);
    }

    Log("Successfully parsed CUE file with %d tracks", (int)m_toc.tracks.size());

    if (m_toc.tracks.empty())
    {
        m_toc.sector_count = 0;
        m_toc.total_length = {0, 0, 0};
    }
    else
    {
        m_toc.sector_count = m_toc.tracks.back().end_lba + 1;
        LbaToMsf(m_toc.sector_count + 150, &m_toc.total_length);
    }

    Debug("CD-ROM length: %02d:%02d:%02d, Total sectors: %d",
        m_toc.total_length.minutes, m_toc.total_length.seconds, m_toc.total_length.frames,
        m_toc.sector_count);

    return !m_toc.tracks.empty();
}

bool CdRomCueBinImage::ReadFromImgFile(ImgFile* img_file, u64 offset, u8* buffer, u32 size)
{
    if (!IsValidPointer(img_file) || !IsValidPointer(buffer))
    {
        Error("ReadFromImgFile failed - Invalid ImgFile pointer or buffer");
        return false;
    }

    u64 read_end;
    if (!checked_add_u64(offset, size, &read_end) || (read_end > img_file->file_size))
    {
        Error("ReadFromImgFile failed - Offset %llu + size %u exceeds file size %llu",
            (unsigned long long)offset, size, (unsigned long long)img_file->file_size);
        return false;
    }

    const u32 chunk_size = img_file->chunk_size;
    u64 chunk_index = offset / chunk_size;
    u32 chunk_offset = offset % chunk_size;
#if defined(GG_ENABLE_CDROM_CUEBIN_READAHEAD)
    u64 last_chunk_index = chunk_index;
#endif

    bool crosses_chunk = chunk_offset + size > chunk_size;
    if (crosses_chunk && (chunk_index + 1 >= img_file->chunk_count))
    {
        Error("ReadFromImgFile failed - chunk boundary crossing exceeds chunk count (chunk %llu, count %llu)",
            (unsigned long long)(chunk_index + 1), (unsigned long long)img_file->chunk_count);
        return false;
    }

    {
#if defined(GG_ENABLE_CDROM_CUEBIN_READAHEAD)
        std::lock_guard<std::mutex> lock(m_chunk_mutex);
#endif

        if (!LoadChunkUnlocked(img_file, chunk_index))
        {
            Error("Failed to load chunk %llu", (unsigned long long)chunk_index);
            return false;
        }
        if (crosses_chunk && !LoadChunkUnlocked(img_file, chunk_index + 1))
        {
            Error("Failed to load chunk %llu", (unsigned long long)(chunk_index + 1));
            return false;
        }

        u8* first_chunk = GetChunkData(img_file, chunk_index);
        u8* second_chunk = crosses_chunk ? GetChunkData(img_file, chunk_index + 1) : NULL;
        if (!first_chunk || (crosses_chunk && !second_chunk))
            return false;

        if (!crosses_chunk)
        {
            memcpy(buffer, first_chunk + chunk_offset, size);
        }
        else
        {
            u32 first_part = chunk_size - chunk_offset;
            memcpy(buffer, first_chunk + chunk_offset, first_part);
            memcpy(buffer + first_part, second_chunk, size - first_part);
#if defined(GG_ENABLE_CDROM_CUEBIN_READAHEAD)
            last_chunk_index = chunk_index + 1;
#endif
        }
    }

#if defined(GG_ENABLE_CDROM_CUEBIN_READAHEAD)
    QueueReadAhead(img_file, last_chunk_index + 1);
#endif

    return true;
}

bool CdRomCueBinImage::LoadChunk(ImgFile* img_file, u64 chunk_index)
{
    if (!IsValidPointer(img_file))
    {
        Error("Cannot load chunk - Invalid ImgFile pointer");
        return false;
    }

    if (chunk_index >= img_file->chunk_count)
    {
        Error("Cannot load chunk - Chunk index %llu out of bounds (count: %llu)",
            (unsigned long long)chunk_index, (unsigned long long)img_file->chunk_count);
        return false;
    }

#if defined(GG_ENABLE_CDROM_CUEBIN_READAHEAD)
    std::lock_guard<std::mutex> lock(m_chunk_mutex);
#endif

    return LoadChunkUnlocked(img_file, chunk_index);
}

bool CdRomCueBinImage::LoadChunkUnlocked(ImgFile* img_file, u64 chunk_index)
{
    if (!IsValidPointer(img_file) || (chunk_index >= img_file->chunk_count))
        return false;

    u64 slot = GetChunkSlot(img_file, chunk_index);

    if (!IsChunkLoaded(img_file, chunk_index))
    {
        if (!IsValidPointer(img_file->file))
        {
            Error("Cannot load chunk - File is not open %s", img_file->file_path);
            return false;
        }

        u64 file_offset = CalculateFileOffset(img_file, chunk_index);

        if (img_file->is_ogg)
        {
            if (!IsValidPointer(img_file->ogg_decoder))
            {
                Error("Cannot load Ogg Vorbis chunk - Decoder is not open for %s", img_file->file_path);
                SafeDeleteArray(img_file->chunks[slot]);
                return false;
            }

            SafeDeleteArray(img_file->chunks[slot]);
            img_file->cached_chunk_indices[slot] = UINT64_MAX;
            img_file->chunks[slot] = new u8[img_file->chunk_size];
            memset(img_file->chunks[slot], 0, img_file->chunk_size);

            u32 to_read = CalculateReadSize(img_file, file_offset);
            u32 decode_size = 0;

            if ((u64)file_offset < img_file->decoded_pcm_size)
            {
                u64 remaining = img_file->decoded_pcm_size - file_offset;
                decode_size = (remaining < to_read) ? (u32)remaining : to_read;
            }

            Debug("Decoding chunk %llu from %s", (unsigned long long)chunk_index, img_file->file_path);

            if ((decode_size != 0) && !img_file->ogg_decoder->ReadPcm(file_offset, img_file->chunks[slot], decode_size))
            {
                Error("Failed to decode Ogg Vorbis chunk %llu from %s", (unsigned long long)chunk_index, img_file->file_path);
                SafeDeleteArray(img_file->chunks[slot]);
                return false;
            }

            img_file->cached_chunk_indices[slot] = chunk_index;
            return true;
        }

        if ((file_offset == UINT64_MAX) || (file_offset > (u64)INT64_MAX) ||
            !img_file->file->Seek((s64)file_offset))
        {
            Error("Cannot load chunk - Failed to seek to offset %llu in file %s (tell after failure: %lld)",
                (unsigned long long)file_offset, img_file->file_path, (long long)img_file->file->Tell());
            return false;
        }

        SafeDeleteArray(img_file->chunks[slot]);
        img_file->cached_chunk_indices[slot] = UINT64_MAX;
        img_file->chunks[slot] = new u8[img_file->chunk_size];

        u32 to_read = CalculateReadSize(img_file, file_offset);

        Debug("Loading chunk %llu from %s", (unsigned long long)chunk_index, img_file->file_path);
        bool read = (to_read != 0) && img_file->file->ReadExact(img_file->chunks[slot], to_read);

        if (!read)
        {
            Error("Failed to read chunk %llu from %s. Expected %u bytes",
                (unsigned long long)chunk_index, img_file->file_path, to_read);
            SafeDeleteArray(img_file->chunks[slot]);
            return false;
        }

        img_file->cached_chunk_indices[slot] = chunk_index;

#if defined(GG_ENABLE_CDROM_CUEBIN_READAHEAD)
        m_keep_alive_file = img_file;
#endif
    }

    return true;
}

u64 CdRomCueBinImage::GetChunkSlot(const ImgFile* img_file, u64 chunk_index) const
{
    return img_file->chunk_cache_count == img_file->chunk_count ? chunk_index :
        chunk_index % img_file->chunk_cache_count;
}

bool CdRomCueBinImage::IsChunkLoaded(const ImgFile* img_file, u64 chunk_index) const
{
    if (!img_file || !img_file->chunks || !img_file->cached_chunk_indices ||
        (chunk_index >= img_file->chunk_count) || (img_file->chunk_cache_count == 0))
    {
        return false;
    }

    u64 slot = GetChunkSlot(img_file, chunk_index);
    return img_file->chunks[slot] && (img_file->cached_chunk_indices[slot] == chunk_index);
}

u8* CdRomCueBinImage::GetChunkData(const ImgFile* img_file, u64 chunk_index) const
{
    return IsChunkLoaded(img_file, chunk_index) ?
        img_file->chunks[GetChunkSlot(img_file, chunk_index)] : NULL;
}

bool CdRomCueBinImage::PreloadChunks(ImgFile* img_file, u64 start_chunk, u64 count)
{
    if (!IsValidPointer(img_file))
    {
        Error("Cannot preload chunks - Invalid ImgFile pointer");
        return false;
    }

    if (start_chunk >= img_file->chunk_count)
    {
        Error("Cannot preload chunks - Start chunk index %llu out of bounds (max: %llu)",
            (unsigned long long)start_chunk, (unsigned long long)(img_file->chunk_count - 1));
        return false;
    }

    u64 end_chunk;
    if (count > img_file->chunk_cache_count)
        count = img_file->chunk_cache_count;
    if (!checked_add_u64(start_chunk, count, &end_chunk) || (end_chunk > img_file->chunk_count))
        end_chunk = img_file->chunk_count;

    Debug("Preloading chunks %llu-%llu from %s", (unsigned long long)start_chunk,
        (unsigned long long)(end_chunk - 1), img_file->file_path);

    for (u64 i = start_chunk; i < end_chunk; i++)
    {
        if (!LoadChunk(img_file, i))
        {
            Error("Failed to preload chunk %llu", (unsigned long long)i);
            return false;
        }
    }

    return true;
}

#if defined(GG_ENABLE_CDROM_CUEBIN_READAHEAD)
void CdRomCueBinImage::QueueReadAhead(ImgFile* img_file, u64 start_chunk)
{
    if (!m_load_options.enable_read_ahead || (m_load_options.read_ahead_chunks == 0))
        return;

    if (!m_read_ahead_running.load())
        return;

    if (!IsValidPointer(img_file) || (start_chunk >= img_file->chunk_count))
        return;

    for (u32 i = 0; i < m_load_options.read_ahead_chunks; i++)
    {
        u64 chunk_index = start_chunk + i;
        if (chunk_index >= img_file->chunk_count)
            break;

        QueueChunk(img_file, chunk_index);
    }
}

void CdRomCueBinImage::QueueChunk(ImgFile* img_file, u64 chunk_index)
{
    if (!m_read_ahead_running.load())
        return;

    if (!IsValidPointer(img_file) || (chunk_index >= img_file->chunk_count))
        return;

    {
        std::lock_guard<std::mutex> lock(m_chunk_mutex);
        if (img_file->chunks[chunk_index] != NULL)
            return;
    }

    std::lock_guard<std::mutex> lock(m_queue_mutex);

    for (u32 i = 0; i < m_request_count; i++)
    {
        u32 queue_index = (m_request_head + i) % GG_CDROM_CUEBIN_READAHEAD_QUEUE_SIZE;
        if ((m_request_queue[queue_index].img_file == img_file) && (m_request_queue[queue_index].chunk_index == chunk_index))
            return;
    }

    if (m_request_count >= GG_CDROM_CUEBIN_READAHEAD_QUEUE_SIZE)
        return;

    m_request_queue[m_request_tail].img_file = img_file;
    m_request_queue[m_request_tail].chunk_index = chunk_index;
    m_request_tail = (m_request_tail + 1) % GG_CDROM_CUEBIN_READAHEAD_QUEUE_SIZE;
    m_request_count++;
    m_queue_condition.notify_one();
}

void CdRomCueBinImage::StartReadAheadWorker()
{
    if (m_read_ahead_running.load())
        return;

    ResetReadAheadQueue();
    m_read_ahead_running.store(true);
    m_read_ahead_thread = std::thread(&CdRomCueBinImage::ReadAheadThread, this);
}

void CdRomCueBinImage::StopReadAheadWorker()
{
    if (!m_read_ahead_running.load() && !m_read_ahead_thread.joinable())
        return;

    m_read_ahead_running.store(false);
    m_queue_condition.notify_one();

    if (m_read_ahead_thread.joinable())
        m_read_ahead_thread.join();

    ResetReadAheadQueue();
}

void CdRomCueBinImage::ReadAheadThread()
{
    std::chrono::steady_clock::time_point last_keep_alive = std::chrono::steady_clock::now();

    while (m_read_ahead_running.load())
    {
        ReadAheadRequest request;
        request.img_file = NULL;
        request.chunk_index = 0;
        bool has_request = false;

        {
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            if (m_request_count == 0)
                m_queue_condition.wait_for(lock, std::chrono::milliseconds(100));

            if (!m_read_ahead_running.load())
                break;

            if (m_request_count != 0)
            {
                request = m_request_queue[m_request_head];
                m_request_head = (m_request_head + 1) % GG_CDROM_CUEBIN_READAHEAD_QUEUE_SIZE;
                m_request_count--;
                has_request = true;
            }
        }

        if (has_request)
        {
            LoadChunk(request.img_file, request.chunk_index);
            last_keep_alive = std::chrono::steady_clock::now();
            continue;
        }

        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_keep_alive).count() >= GG_CDROM_CUEBIN_KEEPALIVE_SECONDS)
        {
            KeepAliveFile();
            last_keep_alive = now;
        }
    }
}

bool CdRomCueBinImage::KeepAliveFile()
{
    std::lock_guard<std::mutex> lock(m_chunk_mutex);

    ImgFile* img_file = m_keep_alive_file;

    if (IsValidPointer(img_file) && img_file->is_ogg)
        img_file = NULL;

    if (!IsValidPointer(img_file))
    {
        for (size_t i = 0; i < m_img_files.size(); i++)
        {
            if (IsValidPointer(m_img_files[i]) && !m_img_files[i]->is_ogg)
            {
                img_file = m_img_files[i];
                break;
            }
        }
    }

    if (!IsValidPointer(img_file))
        return true;

    if (!IsValidPointer(img_file) || !IsValidPointer(img_file->file) || (img_file->file_size == 0))
        return false;

    u64 file_end_u64 = img_file->file_size;
    if (img_file->is_wav)
    {
        if (!checked_add_u64(file_end_u64, img_file->wav_data_offset, &file_end_u64))
            return false;
    }

    if (file_end_u64 > (u64)INT64_MAX)
        return false;
    s64 file_end = (s64)file_end_u64;

    s64 offset = img_file->file->Tell();
    if ((offset < 0) || (offset >= file_end))
        offset = img_file->is_wav ? (s64)img_file->wav_data_offset : 0;

    if (!img_file->file->Seek(offset))
        return false;

    u32 read_size = (u32)MIN((s64)GG_CDROM_CUEBIN_KEEPALIVE_SIZE, file_end - offset);
    if (read_size == 0)
        return false;

    u8 buffer[GG_CDROM_CUEBIN_KEEPALIVE_SIZE];
    s64 read = img_file->file->Read(buffer, read_size);

    if (read > 0)
    {
        m_keep_alive_file = img_file;
        return true;
    }

    return false;
}

void CdRomCueBinImage::ResetReadAheadQueue()
{
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    m_request_head = 0;
    m_request_tail = 0;
    m_request_count = 0;
}
#endif

void CdRomCueBinImage::CalculateCRC()
{
    m_crc = 0;

    if (m_toc.tracks.empty())
    {
        Log("No tracks to calculate CRC from");
        return;
    }

    Track* first_data_track = NULL;
    TrackFile* first_data_track_file = NULL;
    size_t track_count = m_toc.tracks.size();

    for (size_t i = 0; i < track_count; i++)
    {
        if (m_toc.tracks[i].type == GG_CDROM_DATA_TRACK_MODE1_2048 ||
            m_toc.tracks[i].type == GG_CDROM_DATA_TRACK_MODE1_2352)
        {
            first_data_track = &m_toc.tracks[i];
            first_data_track_file = &m_track_files[i];
            break;
        }
    }

    if (!first_data_track)
    {
        Log("No data tracks found for CRC calculation");
        return;
    }

    if (first_data_track->sector_count == 0)
    {
        Log("First data track has no sectors, cannot calculate CRC");
        return;
    }

    u32 sector_data_size = 2048;
    u8* buffer = new u8[sector_data_size];
    ImgFile* img_file = first_data_track_file->img_file;

    if (!IsValidPointer(img_file))
    {
        Error("Invalid ImgFile pointer for first data track");
        SafeDeleteArray(buffer);
        return;
    }

    if (!IsValidPointer(img_file->file))
    {
        Error("File %s is not open for CRC calculation", img_file->file_path);
        SafeDeleteArray(buffer);
        return;
    }

    u32 sectors_to_crc = 64;
    u32 first_sector = 1;
    u32 last_needed = first_sector + sectors_to_crc - 1;
    u32 max_index = ((first_data_track->sector_count - 1) >= last_needed)
                     ? last_needed
                     : (first_data_track->sector_count - 1);

    for (u32 sec = first_sector; sec <= max_index; sec++)
    {
        u64 sector_offset;
        u64 file_offset;
        if (!checked_multiply_u64(first_data_track->sector_size, sec, &sector_offset) ||
            !checked_add_u64(first_data_track->file_offset, sector_offset, &file_offset))
        {
            Error("CRC file offset overflow for sector %u", sec);
            break;
        }

        if (first_data_track->sector_size == 2352)
        {
            if (!checked_add_u64(file_offset, 16, &file_offset))
                break;
        }

        if (!ReadFromImgFile(img_file, file_offset, buffer, sector_data_size))
        {
            Error("CRC read failed for sector %u in file %s", sec, img_file->file_path);
            break;
        }

        m_crc = CalculateCRC32(m_crc, buffer, sector_data_size);
    }

    SafeDeleteArray(buffer);
}
