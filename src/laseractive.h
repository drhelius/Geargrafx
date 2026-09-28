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

#ifndef LASERACTIVE_H
#define LASERACTIVE_H

#include <iostream>
#include <vector>

#if defined(__LIBRETRO__) && !defined(GG_DISABLE_MMI_THREADS)
#if (defined(__GLIBCXX__) && !defined(_GLIBCXX_HAS_GTHREADS)) || \
    (defined(_LIBCPP_HAS_THREADS) && !_LIBCPP_HAS_THREADS) || \
    defined(_LIBCPP_HAS_NO_THREADS) || \
    (defined(__EMSCRIPTEN__) && !defined(__EMSCRIPTEN_PTHREADS__))
#define GG_DISABLE_MMI_THREADS
#endif
#endif

#if !defined(GG_DISABLE_MMI_THREADS)
#include <condition_variable>
#include <mutex>
#include <thread>
#endif
#include "common.h"

class CdRom;
class CdRomAudio;
class CdRomMedia;
class Memory;
class MediaFile;

class LaserActive
{
public:
    static const u32 VIDEO_LINE_STATE_SIZE = 8;

    enum DriveMode
    {
        DRIVE_INACTIVE = 0,
        DRIVE_SEEKING,
        DRIVE_READING,
        DRIVE_PLAYING,
        DRIVE_PAUSED,
        DRIVE_STOPPED
    };

    struct Status
    {
        u8 input_registers[0x20];
        u8 live_input_registers[0x20];
        u8 output_registers[0x20];
        DriveMode drive_mode;
        s32 head_lba;
        u32 seek_latency;
        u8 current_track;
        u8 current_drive_state;
        bool paused;
        bool sram_enabled;
        bool ejected;
        bool input_frozen;
        bool output_frozen;
        u32 sample;
        s32 audio_end_lba;
        u8 playback_mode;
        bool digital_sector_valid;
        bool audio_end_pending;
        s32 video_frame;
        u32 search_sectors;
        u32 analog_fade_samples_left;
        u32 analog_fade_samples_right;
        bool analog_muted_left;
        bool analog_muted_right;
        u8 analog_attenuation_left;
        u8 analog_attenuation_right;
    };

public:
    LaserActive(CdRomMedia* cdrom_media);
    ~LaserActive();
    void Init(CdRom* cdrom, Memory* memory, CdRomAudio* cdrom_audio = NULL);
    void Reset();
    void Clock(u32 cycles);
    u8 ReadRegister(u8 reg, bool output);
    u8 PeekRegister(u8 reg, bool output);
    void WriteRegister(u8 reg, bool output, u8 data);
    u8 ReadSramControl(u16 address) const;
    void WriteSramControl(u8 data);
    bool IsSramEnabled() const;
    void NotifyMediaEjected(bool ejected);
    void NotifyMediaChanged();
    u32 NotifyScsiReadStart(u32 lba);
    void NotifyScsiSectorRead(u32 lba, bool final_sector);
    void NotifyAudioStart(u32 lba, bool paused);
    void NotifyAudioStop(bool paused);
    void SetAudioEnd(u32 lba);
    void Sample(s16& left, s16& right);
    void GetStatus(Status& status);
    void SaveState(std::ostream& stream) const;
    void LoadState(std::istream& stream);

    u8 GetVideoMixingMode() const;
    u8 GetVideoControl() const;
    u8 GetGraphicsFader(u8 source) const;
    s32 GetHeadLba() const;

    DriveMode GetDriveMode() const
    {
        return m_drive_mode;
    }

    u32 GetCurrentSample() const
    {
        return m_current_sample;
    }

    u32 GetSeekLatency() const
    {
        return m_seek_latency;
    }

