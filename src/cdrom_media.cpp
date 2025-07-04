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

#include "cdrom_media.h"
#include "cdrom_cuebin_image.h"
#include "cdrom_chd_image.h"

CdRomMedia::CdRomMedia()
{
    InitPointer(m_current_image);
}

CdRomMedia::~CdRomMedia()
{
    SafeDelete(m_cue_bin_image);
    SafeDelete(m_chd_image);
}

void CdRomMedia::Init()
{
    m_cue_bin_image = new CdRomCueBinImage();
    m_cue_bin_image->Init();

    m_chd_image = new CdRomChdImage();
    m_chd_image->Init();

    Reset();
}

void CdRomMedia::Reset()
{
    InitPointer(m_current_image);

    m_cue_bin_image->Reset();
    m_chd_image->Reset();
}

bool CdRomMedia::LoadCueFromFile(const char* path, bool preload)
{
    if (m_cue_bin_image->LoadFromFile(path, preload))
    {
        m_current_image = m_cue_bin_image;
        return true;
    }
    else
    {
        Log("ERROR: Failed to load CUE file from %s", path);
        Reset();
        return false;
    }
}

bool CdRomMedia::LoadChdFromFile(const char* path, bool preload)
{
    if (m_chd_image->LoadFromFile(path, preload))
    {
        m_current_image = m_chd_image;
        return true;
    }
    else
    {
        Log("ERROR: Failed to load CHD file from %s", path);
        Reset();
        return false;
    }
}

bool CdRomMedia::ReadSector(u32 lba, u8* buffer)
{
    if (IsValidPointer(m_current_image))
    {
        return m_current_image->ReadSector(lba, buffer);
    }
    else
    {
        Log("ERROR: ReadSector failed - Current image is NULL");
        return false;
    }
}

bool CdRomMedia::ReadSamples(u32 lba, u32 offset, s16* buffer, u32 count)
{
    if (IsValidPointer(m_current_image))
    {
        return m_current_image->ReadSamples(lba, offset, buffer, count);
    }
    else
    {
        Log("ERROR: ReadBytes failed - Current image is NULL");
        return false;
    }
}

bool CdRomMedia::PreloadTrack(u32 track_number)
{
    if (IsValidPointer(m_current_image))
    {
        return m_current_image->PreloadTrack(track_number);
    }
    else
    {
        Log("ERROR: PreloadTrack failed - Current image is NULL");
        return false;
    }
}

u32 CdRomMedia::GetFirstSectorOfTrack(u8 track)
{
    if (IsValidPointer(m_current_image))
    {
        return m_current_image->GetFirstSectorOfTrack(track);
    }
    else
    {
        Log("ERROR: GetFirstSectorOfTrack failed - Current image is NULL");
        return 0;
    }
}

u32 CdRomMedia::GetLastSectorOfTrack(u8 track)
{
    if (IsValidPointer(m_current_image))
    {
        return m_current_image->GetLastSectorOfTrack(track);
    }
    else
    {
        Log("ERROR: GetLastSectorOfTrack failed - Current image is NULL");
        return 0;
    }
}

s32 CdRomMedia::GetTrackFromLBA(u32 lba)
{
    if (IsValidPointer(m_current_image))
    {
        return m_current_image->GetTrackFromLBA(lba);
    }
    else
    {
        Log("ERROR: GetTrackFromLBA failed - Current image is NULL");
        return -1;
    }
}

///////////////////////////////////////////////////////////////
// Seek time, based on the work by Dave Shadoff
// https://github.com/pce-devel/PCECD_seek

u32 CdRomMedia::SeekFindGroup(u32 lba)
{
    for (u32 i = 0; i < GG_SEEK_NUM_SECTOR_GROUPS; i++)
        if ((lba >= k_seek_sector_list[i].sec_start) && (lba <= k_seek_sector_list[i].sec_end))
            return i;
    return 0;
}

// In milliseconds
u32 CdRomMedia::SeekTime(u32 start_lba, u32 end_lba)
{
    u32 start_index = SeekFindGroup(start_lba);
    u32 target_index = SeekFindGroup(end_lba);
    u32 lba_difference = (u32)std::abs((int)end_lba - (int)start_lba);
    double track_difference = 0.0;

    // Now we find the track difference
    //
    // Note: except for the first and last sector groups, all groups are 1606.48 tracks per group.
    //
    if (target_index == start_index)
    {
        track_difference = (lba_difference / k_seek_sector_list[target_index].sec_per_revolution);
    }
    else if (target_index > start_index)
    {
        track_difference = (k_seek_sector_list[start_index].sec_end - start_lba) / k_seek_sector_list[start_index].sec_per_revolution;
        track_difference += (end_lba - k_seek_sector_list[target_index].sec_start) / k_seek_sector_list[target_index].sec_per_revolution;
        track_difference += (1606.48 * (target_index - start_index - 1));
    }
    else // start_index > target_index
    {
        track_difference = (start_lba - k_seek_sector_list[start_index].sec_start) / k_seek_sector_list[start_index].sec_per_revolution;
        track_difference += (k_seek_sector_list[target_index].sec_end - end_lba) / k_seek_sector_list[target_index].sec_per_revolution;
        track_difference += (1606.48 * (start_index - target_index - 1));
    }

    // Now, we use the algorithm to determine how long to wait
    if (lba_difference < 2)
        return (u32)((9 * 1000 / 60));
    if (lba_difference < 5)
        return (u32)((9 * 1000 / 60) + (k_seek_sector_list[target_index].rotation_ms / 2));
    else if (track_difference <= 80)
        return (u32)((18 * 1000 / 60) + (k_seek_sector_list[target_index].rotation_ms / 2));
    else if (track_difference <= 160)
        return (u32)((22 * 1000 / 60) + (k_seek_sector_list[target_index].rotation_ms / 2));
    else if (track_difference <= 644)
        return (u32)((22 * 1000 / 60) + (k_seek_sector_list[target_index].rotation_ms / 2) + ((track_difference - 161) * 16.66 / 80));
    else
        return (u32)((48 * 1000 / 60) + ((track_difference - 644) * 16.66 / 195));
}
