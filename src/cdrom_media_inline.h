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
    if (IsValidPointer(m_current_image))
    {
        return m_current_image->IsReady();
    }
    else
    {
        Log("ERROR: CdRomMedia::IsReady failed - Current image is NULL");
        return false;
    }
}

INLINE u32 CdRomMedia::GetCRC()
{
    if (IsValidPointer(m_current_image))
    {
        return m_current_image->GetCRC();
    }
    else
    {
        Log("ERROR: CdRomMedia::GetCRC failed - Current image is NULL");
        return 0;
    }
}

INLINE const char* CdRomMedia::GetFilePath()
{
    if (IsValidPointer(m_current_image))
    {
        return m_current_image->GetFilePath();
    }
    else
    {
        Log("ERROR: CdRomMedia::GetFilePath failed - Current image is NULL");
        return "";
    }
}

INLINE const char* CdRomMedia::GetFileDirectory()
{
    if (IsValidPointer(m_current_image))
    {
        return m_current_image->GetFileDirectory();
    }
    else
    {
        Log("ERROR: CdRomMedia::GetFileDirectory failed - Current image is NULL");
        return "";
    }
}

INLINE const char* CdRomMedia::GetFileName()
{
    if (IsValidPointer(m_current_image))
    {
        return m_current_image->GetFileName();
    }
    else
    {
        Log("ERROR: CdRomMedia::GetFileName failed - Current image is NULL");
        return "";
    }
}

INLINE const char* CdRomMedia::GetFileExtension()
{
    if (IsValidPointer(m_current_image))
    {
        return m_current_image->GetFileExtension();
    }
    else
    {
        Log("ERROR: CdRomMedia::GetFileExtension failed - Current image is NULL");
        return "";
    }
}

INLINE const std::vector<CdRomImage::Track>& CdRomMedia::GetTracks()
{
    if (IsValidPointer(m_current_image))
    {
        return m_current_image->GetTOC()->tracks;
    }
    else
    {
        Log("ERROR: CdRomMedia::GetTracks failed - Current image is NULL");
        static std::vector<CdRomImage::Track> empty_tracks;
        return empty_tracks;
    }
}

INLINE u8 CdRomMedia::GetTrackCount()
{
    if (IsValidPointer(m_current_image))
    {
        return static_cast<u8>(m_current_image->GetTOC()->tracks.size());
    }
    else
    {
        Log("ERROR: CdRomMedia::GetTrackCount failed - Current image is NULL");
        return 0;
    }
}

INLINE GG_CdRomTrackType CdRomMedia::GetTrackType(u8 track_number)
{
    if (IsValidPointer(m_current_image))
    {
        if (track_number < m_current_image->GetTOC()->tracks.size())
            return m_current_image->GetTOC()->tracks[track_number].type;
        else
        {
            Log("ERROR: CdRomMedia::GetTrackType failed - Track number %d out of bounds", track_number);
            return GG_CDROM_AUDIO_TRACK;
        }
    }
    else
    {
        Log("ERROR: CdRomMedia::GetTrackType failed - Current image is NULL");
        return GG_CDROM_AUDIO_TRACK;
    }
}

INLINE GG_CdRomMSF CdRomMedia::GetCdRomLength()
{
    if (IsValidPointer(m_current_image))
    {
        return m_current_image->GetTOC()->total_length;
    }
    else
    {
        Log("ERROR: CdRomMedia::GetCdRomLength failed - Current image is NULL");
        return { 0, 0, 0 };
    }
}

INLINE u32 CdRomMedia::GetSectorCount()
{
    if (IsValidPointer(m_current_image))
    {
        return m_current_image->GetTOC()->sector_count;
    }
    else
    {
        Log("ERROR: CdRomMedia::GetSectorCount failed - Current image is NULL");
        return 0;
    }
}

INLINE u32 CdRomMedia::GetCurrentSector()
{
    if (IsValidPointer(m_current_image))
    {
        return m_current_image->GetCurrentSector();
    }
    else
    {
        Log("ERROR: CdRomMedia::GetCurrentSector failed - Current image is NULL");
        return 0;
    }
}

INLINE void CdRomMedia::SetCurrentSector(u32 sector)
{
    if (IsValidPointer(m_current_image))
    {
        m_current_image->SetCurrentSector(sector);
    }
    else
    {
        Log("ERROR: CdRomMedia::SetCurrentSector failed - Current image is NULL");
    }
}

INLINE u32 CdRomMedia::SectorTransferCycles()
{
    // Standard CD-ROM 1x speed: 75 sectors/sec
    return GG_MASTER_CLOCK_RATE / 75;
}

#endif /* CDROM_MEDIA_H */