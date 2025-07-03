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


#include "cdrom_chd_image.h"

CdRomChdImage::CdRomChdImage() : CdRomImage()
{
    InitPointer(m_chd_file);
}

CdRomChdImage::~CdRomChdImage()
{
    chd_close(m_chd_file);
}

void CdRomChdImage::Init()
{
    CdRomImage::Init();
    Reset();
}

void CdRomChdImage::Reset()
{
    CdRomImage::Reset();

    chd_close(m_chd_file);
    InitPointer(m_chd_file);
}

bool CdRomChdImage::LoadFromFile(const char* path)
{
    using namespace std;

    Log("Loading CHD from %s...", path);

    if (!IsValidPointer(path))
    {
        Log("ERROR: Invalid path %s", path);
        m_ready = false;
        return m_ready;
    }

    Reset();
    GatherPaths(path);

    if (strcmp(m_file_extension, "chd") != 0)
    {
        Log("ERROR: Invalid file extension %s. Expected .chd", m_file_extension);
        m_ready = false;
        return m_ready;
    }

    chd_error err = chd_open(path, CHD_OPEN_READ, NULL, &m_chd_file);

    if (err == CHDERR_NONE)
    {
        const chd_header* header = chd_get_header(m_chd_file);

        Debug("CHD Header: Version: %d, Hunk Size: %d, Total Hunks: %d, Flags: %04X",
              header->version, header->hunkbytes, header->totalhunks, header->flags);

        m_ready = ReadTOC();
    }
    else
    {
        chd_close(m_chd_file);
        m_ready = false;
        Log("ERROR: Unable to open CHD file %s.", path);
        Log("CHD ERROR: %d, %s", err, chd_error_string(err));
    }

    if (!m_ready)
        Reset();

    return m_ready;
}

bool CdRomChdImage::ReadSector(u32 lba, u8* buffer)
{
    if (!m_ready || buffer == NULL)
    {
        Debug("ERROR: ReadSector failed - Media not ready or buffer is NULL");
        return false;
    }

    

    Debug("ERROR: ReadSector failed - LBA %d not found in any track", lba);

    return false;
}

bool CdRomChdImage::ReadBytes(u32 lba, u32 offset, u8* buffer, u32 size)
{
    if (!m_ready || buffer == NULL)
    {
        Debug("ERROR: ReadBytes failed - Media not ready or buffer is NULL");
        return false;
    }

    if (lba >= m_toc.sector_count)
    {
        Debug("ERROR: ReadBytes failed - LBA %d out of bounds (max: %d)", lba, m_toc.sector_count - 1);
        return false;
    }

    

    Debug("ERROR: ReadBytes failed - LBA %d not found in any track", lba);

    return false;
}

bool CdRomChdImage::PreloadDisc()
{
    return true;
}

bool CdRomChdImage::PreloadTrack(u32 track_number)
{
    return true;
}

bool CdRomChdImage::ReadTOC()
{
    chd_error err;
    char metadata[512];

    u32 current_lba = 0;

    for (int i = 0; i < 99; i++)
    {
        bool track_exists = false;
        int track = 0, frames = 0, pad = 0, pregap = 0, postgap = 0;
        char type[64], subtype[32], pgtype[32], pgsub[32];
        type[0] = subtype[0] = pgtype[0] = pgsub[0] = 0;

        err = chd_get_metadata(m_chd_file, CDROM_TRACK_METADATA2_TAG, i, metadata, sizeof(metadata), NULL, NULL, NULL);

        if (err == CHDERR_NONE)
        {
            if (sscanf(metadata, CDROM_TRACK_METADATA2_FORMAT, &track, type, subtype, &frames, &pregap, pgtype, pgsub, &postgap) != 8)
            {
                Log("ERROR: Failed to parse CDROM_TRACK_METADATA2_FORMAT for track %d", i + 1);
                return false;
            }

            track_exists = true;
        }
        else
        {
            err = chd_get_metadata(m_chd_file, CDROM_TRACK_METADATA_TAG, i, metadata, sizeof(metadata), NULL, NULL, NULL);

            if (err == CHDERR_NONE)
            {
                if (sscanf(metadata, CDROM_TRACK_METADATA_FORMAT, &track, type, subtype, &frames) != 4)
                {
                    Log("ERROR: Failed to parse CDROM_TRACK_METADATA_FORMAT for track %d", i + 1);
                    return false;
                }

                track_exists = true;
            }
        }

        if (!track_exists)
            break;

        Debug("Track %d: Type: %s, Subtype: %s, Frames: %d, Pregap: %d, Postgap: %d, PGType: %s, PGSub: %s", 
                track, type, subtype, frames, pregap, postgap, pgtype, pgsub);

        Track new_track;
        InitTrack(new_track);

        new_track.type = GetTrackType(type);
        new_track.sector_size = TrackTypeSectorSize(new_track.type);
        new_track.sector_count = frames;

        new_track.start_lba = current_lba;
        LbaToMsf(current_lba, &new_track.start_msf);

        new_track.end_lba = current_lba + frames - 1;
        LbaToMsf(new_track.end_lba, &new_track.end_msf);

        if (pregap > 0)
        {
            new_track.has_lead_in = true;
            new_track.lead_in_lba = current_lba - pregap;
            LbaToMsf(new_track.lead_in_lba, &new_track.start_msf);
        }
        else
        {
            new_track.has_lead_in = false;
            new_track.lead_in_lba = 0;
        }

        current_lba += frames;

        m_toc.tracks.push_back(new_track);
    }

    for (size_t i = 0; i < m_toc.tracks.size(); ++i)
    {
        Track& track = m_toc.tracks[i];

        Log("Track %2d (%s): Start LBA: %6u, End LBA: %6u, Sectors: %6u",
                i + 1,
                TrackTypeName(track.type),
                track.start_lba,
                track.end_lba,
                track.sector_count);
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

    CalculateCRC();

    return !m_toc.tracks.empty();
}

void CdRomChdImage::CalculateCRC()
{
    m_crc = 0;
}

GG_CdRomTrackType CdRomChdImage::GetTrackType(const char* type_str)
{
    if (strcmp(type_str, "AUDIO") == 0)
        return GG_CDROM_AUDIO_TRACK;
    else if (strcmp(type_str, "MODE1") == 0)
        return GG_CDROM_DATA_TRACK_MODE1_2048;
    else if (strcmp(type_str, "MODE1_RAW") == 0)
        return GG_CDROM_DATA_TRACK_MODE1_2352;
    else
    {
        Debug("WARNING: Unknown track type '%s', defaulting to AUDIO", type_str);
        return GG_CDROM_AUDIO_TRACK;
    }
}