    s32 GetCurrentVideoFrame() const;
    const u8* GetVideoFrameBuffer() const;
    u32 GetVideoWidth() const;
    u32 GetVideoHeight() const;
    bool HasVideoFrame() const;
    bool IsActive() const;
    void BeginVideoFrame();
    void CaptureVideoLineState(u8* state) const;
    void ComposeLine(u32 line, const u8* pce_pixels, const u8* classifications,
        const u8* line_state, u8* output, GG_Pixel_Format pixel_format) const;

private:
    enum SeekPointRegister
    {
        SEEK_CHAPTER = 0,
        SEEK_HOURS_OR_FRAME_HIGH,
        SEEK_MINUTES_OR_FRAME_MIDDLE,
        SEEK_SECONDS_OR_FRAME_LOW,
        SEEK_FRAMES,
        SEEK_POINT_REGISTER_COUNT
    };

    enum SeekMode
    {
        SEEK_REDBOOK_TIME = 0,
        SEEK_REDBOOK_RELATIVE_TIME,
        SEEK_VIDEO_FRAME,
        SEEK_VIDEO_TIME
    };

    static bool GetBit(u8 value, u8 bit);
    static u8 GetBits(u8 value, u8 low, u8 high);
    static void SetBit(u8& value, u8 bit, bool state);
    static void SetBits(u8& value, u8 low, u8 high, u8 field);
    static u8 DecodeBcd(u8 value);
    static u8 EncodeBcd(u8 value);

    bool IsDiscLoaded() const;
    bool IsLaserDisc() const;
    bool IsCLVDisc() const;
    u8 GetTrackCount() const;
    u8 GetCurrentTrack() const;
    bool IsAudioTrack(u8 track) const;
    s32 GetTrackStartLBA(u8 track) const;
    s32 GetTrackEndLBA(u8 track) const;
    s32 GetMinimumLBA() const;
    u8 GetTrackFromLBA(s32 lba) const;
    void GetTrackTOC(u8 track, u8& flags, u8& minute, u8& second, u8& frame) const;
    void GetTimecode(s32 lba, u8& minute, u8& second, u8& frame) const;
    void GetRelativeTimecode(u8& minute, u8& second, u8& frame) const;
    s32 GetABAFromTime(u8 hour, u8 minute, u8 second, u8 frame) const;

    u8 GetOutputRegisterValue(u8 reg, bool side_effects = true);
    void ProcessInputRegisterWrite(u8 reg, u8 data, u8 previous_data, bool deferred);
    void ApplyFrozenInputRegisters();
    void UpdateStopPoint();
    void ResetSeekTarget();
    bool HasLiveSeekTarget() const;
    bool LatchSeekTarget();
    void PerformLatchedSeek();
    void HandleStopPoint(s32 aba);

    double GetNormalizedPosition(s32 lba) const;
    u32 CalculateSeekLatency(s32 target_lba) const;
    void SeekToSector(s32 lba, bool paused);
    void SeekToTrack(u8 track, bool paused);
    void SeekToRelativeTime(u8 track, u8 minute, u8 second, u8 frame, bool paused);
    void SetDrivePlaying();
    void SetDrivePaused();
    void SetDriveStopped();
    void ClockSector();
    s32 GetSectorAdvance() const;
    void UpdateVideoFrame(s32 aba);
    s32 GetVideoFrameFromABA(s32 aba, bool lead_in) const;
    s32 GetABAFromVideoFrame(s32 frame) const;
    void VideoTimeToRedbookTime(u8& hour, u8& minute, u8& second, u8& frame) const;
    bool ReadAnalogSample(s16& left, s16& right);
    bool FillAnalogCache(u64 offset);
    bool StartVideoDecoder();
    void StopVideoDecoder();
    u32 GetAbsoluteVideoFrame(s32 frame, bool lead_in, bool lead_out) const;
    bool DecodeVideoFrame(MediaFile* file, u32 frame, std::vector<u8>& rgb_data, std::vector<u8>& compressed_data);
    bool LoadCurrentVideoFrame();
    void CopyDisplayField(const u8* frame, bool even_field);
    u32 PredictNextVideoFrame() const;
    u32 DecodeBiphaseCode(u32 line) const;

#if !defined(GG_DISABLE_MMI_THREADS)
    void VideoThread();
    void QueueVideoPrefetch(u32 frame);
#endif

private:
    struct VideoResampleInfo
    {
        u32 first_pixel;
        u32 pixel_count;
        u32 weights[5];
    };

