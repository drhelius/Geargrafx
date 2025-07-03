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

#ifndef CDROM_IMAGE_H
#define CDROM_IMAGE_H

#include "common.h"
#include "cdrom_common.h"
#include <vector>

class CdRomImage
{
public:

    struct Track
    {
        GG_CdRomTrackType type;
        u32 sector_size;
        u32 sector_count;
        u32 start_lba;
        GG_CdRomMSF start_msf;
        u32 end_lba;
        GG_CdRomMSF end_msf;
        bool has_lead_in;
        u32 lead_in_lba;
    };

    struct TableOfContents
    {
        std::vector<Track> tracks;
        GG_CdRomMSF total_length;
        u32 sector_count;
    };

public:
    CdRomImage();
    virtual ~CdRomImage();
    virtual void Init();
    virtual void Reset();
    virtual bool LoadFromFile(const char* path) = 0;
    virtual bool ReadSector(u32 lba, u8* buffer) = 0;
    virtual bool ReadBytes(u32 lba, u32 offset, u8* buffer, u32 size) = 0;
    virtual bool PreloadDisc() = 0;
    virtual bool PreloadTrack(u32 track_number) = 0;
    bool IsReady();
    u32 GetFirstSectorOfTrack(u8 track);
    u32 GetLastSectorOfTrack(u8 track);
    s32 GetTrackFromLBA(u32 lba);
    const char* GetFilePath();
    const char* GetFileDirectory();
    const char* GetFileName();
    const char* GetFileExtension();
    TableOfContents* GetTOC();
    u32 GetCRC();
    u32 GetCurrentSector();
    void SetCurrentSector(u32 sector);

protected:
    void GatherPaths(const char* path);
    void InitTrack(Track& track);

protected:
    TableOfContents m_toc;
    bool m_ready;
    char m_file_path[512];
    char m_file_directory[512];
    char m_file_name[512];
    char m_file_extension[512];
    u32 m_current_sector;
    u32 m_crc;
};

#endif /* CDROM_IMAGE_H */