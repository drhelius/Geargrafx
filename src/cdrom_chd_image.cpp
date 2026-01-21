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
#include "crc.h"

CdRomChdImage::CdRomChdImage() : CdRomImage()
{
    InitPointer(m_chd_file);
    InitPointer(m_hunk_cache);
}

CdRomChdImage::~CdRomChdImage()
{
    chd_close(m_chd_file);
    DestroyHunkCache();
}

void CdRomChdImage::Init()
{
    CdRomImage::Init();
    Reset();
}

void CdRomChdImage::Reset()
{
    CdRomImage::Reset();

    m_hunk_bytes = 0;
    m_hunk_count = 0;
    m_sectors_per_hunk = 0;

    chd_close(m_chd_file);
    InitPointer(m_chd_file);

    DestroyHunkCache();
}

bool CdRomChdImage::LoadFromFile(const char* path, bool preload)
{
    using namespace std;

    Log("Loading CHD from %s...", path);

    if (!IsValidPointer(path))
    {
        Error("Invalid path %s", path);
        m_ready = false;
        return m_ready;
    }

    Reset();
    GatherPaths(path);

    if (strcmp(m_file_extension, "chd") != 0)
    {
        Error("Invalid file extension %s. Expected .chd", m_file_extension);
        m_ready = false;
        return m_ready;
    }

    chd_error err = chd_open(path, CHD_OPEN_READ, NULL, &m_chd_file);

    if (err == CHDERR_NONE)
    {
        const chd_header* header = chd_get_header(m_chd_file);

        if (IsValidPointer(header))
        {
            Debug("CHD Header: Version: %d, Hunk Size: %d, Total Hunks: %d, Flags: %04X",
                    header->version, header->hunkbytes, header->totalhunks, header->flags);

            m_hunk_bytes = header->hunkbytes;
            m_hunk_count = header->totalhunks;
            m_sectors_per_hunk = m_hunk_bytes / (2352 + 96);

            if (m_hunk_bytes == 0 || m_hunk_count == 0)
            {
                Error("Invalid CHD header - hunk size or count is zero");
                chd_close(m_chd_file);
                m_ready = false;
            }
            else if (m_hunk_bytes % (2352 + 96) != 0)
            {
                Error("Invalid CHD hunk size %d, must be a multiple of 2448 (2352 + 96)", m_hunk_bytes);
                chd_close(m_chd_file);
                m_ready = false;
            }
            else
            {
                InitHunkCache();
                m_ready = ReadTOC();

                if (preload && m_ready)
                    m_ready = PreloadDisc();

                CalculateCRC();
            }
        }
        else
        {
            Error("Failed to get CHD header for %s", path);
            chd_close(m_chd_file);
            m_ready = false;
        }
    }
    else
    {
        Error("Unable to open CHD file %s.", path);
        Error("CHD: %d, %s", err, chd_error_string(err));
        chd_close(m_chd_file);
        m_ready = false;
    }

    if (!m_ready)
        Reset();

    return m_ready;
}

bool CdRomChdImage::ReadSector(u32 lba, u8* buffer)
{
    if (!m_ready || buffer == NULL)
    {
        Error("ReadSector failed - Media not ready or buffer is NULL");
        return false;
    }

    size_t track_count = m_toc.tracks.size();

    for (size_t i = 0; i < track_count; i++)
    {
        const Track& track = m_toc.tracks[i];

        u32 start = track.start_lba;
        u32 end = start + track.sector_count;

        if (lba >= start && lba < end)
        {
            u32 sector_index = (lba - track.start_lba) + track.file_offset;
            u32 hunk_index  = sector_index / m_sectors_per_hunk;
            u32 hunk_offset = sector_index % m_sectors_per_hunk;
            u32 byte_offset_in_hunk = hunk_offset * (2352 + 96);

            if (!LoadHunk(hunk_index))
                return false;

            u32 sector_offset = 0;

            if (track.sector_size == 2352)
                sector_offset = 16;

            u32 final_offset = byte_offset_in_hunk + sector_offset;
            assert(final_offset + 2048 <= m_hunk_bytes);

            Debug("Reading LBA %d, sector_index %u, hunk_index %u, hunk_offset %u, byte_offset_in_hunk %d, sector_offset %d",
                lba, sector_index, hunk_index, hunk_offset, byte_offset_in_hunk, sector_offset);

            memcpy(buffer, m_hunk_cache[hunk_index] + final_offset, 2048);

            return true;
        }
    }

    Error("ReadSector failed - LBA %d not found in any track", lba);

    return false;
}

