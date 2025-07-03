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

#include <string>
#include <algorithm>
#include "cdrom_image.h"

CdRomImage::CdRomImage()
{
    Reset();
}

CdRomImage::~CdRomImage()
{
}

void CdRomImage::Init()
{
}

void CdRomImage::Reset()
{
    m_toc.tracks.clear();
    m_toc.total_length = {0, 0, 0};
    m_toc.sector_count = 0;
    m_ready = false;
    m_file_path[0] = 0;
    m_file_directory[0] = 0;
    m_file_name[0] = 0;
    m_file_extension[0] = 0;
    m_current_sector = 0;
    m_crc = 0;
}

bool CdRomImage::IsReady()
{
    return m_ready;
}

u32 CdRomImage::GetFirstSectorOfTrack(u8 track)
{
    if (track < m_toc.tracks.size())
    {
        return m_toc.tracks[track].start_lba;
    }
    else if ((track > 0) && (track == m_toc.tracks.size()))
    {
        return m_toc.tracks[track - 1].end_lba;
    }

    Debug("ERROR: GetFirstSectorOfTrack failed - Track number %d out of bounds (max: %d)", track, m_toc.tracks.size());
    return 0;
}

u32 CdRomImage::GetLastSectorOfTrack(u8 track)
{
    if (track < m_toc.tracks.size())
    {
        return m_toc.tracks[track].end_lba;
    }

    Log("ERROR: GetLastSectorOfTrack failed - Track number %d out of bounds (max: %d)", track, m_toc.tracks.size());
    return 0;
}

s32 CdRomImage::GetTrackFromLBA(u32 lba)
{
    if (lba >= m_toc.sector_count)
    {
        Debug("ERROR: GetTrackNumber failed - LBA %d out of bounds (max: %d)", lba, m_toc.sector_count - 1);
        return -1;
    }

    size_t track_count = m_toc.tracks.size();

    for (size_t i = 0; i < track_count; i++)
    {
        if ((lba >= m_toc.tracks[i].start_lba) && (lba <= m_toc.tracks[i].end_lba))
        {
            return i;
        }
    }

    Debug("ERROR: GetTrackNumber failed - LBA %d not found in any track", lba);
    return -1;
}

const char* CdRomImage::GetFilePath()
{
    return m_file_path;
}

const char* CdRomImage::GetFileDirectory()
{
    return m_file_directory;
}

const char* CdRomImage::GetFileName()
{
    return m_file_name;
}

const char* CdRomImage::GetFileExtension()
{
    return m_file_extension;
}

CdRomImage::TableOfContents* CdRomImage::GetTOC()
{
    return &m_toc;
}

u32 CdRomImage::GetCRC()
{
    return m_crc;
}

u32 CdRomImage::GetCurrentSector()
{
    return m_current_sector;
}

void CdRomImage::SetCurrentSector(u32 sector)
{
    if (sector < m_toc.sector_count)
        m_current_sector = sector;
    else
        m_current_sector = m_toc.sector_count - 1;
}

void CdRomImage::GatherPaths(const char* path)
{
    using namespace std;

    string fullpath(path);
    string directory;
    string filename;
    string extension;

    size_t pos = fullpath.find_last_of("/\\");
    if (pos != string::npos)
    {
        filename = fullpath.substr(pos + 1);
        directory = fullpath.substr(0, pos);
    }
    else
    {
        filename = fullpath;
        directory = "";
    }

    extension = fullpath.substr(fullpath.find_last_of(".") + 1);
    transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return std::tolower(c); });

    snprintf(m_file_path, sizeof(m_file_path), "%s", path);
    snprintf(m_file_directory, sizeof(m_file_directory), "%s", directory.c_str());
    snprintf(m_file_name, sizeof(m_file_name), "%s", filename.c_str());
    snprintf(m_file_extension, sizeof(m_file_extension), "%s", extension.c_str());
}

void CdRomImage::InitTrack(Track& track)
{
    track.type = GG_CDROM_AUDIO_TRACK;
    track.sector_size = 0;
    track.sector_count = 0;
    track.start_lba = 0;
    track.start_msf = {0, 0, 0};
    track.end_lba = 0;
    track.end_msf = {0, 0, 0};
    track.has_lead_in = false;
    track.lead_in_lba = 0;
}
