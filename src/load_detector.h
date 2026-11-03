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

#ifndef LOAD_DETECTOR_H
#define LOAD_DETECTOR_H

#include "common.h"

class Audio;
class CdRomAudio;
class Input;
class ScsiController;

#define LOAD_DETECTOR_HISTORY 120

class LoadDetector
{
public:
    LoadDetector();
    void Init(ScsiController* scsi_controller, CdRomAudio* cdrom_audio, Audio* audio, Input* input,
        GG_Pixel_Format pixel_format);
    void Reset();
    bool Update(const u8* frame_buffer, int width, int height, bool rendered);

private:
    bool IsSilent() const;
    bool IsStatic(const u8* frame_buffer, int width, int height);
    u64 HashFrame(const u8* frame_buffer, int width, int height, bool* black) const;

private:
    ScsiController* m_scsi_controller;
    CdRomAudio* m_cdrom_audio;
    Audio* m_audio;
    Input* m_input;
    int m_bytes_per_pixel;
    bool m_loading;
    bool m_session;
    bool m_veto;
    bool m_black;
    bool m_hash_valid;
    u64 m_hash;
    u32 m_access_count;
    u32 m_input_count;
    int m_idle_frames;
    int m_quiet_frames;
    int m_unverified_frames;
    int m_check_interval;
    u32 m_check_seed;
    bool m_history[LOAD_DETECTOR_HISTORY];
    int m_history_index;
    int m_history_count;
};

static const int k_load_detector_enter_frames = 2;
static const int k_load_detector_idle_frames = 90;
static const int k_load_detector_black_idle_frames = 1200;
static const int k_load_detector_veto_frames = 15;
static const int k_load_detector_veto_clear_frames = 120;
static const int k_load_detector_check_interval_min = 3;
static const int k_load_detector_check_interval_max = 5;

#endif /* LOAD_DETECTOR_H */