    CdRomMedia* m_cdrom_media;
    CdRom* m_cdrom;
    CdRomAudio* m_cdrom_audio;
    Memory* m_memory;

    u8 m_input_regs[0x20];
    u8 m_input_frozen_regs[0x20];
    u8 m_output_regs[0x20];
    u8 m_output_frozen_regs[0x20];
    u8 m_output_written_data[0x20];
    u8 m_output_cooldown[0x20];
    bool m_input_frozen;
    bool m_output_frozen;
    bool m_operation_error_1;
    bool m_operation_error_2;
    bool m_operation_error_3;
    bool m_seek_enabled;
    u8 m_current_seek_mode;
    bool m_current_seek_time_format;
    bool m_current_seek_repeat;
    u8 m_analog_attenuation_left;
    u8 m_analog_attenuation_right;
    bool m_analog_fade_muted_left;
    bool m_analog_fade_muted_right;
    u32 m_analog_fade_samples_left;
    u32 m_analog_fade_samples_right;
    u8 m_active_seek_mode;
    u8 m_seek_point_regs[SEEK_POINT_REGISTER_COUNT];
    u8 m_stop_point_regs[SEEK_POINT_REGISTER_COUNT];
    bool m_reached_stop_point;
    bool m_reached_stop_point_previously;
    u8 m_playback_mode;
    u8 m_playback_speed;
    bool m_playback_reverse;
    u8 m_target_drive_state;
    u8 m_current_drive_state;
    bool m_target_pause;
    bool m_current_pause;
    bool m_seek_frame_pending;
    u8 m_drive_state_delay;
    u8 m_selected_track_info;

    DriveMode m_drive_mode;
    DriveMode m_seek_drive_mode;
    s32 m_head_lba;
    s32 m_seek_target_lba;
    s32 m_audio_end_lba;
    u32 m_seek_latency;
    s32 m_sector_repeat_count;
    u32 m_search_sectors;
    bool m_stop_point_enabled;
    s32 m_stop_point_aba;
    u8 m_current_track;
    alignas(s16) u8 m_digital_sector[2352];
    bool m_digital_sector_valid;
    bool m_audio_end_pending;
    u32 m_current_sample;
    u64 m_sector_clock;

    u16 m_sram_shift;
    bool m_sram_enabled;

    s32 m_current_video_frame;
    bool m_current_video_lead_in;
    bool m_current_video_lead_out;
    s32 m_frame_skip_base;
    s32 m_frame_skip_counter;

    u8 m_analog_cache[2352];
    u64 m_analog_cache_offset;
    bool m_analog_cache_valid;
    bool m_analog_read_error_reported;
    s64 m_analog_lead_in_samples;
    u64 m_analog_audio_size;

    MediaFile* m_video_file;
    MediaFile* m_video_prefetch_file;
    std::vector<u8> m_video_frame;
    std::vector<u8> m_video_prefetch_frame;
    std::vector<u8> m_video_display_field;
    std::vector<u8> m_video_compressed_data;
    std::vector<u8> m_video_prefetch_compressed_data;
    std::vector<VideoResampleInfo> m_video_resampling;
    u32 m_video_frame_index;
    u32 m_video_generation;
    bool m_video_frame_valid;
    bool m_video_decode_error_reported;
    bool m_video_even_field;
    bool m_video_new_frame;
    bool m_video_memory_latched;
    bool m_video_field_selected;
    bool m_video_selected_even;
    u32 m_video_display_frame_index;
    bool m_video_display_valid;
    bool m_video_display_even;

#if !defined(GG_DISABLE_MMI_THREADS)
    std::thread m_video_thread;
    std::mutex m_video_mutex;
    std::condition_variable m_video_condition;
    bool m_video_thread_stop;
    bool m_video_job_pending;
    bool m_video_result_ready;
    u32 m_video_job_frame;
    u32 m_video_job_generation;
    u32 m_video_result_frame;
    u32 m_video_result_generation;
#endif
};

#endif /* LASERACTIVE_H */
