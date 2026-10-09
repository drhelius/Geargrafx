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
#include "cdrom_mmi_image.h"
#include "laseractive.h"
#if defined(GG_ENABLE_PHYSICAL_CDROM)
#include "cdrom_physical_image.h"
#endif

CdRomMedia::CdRomMedia()
{
    InitPointer(m_current_image);
    m_media_generation = 0;
    InitPointer(m_cue_bin_image);
    InitPointer(m_chd_image);
    InitPointer(m_mmi_image);
    InitPointer(m_laseractive);
#if defined(GG_ENABLE_PHYSICAL_CDROM)
    InitPointer(m_physical_image);
#endif
}

CdRomMedia::~CdRomMedia()
{
    SafeDelete(m_cue_bin_image);
    SafeDelete(m_chd_image);
    SafeDelete(m_mmi_image);
#if defined(GG_ENABLE_PHYSICAL_CDROM)
    SafeDelete(m_physical_image);
#endif
}

void CdRomMedia::Init(LaserActive* laseractive)
{
    m_laseractive = laseractive;

    m_cue_bin_image = new CdRomCueBinImage();
    m_cue_bin_image->Init();

    m_chd_image = new CdRomChdImage();
    m_chd_image->Init();

    m_mmi_image = new CdRomMmiImage();
    m_mmi_image->Init();

#if defined(GG_ENABLE_PHYSICAL_CDROM)
    m_physical_image = new CdRomPhysicalImage();
    m_physical_image->Init();
#endif

    Reset();
}

void CdRomMedia::Reset()
{
    if (IsValidPointer(m_laseractive))
        m_laseractive->NotifyMediaEjected(true);

    InitPointer(m_current_image);
    m_media_generation++;

    m_cue_bin_image->Reset();
    m_chd_image->Reset();
    m_mmi_image->Reset();
#if defined(GG_ENABLE_PHYSICAL_CDROM)
    m_physical_image->Reset();
#endif
}

bool CdRomMedia::LoadCueFromFile(const char* path, bool preload)
{
    bool cdrom_uri_path = IsCdRomUriPath(path);
    GG_CdRomCueBinLoadOptions options = cdrom_uri_path ?
        GG_CdRomCueBinStreamingLoadOptions() : GG_CdRomCueBinDefaultLoadOptions();

    m_cue_bin_image->SetLoadOptions(options);

    if (m_cue_bin_image->LoadFromFile(path, preload))
    {
        m_current_image = m_cue_bin_image;
        m_media_generation++;
        return true;
    }
    else
    {
        Error("Failed to load CUE file from %s", path);
        Reset();
        return false;
    }
}

bool CdRomMedia::LoadChdFromFile(const char* path, bool preload)
{
    if (m_chd_image->LoadFromFile(path, preload))
    {
        m_current_image = m_chd_image;
        m_media_generation++;
        return true;
    }
    else
    {
        Error("Failed to load CHD file from %s", path);
        Reset();
        return false;
    }
}

bool CdRomMedia::LoadMmiFromFile(const char* path)
{
    if (m_mmi_image->LoadFromFile(path, false))
    {
        m_current_image = m_mmi_image;
        m_media_generation++;
        return true;
    }

    Error("Failed to load MMI file from %s", path);
    Reset();
    return false;
}

bool CdRomMedia::IsMmi() const
{
    return IsValidPointer(m_mmi_image) && (m_current_image == m_mmi_image);
}

bool CdRomMedia::IsLaserDisc() const
{
    return IsMmi() && m_mmi_image->IsLaserDisc();
}

bool CdRomMedia::IsMmiEjected() const
{
    return IsMmi() && m_mmi_image->IsEjected();
}

bool CdRomMedia::EjectMmi()
{
    if (!IsMmi())
        return false;
    if (m_mmi_image->IsEjected())
        return true;

    m_mmi_image->SetEjected(true);
    if (IsValidPointer(m_laseractive))
        m_laseractive->NotifyMediaEjected(true);
    m_media_generation++;
    return true;
}

