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

#ifndef CDROM_AUDIO_H
#define CDROM_AUDIO_H

#include <iostream>
#include <fstream>
#include "common.h"

class CdRomMedia;
class ScsiController;

class CdRomAudio
{
public:
    enum CdAudioStopEvent
    {
        CD_AUDIO_STOP_EVENT_STOP,
        CD_AUDIO_STOP_EVENT_LOOP,
        CD_AUDIO_STOP_EVENT_IRQ
    };

    enum CdAudioState
    {
        CD_AUDIO_STATE_PLAYING = 0x00,
        CD_AUDIO_STATE_IDLE = 0x01,
        CD_AUDIO_STATE_PAUSED = 0x02,
        CD_AUDIO_STATE_STOPPED = 0x03
    };

    struct CdRomAudio_State
    {
        CdAudioState* CURRENT_STATE;
        u32* START_LBA;
        u32* STOP_LBA;
        u32* CURRENT_LBA;
        CdAudioStopEvent* STOP_EVENT;
        s32* SEEK_CYCLES;
        u8* FADER;
        s32* FRAME_SAMPLES;
        s16* BUFFER;
    };

public:
    CdRomAudio(CdRomMedia* cdrom_media);
    ~CdRomAudio();
    void Init(ScsiController* scsi_controller);
    void Reset();
    void Clock(u32 cycles);
    int EndFrame(s16* sample_buffer);
    CdAudioState GetCurrentState();
    CdRomAudio_State* GetState();
    void StartAudio(u32 lba, bool pause);
    void StopAudio();
    void PauseAudio();
    void SetIdle();
    void SetStopLBA(u32 lba, CdAudioStopEvent event);
    void WriteFader(u8 fader);
    u8 ReadFader();
    s16 GetLeftSample();
    s16 GetRightSample();
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);

private:
    void GenerateSamples();

private:
    CdRomMedia* m_cdrom_media;
    ScsiController* m_scsi_controller;
    CdRomAudio_State m_state;
    s32 m_sample_cycle_counter;
    s32 m_buffer_index;
    s32 m_frame_samples;
    s16 m_buffer[GG_AUDIO_BUFFER_SIZE] = {};
    CdAudioState m_current_state;
    u32 m_start_lba;
    u32 m_stop_lba;
    u32 m_current_lba;
    u32 m_current_sample;
    CdAudioStopEvent m_stop_event;
    s32 m_seek_cycles;
    u8 m_fader;
    s16 m_left_sample;
    s16 m_right_sample;
};

#include "cdrom_audio_inline.h"

#endif /* CDROM_AUDIO_H */