bool CdRomChdImage::ReadSamples(u32 lba, u32 offset, s16* buffer, u32 count)
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

    size_t track_count = m_toc.tracks.size();

    for (size_t i = 0; i < track_count; i++)
    {
        const Track& track = m_toc.tracks[i];

        u32 start = track.start_lba;
        u32 end = start + track.sector_count;

        if (lba >= start && lba < end)
        {
            u32 sector_index = (lba - track.start_lba) + track.file_offset;
            u32 hunk_index  = sector_index / m_sectors_per_hunk;
            u32 hunk_offset = sector_index % m_sectors_per_hunk;
            u32 byte_offset_in_hunk = hunk_offset * (2352 + 96);

            if (!LoadHunk(hunk_index))
                return false;

            u32 size = count * 2;
            u32 final_offset = byte_offset_in_hunk + offset;
            assert(final_offset + size <= m_hunk_bytes);

            memcpy(buffer, m_hunk_cache[hunk_index] + final_offset, size);

#ifdef GG_LITTLE_ENDIAN
            for (u32 i = 0; i < count; i++)
            {
                u16 u = (u16)buffer[i];
                buffer[i] = (s16)((u >> 8) | (u << 8));
            }
#endif

            return true;
        }
    }

    Error("ReadBytes failed - LBA %d not found in any track", lba);

    return false;
}

bool CdRomChdImage::PreloadDisc()
{
    if (!m_ready)
    {
        Error("PreloadDisc failed - Media not ready");
        return false;
    }

    if (m_hunk_cache == NULL)
    {
        Error("PreloadDisc failed - Hunk cache not initialized");
        return false;
    }

    Log("Preloading CHD disc...");
    chd_error err = chd_precache(m_chd_file);

    if (err != CHDERR_NONE)
    {
        Error("PreloadDisc failed - chd_precache returned error %d, %s", err, chd_error_string(err));
        return false;
    }

    return true;
}

bool CdRomChdImage::PreloadTrack(u32 track_number)
{
    if (track_number >= m_toc.tracks.size())
    {
        Error("PreloadTrack failed - Invalid track number %d", track_number);
        return false;
    }

    const Track& track = m_toc.tracks[track_number];

    u32 first_sector = track.file_offset;
    u32 last_sector  = track.file_offset + track.sector_count - 1;
    u32 first_hunk = first_sector / m_sectors_per_hunk;
    u32 last_hunk  = last_sector  / m_sectors_per_hunk;

    Debug("Preloading track %u: hunks %u to %u", track_number + 1, first_hunk, last_hunk);

    for (u32 hunk = first_hunk; hunk <= last_hunk; hunk++)
    {
        if (!LoadHunk(hunk))
        {
            Error("PreloadTrack failed - Unable to load hunk %u", hunk);
            return false;
        }
    }

    return true;
}

