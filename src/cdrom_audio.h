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
        CD_AUDIO_STATE_STOPPED,
        CD_AUDIO_STATE_PLAYING,
        CD_AUDIO_STATE_PAUSED,
        CD_AUDIO_STATE_IDLE
    };

public:
    CdRomAudio(CdRomMedia* cdrom_media);
    ~CdRomAudio();
    void Init();
    void Reset();
    void Clock(u32 cycles);
    int EndFrame(s16* sample_buffer);
    void StartAudio(u32 lba, bool pause);
    void StopAudio();
    void PauseAudio();
    void SetStopLBA(u32 lba, CdAudioStopEvent event);
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);

private:
    void Sync();

private:
    CdRomMedia* m_cdrom_media;
    s32 m_sample_cycle_counter;
    s32 m_buffer_index;
    s16 m_buffer[GG_AUDIO_BUFFER_SIZE] = {};
    CdAudioState m_state;
    u32 m_start_lba;
    u32 m_stop_lba;
    u32 m_current_lba;
    u32 m_currunt_sample;
    CdAudioStopEvent m_stop_event;
    s32 m_seek_cycles;
    s32 m_elapsed_cycles;
};

#include "cdrom_audio_inline.h"

#endif /* CDROM_AUDIO_H */