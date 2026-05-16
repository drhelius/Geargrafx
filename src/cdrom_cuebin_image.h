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

#include <vector>
#include "cdrom_image.h"

#define GG_CDROM_CUEBIN_DEFAULT_CHUNK_SIZE (2352 * 128)
#define GG_CDROM_CUEBIN_STREAMING_CHUNK_SIZE (2352 * 16)
#define GG_CDROM_CUEBIN_PRELOAD_FULL_TRACK 0

struct GG_CdRomCueBinLoadOptions
{
    u32 chunk_size;
    u32 max_preload_chunks;
    bool allow_disc_preload;
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
    void CalculateCRC();

private:
    std::vector<ImgFile*> m_img_files;
    std::vector<TrackFile> m_track_files;
    GG_CdRomCueBinLoadOptions m_load_options;
};

INLINE GG_CdRomCueBinLoadOptions GG_CdRomCueBinDefaultLoadOptions()
{
    GG_CdRomCueBinLoadOptions options;
    options.chunk_size = GG_CDROM_CUEBIN_DEFAULT_CHUNK_SIZE;
    options.max_preload_chunks = GG_CDROM_CUEBIN_PRELOAD_FULL_TRACK;
    options.allow_disc_preload = true;
    return options;
}

INLINE GG_CdRomCueBinLoadOptions GG_CdRomCueBinStreamingLoadOptions()
{
    GG_CdRomCueBinLoadOptions options;
    options.chunk_size = GG_CDROM_CUEBIN_STREAMING_CHUNK_SIZE;
    options.max_preload_chunks = GG_CDROM_CUEBIN_PRELOAD_FULL_TRACK;
    options.allow_disc_preload = false;
    return options;
}

#endif /* CDROM_CUEBIN_IMAGE_H */