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

#include <vector>
#include <string>
#include "common.h"
#include "cdrom_common.h"
#include "cdrom_image.h"

class CdRomCueBinImage;
class CdRomChdImage;

class CdRomMedia
{
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
    const std::vector<CdRomImage::Track>& GetTracks();
    GG_CdRomTrackType GetTrackType(u8 track_number);
    u8 GetTrackCount();
    GG_CdRomMSF GetCdRomLength();
    u32 GetSectorCount();
    u32 GetCurrentSector();
    void SetCurrentSector(u32 sector);
    bool LoadCueFromFile(const char* path);
    bool LoadChdFromFile(const char* path);
    bool ReadSector(u32 lba, u8* buffer);
    bool ReadSamples(u32 lba, u32 offset, s16* buffer, u32 count);
    u32 SeekTime(u32 start_lba, u32 end_lba);
    u32 SectorTransferCycles();
    u32 GetFirstSectorOfTrack(u8 track);
    u32 GetLastSectorOfTrack(u8 track);
    s32 GetTrackFromLBA(u32 lba);
    bool PreloadTrack(u32 track_number);

private:
    u32 SeekFindGroup(u32 lba);

private:
    CdRomImage* m_current_image;
    CdRomCueBinImage* m_cue_bin_image;
    CdRomChdImage* m_chd_image;
};


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
