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

#ifndef CDROM_MEDIA_INLINE_H
#define CDROM_MEDIA_INLINE_H

#include "cdrom_media.h"

INLINE bool CdRomMedia::IsReady()
{
    return m_ready;
}

INLINE const char* CdRomMedia::GetFilePath()
{
    return m_file_path;
}

INLINE const char* CdRomMedia::GetFileDirectory()
{
    return m_file_directory;
}

INLINE const char* CdRomMedia::GetFileName()
{
    return m_file_name;
}

INLINE const char* CdRomMedia::GetFileExtension()
{
    return m_file_extension;
}

INLINE const std::vector<CdRomMedia::Track>& CdRomMedia::GetTracks()
{
    return m_tracks;
}

INLINE const std::vector<CdRomMedia::ImgFile*>& CdRomMedia::GetImgFiles()
{
    return m_img_files;
}

INLINE u32 CdRomMedia::GetTrackSectorSize(CdRomMedia::TrackType type)
{
    return k_cdrom_track_type_size[type];
}

INLINE u32 CdRomMedia::GetTrackSectorSize(u8 track_number)
{
    if (track_number >= m_tracks.size())
        return 0;
    return m_tracks[track_number].sector_size;
}

INLINE CdRomMedia::TrackType CdRomMedia::GetTrackType(u8 track_number)
{
    if (track_number >= m_tracks.size())
        return AUDIO_TRACK;
    return m_tracks[track_number].type;
}

INLINE const char* CdRomMedia::GetTrackTypeName(CdRomMedia::TrackType type)
{
    return k_cdrom_track_type_name[type];
}

INLINE const char* CdRomMedia::GetTrackTypeName(u8 track_number)
{
    if (track_number >= m_tracks.size())
        return "INVALID";
    return k_cdrom_track_type_name[m_tracks[track_number].type];
}

INLINE u8 CdRomMedia::GetTrackCount()
{
    return (u8)m_tracks.size();
}

INLINE GG_CdRomMSF CdRomMedia::GetCdRomLength()
{
    return m_cdrom_length;
}

INLINE u32 CdRomMedia::GetSectorCount()
{
    return m_sector_count;
}

INLINE u32 CdRomMedia::GetCurrentSector()
{
    return m_current_sector;
}

INLINE void CdRomMedia::SetCurrentSector(u32 sector)
{
    if (sector < m_sector_count)
        m_current_sector = sector;
    else
        m_current_sector = m_sector_count - 1;
}

#endif /* CDROM_MEDIA_H */