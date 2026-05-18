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

#ifndef CDROM_CUEBIN_IMAGE_H
#define CDROM_CUEBIN_IMAGE_H

#if defined(GG_ENABLE_CDROM_CUEBIN_READAHEAD)
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#endif

#include <vector>
#include "cdrom_image.h"

#define GG_CDROM_CUEBIN_KEEPALIVE_SECONDS 5
#define GG_CDROM_CUEBIN_KEEPALIVE_SIZE 2352
#define GG_CDROM_CUEBIN_PRELOAD_FULL_TRACK 0
#define GG_CDROM_CUEBIN_READAHEAD_QUEUE_SIZE 32

struct GG_CdRomCueBinLoadOptions
{
    u32 chunk_size;
    u32 max_preload_chunks;
    u32 read_ahead_chunks;
    bool allow_disc_preload;
    bool enable_read_ahead;
    bool track_files_start_at_index1;
};

class CdRomFile;

class CdRomCueBinImage : public CdRomImage
{
private:

    struct ImgFile
    {
        char file_name[256];
        char file_path[1024];
        u32 file_size;
        u32 chunk_size;
        u32 chunk_count;
        u8** chunks;
        CdRomFile* file;
        bool is_wav;
        u32 wav_data_offset;
    };

    struct ParsedCueTrack
    {
        u32 number;
        GG_CdRomTrackType type;
        bool has_index0;
        u32 index0_lba;
        bool has_pregap;
        uint32_t pregap_length;
        uint32_t index1_lba;
    };

    struct ParsedCueFile
    {
        ImgFile* img_file;
        std::vector<ParsedCueTrack> tracks;
    };

    struct TrackFile
    {
        ImgFile* img_file;
    };

#if defined(GG_ENABLE_CDROM_CUEBIN_READAHEAD)
    struct ReadAheadRequest
    {
        ImgFile* img_file;
        u32 chunk_index;
    };
#endif

public:
    CdRomCueBinImage();
    virtual ~CdRomCueBinImage();
    virtual void Init() override;
    virtual void Reset() override;
    virtual bool LoadFromFile(const char* path, bool preload) override;
    virtual bool ReadSector(u32 lba, u8* buffer) override;
    virtual bool ReadSamples(u32 lba, u32 offset, s16* buffer, u32 count) override;
    virtual bool PreloadDisc() override;
    virtual bool PreloadTrack(u32 track_number) override;
    void SetLoadOptions(const GG_CdRomCueBinLoadOptions& options);

private:
    void InitImgFile(ImgFile* img_file);
    void InitParsedCueTrack(ParsedCueTrack& track);
    void InitParsedCueFile(ParsedCueFile& cue_file);
    void InitTrackFile(TrackFile& track_file);
    void DestroyImgFiles();
    bool GatherImgInfo(ImgFile* img_file);
    bool OpenImgFile(ImgFile* img_file);
    bool ProcessFileFormat(ImgFile* img_file);
    bool ProcessWavFormat(ImgFile* img_file);
    bool FindWavDataChunk(ImgFile* img_file, CdRomFile& file);
    void SetupFileChunks(ImgFile* img_file);
    u32 CalculateFileOffset(ImgFile* img_file, u32 chunk_index);
    u32 CalculateReadSize(ImgFile* img_file, u32 file_offset);
    bool IsUriPath(const char* path);
    bool ParseCueFile(const char* cue_content);
    bool ReadFromImgFile(ImgFile* img_file, u32 offset, u8* buffer, u32 size);
    bool LoadChunk(ImgFile* img_file, u32 chunk_index);
    bool PreloadChunks(ImgFile* img_file, u32 start_chunk, u32 count);
#if defined(GG_ENABLE_CDROM_CUEBIN_READAHEAD)
    void QueueReadAhead(ImgFile* img_file, u32 start_chunk);
    void QueueChunk(ImgFile* img_file, u32 chunk_index);
    void StartReadAheadWorker();
    void StopReadAheadWorker();
    void ReadAheadThread();
    bool KeepAliveFile();
    void ResetReadAheadQueue();
#endif
    void CalculateCRC();

private:
    std::vector<ImgFile*> m_img_files;
    std::vector<TrackFile> m_track_files;
    GG_CdRomCueBinLoadOptions m_load_options;
#if defined(GG_ENABLE_CDROM_CUEBIN_READAHEAD)
    std::mutex m_chunk_mutex;
    std::mutex m_queue_mutex;
    std::condition_variable m_queue_condition;
    std::thread m_read_ahead_thread;
    std::atomic<bool> m_read_ahead_running;
    ImgFile* m_keep_alive_file;
    ReadAheadRequest m_request_queue[GG_CDROM_CUEBIN_READAHEAD_QUEUE_SIZE];
    u32 m_request_head;
    u32 m_request_tail;
    u32 m_request_count;
#endif
};

INLINE GG_CdRomCueBinLoadOptions GG_CdRomCueBinDefaultLoadOptions()
{
    GG_CdRomCueBinLoadOptions options;

    options.chunk_size = (2352 * 128);
    options.max_preload_chunks = GG_CDROM_CUEBIN_PRELOAD_FULL_TRACK;
    options.read_ahead_chunks = 0;
    options.allow_disc_preload = true;
    options.enable_read_ahead = false;
    options.track_files_start_at_index1 = false;

    return options;
}

INLINE GG_CdRomCueBinLoadOptions GG_CdRomCueBinStreamingLoadOptions()
{
    GG_CdRomCueBinLoadOptions options;

#if defined(GG_ENABLE_CDROM_CUEBIN_READAHEAD)
    options.chunk_size = (2352 * 1);
    options.max_preload_chunks = 8;
    options.read_ahead_chunks = 2;
    options.allow_disc_preload = false;
    options.enable_read_ahead = true;
    options.track_files_start_at_index1 = true;
#else
    options.chunk_size = (2352 * 128);
    options.max_preload_chunks = GG_CDROM_CUEBIN_PRELOAD_FULL_TRACK;
    options.read_ahead_chunks = 0;
    options.allow_disc_preload = false;
    options.enable_read_ahead = false;
    options.track_files_start_at_index1 = true;
#endif

    return options;
}

#endif /* CDROM_CUEBIN_IMAGE_H */