bool CdRomMedia::InsertMmi()
{
    if (!IsMmi())
        return false;
    if (!m_mmi_image->IsEjected())
        return true;

    m_mmi_image->SetEjected(false);
    m_media_generation++;
    if (IsValidPointer(m_laseractive))
        m_laseractive->NotifyMediaEjected(false);
    return true;
}

bool CdRomMedia::SelectMmiMedia(u32 index)
{
    if (!IsMmi() || !m_mmi_image->IsEjected())
        return false;

    if (index == m_mmi_image->GetSelectedMediaIndex() && m_mmi_image->IsReady())
        return true;

    if (!m_mmi_image->SelectMediaByIndex(index))
        return false;

    m_mmi_image->SetEjected(true);
    m_media_generation++;
    if (IsValidPointer(m_laseractive))
    {
        m_laseractive->NotifyMediaChanged();
        m_laseractive->NotifyMediaEjected(true);
    }
    return true;
}

u32 CdRomMedia::GetSelectedMmiMediaIndex() const
{
    return IsMmi() ? m_mmi_image->GetSelectedMediaIndex() : 0;
}

const GG_MmiInfo* CdRomMedia::GetMmiInfo() const
{
    return IsMmi() ? m_mmi_image->GetMmiInfo() : NULL;
}

const GG_MmiMediaInfo* CdRomMedia::GetSelectedMmiMedia() const
{
    return IsMmi() ? m_mmi_image->GetSelectedMedia() : NULL;
}

const GG_QonInfo* CdRomMedia::GetQonInfo() const
{
    return IsMmi() ? m_mmi_image->GetQonInfo() : NULL;
}

bool CdRomMedia::ReadMmiAnalogAudio(u64 offset, void* buffer, u32 size)
{
    return IsMmi() && m_mmi_image->ReadAnalogAudio(offset, buffer, size);
}

u64 CdRomMedia::GetMmiAnalogAudioSize() const
{
    return IsMmi() ? m_mmi_image->GetAnalogAudioSize() : 0;
}

MediaFile* CdRomMedia::OpenMmiVideoStream() const
{
    return IsMmi() ? m_mmi_image->OpenVideoStream() : NULL;
}

#if defined(GG_ENABLE_PHYSICAL_CDROM)
bool CdRomMedia::LoadPhysicalDrive(const char* device_id, bool preload)
{
    if (m_physical_image->LoadFromDevice(device_id, preload))
    {
        m_current_image = m_physical_image;
        m_media_generation++;
        return true;
    }
    else
    {
        Error("Failed to load physical CD-ROM from %s", device_id);
        Reset();
        return false;
    }
}

bool CdRomMedia::HasPhysicalDriveError()
{
    return IsValidPointer(m_physical_image) && m_physical_image->HasDiscError();
}
#endif

bool CdRomMedia::ReadSector(u32 lba, u8* buffer)
{
    if (IsValidPointer(m_current_image))
    {
        return m_current_image->ReadSector(lba, buffer);
    }
    else
    {
        Error("ReadSector failed - Current image is NULL");
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
        Error("ReadBytes failed - Current image is NULL");
        return false;
    }
}

bool CdRomMedia::ReadSubchannelQ(s32 lba, u8* buffer)
{
    return IsValidPointer(m_current_image) && m_current_image->ReadSubchannelQ(lba, buffer);
}

bool CdRomMedia::PreloadTrack(u32 track_number, u32 lba)
{
    if (IsValidPointer(m_current_image))
    {
        return m_current_image->PreloadTrack(track_number, lba);
    }
    else
    {
        Error("PreloadTrack failed - Current image is NULL");
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
        Error("GetFirstSectorOfTrack failed - Current image is NULL");
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
        Error("GetTrackFromLBA failed - Current image is NULL");
        return -1;
    }
}

s32 CdRomMedia::FindTrackFromLBA(u32 lba, bool include_lead_in)
{
    if (IsValidPointer(m_current_image))
        return m_current_image->FindTrackFromLBA(lba, include_lead_in);

    return -1;
}

bool CdRomMedia::IsCdRomUriPath(const char* path)
{
    return IsValidPointer(path) && (strncmp(path, "cdrom://", 8) == 0);
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
    u32 lba_difference = (u32)abs((int)end_lba - (int)start_lba);
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
