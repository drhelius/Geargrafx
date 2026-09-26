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

#include <algorithm>
#include <cmath>
#include <limits>
#include "laseractive.h"
#include "cdrom.h"
#include "cdrom_audio.h"
#include "cdrom_media.h"
#include "cdrom_mmi_image.h"
#include "memory.h"
#include "media_file.h"
#include "../platforms/shared/dependencies/qon/qoi2.h"

/*
 * PD6103A, transport and mixing adapted from ares/ares/pce/pcd (ISC).
 * Reference: ares 7b51c8ab719e403a150aa700e0933d9e93a06851.
 * Copyright (c) 2004-2025 ares team, Near et al
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

static const double k_laseractive_video_fps = 30000.0 / 1001.0;
static const u32 k_laseractive_state_magic = 0x4C414D4D;
static const u32 k_laseractive_search_sectors = 56;
static const u32 k_laseractive_fade_samples = GG_AUDIO_SAMPLE_RATE / 2;

LaserActive::LaserActive(CdRomMedia* cdrom_media)
{
    m_cdrom_media = cdrom_media;
    InitPointer(m_cdrom);
    InitPointer(m_cdrom_audio);
    InitPointer(m_memory);
    InitPointer(m_video_file);
    InitPointer(m_video_prefetch_file);

#if !defined(GG_DISABLE_MMI_THREADS)
    m_video_thread_stop = false;
    m_video_job_pending = false;
    m_video_result_ready = false;
#endif

    Reset();
}

LaserActive::~LaserActive()
{
    StopVideoDecoder();
}

void LaserActive::Init(CdRom* cdrom, Memory* memory, CdRomAudio* cdrom_audio)
{
    m_cdrom = cdrom;
    m_cdrom_audio = cdrom_audio;
    m_memory = memory;
    Reset();
}

void LaserActive::Reset()
{
    StopVideoDecoder();
    memset(m_input_regs, 0, sizeof(m_input_regs));
    memset(m_input_frozen_regs, 0, sizeof(m_input_frozen_regs));
    memset(m_output_regs, 0, sizeof(m_output_regs));
    memset(m_output_frozen_regs, 0, sizeof(m_output_frozen_regs));
    memset(m_output_written_data, 0, sizeof(m_output_written_data));
    memset(m_output_cooldown, 0, sizeof(m_output_cooldown));
    m_input_frozen = false;
    m_output_frozen = false;
    m_operation_error_1 = false;
    m_operation_error_2 = false;
    m_operation_error_3 = false;
    m_seek_enabled = false;
    m_current_seek_mode = 0;
    m_current_seek_time_format = false;
    m_current_seek_repeat = false;
    m_analog_attenuation_left = 0;
    m_analog_attenuation_right = 0;
    m_analog_fade_muted_left = false;
    m_analog_fade_muted_right = false;
    m_analog_fade_samples_left = 0;
    m_analog_fade_samples_right = 0;
    m_active_seek_mode = SEEK_REDBOOK_TIME;
    memset(m_seek_point_regs, 0, sizeof(m_seek_point_regs));
    memset(m_stop_point_regs, 0, sizeof(m_stop_point_regs));
    m_reached_stop_point = false;
    m_reached_stop_point_previously = false;
    m_playback_mode = 0;
    m_playback_speed = 0;
    m_playback_reverse = false;
    m_target_drive_state = 2;
    m_current_drive_state = 2;
    m_target_pause = false;
    m_current_pause = false;
    m_seek_frame_pending = false;
    m_drive_state_delay = 0;
    m_selected_track_info = 0;

    m_drive_mode = DRIVE_INACTIVE;
    m_seek_drive_mode = DRIVE_INACTIVE;
    m_head_lba = 0;
    m_seek_target_lba = 0;
    m_audio_end_lba = 0;
    m_seek_latency = 0;
    m_sector_repeat_count = 0;
    m_search_sectors = 0;
    m_stop_point_enabled = false;
    m_stop_point_aba = 0;
    m_current_track = 1;
    memset(m_digital_sector, 0, sizeof(m_digital_sector));
    m_digital_sector_valid = false;
    m_current_sample = 0;
    m_audio_end_pending = false;
    m_sector_clock = 0;

    m_sram_shift = 0;
    m_sram_enabled = false;

    m_current_video_frame = -1;
    m_current_video_lead_in = false;
    m_current_video_lead_out = false;
    m_frame_skip_base = 0;
    m_frame_skip_counter = 0;

    m_analog_cache_offset = 0;
    m_analog_cache_valid = false;
    m_analog_lead_in_samples = 0;
    m_analog_audio_size = 0;

    InitPointer(m_video_file);
    InitPointer(m_video_prefetch_file);
    m_video_frame.clear();
    m_video_prefetch_frame.clear();
    m_video_compressed_data.clear();
    m_video_prefetch_compressed_data.clear();
    m_video_frame_index = 0;
    m_video_generation = 0;
    m_video_frame_valid = false;
    m_video_even_field = false;
    m_video_new_frame = false;
    m_video_memory_latched = false;
    m_video_field_selected = false;
    m_video_selected_even = false;
    m_video_display_frame_index = 0;
    m_video_display_valid = false;
    m_video_display_even = false;

#if !defined(GG_DISABLE_MMI_THREADS)
    m_video_thread_stop = false;
    m_video_job_pending = false;
    m_video_result_ready = false;
    m_video_job_frame = 0;
    m_video_job_generation = 0;
    m_video_result_frame = 0;
    m_video_result_generation = 0;
#endif

    m_input_regs[0x1A] = 0xFF;
    m_input_regs[0x1B] = 0xFF;
    m_input_regs[0x1C] = 0xFF;
    m_input_regs[0x1D] = 0xFF;

    if (IsValidPointer(m_memory))
        m_memory->UpdateLaserActiveSram();

    if (IsLaserDisc())
        StartVideoDecoder();
}

bool LaserActive::GetBit(u8 value, u8 bit)
{
    return (value & (1U << bit)) != 0;
}

u8 LaserActive::GetBits(u8 value, u8 low, u8 high)
{
    u8 width = high - low + 1;
    u8 mask = (u8)((1U << width) - 1U);

    return (value >> low) & mask;
}

void LaserActive::SetBit(u8& value, u8 bit, bool state)
{
    u8 mask = (u8)(1U << bit);

    value = state ? (value | mask) : (value & (u8)~mask);
}

void LaserActive::SetBits(u8& value, u8 low, u8 high, u8 field)
{
    u8 width = high - low + 1;
    u8 mask = (u8)(((1U << width) - 1U) << low);

    value = (value & (u8)~mask) | ((field << low) & mask);
}

u8 LaserActive::DecodeBcd(u8 value)
{
    return BcdToDec(value);
}

u8 LaserActive::EncodeBcd(u8 value)
{
    return DecToBcd(value);
}

void LaserActive::Clock(u32 cycles)
{
    if (!m_cdrom_media->IsLaserDisc())
        return;

    m_sector_clock += (u64)cycles * 75;

    while (m_sector_clock >= GG_MASTER_CLOCK_RATE)
    {
        m_sector_clock -= GG_MASTER_CLOCK_RATE;
        ClockSector();
    }
}

u8 LaserActive::ReadSramControl(u16 address) const
{
    switch (address)
    {
        case 0x18C0:
            return 0x00;

        case 0x18C1:
            return 0xAA;

        case 0x18C2:
            return 0x55;

        case 0x18C3:
            return 0x03;

        default:
            return 0xFF;
    }
}

void LaserActive::WriteSramControl(u8 data)
{
    m_sram_shift = (u16)((m_sram_shift << 8) | data);

    if ((m_sram_shift == 0xAA55) && !m_sram_enabled)
    {
        m_sram_enabled = true;

        if (IsValidPointer(m_memory))
            m_memory->UpdateLaserActiveSram();
    }
}

bool LaserActive::IsSramEnabled() const
{
    return m_sram_enabled;
}

void LaserActive::NotifyMediaEjected(bool ejected)
{
    if (ejected)
    {
        StopVideoDecoder();
        m_current_drive_state = 1;
        m_target_drive_state = 1;
        m_drive_mode = DRIVE_INACTIVE;
        m_digital_sector_valid = false;
        m_audio_end_pending = false;
        m_seek_latency = 0;
        m_seek_frame_pending = false;
        m_current_sample = 0;
        m_current_video_frame = -1;
        m_video_memory_latched = false;
        m_search_sectors = 0;
    }
    else
    {
        m_current_drive_state = 2;
        m_target_drive_state = 2;

        if (IsLaserDisc())
            StartVideoDecoder();
    }

    m_analog_cache_valid = false;
}

void LaserActive::NotifyMediaChanged()
{
    StopVideoDecoder();
    m_audio_end_pending = false;
    m_search_sectors = 0;
    m_digital_sector_valid = false;
    m_analog_cache_valid = false;
    m_current_video_frame = -1;
    m_head_lba = 0;
    m_current_track = 1;

    if (IsLaserDisc())
        StartVideoDecoder();
}

u32 LaserActive::NotifyScsiReadStart(u32 lba)
{
    if (!IsLaserDisc())
        return 0;

    SeekToSector((s32)lba, false);
    m_seek_drive_mode = DRIVE_READING;

    return (u32)(((u64)m_seek_latency * GG_MASTER_CLOCK_RATE - m_sector_clock + 74) / 75);
}

void LaserActive::NotifyScsiSectorRead(u32 lba, bool final_sector)
{
    if (!m_cdrom_media->IsLaserDisc())
        return;

    m_head_lba = (s32)lba + 1;
    m_cdrom_media->SetCurrentSector((u32)m_head_lba);
    m_current_track = GetTrackFromLBA(m_head_lba);
    m_current_sample = 0;
    UpdateVideoFrame(m_head_lba + 150);

    if (final_sector)
        m_drive_mode = DRIVE_INACTIVE;
}

void LaserActive::NotifyAudioStart(u32 lba, bool paused)
{
    if (!IsLaserDisc())
        return;

    SeekToSector((s32)lba, paused);
    m_audio_end_lba = (s32)m_cdrom_media->GetSectorCount();
}

void LaserActive::SetAudioEnd(u32 lba)
{
    m_audio_end_lba = (s32)lba + 1;
    SetDrivePlaying();
}

void LaserActive::NotifyAudioStop(bool paused)
{
    if (!m_cdrom_media->IsLaserDisc())
        return;

    if (paused)
        SetDrivePaused();
    else
        SetDriveStopped();
}

bool LaserActive::IsDiscLoaded() const
{
    return m_cdrom_media->IsMmi() && !m_cdrom_media->IsMmiEjected() && m_cdrom_media->IsReady();
}

bool LaserActive::IsLaserDisc() const
{
    return IsDiscLoaded() && m_cdrom_media->IsLaserDisc();
}

bool LaserActive::IsCLVDisc() const
{
    const GG_MmiMediaInfo* media_info = m_cdrom_media->GetSelectedMmiMedia();

    if (!IsLaserDisc() || !media_info)
        return false;

    return (media_info->format.length() >= 3) &&
        (media_info->format.compare(media_info->format.length() - 3, 3, "CLV") == 0);
}

u8 LaserActive::GetTrackCount() const
{
    return m_cdrom_media->GetTrackCount();
}

u8 LaserActive::GetCurrentTrack() const
{
    return m_current_track;
}

bool LaserActive::IsAudioTrack(u8 track) const
{
    return (track >= 1) && (track <= GetTrackCount()) &&
        (m_cdrom_media->GetTrackType(track - 1) == GG_CDROM_AUDIO_TRACK);
}

s32 LaserActive::GetTrackStartLBA(u8 track) const
{
    if ((track < 1) || (track > GetTrackCount()))
        return -1;

    return (s32)m_cdrom_media->GetFirstSectorOfTrack(track - 1);
}

s32 LaserActive::GetTrackEndLBA(u8 track) const
{
    if ((track < 1) || (track > GetTrackCount()))
        return -1;

    return (s32)m_cdrom_media->GetLastSectorOfTrack(track - 1);
}

s32 LaserActive::GetMinimumLBA() const
{
    const GG_MmiMediaInfo* media_info = m_cdrom_media->GetSelectedMmiMedia();

    if (!media_info)
        return -150;

    for (size_t i = 0; i < media_info->streams.size(); i++)
    {
        if (media_info->streams[i].role == GG_MMI_STREAM_RAW_VIDEO)
        {
            s64 frames = media_info->streams[i].frames_in_lead_in_region;
            s64 sectors = (s64)llround(((double)frames / k_laseractive_video_fps) * 75.0);
            s64 lba = -sectors - 150;

            return (s32)CLAMP(lba, (s64)INT32_MIN, (s64)-150);
        }
    }

    return -150;
}

u8 LaserActive::GetTrackFromLBA(s32 lba) const
{
    if (GetTrackCount() == 0)
        return 0;

    if ((lba < 0) && (lba >= GetMinimumLBA()))
        return 1;

    if (lba < 0)
        return 0;

    s32 track = m_cdrom_media->FindTrackFromLBA((u32)lba, true);

    return (track >= 0) ? (u8)(track + 1) : 0;
}

void LaserActive::GetTrackTOC(u8 track, u8& flags, u8& minute, u8& second, u8& frame) const
{
    flags = 0;
    minute = 0;
    second = 0;
    frame = 0;

    s32 lba = GetTrackStartLBA(track);

    if (lba < 0)
        return;

    flags = IsAudioTrack(track) ? 0 : 4;
    GetTimecode(lba, minute, second, frame);
}

void LaserActive::GetTimecode(s32 lba, u8& minute, u8& second, u8& frame) const
{
    s32 aba = lba + 150;

    if (aba < 0)
        aba = 0;

    minute = (u8)(aba / (75 * 60));
    second = (u8)((aba / 75) % 60);
    frame = (u8)(aba % 75);
}

void LaserActive::GetRelativeTimecode(u8& minute, u8& second, u8& frame) const
{
    s32 start = GetTrackStartLBA(m_current_track);
    s32 relative = (start >= 0) ? m_head_lba - start : 0;

    if (relative < 0)
        relative = 0;

    minute = (u8)(relative / (75 * 60));
    second = (u8)((relative / 75) % 60);
    frame = (u8)(relative % 75);
}

s32 LaserActive::GetABAFromTime(u8 hour, u8 minute, u8 second, u8 frame) const
{
    return (((((s32)hour * 60 + minute) * 60 + second) * 75) + frame);
}

double LaserActive::GetNormalizedPosition(s32 lba) const
{
    static const double sector_count = 7500.0 + 330000.0 + 6750.0;
    static const double radius_range = 0.058 - 0.024;
    static const double inner_radius_squared = 0.024 * 0.024;
    static const double outer_radius_squared = 0.058 * 0.058;
    double sector = (double)(lba + 150);
    double radius_squared = (sector / sector_count) *
        (outer_radius_squared - inner_radius_squared) + inner_radius_squared;

    if (radius_squared < 0.0)
        radius_squared = 0.0;

    return sqrt(radius_squared) / radius_range;
}

u32 LaserActive::CalculateSeekLatency(s32 target_lba) const
{
    double distance = fabs(GetNormalizedPosition(m_head_lba) - GetNormalizedPosition(target_lba));

    return (u32)(20.0 + 10.0 * distance);
}

void LaserActive::SeekToSector(s32 lba, bool paused)
{
    m_audio_end_pending = false;
    m_search_sectors = 0;
    m_drive_mode = DRIVE_SEEKING;
    m_seek_drive_mode = paused ? DRIVE_PAUSED : DRIVE_PLAYING;
    m_seek_target_lba = lba;
    m_seek_latency = CalculateSeekLatency(lba);
    m_audio_end_lba = 0x00FFFFFF;
    m_current_sample = 0;
    m_digital_sector_valid = false;
    m_analog_cache_valid = false;
    m_seek_frame_pending = true;
}

void LaserActive::SeekToTrack(u8 track, bool paused)
{
    s32 lba = GetTrackStartLBA(track);

    if (lba < 0)
        return;

    if ((track == 1) && IsLaserDisc())
        lba -= 150;

    SeekToSector(lba, paused);
}

void LaserActive::SeekToRelativeTime(u8 track, u8 minute, u8 second, u8 frame, bool paused)
{
    s32 lba = GetTrackStartLBA(track);

    if (lba < 0)
        return;

    lba += ((s32)minute * 60 * 75) + ((s32)second * 75) + frame;
    SeekToSector(lba, paused);
}

void LaserActive::SetDrivePlaying()
{
    if (m_drive_mode == DRIVE_SEEKING)
        m_seek_drive_mode = DRIVE_PLAYING;
    else
        m_drive_mode = DRIVE_PLAYING;
}

void LaserActive::SetDrivePaused()
{
    if (m_drive_mode == DRIVE_SEEKING)
        m_seek_drive_mode = DRIVE_PAUSED;
    else
        m_drive_mode = DRIVE_PAUSED;
}

void LaserActive::SetDriveStopped()
{
    m_audio_end_pending = false;
    m_search_sectors = 0;
    m_drive_mode = IsDiscLoaded() ? DRIVE_STOPPED : DRIVE_INACTIVE;
}

s32 LaserActive::GetSectorAdvance() const
{
    s32 advance = 1;

    if (m_audio_end_lba != 0x00FFFFFF)
        return advance;

    switch (m_playback_mode)
    {
        case 0:
        case 1:
            advance = 1;
            break;

        case 2:
        {
            static const s32 sector_repeat_counts[8] = { 0, 1, 2, 4, 8, 16, 30, 90 };

            if (m_playback_speed == 0)
                advance = 0;
            else if (m_playback_speed == 1)
                advance = 1;
            else
                advance = (m_sector_repeat_count + 1 >= sector_repeat_counts[m_playback_speed]) ? 1 : 0;

            if (m_playback_reverse)
                advance = -advance;

            break;
        }

        case 3:
        {
            // Both search directions play forwards, then jump four seconds.
            if (m_playback_speed >= 6)
                return m_search_sectors + 1 >= k_laseractive_search_sectors ? 1 + (m_playback_reverse ? -300 : 300) : 1;

            static const s32 sector_advances[8] = { 1, 2, 3, 8, 14, 20, 1, 1 };

            advance = sector_advances[m_playback_speed & 7];

            if (m_playback_reverse)
                advance = -advance;

            break;
        }

        default:
            break;
    }

    return advance;
}

void LaserActive::ClockSector()
{
    if (!IsDiscLoaded())
    {
        m_drive_mode = DRIVE_INACTIVE;
        return;
    }

    if (m_drive_mode == DRIVE_SEEKING)
    {
        if ((m_seek_latency != 0) && (--m_seek_latency != 0))
            return;

        m_drive_mode = m_seek_drive_mode;
        m_head_lba = m_seek_target_lba;
        m_current_track = GetTrackFromLBA(m_head_lba);

        if (m_head_lba >= 0)
            m_cdrom_media->SetCurrentSector((u32)m_head_lba);

        if (m_drive_mode == DRIVE_PAUSED || m_drive_mode == DRIVE_READING)
            UpdateVideoFrame(m_head_lba + 150);
    }

    if (m_drive_mode != DRIVE_PLAYING || m_audio_end_pending)
        return;

    s32 sector_lba = m_head_lba;

    m_digital_sector_valid = false;

    if ((sector_lba >= 0) && ((u32)sector_lba < m_cdrom_media->GetSectorCount()) &&
        (GetTrackFromLBA(sector_lba) != 0) && IsAudioTrack(GetTrackFromLBA(sector_lba)))
    {
        m_digital_sector_valid = m_cdrom_media->ReadSamples((u32)sector_lba, 0,
            reinterpret_cast<s16*> (m_digital_sector), sizeof(m_digital_sector) / sizeof(s16));
    }

    if (!m_digital_sector_valid)
        memset(m_digital_sector, 0, sizeof(m_digital_sector));

    m_current_sample = 0;

    s32 advance = GetSectorAdvance();

    if (m_audio_end_lba == 0x00FFFFFF && m_playback_mode == 3 && m_playback_speed >= 6)
        m_search_sectors = (m_search_sectors + 1) % k_laseractive_search_sectors;
    else
        m_search_sectors = 0;

    if ((m_playback_mode == 2) && (m_playback_speed >= 2))
    {
        if (advance == 0)
            m_sector_repeat_count++;
        else
            m_sector_repeat_count = 0;
    }
    else
        m_sector_repeat_count = 0;

    s64 next_lba = (s64)m_head_lba + advance;

    if ((next_lba < INT32_MIN) || (next_lba > INT32_MAX))
    {
        m_drive_mode = DRIVE_INACTIVE;
        return;
    }

    if (m_audio_end_lba != 0x00FFFFFF && next_lba >= m_audio_end_lba)
    {
        m_audio_end_pending = true;
        return;
    }

    u8 track = GetTrackFromLBA((s32)next_lba);

    if (track == 0)
    {
        m_drive_mode = DRIVE_INACTIVE;
        return;
    }

    m_head_lba = (s32)next_lba;

    if (m_head_lba >= 0)
        m_cdrom_media->SetCurrentSector((u32)m_head_lba);

    m_current_track = track;
    UpdateVideoFrame(m_head_lba + 150);

    if (m_stop_point_enabled && ((m_head_lba + 150) == m_stop_point_aba))
        HandleStopPoint(m_head_lba + 150);

    if ((m_audio_end_lba != 0x00FFFFFF) && (m_head_lba == m_audio_end_lba))
        m_drive_mode = DRIVE_INACTIVE;
}

s32 LaserActive::GetVideoFrameFromABA(s32 aba, bool lead_in) const
{
    if (!lead_in && (aba < 0))
        return 0;

    s32 frame = (s32)llround(((double)aba / 75.0) * k_laseractive_video_fps);

    return (lead_in && (aba < 0)) ? (-frame) - 1 : frame;
}

s32 LaserActive::GetABAFromVideoFrame(s32 frame) const
{
    return (s32)llround(((double)frame / k_laseractive_video_fps) * 75.0);
}

void LaserActive::VideoTimeToRedbookTime(u8& hour, u8& minute, u8& second, u8& frame) const
{
    s32 video_frames = (((((s32)hour * 60 + minute) * 60 + second) * 30) + frame);
    s32 redbook_frames = (s32)llround(((double)video_frames / k_laseractive_video_fps) * 75.0);

    frame = (u8)(redbook_frames % 75);
    redbook_frames /= 75;
    second = (u8)(redbook_frames % 60);
    redbook_frames /= 60;
    minute = (u8)(redbook_frames % 60);
    redbook_frames /= 60;
    hour = (u8)redbook_frames;
}

void LaserActive::UpdateVideoFrame(s32 aba)
{
    const GG_MmiMediaInfo* media_info = m_cdrom_media->GetSelectedMmiMedia();

    if (!media_info)
        return;

    const GG_MmiStreamInfo* video_stream = NULL;

    for (size_t i = 0; i < media_info->streams.size(); i++)
    {
        if (media_info->streams[i].role == GG_MMI_STREAM_RAW_VIDEO)
        {
            video_stream = &media_info->streams[i];
            break;
        }
    }

    if (!video_stream)
        return;

    bool completed_seek = m_seek_frame_pending;

    m_seek_frame_pending = false;

    s32 frame = GetVideoFrameFromABA(aba, true);
    bool lead_in = aba < 0;
    bool lead_out = !lead_in && (frame >= video_stream->frames_in_active_region);

    if (lead_out)
        frame -= (s32)video_stream->frames_in_active_region;

    if (lead_in)
    {
        if (frame >= video_stream->frames_in_lead_in_region)
            frame = video_stream->frames_in_lead_in_region > 0 ? (s32)video_stream->frames_in_lead_in_region - 1 : 0;
    }
    else if (lead_out)
    {
        if (frame >= video_stream->frames_in_lead_out_region)
            frame = video_stream->frames_in_lead_out_region > 0 ? (s32)video_stream->frames_in_lead_out_region - 1 : 0;
    }

    bool same_frame = (frame == m_current_video_frame) &&
        (lead_in == m_current_video_lead_in) && (lead_out == m_current_video_lead_out);
    bool looping = (m_current_drive_state == 5) && !m_current_pause &&
        (m_playback_mode == 2) && (m_playback_speed == 0);

    if (!completed_seek && same_frame && !looping && m_video_memory_latched)
        return;

    if (!completed_seek && (m_playback_mode == 1))
    {
        static const s32 frame_skip_counts[8] = { 1, -1, 2, 4, 8, 16, 30, 90 };
        s32 skip_count = frame_skip_counts[m_playback_speed & 7];

        if (m_frame_skip_counter != skip_count)
        {
            m_frame_skip_base = frame;
            m_frame_skip_counter = skip_count;

            if (m_playback_speed == 1)
                m_playback_reverse = !m_playback_reverse;
        }
        else if (skip_count == -1)
            return;
        else if ((skip_count > 0) && ((abs(frame - m_frame_skip_base) % skip_count) != 0))
            return;
    }
    else if ((m_playback_mode == 2) && (m_playback_speed == 1))
        m_playback_speed = 0;
    else
    {
        m_frame_skip_base = 0;
        m_frame_skip_counter = 0;
    }

    if ((GetBit(m_input_regs[0x0C], 5) && m_video_memory_latched) || GetBit(m_input_regs[0x0C], 2))
    {
        if (GetBit(m_input_regs[0x0C], 2) && !GetBit(m_input_regs[0x0C], 5))
            m_video_memory_latched = false;

        return;
    }

    m_video_memory_latched = true;
    m_video_field_selected = (GetVideoMixingMode() >= 2) &&
        (GetBit(m_input_regs[0x0C], 1) || GetBit(m_input_regs[0x0C], 3));
    m_video_selected_even = m_video_field_selected && GetBit(m_input_regs[0x0C], 0);

    if (same_frame && m_video_frame_valid)
        return;

    m_current_video_frame = frame;
    m_current_video_lead_in = lead_in;
    m_current_video_lead_out = lead_out;

    bool decoded = LoadCurrentVideoFrame();

    if (decoded && !IsCLVDisc() && !GetBit(m_input_regs[0x0C], 7) && ((m_playback_mode != 2) || (m_playback_speed > 1)))
    {
        u32 even_code = DecodeBiphaseCode(15);

        if (even_code == 0)
            even_code = DecodeBiphaseCode(16);

        u32 odd_code = DecodeBiphaseCode(278);

        if (odd_code == 0)
            odd_code = DecodeBiphaseCode(279);

        if ((even_code == 0x82CFFF) || (odd_code == 0x82CFFF))
        {
            m_reached_stop_point_previously = true;
            m_operation_error_1 = false;
            m_operation_error_2 = false;
            m_operation_error_3 = true;
            m_reached_stop_point = true;
            m_drive_mode = DRIVE_PAUSED;
            m_stop_point_enabled = false;
        }
    }
#if !defined(GG_DISABLE_MMI_THREADS)
    QueueVideoPrefetch(PredictNextVideoFrame());
#endif
}

bool LaserActive::FillAnalogCache(u64 offset)
{
    u64 aligned_offset = offset - (offset % sizeof(m_analog_cache));

    if (m_analog_cache_valid && (m_analog_cache_offset == aligned_offset))
        return true;

    u64 file_size = m_analog_audio_size;

    if (aligned_offset >= file_size)
    {
        m_analog_cache_valid = false;
        return false;
    }

    u32 read_size = (u32)MIN((u64)sizeof(m_analog_cache), file_size - aligned_offset);

    memset(m_analog_cache, 0, sizeof(m_analog_cache));

    if (!m_cdrom_media->ReadMmiAnalogAudio(aligned_offset, m_analog_cache, read_size))
    {
        m_analog_cache_valid = false;
        return false;
    }

    m_analog_cache_offset = aligned_offset;
    m_analog_cache_valid = true;

    return true;
}

bool LaserActive::ReadAnalogSample(s16& left, s16& right)
{
    left = 0;
    right = 0;

    s64 aba = (s64)m_head_lba + 150;
    s64 sample_index = (aba * 588) + (s64)m_current_sample;

    if ((sample_index > 0) && ((u64)sample_index > ((u64)INT64_MAX - (u64)m_analog_lead_in_samples)))
        return false;

    sample_index += (s64)m_analog_lead_in_samples;

    if (sample_index < 0)
        return true;

    u64 byte_offset;
    u64 sample_end;

    if (!checked_multiply_u64((u64)sample_index, 4, &byte_offset) ||
        !checked_add_u64(byte_offset, 4, &sample_end) ||
        (sample_end > m_analog_audio_size) || !FillAnalogCache(byte_offset))
        return true;

    u64 cache_position = byte_offset - m_analog_cache_offset;

    if ((cache_position + 4) > sizeof(m_analog_cache))
        return true;

    const u8* sample = m_analog_cache + cache_position;

    left = (s16)read_u16_le(sample);
    right = (s16)read_u16_le(sample + 2);

    return true;
}

void LaserActive::Sample(s16& left, s16& right)
{
    left = 0;
    right = 0;

    u32 fade_left = m_analog_fade_samples_left;
    u32 fade_right = m_analog_fade_samples_right;

    if (m_analog_fade_samples_left > 0)
        m_analog_fade_samples_left--;

    if (m_analog_fade_samples_right > 0)
        m_analog_fade_samples_right--;

    if (!IsLaserDisc())
        return;

    s16 digital_left = 0;
    s16 digital_right = 0;

    if ((m_drive_mode == DRIVE_PLAYING) && m_digital_sector_valid && (m_current_sample < 588))
    {
        u32 offset = m_current_sample * 4;

        digital_left = (s16)read_u16_le(m_digital_sector + offset);
        digital_right = (s16)read_u16_le(m_digital_sector + offset + 2);
    }

    if (IsValidPointer(m_cdrom) && m_cdrom->IsFaderEnabled(false))
    {
        double fader = m_cdrom->GetFaderValue();

        digital_left = (s16)(digital_left * fader);
        digital_right = (s16)(digital_right * fader);
    }

    u8 analog_mode = GetBits(m_input_regs[0x01], 6, 7);
    u8 audio_selection = GetBits(m_input_regs[0x0D], 6, 7);
    bool digital_disabled = (analog_mode > 0) &&
        ((audio_selection == 2) || ((analog_mode == 1) && GetBit(m_input_regs[0x0D], 4)));
    bool digital_attenuation_disabled = (analog_mode == 0) || (audio_selection == 3);
    bool analog_disabled = (analog_mode == 0) || ((analog_mode == 1) && !GetBit(m_input_regs[0x0D], 4));

    if (digital_disabled)
    {
        digital_left = 0;
        digital_right = 0;
    }

    if (!digital_attenuation_disabled)
    {
        digital_left = (s16)(((s32)digital_left * m_input_regs[0x0F]) / 255);
        digital_right = (s16)(((s32)digital_right * m_input_regs[0x0F]) / 255);
    }

    if ((analog_mode != 0) && GetBit(m_input_regs[0x0D], 0) && GetBit(m_input_regs[0x0D], 1))
    {
        digital_left /= 2;
        digital_right /= 2;
    }
    else if (GetBit(m_input_regs[0x0D], 0))
        digital_right = digital_left;
    else if (GetBit(m_input_regs[0x0D], 1))
        digital_left = digital_right;

    s16 analog_left = 0;
    s16 analog_right = 0;

    if (m_drive_mode == DRIVE_PLAYING && m_current_sample < 588)
        m_current_sample++;

    ReadAnalogSample(analog_left, analog_right);

    if (analog_disabled || GetBit(m_input_regs[0x0E], 7) ||
        (GetBit(m_input_regs[0x0E], 0) && GetBit(m_input_regs[0x0E], 1)))
    {
        analog_left = 0;
        analog_right = 0;
    }
    else if (GetBit(m_input_regs[0x0E], 0))
        analog_right = analog_left;
    else if (GetBit(m_input_regs[0x0E], 1))
        analog_left = analog_right;

    u32 analog_scale_left = 0x100 - m_analog_attenuation_left;
    u32 analog_scale_right = 0x100 - m_analog_attenuation_right;

    analog_left = (s16)(((s32)analog_left * (s32)analog_scale_left) >> 8);
    analog_right = (s16)(((s32)analog_right * (s32)analog_scale_right) >> 8);

    if (m_analog_fade_muted_left)
        analog_left = (s16)((s32)analog_left * (s32)fade_left / (s32)k_laseractive_fade_samples);

    if (m_analog_fade_muted_right)
        analog_right = (s16)((s32)analog_right * (s32)fade_right / (s32)k_laseractive_fade_samples);

    if (m_playback_mode == 2)
    {
        digital_left = 0;
        digital_right = 0;
        analog_left = 0;
        analog_right = 0;
    }

    s32 combined_left = (s32)digital_left + analog_left;
    s32 combined_right = (s32)digital_right + analog_right;

    left = (s16)CLAMP(combined_left, -32768, 32767);
    right = (s16)CLAMP(combined_right, -32768, 32767);

    if (m_audio_end_pending && m_drive_mode == DRIVE_PLAYING && m_current_sample == 588)
    {
        m_audio_end_pending = false;
        m_drive_mode = DRIVE_STOPPED;

        if (IsValidPointer(m_cdrom_audio))
            m_cdrom_audio->FinishLaserActivePlayback();
    }
}

u8 LaserActive::ReadRegister(u8 reg, bool output)
{
    reg &= 0x1F;

    if (!output)
        return m_input_frozen ? m_input_frozen_regs[reg] : m_input_regs[reg];

    if (m_output_cooldown[reg] > 0)
    {
        m_output_cooldown[reg]--;
        return m_output_written_data[reg];
    }

    if (m_output_frozen)
        return m_output_frozen_regs[reg];

    return GetOutputRegisterValue(reg);
}

u8 LaserActive::PeekRegister(u8 reg, bool output)
{
    reg &= 0x1F;

    if (!output)
        return m_input_frozen ? m_input_frozen_regs[reg] : m_input_regs[reg];

    if (m_output_cooldown[reg] > 0)
        return m_output_written_data[reg];

    if (m_output_frozen)
        return m_output_frozen_regs[reg];

    return GetOutputRegisterValue(reg, false);
}

void LaserActive::WriteRegister(u8 reg, bool output, u8 data)
{
    reg &= 0x1F;

    if (!output)
    {
        if (reg == 0)
        {
            if (m_input_frozen)
                m_input_frozen_regs[0] = data;

            bool previous_output_frozen = m_output_frozen;

            m_output_frozen = GetBit(data, 6);

            if (m_output_frozen && !previous_output_frozen)
            {
                for (u8 i = 0; i < 0x20; i++)
                    m_output_frozen_regs[i] = m_output_regs[i] = GetOutputRegisterValue(i);
            }

            bool previous_input_frozen = m_input_frozen;

            m_input_frozen = !GetBit(data, 7);

            if (!m_input_frozen && previous_input_frozen)
                ApplyFrozenInputRegisters();
            else if (m_input_frozen && !previous_input_frozen)
                memcpy(m_input_frozen_regs, m_input_regs, sizeof(m_input_regs));
        }

        if (m_input_frozen)
        {
            m_input_frozen_regs[reg] = data;

            if (reg >= 0x19)
                ProcessInputRegisterWrite(reg, data, m_input_regs[reg], false);
        }
        else
            ProcessInputRegisterWrite(reg, data, m_input_regs[reg], false);

        return;
    }

    if (m_output_frozen)
    {
        if (reg == 0)
        {
            m_output_frozen_regs[0] = (data & 0x7F) | (m_output_regs[0] & 0x80);
            m_output_regs[0] = (data & 0x3E) | (m_output_regs[0] & 0xC1);
        }
        else
        {
            m_output_frozen_regs[reg] = data;

            if (reg >= 0x1A)
                m_output_regs[reg] = data;
        }
    }
    else
    {
        m_output_written_data[reg] = data;
        m_output_cooldown[reg] = 5;

        if (reg == 0)
            m_output_regs[0] = (data & 0x3E) | (m_output_regs[0] & 0xC1);
        else if (reg >= 0x1A)
            m_output_regs[reg] = data;
    }
}

void LaserActive::ApplyFrozenInputRegisters()
{
    u8 previous_seek_regs[6];

    memcpy(previous_seek_regs, m_input_regs + 6, sizeof(previous_seek_regs));
    memcpy(m_input_regs + 6, m_input_frozen_regs + 6, sizeof(previous_seek_regs));

    if (HasLiveSeekTarget())
        LatchSeekTarget();

    ProcessInputRegisterWrite(0x02, m_input_frozen_regs[0x02], m_input_regs[0x02], true);
    ProcessInputRegisterWrite(0x03, m_input_frozen_regs[0x03], m_input_regs[0x03], true);

    for (u8 i = 0; i < 0x20; i++)
    {
        u8 previous_data = ((i >= 6) && (i <= 0x0B)) ? previous_seek_regs[i - 6] : m_input_regs[i];

        ProcessInputRegisterWrite(i, m_input_frozen_regs[i], previous_data, true);
    }
}

u8 LaserActive::GetOutputRegisterValue(u8 reg, bool side_effects)
{
    u8 data = m_output_regs[reg];
    u8 drive_state_delay = m_drive_state_delay;
    u8 flags = 0;
    u8 minute = 0;
    u8 second = 0;
    u8 frame = 0;

    switch (reg)
    {
        case 0x00:
            SetBit(data, 7, true);
            SetBit(data, 6, false);
            SetBit(data, 0, GetBit(m_input_regs[0], 0));
            break;

        case 0x01:
            data = 0x80;
            break;

        case 0x02:
            data = 0;

            if (m_current_drive_state < 2)
                data = 0;
            else if (!IsDiscLoaded())
                data = 0x10;
            else if (IsLaserDisc())
                data = (m_current_drive_state >= 4) ? (IsCLVDisc() ? 0xC6 : 0xC5) : 0x80;
            else
                data = 0x20;

            break;

        case 0x03:
            if (m_current_drive_state < 2)
                data = 0;
            else if (m_current_drive_state >= 4)
            {
                SetBit(data, 3, IsDiscLoaded());
                SetBits(data, 6, 7, IsLaserDisc() && m_cdrom_media->GetSelectedMmiMedia() &&
                    (m_cdrom_media->GetSelectedMmiMedia()->side_number > 1) ? 2 :
                    (IsLaserDisc() ? 1 : 0));
            }

            break;

        case 0x04:
            data = 0;
            SetBit(data, 5, !IsDiscLoaded() || !IsLaserDisc() || (m_current_drive_state < 4));

            if (!IsDiscLoaded() || IsLaserDisc() || (m_current_drive_state < 2))
                SetBit(data, 7, GetBit(m_input_regs[0x0D], 4));

            if (!IsDiscLoaded() || IsLaserDisc() || (m_current_drive_state >= 4))
                SetBits(data, 0, 3, IsAudioTrack(GetCurrentTrack()) ? 0x0C : 0x0E);
            else
                SetBits(data, 0, 3, 2);

            break;

        case 0x05:
            data = 0x7F;
            break;

        case 0x06:
        {
            u8 previous_state = GetBits(data, 0, 3);

            if ((previous_state != m_current_drive_state) || (drive_state_delay > 0))
            {
                if (!GetBit(data, 7) || (drive_state_delay == 0))
                    drive_state_delay = 10;
                else if (drive_state_delay == 1)
                {
                    SetBits(data, 0, 3, m_current_drive_state);
                    drive_state_delay = 0;
                }
                else
                    drive_state_delay--;
            }

            bool seeking = (m_drive_mode == DRIVE_SEEKING) || ((m_drive_mode == DRIVE_PLAYING) && m_seek_frame_pending);

            if ((GetBits(data, 0, 3) == 5) && seeking)
            {
                SetBit(data, 7, true);
                SetBit(data, 5, true);
            }
            else
            {
                SetBit(data, 7, drive_state_delay > 0);
                SetBit(data, 5, false);
            }

            SetBit(data, 4, m_current_pause || m_target_pause);
            break;
        }

        case 0x07:
            data = 0;
            SetBits(data, 4, 7, m_playback_mode);
            SetBit(data, 3, m_playback_reverse);
            SetBits(data, 0, 2, m_playback_speed);

            if (m_reached_stop_point)
                data = 0x20;
            else if ((m_playback_mode == 2) && (m_playback_speed == 0))
                SetBit(data, 3, false);

            break;

        case 0x08:
            SetBit(data, 7, IsDiscLoaded() && IsLaserDisc() && (m_current_drive_state >= 5));
            SetBit(data, 6, IsDiscLoaded() && ((!IsLaserDisc() && (m_current_drive_state >= 2)) ||
                (IsLaserDisc() && (m_current_drive_state >= 4))));
            SetBit(data, 0, IsDiscLoaded() && (m_current_drive_state >= 4));
            break;

        case 0x09:
            SetBit(data, 4, m_operation_error_1);
            SetBit(data, 1, m_operation_error_2);
            SetBit(data, 0, m_operation_error_3);
            break;

        case 0x0A:
            data = m_input_regs[0x06];
            break;

        case 0x0B:
            data = m_input_regs[0x07];
            break;

        case 0x0C:
            data = m_input_regs[0x08];
            break;

        case 0x0D:
            data = m_input_regs[0x09];
            break;

        case 0x0E:
            data = m_input_regs[0x0A];
            break;

        case 0x0F:
            data = m_input_regs[0x0B];
            break;

        case 0x10:
            data = m_input_regs[0x05];
            break;

        case 0x11:
        {
            if (m_selected_track_info > GetTrackCount())
                data = 0;
            else
            {
                u8 track = (m_selected_track_info == 0) ? GetCurrentTrack() : m_selected_track_info;

                GetTrackTOC(track, flags, minute, second, frame);
                data = (u8)((flags << 4) | 1);
            }

            break;
        }

        case 0x12:
        case 0x13:
        case 0x14:
        {
            if ((m_selected_track_info == 0xA0) || (m_selected_track_info == 0xB0))
            {
                if (reg == 0x12)
                    data = EncodeBcd(1);
                else if (reg == 0x13)
                    data = EncodeBcd(GetTrackCount());
                else
                    data = 0;
            }
            else if ((m_selected_track_info == 0xA1) || (m_selected_track_info == 0xB1))
            {
                GetTimecode((s32)m_cdrom_media->GetSectorCount(), minute, second, frame);
                data = EncodeBcd(reg == 0x12 ? minute : (reg == 0x13 ? second : frame));
            }
            else if ((m_selected_track_info == 0) || (m_selected_track_info == 0xFF))
            {
                GetTimecode(m_head_lba, minute, second, frame);
                data = EncodeBcd(reg == 0x12 ? minute : (reg == 0x13 ? second : frame));
            }
            else if (m_selected_track_info > GetTrackCount())
                data = 0xFF;
            else
            {
                GetTrackTOC(m_selected_track_info, flags, minute, second, frame);
                data = EncodeBcd(reg == 0x12 ? minute : (reg == 0x13 ? second : frame));
            }

            break;
        }

        case 0x15:
            data = IsDiscLoaded() && (m_current_drive_state >= 5) ? EncodeBcd(GetCurrentTrack()) :
                (IsDiscLoaded() && (m_current_drive_state >= 2) ? 1 : 0);
            break;

        case 0x16:
        case 0x17:
        case 0x18:
        case 0x19:
        {
            data = 0;

            if (IsDiscLoaded() && (m_current_drive_state >= 5))
            {
                if (IsLaserDisc())
                {
                    s32 video_frame = GetVideoFrameFromABA(m_head_lba + 150, false) + 1;

                    if (IsCLVDisc())
                    {
                        if (reg == 0x16)
                            data = EncodeBcd((u8)((video_frame / (60 * 60 * 30)) % 60));

                        if (reg == 0x17)
                            data = EncodeBcd((u8)((video_frame / (60 * 30)) % 60));

                        if (reg == 0x18)
                            data = EncodeBcd((u8)((video_frame / 30) % 60));

                        if (reg == 0x19)
                            data = EncodeBcd((u8)(video_frame % 30));
                    }
                    else
                    {
                        if (reg == 0x16)
                            data = EncodeBcd((u8)((video_frame / 10000) % 100));

                        if (reg == 0x17)
                            data = EncodeBcd((u8)((video_frame / 100) % 100));

                        if (reg == 0x18)
                            data = EncodeBcd((u8)(video_frame % 100));
                    }
                }
                else
                {
                    GetRelativeTimecode(minute, second, frame);

                    if (reg == 0x16)
                        data = 1;

                    if (reg == 0x17)
                        data = EncodeBcd(minute);

                    if (reg == 0x18)
                        data = EncodeBcd(second);

                    if (reg == 0x19)
                        data = EncodeBcd(frame);
                }
            }

            break;
        }

        default:
            break;
    }

    if (side_effects)
    {
        m_output_regs[reg] = data;
        m_drive_state_delay = drive_state_delay;
    }

    return data;
}

void LaserActive::ProcessInputRegisterWrite(u8 reg, u8 data, u8 previous_data, bool deferred)
{
    switch (reg)
    {
        case 0x00:
        {
            if (GetBit(previous_data, 0) != GetBit(data, 0))
            {
                m_operation_error_1 = false;
                m_operation_error_2 = false;
                m_operation_error_3 = false;
                m_reached_stop_point = false;

                if (m_seek_enabled)
                {
                    if (HasLiveSeekTarget())
                    {
                        if (LatchSeekTarget())
                            PerformLatchedSeek();
                    }
                    else
                        PerformLatchedSeek();
                }

                u8 live_mode = GetBits(m_input_regs[0x03], 4, 7);
                u8 live_speed = GetBits(m_input_regs[0x03], 0, 2);

                if ((live_mode == 1) && (m_playback_mode == 1) && (live_speed == 1) &&
                    (m_playback_speed == 1) && (m_frame_skip_counter == -1))
                {
                    m_frame_skip_base = 0;
                    m_frame_skip_counter = 0;
                }
                else if ((live_mode == 2) && (m_playback_mode == 2) && (live_speed == 1) && (m_playback_speed == 0))
                {
                    m_playback_speed = 1;
                }

                if (m_current_drive_state == 5)
                {
                    if (m_current_pause)
                        SetDrivePaused();
                    else
                        SetDrivePlaying();
                }
            }

            break;
        }

        case 0x01:
            break;

        case 0x02:
        {
            if (GetBits(data, 0, 5) == GetBits(previous_data, 0, 5))
                break;

            m_seek_enabled = GetBit(data, 5);
            m_operation_error_1 = false;
            m_operation_error_2 = false;
            m_operation_error_3 = false;

            u8 new_drive_state = GetBits(data, 0, 3);
            bool pause = GetBit(data, 4);

            if (pause != GetBit(previous_data, 4))
            {
                if ((new_drive_state == 0) || (new_drive_state == 5) || (new_drive_state == 7))
                {
                    m_target_pause = pause;
                    m_current_pause = pause;
                }
            }

            m_target_drive_state = new_drive_state;

            switch (new_drive_state)
            {
                case 1:
                    m_current_drive_state = 1;
                    m_cdrom_media->EjectMmi();

                    if (IsValidPointer(m_cdrom))
                        m_cdrom->NotifyMediaEjected();

                    m_drive_mode = DRIVE_INACTIVE;
                    ResetSeekTarget();
                    break;

                case 2:
                    if (m_cdrom_media->IsMmiEjected())
                        m_cdrom_media->InsertMmi();

                    SetDriveStopped();
                    m_current_drive_state = 2;
                    ResetSeekTarget();
                    break;

                case 3:
                    SetDriveStopped();
                    m_current_drive_state = 3;
                    ResetSeekTarget();
                    break;

                case 4:
                    if (!IsDiscLoaded())
                    {
                        m_operation_error_1 = true;
                        m_operation_error_2 = true;
                        break;
                    }

                    if (m_current_drive_state <= 3)
                    {
                        ResetSeekTarget();
                        SeekToTrack(1, true);
                        m_current_pause = true;
                    }

                    if (m_current_drive_state < 4)
                        m_current_drive_state = 4;
                    else
                        m_operation_error_1 = true;

                    break;

                case 5:
                {
                    if (!IsDiscLoaded())
                    {
                        m_operation_error_1 = true;
                        m_operation_error_2 = true;
                        break;
                    }

                    if (m_seek_enabled)
                        m_reached_stop_point = false;

                    bool loaded = false;

                    if (m_current_drive_state <= 3)
                    {
                        ResetSeekTarget();
                        SeekToTrack(1, true);
                        m_seek_drive_mode = DRIVE_PAUSED;
                        m_current_pause = true;
                        loaded = true;
                    }

                    m_current_pause = m_target_pause;
                    m_current_drive_state = 5;

                    if (m_seek_enabled && (loaded || !deferred))
                    {
                        if (HasLiveSeekTarget())
                        {
                            if (LatchSeekTarget())
                                PerformLatchedSeek();
                        }
                        else
                            PerformLatchedSeek();
                    }

                    if (m_current_pause || m_reached_stop_point)
                        SetDrivePaused();
                    else
                        SetDrivePlaying();

                    break;
                }

                case 0:
                case 7:
                    if (!IsDiscLoaded())
                    {
                        if (new_drive_state == 7)
                        {
                            m_operation_error_1 = true;
                            m_operation_error_2 = true;
                        }
                    }
                    else if (m_current_drive_state == 5)
                    {
                        if (m_target_pause || m_reached_stop_point)
                            SetDrivePaused();
                        else
                            SetDrivePlaying();
                    }

                    break;

                default:
                    m_operation_error_1 = true;

                    if (!IsDiscLoaded())
                        m_operation_error_2 = true;

                    break;
            }

            break;
        }

        case 0x03:
        {
            if (data == previous_data)
                break;

            m_operation_error_1 = false;
            m_operation_error_2 = false;
            m_operation_error_3 = false;

            u8 mode = GetBits(data, 4, 7);

            if (mode >= 4)
                m_operation_error_1 = true;
            else
            {
                u8 speed = GetBits(data, 0, 2);
                bool reverse = GetBit(data, 3);

                if (mode == 0)
                    speed = 0;
                else if (mode == 1)
                    reverse = false;
                else if ((mode == 2) && (speed == 0))
                    reverse = false;
                else if ((mode == 3) && (speed == 7))
                    speed = 6;

                if (mode != m_playback_mode || speed != m_playback_speed || reverse != m_playback_reverse)
                    m_search_sectors = 0;

                m_playback_mode = mode;
                m_playback_speed = speed;
                m_playback_reverse = reverse;
                m_reached_stop_point = false;

                if (m_current_drive_state == 5)
                {
                    if (m_current_pause)
                        SetDrivePaused();
                    else
                        SetDrivePlaying();
                }
            }

            break;
        }

        case 0x05:
            m_selected_track_info = (data < 0x99) ? DecodeBcd(data) : data;
            break;

        case 0x06:
        {
            bool updated = false;
            u8 target_mode = GetBits(data, 0, 1);

            if (((target_mode == 1) || (target_mode == 2)) && (m_current_seek_mode != target_mode))
            {
                m_current_seek_mode = target_mode;
                updated = true;
            }

            bool time_format = GetBit(data, 2);

            if (((target_mode == 1) || (target_mode == 2)) && (m_current_seek_time_format != time_format))
            {
                m_current_seek_time_format = time_format;
                updated = true;
            }

            m_current_seek_repeat = GetBit(data, 7);

            if (target_mode == 3)
            {
                if (GetBits(previous_data, 0, 2) != GetBits(data, 0, 2))
                    UpdateStopPoint();
            }
            else if (updated && m_seek_enabled && (m_current_drive_state == 5) &&
                (((m_current_seek_mode == 1) && m_current_seek_time_format) ||
                (m_current_seek_mode == 2)))
            {
                if (LatchSeekTarget())
                    PerformLatchedSeek();
            }

            break;
        }

        case 0x07:
        case 0x08:
        case 0x09:
        case 0x0A:
        case 0x0B:
        {
            m_input_regs[reg] = data;

            if ((GetBits(m_input_regs[0x06], 0, 1) == 3) && (previous_data != data))
                UpdateStopPoint();

            if (!deferred && m_seek_enabled && (m_current_drive_state == 5) &&
                (((m_current_seek_mode == 1) && m_current_seek_time_format) ||
                (m_current_seek_mode == 2)))
            {
                if (LatchSeekTarget())
                    PerformLatchedSeek();
            }

            break;
        }

        case 0x0C:
            if (GetBit(data, 2) && !GetBit(data, 5))
                m_video_memory_latched = false;

            break;

        case 0x1E:
            m_input_regs[reg] = data;

            if (GetBit(m_input_regs[0x1E], 1) && GetBit(m_input_regs[0x1E], 3))
            {
                // Only the first mute operation fades. Muting the other channel while
                // either remains muted is immediate; repeated writes do not restart it.
                u32 samples = !m_analog_fade_muted_left && !m_analog_fade_muted_right ? k_laseractive_fade_samples : 0;

                if (!GetBit(data, 2) || !GetBit(data, 0))
                {
                    if (!m_analog_fade_muted_left)
                        m_analog_fade_samples_left = samples;

                    m_analog_fade_muted_left = true;
                }

                if (!GetBit(data, 2) || GetBit(data, 0))
                {
                    if (!m_analog_fade_muted_right)
                        m_analog_fade_samples_right = samples;

                    m_analog_fade_muted_right = true;
                }
            }
            // Register 1E deliberately also applies the current 1F attenuation value.
            ProcessInputRegisterWrite(0x1F, m_input_regs[0x1F], m_input_regs[0x1F], deferred);
            break;

        case 0x1F:
            if (GetBit(m_input_regs[0x1E], 1) && !GetBit(m_input_regs[0x1E], 3))
            {
                if (!GetBit(m_input_regs[0x1E], 2))
                {
                    m_analog_attenuation_left = data;
                    m_analog_attenuation_right = data;
                    m_analog_fade_muted_left = false;
                    m_analog_fade_muted_right = false;
                    m_analog_fade_samples_left = 0;
                    m_analog_fade_samples_right = 0;
                }
                else if (!GetBit(m_input_regs[0x1E], 0))
                {
                    m_analog_attenuation_left = data;
                    m_analog_fade_muted_left = false;
                    m_analog_fade_samples_left = 0;
                }
                else
                {
                    m_analog_attenuation_right = data;
                    m_analog_fade_muted_right = false;
                    m_analog_fade_samples_right = 0;
                }
            }

            break;

        default:
            break;
    }

    m_input_regs[reg] = data;
}

void LaserActive::UpdateStopPoint()
{
    if (m_input_regs[0x07] == 0xFF)
    {
        for (u8 i = 0x1A; i <= 0x1E; i++)
            m_output_regs[i] = 0xFF;

        m_output_regs[0x1F] = 1;
        m_stop_point_enabled = false;
        return;
    }

    m_stop_point_regs[SEEK_CHAPTER] = (m_input_regs[0x07] == 0) ? 0xFF : m_input_regs[0x07];
    m_stop_point_regs[SEEK_HOURS_OR_FRAME_HIGH] = IsLaserDisc() ? (m_input_regs[0x08] & 0x0F) : 0;
    m_stop_point_regs[SEEK_MINUTES_OR_FRAME_MIDDLE] = m_input_regs[0x09];
    m_stop_point_regs[SEEK_SECONDS_OR_FRAME_LOW] = m_input_regs[0x0A];
    m_stop_point_regs[SEEK_FRAMES] = IsLaserDisc() && !IsCLVDisc() ? 0 : m_input_regs[0x0B];
    m_output_regs[0x1A] = m_stop_point_regs[SEEK_CHAPTER];
    m_output_regs[0x1B] = m_stop_point_regs[SEEK_FRAMES];
    m_output_regs[0x1C] = m_stop_point_regs[SEEK_SECONDS_OR_FRAME_LOW];
    m_output_regs[0x1D] = m_stop_point_regs[SEEK_MINUTES_OR_FRAME_MIDDLE];
    m_output_regs[0x1E] = m_stop_point_regs[SEEK_HOURS_OR_FRAME_HIGH];
    m_output_regs[0x1F] = 1;

    if (!IsLaserDisc())
    {
        m_stop_point_aba = GetABAFromTime(0, DecodeBcd(m_stop_point_regs[SEEK_MINUTES_OR_FRAME_MIDDLE]),
            DecodeBcd(m_stop_point_regs[SEEK_SECONDS_OR_FRAME_LOW]),
            DecodeBcd(m_stop_point_regs[SEEK_FRAMES]));
    }
    else if (!IsCLVDisc())
    {
        s32 video_frame = (s32)DecodeBcd(m_stop_point_regs[SEEK_HOURS_OR_FRAME_HIGH]) * 10000 +
            (s32)DecodeBcd(m_stop_point_regs[SEEK_MINUTES_OR_FRAME_MIDDLE]) * 100 +
            DecodeBcd(m_stop_point_regs[SEEK_SECONDS_OR_FRAME_LOW]);
        m_stop_point_aba = GetABAFromVideoFrame(video_frame - 1);
    }
    else
    {
        u8 hour = DecodeBcd(m_stop_point_regs[SEEK_HOURS_OR_FRAME_HIGH]);
        u8 minute = DecodeBcd(m_stop_point_regs[SEEK_MINUTES_OR_FRAME_MIDDLE]);
        u8 second = DecodeBcd(m_stop_point_regs[SEEK_SECONDS_OR_FRAME_LOW]);
        u8 frame = DecodeBcd(m_stop_point_regs[SEEK_FRAMES]);

        VideoTimeToRedbookTime(hour, minute, second, frame);
        m_stop_point_aba = GetABAFromTime(hour, minute, second, frame);
    }

    m_stop_point_enabled = true;
    m_reached_stop_point_previously = false;

    if (m_reached_stop_point)
        HandleStopPoint(m_head_lba + 150);
}

void LaserActive::ResetSeekTarget()
{
    m_current_seek_mode = 1;
    m_current_seek_time_format = true;
    memset(m_seek_point_regs, 0, sizeof(m_seek_point_regs));
    m_active_seek_mode = SEEK_REDBOOK_RELATIVE_TIME;
}

bool LaserActive::HasLiveSeekTarget() const
{
    u8 mode = GetBits(m_input_regs[0x06], 0, 1);

    return (mode != 0) && (mode != 3);
}

bool LaserActive::LatchSeekTarget()
{
    if (!HasLiveSeekTarget())
        return false;

    u8 mode = GetBits(m_input_regs[0x06], 0, 1);
    bool time_format = GetBit(m_input_regs[0x06], 2);

    m_current_seek_mode = mode;
    m_current_seek_time_format = time_format;

    if (!m_reached_stop_point)
    {
        m_operation_error_1 = false;
        m_operation_error_2 = false;
        m_operation_error_3 = false;
    }

    if (!IsDiscLoaded() || (m_current_drive_state <= 4) || (m_target_drive_state != 5))
    {
        m_operation_error_1 = true;
        m_operation_error_2 = false;
        m_operation_error_3 = true;
        return false;
    }

    if (mode == 1)
    {
        u8 target = m_input_regs[0x07];
        u8 track = IsLaserDisc() && (target == 0) ? 1 : DecodeBcd(target);

        if ((target > 0x99) || (track < 1) || (track > GetTrackCount()))
        {
            m_operation_error_1 = true;
            m_operation_error_2 = false;
            m_operation_error_3 = true;
            m_reached_stop_point = false;
            return false;
        }

        bool allow_time = !IsLaserDisc() || (!time_format && GetTrackCount() > 0);

        m_seek_point_regs[SEEK_CHAPTER] = target;
        m_seek_point_regs[SEEK_HOURS_OR_FRAME_HIGH] = 0;
        m_seek_point_regs[SEEK_MINUTES_OR_FRAME_MIDDLE] = allow_time ? m_input_regs[0x09] : 0;
        m_seek_point_regs[SEEK_SECONDS_OR_FRAME_LOW] = allow_time ? m_input_regs[0x0A] : 0;
        m_seek_point_regs[SEEK_FRAMES] = allow_time ? m_input_regs[0x0B] : 0;
        m_active_seek_mode = SEEK_REDBOOK_RELATIVE_TIME;
    }
    else if (mode == 2)
    {
        bool redbook_time = !IsLaserDisc() || (!time_format && IsLaserDisc());
        bool video_frame = IsLaserDisc() && time_format && !IsCLVDisc();
        bool video_time = IsLaserDisc() && IsCLVDisc();

        m_seek_point_regs[SEEK_CHAPTER] = 0;

        if (redbook_time)
        {
            m_seek_point_regs[SEEK_HOURS_OR_FRAME_HIGH] = 0;
            m_seek_point_regs[SEEK_MINUTES_OR_FRAME_MIDDLE] = m_input_regs[0x09];
            m_seek_point_regs[SEEK_SECONDS_OR_FRAME_LOW] = m_input_regs[0x0A];
            m_seek_point_regs[SEEK_FRAMES] = m_input_regs[0x0B];
            m_active_seek_mode = SEEK_REDBOOK_TIME;
        }
        else if (video_frame)
        {
            m_seek_point_regs[SEEK_HOURS_OR_FRAME_HIGH] = m_input_regs[0x08];
            m_seek_point_regs[SEEK_MINUTES_OR_FRAME_MIDDLE] = m_input_regs[0x09];
            m_seek_point_regs[SEEK_SECONDS_OR_FRAME_LOW] = m_input_regs[0x0A];
            m_seek_point_regs[SEEK_FRAMES] = 0;
            m_active_seek_mode = SEEK_VIDEO_FRAME;
        }
        else if (video_time)
        {
            m_seek_point_regs[SEEK_HOURS_OR_FRAME_HIGH] = m_input_regs[0x08] & 0x0F;
            m_seek_point_regs[SEEK_MINUTES_OR_FRAME_MIDDLE] = m_input_regs[0x09];
            m_seek_point_regs[SEEK_SECONDS_OR_FRAME_LOW] = m_input_regs[0x0A];
            m_seek_point_regs[SEEK_FRAMES] = m_input_regs[0x0B];
            m_active_seek_mode = SEEK_VIDEO_TIME;
        }
    }

    return true;
}

void LaserActive::PerformLatchedSeek()
{
    m_frame_skip_base = 0;
    m_frame_skip_counter = 0;

    bool paused = m_target_pause && !m_reached_stop_point;

    switch ((SeekMode)m_active_seek_mode)
    {
        case SEEK_REDBOOK_TIME:
        {
            s32 aba = GetABAFromTime(0,
                DecodeBcd(m_seek_point_regs[SEEK_MINUTES_OR_FRAME_MIDDLE]),
                DecodeBcd(m_seek_point_regs[SEEK_SECONDS_OR_FRAME_LOW]),
                DecodeBcd(m_seek_point_regs[SEEK_FRAMES]));
            SeekToSector(aba - 150, paused);
            break;
        }

        case SEEK_REDBOOK_RELATIVE_TIME:
        {
            u8 track = DecodeBcd(m_seek_point_regs[SEEK_CHAPTER]);

            if (IsLaserDisc() && (track == 0))
                track = 1;

            SeekToRelativeTime(track,
                DecodeBcd(m_seek_point_regs[SEEK_MINUTES_OR_FRAME_MIDDLE]),
                DecodeBcd(m_seek_point_regs[SEEK_SECONDS_OR_FRAME_LOW]),
                DecodeBcd(m_seek_point_regs[SEEK_FRAMES]), paused);
            break;
        }

        case SEEK_VIDEO_FRAME:
        {
            s32 frame = (s32)DecodeBcd(m_seek_point_regs[SEEK_HOURS_OR_FRAME_HIGH]) * 10000 +
                (s32)DecodeBcd(m_seek_point_regs[SEEK_MINUTES_OR_FRAME_MIDDLE]) * 100 +
                DecodeBcd(m_seek_point_regs[SEEK_SECONDS_OR_FRAME_LOW]);
            frame = MAX(frame, 1);
            SeekToSector(GetABAFromVideoFrame(frame - 1) - 150, paused);
            break;
        }

        case SEEK_VIDEO_TIME:
        {
            u8 hour = DecodeBcd(m_seek_point_regs[SEEK_HOURS_OR_FRAME_HIGH]);
            u8 minute = DecodeBcd(m_seek_point_regs[SEEK_MINUTES_OR_FRAME_MIDDLE]);
            u8 second = DecodeBcd(m_seek_point_regs[SEEK_SECONDS_OR_FRAME_LOW]);
            u8 frame = DecodeBcd(m_seek_point_regs[SEEK_FRAMES]);

            VideoTimeToRedbookTime(hour, minute, second, frame);
            SeekToSector(GetABAFromTime(hour, minute, second, frame) - 150, paused);
            break;
        }
    }

    m_seek_frame_pending = true;
}

void LaserActive::HandleStopPoint(s32 aba)
{
    UNUSED(aba);

    if ((m_playback_mode == 2) && (m_playback_speed <= 1))
        return;

    if (GetBit(m_input_regs[0x06], 7))
    {
        m_reached_stop_point = false;
        m_drive_mode = DRIVE_PLAYING;
        PerformLatchedSeek();
    }
    else
    {
        for (u8 i = 0x1A; i <= 0x1E; i++)
            m_output_regs[i] = 0xFF;

        m_output_regs[0x1F] = 0;
        m_reached_stop_point_previously = true;
        m_operation_error_1 = false;
        m_operation_error_2 = true;
        m_operation_error_3 = true;
        m_reached_stop_point = true;
        m_drive_mode = DRIVE_PAUSED;
        m_stop_point_enabled = false;
    }
}

bool LaserActive::StartVideoDecoder()
{
    StopVideoDecoder();
    m_analog_audio_size = m_cdrom_media->GetMmiAnalogAudioSize();
    m_analog_lead_in_samples = 0;

    const GG_MmiMediaInfo* media_info = m_cdrom_media->GetSelectedMmiMedia();

    if (media_info)
    {
        for (size_t i = 0; i < media_info->streams.size(); i++)
        {
            if (media_info->streams[i].role == GG_MMI_STREAM_RAW_VIDEO)
                m_analog_lead_in_samples = media_info->streams[i].frames_in_lead_in_region *
                    (44100LL * 1001LL) / 30000LL;
        }
    }

    const GG_QonInfo* qon_info = m_cdrom_media->GetQonInfo();

    if (!qon_info || (qon_info->decoded_rgb_size == 0) || (qon_info->decoded_rgb_size > (u64)SIZE_MAX))
        return false;

    m_video_file = m_cdrom_media->OpenMmiVideoStream();

    if (!m_video_file)
        return false;

    m_video_frame.resize((size_t)qon_info->decoded_rgb_size);
    m_video_prefetch_frame.resize((size_t)qon_info->decoded_rgb_size);
    m_video_display_field.resize((size_t)qon_info->width * 263 * 3);

    u32 max_frame_size = 0;

    for (size_t i = 0; i < qon_info->frames.size(); i++)
        max_frame_size = MAX(max_frame_size, qon_info->frames[i].compressed_size);

    m_video_compressed_data.reserve(max_frame_size);
    m_video_prefetch_compressed_data.reserve(max_frame_size);
    m_video_resampling.clear();

    if (qon_info->width > 119)
    {
        m_video_resampling.resize(1176);

        u32 source_width = qon_info->width - 118;

        for (u32 x = 0; x < 1176; x++)
        {
            u64 start = ((u64)x * source_width << 16) / 1176;
            u64 end = ((u64)(x + 1) * source_width << 16) / 1176;
            VideoResampleInfo& sample = m_video_resampling[x];

            sample.first_pixel = (u32)(start >> 16);
            sample.pixel_count = (u32)((end - 1) >> 16) - sample.first_pixel + 1;
            // The validated maximum QON width (4096) spans at most five pixels.
            assert(sample.pixel_count <= 5);

            u32 remaining_weight = 65536;

            for (u32 i = 0; i < sample.pixel_count; i++)
            {
                u64 pixel_start = (u64)(sample.first_pixel + i) << 16;
                u64 overlap = MIN(end, pixel_start + 65536) - MAX(start, pixel_start);

                sample.weights[i] = (i + 1 == sample.pixel_count) ? remaining_weight :
                    (u32)((overlap * 65536) / (end - start));
                remaining_weight -= sample.weights[i];
            }
        }
    }

    m_video_generation = m_cdrom_media->GetMediaGeneration();
    m_video_frame_valid = false;

#if !defined(GG_DISABLE_MMI_THREADS)
    m_video_prefetch_file = m_cdrom_media->OpenMmiVideoStream();

    if (m_video_prefetch_file)
    {
        m_video_thread_stop = false;
        m_video_job_pending = false;
        m_video_result_ready = false;
        m_video_thread = std::thread(&LaserActive::VideoThread, this);
    }
#endif

    return true;
}

void LaserActive::StopVideoDecoder()
{
#if !defined(GG_DISABLE_MMI_THREADS)
    if (m_video_thread.joinable())
    {
        {
            std::lock_guard<std::mutex> lock(m_video_mutex);

            m_video_thread_stop = true;
            m_video_job_pending = false;
        }

        m_video_condition.notify_all();
        m_video_thread.join();
    }

    m_video_thread_stop = false;
    m_video_job_pending = false;
    m_video_result_ready = false;
#endif

    SafeDelete(m_video_file);
    SafeDelete(m_video_prefetch_file);
    m_video_frame_valid = false;
    m_video_display_valid = false;
}

u32 LaserActive::GetAbsoluteVideoFrame(s32 frame, bool lead_in, bool lead_out) const
{
    const GG_MmiMediaInfo* media_info = m_cdrom_media->GetSelectedMmiMedia();

    if (!media_info || (frame < 0))
        return UINT32_MAX;

    const GG_MmiStreamInfo* video_stream = NULL;

    for (size_t i = 0; i < media_info->streams.size(); i++)
    {
        if (media_info->streams[i].role == GG_MMI_STREAM_RAW_VIDEO)
        {
            video_stream = &media_info->streams[i];
            break;
        }
    }

    if (!video_stream)
        return UINT32_MAX;

    u64 frame_index = (u64)frame;

    if (!lead_in)
        frame_index += (u64)video_stream->frames_in_lead_in_region;

    if (lead_out)
        frame_index += (u64)video_stream->frames_in_active_region;

    const GG_QonInfo* qon_info = m_cdrom_media->GetQonInfo();

    if (lead_out && qon_info && !qon_info->frames.empty() && frame_index >= qon_info->frames.size())
        frame_index = qon_info->frames.size() - 1;

    return (frame_index <= UINT32_MAX) ? (u32)frame_index : UINT32_MAX;
}

bool LaserActive::DecodeVideoFrame(MediaFile* file, u32 frame, std::vector<u8>& rgb_data,
    std::vector<u8>& compressed_data)
{
    const GG_QonInfo* qon_info = m_cdrom_media->GetQonInfo();

    if (!file || !qon_info || (frame >= qon_info->frames.size()) || (qon_info->decoded_rgb_size > (u64)SIZE_MAX))
    {
        return false;
    }

    const GG_QonFrameInfo& frame_info = qon_info->frames[frame];

    compressed_data.resize(frame_info.compressed_size);

    if (compressed_data.empty() || !file->ReadAt(frame_info.record_offset + 4,
        &compressed_data[0], compressed_data.size()))
    {
        return false;
    }

    rgb_data.resize((size_t)qon_info->decoded_rgb_size);

    qoi2_desc image_desc;

    image_desc.width = qon_info->width;
    image_desc.height = qon_info->height;
    image_desc.channels = qon_info->channels;
    image_desc.colorspace = qon_info->colorspace;

    return qoi2_decode_data(&compressed_data[0], compressed_data.size(), &image_desc, NULL, &rgb_data[0], 3) != 0;
}

bool LaserActive::LoadCurrentVideoFrame()
{
    u32 frame = GetAbsoluteVideoFrame(m_current_video_frame, m_current_video_lead_in, m_current_video_lead_out);
    const GG_QonInfo* qon_info = m_cdrom_media->GetQonInfo();

    if (!qon_info || (frame >= qon_info->frames.size()))
        return false;

#if !defined(GG_DISABLE_MMI_THREADS)
    {
        std::lock_guard<std::mutex> lock(m_video_mutex);

        if (m_video_result_ready && (m_video_result_generation == m_cdrom_media->GetMediaGeneration()) &&
            (m_video_result_frame == frame))
        {
            m_video_frame.swap(m_video_prefetch_frame);
            m_video_result_ready = false;
            m_video_frame_index = frame;
            m_video_generation = m_cdrom_media->GetMediaGeneration();
            m_video_frame_valid = true;
            m_video_new_frame = true;
            return true;
        }

        m_video_result_ready = false;
    }
#endif

    m_video_frame_valid = DecodeVideoFrame(m_video_file, frame, m_video_frame, m_video_compressed_data);

    if (m_video_frame_valid)
    {
        m_video_frame_index = frame;
        m_video_generation = m_cdrom_media->GetMediaGeneration();
        m_video_new_frame = true;
    }

    return m_video_frame_valid;
}

u32 LaserActive::PredictNextVideoFrame() const
{
    const GG_QonInfo* qon_info = m_cdrom_media->GetQonInfo();

    if (!qon_info || qon_info->frames.empty())
        return UINT32_MAX;

    s32 frame_advance = m_playback_reverse ? -1 : 1;

    if (m_playback_mode == 3)
    {
        static const s32 frame_advances[8] = { 1, 2, 3, 8, 14, 20, 1, 1 };

        frame_advance = frame_advances[m_playback_speed & 7] * (m_playback_reverse ? -1 : 1);
    }

    s64 next_frame = (s64)m_video_frame_index + frame_advance;

    if (next_frame < 0)
        next_frame = 0;

    if (next_frame >= (s64)qon_info->frames.size())
        next_frame = (s64)qon_info->frames.size() - 1;

    return (u32)next_frame;
}

u32 LaserActive::DecodeBiphaseCode(u32 line) const
{
    const GG_QonInfo* qon_info = m_cdrom_media->GetQonInfo();
    const u8* frame = GetVideoFrameBuffer();

    if (!qon_info || !frame || (line >= qon_info->height) || (qon_info->width < 64))
        return 0;

    double lines_per_second = k_laseractive_video_fps * 525.0;
    double microseconds_per_line = 1000000.0 / lines_per_second;
    double cell_length = (double)qon_info->width * (2.0 / microseconds_per_line);
    size_t quarter_cell = (size_t)(cell_length / 4.0);

    if (quarter_cell == 0)
        quarter_cell = 1;

    size_t search_start = (size_t)((double)qon_info->width * 0.15);
    size_t search_end = (size_t)((double)qon_info->width * 0.25);
    size_t line_offset = (size_t)line * qon_info->width * 3;
    size_t start = search_start;
    bool found = false;

    while (!found && (start < search_end))
    {
        if ((start + quarter_cell) >= qon_info->width)
            break;

        found = true;

        for (size_t i = 0; i < quarter_cell; i++)
        {
            size_t position = line_offset + (start + i) * 3;

            if ((frame[position + 0] < 0x80) || (frame[position + 1] < 0x80) || (frame[position + 2] < 0x80))
            {
                found = false;
                break;
            }
        }

        if (!found)
            start++;
    }

    if (!found)
        return 0;

    size_t half_cell = (size_t)(cell_length / 2.0);

    if (start < half_cell)
        return 0;

    start -= half_cell;

    u32 code = 0;

    for (u32 bit = 0; bit < 24; bit++)
    {
        size_t leading = start + (size_t)(cell_length * bit + cell_length / 4.0);
        size_t trailing = start + (size_t)(cell_length * bit + (cell_length * 3.0) / 4.0);

        if ((leading >= qon_info->width) || (trailing >= qon_info->width))
            return 0;

        size_t leading_position = line_offset + leading * 3;
        size_t trailing_position = line_offset + trailing * 3;
        bool leading_high = (frame[leading_position + 0] >= 0xC0) &&
            (frame[leading_position + 1] >= 0xC0) && (frame[leading_position + 2] >= 0xC0);
        bool trailing_high = (frame[trailing_position + 0] >= 0xC0) &&
            (frame[trailing_position + 1] >= 0xC0) && (frame[trailing_position + 2] >= 0xC0);

        if (leading_high == trailing_high)
            return 0;

        code = (code << 1) | (trailing_high ? 1U : 0U);
    }

    return code;
}

#if !defined(GG_DISABLE_MMI_THREADS)
void LaserActive::QueueVideoPrefetch(u32 frame)
{
    const GG_QonInfo* qon_info = m_cdrom_media->GetQonInfo();

    if (!m_video_thread.joinable() || !qon_info || (frame >= qon_info->frames.size()) || (frame == m_video_frame_index))
    {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_video_mutex);

        m_video_job_frame = frame;
        m_video_job_generation = m_cdrom_media->GetMediaGeneration();
        m_video_job_pending = true;
        m_video_result_ready = false;
    }

    m_video_condition.notify_one();
}

void LaserActive::VideoThread()
{
    while (true)
    {
        u32 frame;
        u32 generation;

        {
            std::unique_lock<std::mutex> lock(m_video_mutex);

            while (!m_video_thread_stop && !m_video_job_pending)
                m_video_condition.wait(lock);

            if (m_video_thread_stop)
                break;

            frame = m_video_job_frame;
            generation = m_video_job_generation;
            m_video_job_pending = false;
            m_video_result_ready = false;
        }

        bool decoded = DecodeVideoFrame(m_video_prefetch_file, frame,
            m_video_prefetch_frame, m_video_prefetch_compressed_data);

        {
            std::lock_guard<std::mutex> lock(m_video_mutex);

            if (!m_video_thread_stop && decoded &&
                (generation == m_cdrom_media->GetMediaGeneration()) &&
                (!m_video_job_pending || ((m_video_job_frame == frame) &&
                (m_video_job_generation == generation))))
            {
                m_video_result_frame = frame;
                m_video_result_generation = generation;
                m_video_result_ready = true;
            }
        }
    }
}
#endif

const u8* LaserActive::GetVideoFrameBuffer() const
{
    return m_video_frame_valid && !m_video_frame.empty() ? &m_video_frame[0] : NULL;
}

u32 LaserActive::GetVideoWidth() const
{
    const GG_QonInfo* qon_info = m_cdrom_media->GetQonInfo();

    return qon_info ? qon_info->width : 0;
}

u32 LaserActive::GetVideoHeight() const
{
    const GG_QonInfo* qon_info = m_cdrom_media->GetQonInfo();

    return qon_info ? qon_info->height : 0;
}

bool LaserActive::HasVideoFrame() const
{
    return GetVideoFrameBuffer() != NULL;
}

bool LaserActive::IsActive() const
{
    return m_cdrom_media->IsLaserDisc();
}

void LaserActive::BeginVideoFrame()
{
    bool interlaced = !GetBit(m_input_regs[0x0C], 5) && !GetBit(m_input_regs[0x0C], 3) &&
        !GetBit(m_input_regs[0x0C], 1);

    if (m_video_new_frame)
    {
        m_video_even_field = false;
        m_video_new_frame = false;
    }
    else if (interlaced)
        m_video_even_field = !m_video_even_field;

    if (m_video_frame_valid)
    {
        bool even_field = m_video_field_selected ? m_video_selected_even : m_video_even_field;

        if (!m_video_display_valid || m_video_display_frame_index != m_video_frame_index ||
            m_video_display_even != even_field)
            CopyDisplayField(&m_video_frame[0], even_field);

        m_video_display_frame_index = m_video_frame_index;
        m_video_display_even = even_field;
        m_video_display_valid = true;
    }
}

void LaserActive::CopyDisplayField(const u8* frame, bool even_field)
{
    const GG_QonInfo* qon_info = m_cdrom_media->GetQonInfo();

    if (!frame || !qon_info || qon_info->height == 0)
        return;

    size_t line_size = (size_t)qon_info->width * 3;
    u32 first_line = MIN(even_field ? 263U : 0U, qon_info->height - 1);
    u32 line_count = MIN(263U, qon_info->height - first_line);

    memcpy(&m_video_display_field[0], frame + first_line * line_size, line_count * line_size);

    for (u32 line = line_count; line < 263; line++)
        memcpy(&m_video_display_field[line * line_size], frame + (qon_info->height - 1) * line_size, line_size);
}

void LaserActive::CaptureVideoLineState(u8* state) const
{
    if (!IsValidPointer(state))
        return;

    state[0] = m_input_regs[0x01];
    state[1] = m_input_regs[0x0C];
    state[2] = m_input_regs[0x19];
    state[3] = m_input_regs[0x1A];
    state[4] = m_input_regs[0x1B];
    state[5] = m_input_regs[0x1C];
    state[6] = m_input_regs[0x1D];
    state[7] = (m_current_pause ? 1 : 0) | (m_video_field_selected ? 2 : 0) |
        (m_video_selected_even ? 4 : 0) | (m_video_even_field ? 8 : 0);
}

void LaserActive::ComposeLine(u32 line, const u8* pce_pixels, const u8* classifications,
    const u8* line_state, u8* output, GG_Pixel_Format pixel_format) const
{
    static const u32 output_width = 1176;

    if (!IsValidPointer(pce_pixels) || !IsValidPointer(classifications) ||
        !IsValidPointer(line_state) || !IsValidPointer(output))
        return;

    u32 bytes_per_pixel = (pixel_format == GG_PIXEL_RGB565) ? 2 : 4;
    u8 mixing_mode = GetBits(line_state[0], 6, 7);

    if (mixing_mode == 0 && pixel_format == GG_PIXEL_RGBA8888)
    {
        memcpy(output, pce_pixels, output_width * bytes_per_pixel);
        return;
    }
    // TODO: Implement input register 19 transparency.
    const u8* analog = m_video_display_valid ? &m_video_display_field[0] : NULL;
    u32 analog_width = GetVideoWidth();
    bool video_blanked = GetBit(line_state[1], 2) || (GetBit(line_state[7], 0) && !GetBit(line_state[1], 5));

    u64 source_line_offset = (u64)MIN(line, 262U) * analog_width * 3;

    for (u32 x = 0; x < output_width; x++)
    {
        u8 pce_red = pce_pixels[x * 4 + 0];
        u8 pce_green = pce_pixels[x * 4 + 1];
        u8 pce_blue = pce_pixels[x * 4 + 2];

        u8 final_red = pce_red;
        u8 final_green = pce_green;
        u8 final_blue = pce_blue;

        if (mixing_mode != 0)
        {
            u8 analog_red = 0;
            u8 analog_green = 0;
            u8 analog_blue = 0;

            if (analog && !video_blanked && !m_video_resampling.empty())
            {
                const VideoResampleInfo& sample = m_video_resampling[x];
                const u8* pixel = analog + source_line_offset + (118 + sample.first_pixel) * 3;
                u32 red_sum = 0;
                u32 green_sum = 0;
                u32 blue_sum = 0;

                for (u32 i = 0; i < sample.pixel_count; i++, pixel += 3)
                {
                    red_sum += pixel[0] * sample.weights[i];
                    green_sum += pixel[1] * sample.weights[i];
                    blue_sum += pixel[2] * sample.weights[i];
                }

                analog_red = (u8)((red_sum + 32768) >> 16);
                analog_green = (u8)((green_sum + 32768) >> 16);
                analog_blue = (u8)((blue_sum + 32768) >> 16);
            }

            u8 source = classifications[x] & 3;
            u32 fader = GetBits(line_state[3 + source], 2, 7);
            u32 normalized_fader = (fader << 10) | (fader << 4) | (fader >> 2);
            u32 inverse_fader = (1U << 16) - normalized_fader;

            u32 pce_r = ((u32)pce_red << 8) | pce_red;
            u32 pce_g = ((u32)pce_green << 8) | pce_green;
            u32 pce_b = ((u32)pce_blue << 8) | pce_blue;
            u32 analog_r = ((u32)analog_red << 8) | analog_red;
            u32 analog_g = ((u32)analog_green << 8) | analog_green;
            u32 analog_b = ((u32)analog_blue << 8) | analog_blue;
            u32 result_r = (u32)(((u64)pce_r * normalized_fader) >> 16) + (u32)(((u64)analog_r * inverse_fader) >> 16);
            u32 result_g = (u32)(((u64)pce_g * normalized_fader) >> 16) + (u32)(((u64)analog_g * inverse_fader) >> 16);
            u32 result_b = (u32)(((u64)pce_b * normalized_fader) >> 16) + (u32)(((u64)analog_b * inverse_fader) >> 16);

            final_red = (u8)(((result_r + 0x80) - ((result_r + 0x80) >> 8)) >> 8);
            final_green = (u8)(((result_g + 0x80) - ((result_g + 0x80) >> 8)) >> 8);
            final_blue = (u8)(((result_b + 0x80) - ((result_b + 0x80) >> 8)) >> 8);
        }

        if (pixel_format == GG_PIXEL_RGB565)
        {
            u16 packed = (u16)((((final_red * 31 + 127) / 255) << 11) |
                (((final_green * 63 + 127) / 255) << 5) | ((final_blue * 31 + 127) / 255));
            reinterpret_cast<u16*> (output)[x] = packed;
        }
        else
        {
            output[x * bytes_per_pixel + 0] = final_red;
            output[x * bytes_per_pixel + 1] = final_green;
            output[x * bytes_per_pixel + 2] = final_blue;
            output[x * bytes_per_pixel + 3] = 0xFF;
        }
    }
}

void LaserActive::GetStatus(Status& status)
{
    for (u8 i = 0; i < 0x20; i++)
    {
        status.input_registers[i] = PeekRegister(i, false);
        status.live_input_registers[i] = m_input_regs[i];
        status.output_registers[i] = PeekRegister(i, true);
    }

    status.drive_mode = m_drive_mode;
    status.head_lba = m_head_lba;
    status.seek_latency = m_seek_latency;
    status.current_track = m_current_track;
    status.current_drive_state = m_current_drive_state;
    status.paused = m_drive_mode == DRIVE_PAUSED;
    status.sram_enabled = m_sram_enabled;
    status.ejected = m_cdrom_media->IsMmiEjected();
    status.input_frozen = m_input_frozen;
    status.output_frozen = m_output_frozen;
    status.sample = m_current_sample;
    status.audio_end_lba = m_audio_end_lba;
    status.playback_mode = m_playback_mode;
    status.digital_sector_valid = m_digital_sector_valid;
    status.audio_end_pending = m_audio_end_pending;
    status.video_frame = m_current_video_frame;
    status.search_sectors = m_search_sectors;
    status.analog_fade_samples_left = m_analog_fade_samples_left;
    status.analog_fade_samples_right = m_analog_fade_samples_right;
    status.analog_muted_left = m_analog_fade_muted_left;
    status.analog_muted_right = m_analog_fade_muted_right;
    status.analog_attenuation_left = m_analog_attenuation_left;
    status.analog_attenuation_right = m_analog_attenuation_right;
}

u8 LaserActive::GetVideoMixingMode() const
{
    return GetBits(m_input_regs[0x01], 6, 7);
}

u8 LaserActive::GetVideoControl() const
{
    return m_input_regs[0x0C];
}

u8 LaserActive::GetGraphicsFader(u8 source) const
{
    if (source > 3)
        source = 3;

    return GetBits(m_input_regs[0x1A + source], 2, 7);
}

s32 LaserActive::GetHeadLba() const
{
    return m_head_lba;
}

s32 LaserActive::GetCurrentVideoFrame() const
{
    return m_current_video_frame;
}

void LaserActive::SaveState(std::ostream& stream) const
{
    stream.write(reinterpret_cast<const char*> (&k_laseractive_state_magic), sizeof(k_laseractive_state_magic));

    const GG_MmiMediaInfo* media_info = m_cdrom_media->GetSelectedMmiMedia();
    s64 selected_sequence = media_info ? media_info->sequence_number : -1;
    bool media_ejected = m_cdrom_media->IsMmiEjected();

    stream.write(reinterpret_cast<const char*> (&selected_sequence), sizeof(selected_sequence));
    stream.write(reinterpret_cast<const char*> (&media_ejected), sizeof(media_ejected));
    stream.write(reinterpret_cast<const char*> (m_input_regs), sizeof(m_input_regs));
    stream.write(reinterpret_cast<const char*> (m_input_frozen_regs), sizeof(m_input_frozen_regs));
    stream.write(reinterpret_cast<const char*> (m_output_regs), sizeof(m_output_regs));
    stream.write(reinterpret_cast<const char*> (m_output_frozen_regs), sizeof(m_output_frozen_regs));
    stream.write(reinterpret_cast<const char*> (m_output_written_data), sizeof(m_output_written_data));
    stream.write(reinterpret_cast<const char*> (m_output_cooldown), sizeof(m_output_cooldown));
    stream.write(reinterpret_cast<const char*> (&m_input_frozen), sizeof(m_input_frozen));
    stream.write(reinterpret_cast<const char*> (&m_output_frozen), sizeof(m_output_frozen));
    stream.write(reinterpret_cast<const char*> (&m_operation_error_1), sizeof(m_operation_error_1));
    stream.write(reinterpret_cast<const char*> (&m_operation_error_2), sizeof(m_operation_error_2));
    stream.write(reinterpret_cast<const char*> (&m_operation_error_3), sizeof(m_operation_error_3));
    stream.write(reinterpret_cast<const char*> (&m_seek_enabled), sizeof(m_seek_enabled));
    stream.write(reinterpret_cast<const char*> (&m_current_seek_mode), sizeof(m_current_seek_mode));
    stream.write(reinterpret_cast<const char*> (&m_current_seek_time_format), sizeof(m_current_seek_time_format));
    stream.write(reinterpret_cast<const char*> (&m_current_seek_repeat), sizeof(m_current_seek_repeat));
    stream.write(reinterpret_cast<const char*> (&m_analog_attenuation_left), sizeof(m_analog_attenuation_left));
    stream.write(reinterpret_cast<const char*> (&m_analog_attenuation_right), sizeof(m_analog_attenuation_right));
    stream.write(reinterpret_cast<const char*> (&m_analog_fade_muted_left), sizeof(m_analog_fade_muted_left));
    stream.write(reinterpret_cast<const char*> (&m_analog_fade_muted_right), sizeof(m_analog_fade_muted_right));
    stream.write(reinterpret_cast<const char*> (&m_active_seek_mode), sizeof(m_active_seek_mode));
    stream.write(reinterpret_cast<const char*> (m_seek_point_regs), sizeof(m_seek_point_regs));
    stream.write(reinterpret_cast<const char*> (m_stop_point_regs), sizeof(m_stop_point_regs));
    stream.write(reinterpret_cast<const char*> (&m_reached_stop_point), sizeof(m_reached_stop_point));
    stream.write(reinterpret_cast<const char*> (&m_reached_stop_point_previously),
        sizeof(m_reached_stop_point_previously));
    stream.write(reinterpret_cast<const char*> (&m_playback_mode), sizeof(m_playback_mode));
    stream.write(reinterpret_cast<const char*> (&m_playback_speed), sizeof(m_playback_speed));
    stream.write(reinterpret_cast<const char*> (&m_playback_reverse), sizeof(m_playback_reverse));
    stream.write(reinterpret_cast<const char*> (&m_target_drive_state), sizeof(m_target_drive_state));
    stream.write(reinterpret_cast<const char*> (&m_current_drive_state), sizeof(m_current_drive_state));
    stream.write(reinterpret_cast<const char*> (&m_target_pause), sizeof(m_target_pause));
    stream.write(reinterpret_cast<const char*> (&m_current_pause), sizeof(m_current_pause));
    stream.write(reinterpret_cast<const char*> (&m_seek_frame_pending), sizeof(m_seek_frame_pending));
    stream.write(reinterpret_cast<const char*> (&m_drive_state_delay), sizeof(m_drive_state_delay));
    stream.write(reinterpret_cast<const char*> (&m_selected_track_info), sizeof(m_selected_track_info));
    stream.write(reinterpret_cast<const char*> (&m_drive_mode), sizeof(m_drive_mode));
    stream.write(reinterpret_cast<const char*> (&m_seek_drive_mode), sizeof(m_seek_drive_mode));
    stream.write(reinterpret_cast<const char*> (&m_head_lba), sizeof(m_head_lba));
    stream.write(reinterpret_cast<const char*> (&m_seek_target_lba), sizeof(m_seek_target_lba));
    stream.write(reinterpret_cast<const char*> (&m_audio_end_lba), sizeof(m_audio_end_lba));
    stream.write(reinterpret_cast<const char*> (&m_seek_latency), sizeof(m_seek_latency));
    stream.write(reinterpret_cast<const char*> (&m_sector_repeat_count), sizeof(m_sector_repeat_count));
    stream.write(reinterpret_cast<const char*> (&m_stop_point_enabled), sizeof(m_stop_point_enabled));
    stream.write(reinterpret_cast<const char*> (&m_stop_point_aba), sizeof(m_stop_point_aba));
    stream.write(reinterpret_cast<const char*> (&m_current_track), sizeof(m_current_track));
    stream.write(reinterpret_cast<const char*> (m_digital_sector), sizeof(m_digital_sector));
    stream.write(reinterpret_cast<const char*> (&m_digital_sector_valid), sizeof(m_digital_sector_valid));
    stream.write(reinterpret_cast<const char*> (&m_current_sample), sizeof(m_current_sample));
    stream.write(reinterpret_cast<const char*> (&m_sector_clock), sizeof(m_sector_clock));
    stream.write(reinterpret_cast<const char*> (&m_sram_shift), sizeof(m_sram_shift));
    stream.write(reinterpret_cast<const char*> (&m_sram_enabled), sizeof(m_sram_enabled));
    stream.write(reinterpret_cast<const char*> (&m_current_video_frame), sizeof(m_current_video_frame));
    stream.write(reinterpret_cast<const char*> (&m_current_video_lead_in), sizeof(m_current_video_lead_in));
    stream.write(reinterpret_cast<const char*> (&m_current_video_lead_out), sizeof(m_current_video_lead_out));
    stream.write(reinterpret_cast<const char*> (&m_frame_skip_base), sizeof(m_frame_skip_base));
    stream.write(reinterpret_cast<const char*> (&m_frame_skip_counter), sizeof(m_frame_skip_counter));
    stream.write(reinterpret_cast<const char*> (&m_video_even_field), sizeof(m_video_even_field));
    stream.write(reinterpret_cast<const char*> (&m_video_new_frame), sizeof(m_video_new_frame));
    stream.write(reinterpret_cast<const char*> (&m_video_memory_latched), sizeof(m_video_memory_latched));
    stream.write(reinterpret_cast<const char*> (&m_video_field_selected), sizeof(m_video_field_selected));
    stream.write(reinterpret_cast<const char*> (&m_video_selected_even), sizeof(m_video_selected_even));
    stream.write(reinterpret_cast<const char*> (&m_video_display_frame_index), sizeof(m_video_display_frame_index));
    stream.write(reinterpret_cast<const char*> (&m_video_display_valid), sizeof(m_video_display_valid));
    stream.write(reinterpret_cast<const char*> (&m_video_display_even), sizeof(m_video_display_even));
    stream.write(reinterpret_cast<const char*> (&m_search_sectors), sizeof(m_search_sectors));
    stream.write(reinterpret_cast<const char*> (&m_analog_fade_samples_left), sizeof(m_analog_fade_samples_left));
    stream.write(reinterpret_cast<const char*> (&m_analog_fade_samples_right), sizeof(m_analog_fade_samples_right));
    stream.write(reinterpret_cast<const char*> (&m_audio_end_pending), sizeof(m_audio_end_pending));
}

void LaserActive::LoadState(std::istream& stream)
{
    StopVideoDecoder();

    u32 state_magic = 0;
    s64 selected_sequence = -1;
    bool media_ejected = false;

    stream.read(reinterpret_cast<char*> (&state_magic), sizeof(state_magic));
    stream.read(reinterpret_cast<char*> (&selected_sequence), sizeof(selected_sequence));
    stream.read(reinterpret_cast<char*> (&media_ejected), sizeof(media_ejected));

    if (stream.fail() || (state_magic != k_laseractive_state_magic))
    {
        stream.setstate(std::ios::failbit);
        return;
    }

    const GG_MmiInfo* mmi_info = m_cdrom_media->GetMmiInfo();

    if (!mmi_info)
    {
        stream.setstate(std::ios::failbit);
        return;
    }

    u32 selected_media_index = UINT32_MAX;

    for (size_t i = 0; i < mmi_info->media.size(); i++)
    {
        if (mmi_info->media[i].sequence_number == selected_sequence)
        {
            selected_media_index = (u32)i;
            break;
        }
    }

    if (selected_media_index == UINT32_MAX)
    {
        stream.setstate(std::ios::failbit);
        return;
    }

    if (selected_media_index != m_cdrom_media->GetSelectedMmiMediaIndex())
    {
        if (!m_cdrom_media->IsMmiEjected())
            m_cdrom_media->EjectMmi();

        if (!m_cdrom_media->SelectMmiMedia(selected_media_index))
        {
            stream.setstate(std::ios::failbit);
            return;
        }
    }

    if (media_ejected)
        m_cdrom_media->EjectMmi();
    else
        m_cdrom_media->InsertMmi();

    stream.read(reinterpret_cast<char*> (m_input_regs), sizeof(m_input_regs));
    stream.read(reinterpret_cast<char*> (m_input_frozen_regs), sizeof(m_input_frozen_regs));
    stream.read(reinterpret_cast<char*> (m_output_regs), sizeof(m_output_regs));
    stream.read(reinterpret_cast<char*> (m_output_frozen_regs), sizeof(m_output_frozen_regs));
    stream.read(reinterpret_cast<char*> (m_output_written_data), sizeof(m_output_written_data));
    stream.read(reinterpret_cast<char*> (m_output_cooldown), sizeof(m_output_cooldown));
    stream.read(reinterpret_cast<char*> (&m_input_frozen), sizeof(m_input_frozen));
    stream.read(reinterpret_cast<char*> (&m_output_frozen), sizeof(m_output_frozen));
    stream.read(reinterpret_cast<char*> (&m_operation_error_1), sizeof(m_operation_error_1));
    stream.read(reinterpret_cast<char*> (&m_operation_error_2), sizeof(m_operation_error_2));
    stream.read(reinterpret_cast<char*> (&m_operation_error_3), sizeof(m_operation_error_3));
    stream.read(reinterpret_cast<char*> (&m_seek_enabled), sizeof(m_seek_enabled));
    stream.read(reinterpret_cast<char*> (&m_current_seek_mode), sizeof(m_current_seek_mode));
    stream.read(reinterpret_cast<char*> (&m_current_seek_time_format), sizeof(m_current_seek_time_format));
    stream.read(reinterpret_cast<char*> (&m_current_seek_repeat), sizeof(m_current_seek_repeat));
    stream.read(reinterpret_cast<char*> (&m_analog_attenuation_left), sizeof(m_analog_attenuation_left));
    stream.read(reinterpret_cast<char*> (&m_analog_attenuation_right), sizeof(m_analog_attenuation_right));
    stream.read(reinterpret_cast<char*> (&m_analog_fade_muted_left), sizeof(m_analog_fade_muted_left));
    stream.read(reinterpret_cast<char*> (&m_analog_fade_muted_right), sizeof(m_analog_fade_muted_right));
    stream.read(reinterpret_cast<char*> (&m_active_seek_mode), sizeof(m_active_seek_mode));
    stream.read(reinterpret_cast<char*> (m_seek_point_regs), sizeof(m_seek_point_regs));
    stream.read(reinterpret_cast<char*> (m_stop_point_regs), sizeof(m_stop_point_regs));
    stream.read(reinterpret_cast<char*> (&m_reached_stop_point), sizeof(m_reached_stop_point));
    stream.read(reinterpret_cast<char*> (&m_reached_stop_point_previously), sizeof(m_reached_stop_point_previously));
    stream.read(reinterpret_cast<char*> (&m_playback_mode), sizeof(m_playback_mode));
    stream.read(reinterpret_cast<char*> (&m_playback_speed), sizeof(m_playback_speed));
    stream.read(reinterpret_cast<char*> (&m_playback_reverse), sizeof(m_playback_reverse));
    stream.read(reinterpret_cast<char*> (&m_target_drive_state), sizeof(m_target_drive_state));
    stream.read(reinterpret_cast<char*> (&m_current_drive_state), sizeof(m_current_drive_state));
    stream.read(reinterpret_cast<char*> (&m_target_pause), sizeof(m_target_pause));
    stream.read(reinterpret_cast<char*> (&m_current_pause), sizeof(m_current_pause));
    stream.read(reinterpret_cast<char*> (&m_seek_frame_pending), sizeof(m_seek_frame_pending));
    stream.read(reinterpret_cast<char*> (&m_drive_state_delay), sizeof(m_drive_state_delay));
    stream.read(reinterpret_cast<char*> (&m_selected_track_info), sizeof(m_selected_track_info));
    stream.read(reinterpret_cast<char*> (&m_drive_mode), sizeof(m_drive_mode));
    stream.read(reinterpret_cast<char*> (&m_seek_drive_mode), sizeof(m_seek_drive_mode));
    stream.read(reinterpret_cast<char*> (&m_head_lba), sizeof(m_head_lba));
    stream.read(reinterpret_cast<char*> (&m_seek_target_lba), sizeof(m_seek_target_lba));
    stream.read(reinterpret_cast<char*> (&m_audio_end_lba), sizeof(m_audio_end_lba));
    stream.read(reinterpret_cast<char*> (&m_seek_latency), sizeof(m_seek_latency));
    stream.read(reinterpret_cast<char*> (&m_sector_repeat_count), sizeof(m_sector_repeat_count));
    stream.read(reinterpret_cast<char*> (&m_stop_point_enabled), sizeof(m_stop_point_enabled));
    stream.read(reinterpret_cast<char*> (&m_stop_point_aba), sizeof(m_stop_point_aba));
    stream.read(reinterpret_cast<char*> (&m_current_track), sizeof(m_current_track));
    stream.read(reinterpret_cast<char*> (m_digital_sector), sizeof(m_digital_sector));
    stream.read(reinterpret_cast<char*> (&m_digital_sector_valid), sizeof(m_digital_sector_valid));
    stream.read(reinterpret_cast<char*> (&m_current_sample), sizeof(m_current_sample));
    stream.read(reinterpret_cast<char*> (&m_sector_clock), sizeof(m_sector_clock));
    stream.read(reinterpret_cast<char*> (&m_sram_shift), sizeof(m_sram_shift));
    stream.read(reinterpret_cast<char*> (&m_sram_enabled), sizeof(m_sram_enabled));
    stream.read(reinterpret_cast<char*> (&m_current_video_frame), sizeof(m_current_video_frame));
    stream.read(reinterpret_cast<char*> (&m_current_video_lead_in), sizeof(m_current_video_lead_in));
    stream.read(reinterpret_cast<char*> (&m_current_video_lead_out), sizeof(m_current_video_lead_out));
    stream.read(reinterpret_cast<char*> (&m_frame_skip_base), sizeof(m_frame_skip_base));
    stream.read(reinterpret_cast<char*> (&m_frame_skip_counter), sizeof(m_frame_skip_counter));
    stream.read(reinterpret_cast<char*> (&m_video_even_field), sizeof(m_video_even_field));
    stream.read(reinterpret_cast<char*> (&m_video_new_frame), sizeof(m_video_new_frame));
    stream.read(reinterpret_cast<char*> (&m_video_memory_latched), sizeof(m_video_memory_latched));
    stream.read(reinterpret_cast<char*> (&m_video_field_selected), sizeof(m_video_field_selected));
    stream.read(reinterpret_cast<char*> (&m_video_selected_even), sizeof(m_video_selected_even));

    u32 display_frame_index = 0;
    bool display_valid = false;
    bool display_even = false;

    stream.read(reinterpret_cast<char*> (&display_frame_index), sizeof(display_frame_index));
    stream.read(reinterpret_cast<char*> (&display_valid), sizeof(display_valid));
    stream.read(reinterpret_cast<char*> (&display_even), sizeof(display_even));
    stream.read(reinterpret_cast<char*> (&m_search_sectors), sizeof(m_search_sectors));
    stream.read(reinterpret_cast<char*> (&m_analog_fade_samples_left), sizeof(m_analog_fade_samples_left));
    stream.read(reinterpret_cast<char*> (&m_analog_fade_samples_right), sizeof(m_analog_fade_samples_right));
    stream.read(reinterpret_cast<char*> (&m_audio_end_pending), sizeof(m_audio_end_pending));

    if (stream.fail())
        return;

    if ((m_drive_mode < DRIVE_INACTIVE) || (m_drive_mode > DRIVE_STOPPED))
        m_drive_mode = DRIVE_INACTIVE;

    if ((m_seek_drive_mode < DRIVE_INACTIVE) || (m_seek_drive_mode > DRIVE_STOPPED))
        m_seek_drive_mode = DRIVE_INACTIVE;

    m_current_seek_mode &= 3;

    if (m_active_seek_mode > SEEK_VIDEO_TIME)
        m_active_seek_mode = SEEK_REDBOOK_TIME;

    m_playback_mode &= 3;
    m_playback_speed &= 7;

    if (m_target_drive_state > 7)
        m_target_drive_state = 2;

    if (m_current_drive_state > 7)
        m_current_drive_state = 2;

    m_seek_latency = MIN(m_seek_latency, 100000U);
    m_sector_repeat_count = CLAMP(m_sector_repeat_count, 0, 90);
    m_search_sectors = MIN(m_search_sectors, k_laseractive_search_sectors - 1);
    m_analog_fade_samples_left = MIN(m_analog_fade_samples_left, k_laseractive_fade_samples);
    m_analog_fade_samples_right = MIN(m_analog_fade_samples_right, k_laseractive_fade_samples);

    for (u32 i = 0; i < 0x20; i++)
        m_output_cooldown[i] = MIN(m_output_cooldown[i], (u8)5);

    m_current_track = CLAMP(m_current_track, 0, GetTrackCount());
    m_head_lba = CLAMP(m_head_lba, INT32_MIN + 150, INT32_MAX - 150);
    m_seek_target_lba = CLAMP(m_seek_target_lba, INT32_MIN + 150, INT32_MAX - 150);
    m_current_sample = MIN(m_current_sample, 588U);
    m_sector_clock %= GG_MASTER_CLOCK_RATE;

    if (m_current_video_frame < -1)
        m_current_video_frame = -1;

    m_analog_cache_valid = false;

    if (IsValidPointer(m_memory))
        m_memory->UpdateLaserActiveSram();

    bool new_frame = m_video_new_frame;

    if (IsLaserDisc() && (!StartVideoDecoder() || ((m_current_video_frame >= 0) && !LoadCurrentVideoFrame())))
        stream.setstate(std::ios::failbit);

    m_video_new_frame = new_frame;

    if (display_valid && IsLaserDisc() && !stream.fail())
    {
        const u8* frame = m_video_frame_valid && display_frame_index == m_video_frame_index ? &m_video_frame[0] : NULL;

        if (!frame && DecodeVideoFrame(m_video_file, display_frame_index,
            m_video_prefetch_frame, m_video_prefetch_compressed_data))
            frame = &m_video_prefetch_frame[0];

        if (!frame)
            stream.setstate(std::ios::failbit);
        else
            CopyDisplayField(frame, display_even);
    }

    m_video_display_frame_index = display_frame_index;
    m_video_display_valid = display_valid && IsLaserDisc() && !stream.fail();
    m_video_display_even = display_even;
}
