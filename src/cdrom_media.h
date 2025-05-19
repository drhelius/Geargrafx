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

#ifndef CDROM_MEDIA_H
#define CDROM_MEDIA_H

#include "common.h"
#include "cdrom_common.h"
#include <vector>
#include <string>

#define CDROM_MEDIA_CHUNK_SIZE 256 * 1024 // 256KB

class CdRomMedia
{
public:

    struct ImgFile
    {
        char file_path[1024];
        u32 file_size;
        u32 chunk_size;
        u32 chunk_count;
        u8** chunks;
    };

    enum TrackType
    {
        AUDIO_TRACK,
        DATA_TRACK_MODE1_2048,
        DATA_TRACK_MODE1_2352
    };

    struct Track
    {
        u32 number;
        TrackType type;
        u32 sector_size;
        u32 sector_count;
        u32 start_lba;
        GG_CdRomMSF start_msf;
        u32 end_lba;
        GG_CdRomMSF end_msf;
        bool has_lead_in;
        u32 lead_in_lba;
        GG_CdRomMSF lead_in_msf;
        ImgFile* img_file;
    };

public:
    CdRomMedia();
    ~CdRomMedia();
    void Init();
    void Reset();
    bool IsReady();
    const char* GetFilePath();
    const char* GetFileDirectory();
    const char* GetFileName();
    const char* GetFileExtension();
    const std::vector<Track>& GetTracks();
    u32 GetTrackSectorSize(TrackType type);
    const char* GetTrackTypeName(TrackType type);
    bool LoadCueFromFile(const char* path);
    bool LoadCueFromBuffer(const u8* buffer, int size, const char* path);
    bool ReadSector(u32 lba, u8* buffer);

private:
    void DestroyImgFiles();
    void GatherPaths(const char* path);
    bool GatherImgInfo(ImgFile* img_file);
    bool ParseCueFile(const char* cue_content);
    bool ReadFromImgFile(ImgFile* img_file, u64 offset, u8* buffer, u32 size);
    bool LoadChunk(ImgFile* img_file, u32 chunk_index);
    bool PreloadChunks(ImgFile* img_file, u32 start_chunk, u32 count);
    bool PreloadTrackChunks(u32 track_number, u32 sectors);

private:
    bool m_ready;
    char m_file_path[512];
    char m_file_directory[512];
    char m_file_name[512];
    char m_file_extension[512];
    std::vector<Track> m_tracks;
    std::vector<ImgFile*> m_img_files;
};

static const u32 k_cdrom_track_type_size[3] = { 2352, 2048, 2352};
static const char* k_cdrom_track_type_name[3] = { "AUDIO", "MODE1/2048", "MODE1/2352" }; 

INLINE u32 CdRomMedia::GetTrackSectorSize(CdRomMedia::TrackType type)
{
    return k_cdrom_track_type_size[type];
}

INLINE const char* CdRomMedia::GetTrackTypeName(CdRomMedia::TrackType type)
{
    return k_cdrom_track_type_name[type];
}

#endif /* CDROM_MEDIA_H */