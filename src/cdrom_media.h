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
        char file_name[256];
        char file_path[1024];
        u32 file_size;
        u32 chunk_size;
        u32 chunk_count;
        u8** chunks;
        bool is_wav;
        u32 wav_data_offset;
    };

    enum TrackType
    {
        AUDIO_TRACK,
        DATA_TRACK_MODE1_2048,
        DATA_TRACK_MODE1_2352
    };

    struct Track
    {
        TrackType type;
        u32 sector_size;
        u32 sector_count;
        u32 start_lba;
        GG_CdRomMSF start_msf;
        u32 end_lba;
        GG_CdRomMSF end_msf;
        ImgFile* img_file;
        u32 file_offset;
        bool has_lead_in;
        u32 lead_in_lba;
    };

private:

    struct ParsedCueTrack
    {
        u32 number;
        TrackType type;
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

public:
    CdRomMedia();
    ~CdRomMedia();
    void Init();
    void Reset();
    bool IsReady();
    u32 GetCRC();
    const char* GetFilePath();
    const char* GetFileDirectory();
    const char* GetFileName();
    const char* GetFileExtension();
    const std::vector<Track>& GetTracks();
    const std::vector<ImgFile*>& GetImgFiles();
    u32 GetTrackSectorSize(TrackType type);
    u32 GetTrackSectorSize(u8 track_number);
    TrackType GetTrackType(u8 track_number);
    const char* GetTrackTypeName(TrackType type);
    const char* GetTrackTypeName(u8 track_number);
    u8 GetTrackCount();
    GG_CdRomMSF GetCdRomLength();
    u32 GetSectorCount();
    u32 GetCurrentSector();
    void SetCurrentSector(u32 sector);
    bool LoadCueFromFile(const char* path);
    bool LoadCueFromBuffer(const u8* buffer, int size, const char* path);
    bool ReadSector(u32 lba, u8* buffer);
    bool ReadBytes(u32 lba, u32 offset, u8* buffer, u32 size);
    u32 SeekTime(u32 start_lba, u32 end_lba);
    u32 SectorTransferCycles();
    u32 GetFirstSectorOfTrack(u8 track);
    u32 GetLastSectorOfTrack(u8 track);
    s32 GetTrackFromLBA(u32 lba);
    bool PreloadChunks(ImgFile* img_file, u32 start_chunk, u32 count);
    bool PreloadTrackChunks(u32 track_number);

private:
    void InitTrack(Track& track);
    void InitParsedCueTrack(ParsedCueTrack& track);
    void InitParsedCueFile(ParsedCueFile& cue_file);
    void DestroyImgFiles();
    void GatherPaths(const char* path);
    bool GatherImgInfo(ImgFile* img_file);
    bool ValidateFile(const char* file_path);
    bool ProcessFileFormat(ImgFile* img_file);
    bool ProcessWavFormat(ImgFile* img_file);
    bool FindWavDataChunk(ImgFile* img_file, std::ifstream& file);
    void SetupFileChunks(ImgFile* img_file);
    u32 CalculateFileOffset(ImgFile* img_file, u32 chunk_index);
    u32 CalculateReadSize(ImgFile* img_file, u32 file_offset);
    bool ParseCueFile(const char* cue_content);
    bool ReadFromImgFile(ImgFile* img_file, u32 offset, u8* buffer, u32 size);
    bool LoadChunk(ImgFile* img_file, u32 chunk_index);
    void CalculateCRC();
    u32 SeekFindGroup(u32 lba);

private:
    bool m_ready;
    u32 m_crc;
    char m_file_path[512];
    char m_file_directory[512];
    char m_file_name[512];
    char m_file_extension[512];
    std::vector<Track> m_tracks;
    std::vector<ImgFile*> m_img_files;
    GG_CdRomMSF m_cdrom_length;
    u32 m_sector_count;
    u32 m_current_sector;
};

static const u32 k_cdrom_track_type_size[3] = { 2352, 2048, 2352};
static const char* const k_cdrom_track_type_name[3] = { "AUDIO", "MODE1/2048", "MODE1/2352" };

INLINE u32 CdRomMedia::SectorTransferCycles()
{
    // Standard CD-ROM 1x speed: 75 sectors/sec
    return GG_MASTER_CLOCK_RATE / 75;
}

// Seek time, based on the work by Dave Shadoff
// https://github.com/pce-devel/PCECD_seek
struct GG_Seek_Sector_Group
{
    u32 sec_per_revolution;
    u32 sec_start;
    u32 sec_end;
    double rotation_ms;
};

#define GG_SEEK_NUM_SECTOR_GROUPS 14
static const GG_Seek_Sector_Group k_seek_sector_list[GG_SEEK_NUM_SECTOR_GROUPS] = {
    { 10,   0,      12572,  133.47 },
    { 11,   12573,  30244,  146.82 },   // Except for the first and last groups,
    { 12,   30245,  49523,  160.17 },   // there are 1606.5 tracks in each range
    { 13,   49524,  70408,  173.51 },
    { 14,   70409,  92900,  186.86 },
    { 15,   92901,  116998, 200.21 },
    { 16,   116999, 142703, 213.56 },
    { 17,   142704, 170014, 226.90 },
    { 18,   170015, 198932, 240.25 },
    { 19,   198933, 229456, 253.60 },
    { 20,   229457, 261587, 266.95 },
    { 21,   261588, 295324, 280.29 },
    { 22,   295325, 330668, 293.64 },
    { 23,   330669, 333012, 306.99 }
};

#include "cdrom_media_inline.h"

#endif /* CDROM_MEDIA_H */