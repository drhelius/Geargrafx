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

#include "load_detector.h"
#include "audio.h"
#include "cdrom_audio.h"
#include "input.h"
#include "scsi_controller.h"

static const u64 k_load_detector_hash_a = 0x9E3779B97F4A7C15ULL;
static const u64 k_load_detector_hash_b = 0xC2B2AE3D27D4EB4FULL;
static const u64 k_load_detector_hash_c = 0x165667B19E3779F9ULL;
static const u64 k_load_detector_hash_d = 0x27D4EB2F165667C5ULL;
static const u64 k_load_detector_color_mask = 0x00FFFFFF00FFFFFFULL;

LoadDetector::LoadDetector()
{
    InitPointer(m_scsi_controller);
    InitPointer(m_cdrom_audio);
    InitPointer(m_audio);
    InitPointer(m_input);
    m_bytes_per_pixel = 4;
    Reset();
}

void LoadDetector::Init(ScsiController* scsi_controller, CdRomAudio* cdrom_audio, Audio* audio, Input* input,
    GG_Pixel_Format pixel_format)
{
    m_scsi_controller = scsi_controller;
    m_cdrom_audio = cdrom_audio;
    m_audio = audio;
    m_input = input;
    m_bytes_per_pixel = (pixel_format == GG_PIXEL_RGB565) ? 2 : 4;
    Reset();
}

void LoadDetector::Reset()
{
    m_loading = false;
    m_session = false;
    m_veto = false;
    m_black = false;
    m_hash_valid = false;
    m_hash = 0;
    m_access_count = IsValidPointer(m_scsi_controller) ? m_scsi_controller->GetAccessCount() : 0;
    m_input_count = IsValidPointer(m_input) ? m_input->GetActivityCount() : 0;
    m_idle_frames = k_load_detector_black_idle_frames + 1;
    m_quiet_frames = 0;
    m_unverified_frames = 0;
    m_check_interval = k_load_detector_check_interval_max;
    m_check_seed = 0;
    memset(m_history, 0, sizeof(m_history));
    m_history_index = 0;
    m_history_count = 0;
}

bool LoadDetector::Update(const u8* frame_buffer, int width, int height, bool rendered)
{
    u32 access_count = m_scsi_controller->GetAccessCount();
    u32 input_count = m_input->GetActivityCount();
    bool cdda = m_cdrom_audio->GetCurrentState() == CdRomAudio::CD_AUDIO_STATE_PLAYING;
    bool data = !cdda && (m_scsi_controller->IsBusy() || (access_count != m_access_count));

    m_access_count = access_count;
    m_idle_frames = data ? 0 : MIN(m_idle_frames + 1, k_load_detector_black_idle_frames + 1);

    if (input_count != m_input_count)
    {
        m_input_count = input_count;
        m_session = false;
        m_veto = false;
    }

    if (data && !m_session)
    {
        m_session = true;
        m_veto = false;
    }

    bool quiet = false;
    bool unknown = false;

    if (m_session && !m_veto && !cdda && IsSilent())
    {
        if (rendered && IsValidPointer(frame_buffer))
            quiet = IsStatic(frame_buffer, width, height);
        else
        {
            unknown = true;
            m_unverified_frames = MIN(m_unverified_frames + 1, k_load_detector_check_interval_max);
        }
    }

    bool busy = data && !quiet && !unknown;
    m_history_count += (busy ? 1 : 0) - (m_history[m_history_index] ? 1 : 0);
    m_history[m_history_index] = busy;
    m_history_index = (m_history_index + 1) % LOAD_DETECTOR_HISTORY;

    if (m_session && (m_history_count >= k_load_detector_veto_frames))
        m_veto = true;

    if (m_veto && (m_idle_frames > k_load_detector_veto_clear_frames))
    {
        m_veto = false;
        m_session = false;
    }

    if (m_idle_frames > (m_black ? k_load_detector_black_idle_frames : k_load_detector_idle_frames))
        m_session = false;

    if (cdda)
        m_session = false;

    if (!unknown)
        m_quiet_frames = quiet ? MIN(m_quiet_frames + 1, k_load_detector_enter_frames) : 0;

    bool loading = m_session && !m_veto && (m_quiet_frames >= k_load_detector_enter_frames);

    if (m_loading && !loading && !data)
        m_session = false;

    m_loading = loading;
    return m_loading;
}

bool LoadDetector::IsSilent() const
{
    int samples = m_audio->GetFrameSamples();

    for (int source = 0; source < Audio::AUDIO_SOURCE_COUNT; source++)
    {
        const s16* buffer = m_audio->GetSourceBuffer(source);

        for (int i = 2; i < samples; i++)
        {
            if (buffer[i] != buffer[i - 2])
                return false;
        }
    }

    return true;
}

bool LoadDetector::IsStatic(const u8* frame_buffer, int width, int height)
{
    if (m_loading && (m_unverified_frames < (m_check_interval - 1)))
    {
        m_unverified_frames++;
        return true;
    }

    bool black = false;
    u64 hash = HashFrame(frame_buffer, width, height, &black);
    bool same = m_hash_valid && (hash == m_hash);

    m_hash = hash;
    m_hash_valid = true;
    m_black = black;
    m_unverified_frames = 0;
    m_check_seed = (m_check_seed * 1664525u) + 1013904223u;
    m_check_interval = k_load_detector_check_interval_min +
        (int)((m_check_seed >> 16) % (k_load_detector_check_interval_max - k_load_detector_check_interval_min + 1));
    return same;
}

u64 LoadDetector::HashFrame(const u8* frame_buffer, int width, int height, bool* black) const
{
    int bytes = width * height * m_bytes_per_pixel;
    int words = bytes / 8;
    u64 mask = (m_bytes_per_pixel == 4) ? k_load_detector_color_mask : ~0ULL;
    u64 a = (u64)width;
    u64 b = (u64)height;
    u64 c = 0;
    u64 d = 0;
    u64 color = 0;
    int i = 0;

    for (; (i + 4) <= words; i += 4)
    {
        u64 values[4];
        memcpy(values, frame_buffer + (i * 8), sizeof(values));
        values[0] &= mask;
        values[1] &= mask;
        values[2] &= mask;
        values[3] &= mask;
        a = (a ^ values[0]) * k_load_detector_hash_a;
        b = (b ^ values[1]) * k_load_detector_hash_b;
        c = (c ^ values[2]) * k_load_detector_hash_c;
        d = (d ^ values[3]) * k_load_detector_hash_d;
        color |= values[0] | values[1] | values[2] | values[3];
    }

    for (; i < words; i++)
    {
        u64 value = 0;
        memcpy(&value, frame_buffer + (i * 8), sizeof(value));
        value &= mask;
        a = (a ^ value) * k_load_detector_hash_a;
        color |= value;
    }

    if ((bytes & 7) != 0)
    {
        u64 value = 0;
        memcpy(&value, frame_buffer + (words * 8), bytes & 7);
        value &= mask;
        b = (b ^ value) * k_load_detector_hash_b;
        color |= value;
    }

    *black = color == 0;
    return *black ? 0 : (a ^ (b << 1) ^ (c << 2) ^ (d << 3));
}