bool CdRomChdImage::ReadTOC()
{
    chd_error err;
    char metadata[512];

    u32 current_lba = 0;
    u32 file_offset = 0;

    for (int i = 0; i < 99; i++)
    {
        bool track_exists = false;
        int track = 0, frames = 0, pregap = 0, postgap = 0;
        char type[64], subtype[32], pgtype[32], pgsub[32];
        type[0] = subtype[0] = pgtype[0] = pgsub[0] = 0;

        err = chd_get_metadata(m_chd_file, CDROM_TRACK_METADATA2_TAG, i, metadata, sizeof(metadata), NULL, NULL, NULL);

        if (err == CHDERR_NONE)
        {
            if (sscanf(metadata, CDROM_TRACK_METADATA2_FORMAT, &track, type, subtype, &frames, &pregap, pgtype, pgsub, &postgap) != 8)
            {
                Error("Failed to parse CDROM_TRACK_METADATA2_FORMAT for track %d", i + 1);
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
                    Error("Failed to parse CDROM_TRACK_METADATA_FORMAT for track %d", i + 1);
                    return false;
                }
                track_exists = true;
            }
        }

        if (!track_exists)
            break;

        Debug("Track %d: Type: %s, Subtype: %s, Frames: %d, Pregap: %d, Postgap: %d, PGType: %s, PGSub: %s", 
                track, type, subtype, frames, pregap, postgap, pgtype, pgsub);

        // Calculate real pregap and pregap_dv
        u32 pregap_real = (pgtype[0] == 'V' ? 0 : pregap);
        u32 pregap_dv = (pgtype[0] == 'V' ? pregap : 0);

        // Advance current LBA by pregaps
        current_lba += pregap_real + pregap_dv;

        Track new_track;
        InitTrack(new_track);

        new_track.type = GetTrackType(type);
        new_track.sector_size = TrackTypeSectorSize(new_track.type);

        new_track.start_lba = current_lba;
        LbaToMsf(new_track.start_lba, &new_track.start_msf);

        u32 data_frames = frames - pregap_dv;
        new_track.sector_count = data_frames;

        new_track.end_lba = new_track.start_lba + data_frames - 1;
        LbaToMsf(new_track.end_lba, &new_track.end_msf);

        // Advance current LBA past data and postgap
        current_lba += data_frames;
        current_lba += postgap;

        // Update file_offset: pregap_dv + data + postgap + alignment to 4 sectors
        file_offset += pregap_dv;
        new_track.file_offset = file_offset;
        file_offset += data_frames;
        file_offset += postgap;
        file_offset += ((frames + 3) & ~3) - frames;

        m_toc.tracks.push_back(new_track);
    }

    for (size_t i = 0; i < m_toc.tracks.size(); ++i)
    {
        Track& track = m_toc.tracks[i];

        Log("Track %2d (%s): Start LBA: %6u, End LBA: %6u, Sectors: %6u, File Offset: %8llu",
                i + 1,
                TrackTypeName(track.type),
                track.start_lba,
                track.end_lba,
                track.sector_count,
                track.file_offset);
    }

    Log("Successfully parsed CHD file with %d tracks", (int)m_toc.tracks.size());

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

void CdRomChdImage::CalculateCRC()
{
    m_crc = 0;

    if (m_toc.tracks.empty())
    {
        Log("No tracks to calculate CRC from");
        return;
    }

    Track* first_data_track = NULL;
    size_t track_count = m_toc.tracks.size();

    for (size_t i = 0; i < track_count; i++)
    {
        if (m_toc.tracks[i].type == GG_CDROM_DATA_TRACK_MODE1_2048 ||
            m_toc.tracks[i].type == GG_CDROM_DATA_TRACK_MODE1_2352)
        {
            first_data_track = &m_toc.tracks[i];
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

    u32 sectors_to_crc = 64;
    u32 first_sector = 1;
    u32 last_needed = first_sector + sectors_to_crc - 1;
    u32 max_index = ((first_data_track->sector_count - 1) >= last_needed)
                     ? last_needed
                     : (first_data_track->sector_count - 1);

    for (u32 sec = first_sector; sec <= max_index; sec++)
    {
        if (!ReadSector(sec + first_data_track->start_lba, buffer))
        {
            Error("CRC ReadSector failed for LBA %u", sec + first_data_track->start_lba);
            SafeDeleteArray(buffer);
            return;
        }

        m_crc = CalculateCRC32(m_crc, buffer, sector_data_size);
    }

    SafeDeleteArray(buffer);
}

void CdRomChdImage::InitHunkCache()
{
    if (IsValidPointer(m_hunk_cache))
    {
        DestroyHunkCache();
    }

    m_hunk_cache = new u8*[m_hunk_count];

    for (u32 i = 0; i < m_hunk_count; i++)
    {
        InitPointer(m_hunk_cache[i]);
    }
}

void CdRomChdImage::DestroyHunkCache()
{
    if (IsValidPointer(m_hunk_cache))
    {
        for (u32 i = 0; i < m_hunk_count; i++)
        {
            SafeDeleteArray(m_hunk_cache[i]);
        }
        SafeDeleteArray(m_hunk_cache);
    }
}

bool CdRomChdImage::LoadHunk(u32 hunk_index)
{
    if (hunk_index >= m_hunk_count)
    {
        Error("LoadHunk failed - hunk index %u out of bounds (max: %u)", hunk_index, m_hunk_count - 1);
        return false;
    }

    if (m_hunk_cache[hunk_index] == NULL)
    {
        m_hunk_cache[hunk_index] = new u8[m_hunk_bytes];

        Debug("Caching hunk %u", hunk_index);

        chd_error err = chd_read(m_chd_file, hunk_index, m_hunk_cache[hunk_index]);

        if (err != CHDERR_NONE)
        {
            Error("CHD read hunk %u failed: %d, %s", hunk_index, err, chd_error_string(err));
            return false;
        }
    }

    return true;
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
