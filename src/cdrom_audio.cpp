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

#include "cdrom_audio.h"
#include "cdrom_media.h"

CdRomAudio::CdRomAudio(CdRomMedia* cdrom_media)
{
    m_cdrom_media = cdrom_media;
    m_sample_cycle_counter = 0;
    m_buffer_index = 0;
    m_state = CD_AUDIO_STATE_STOPPED;
    m_start_lba = 0;
    m_stop_lba = 0;
    m_current_lba = 0;
    m_currunt_sample = 0;
    m_stop_event = CD_AUDIO_STOP_EVENT_STOP;
    m_seek_cycles = 0;
    m_elapsed_cycles = 0;
}

CdRomAudio::~CdRomAudio()
{

}

void CdRomAudio::Init()
{
    Reset();
}

void CdRomAudio::Reset()
{
    m_sample_cycle_counter = 0;
    m_buffer_index = 0;
    m_state = CD_AUDIO_STATE_STOPPED;
    m_start_lba = 0;
    m_stop_lba = 0;
    m_current_lba = 0;
    m_currunt_sample = 0;
    m_stop_event = CD_AUDIO_STOP_EVENT_STOP;
    m_seek_cycles = 0;
    m_elapsed_cycles = 0;
}

int CdRomAudio::EndFrame(s16* sample_buffer)
{
    Sync();

    int samples = 0;

    if (IsValidPointer(sample_buffer))
    {
        samples = m_buffer_index;
        memcpy(sample_buffer, m_buffer, samples * sizeof(s16));
    }

    m_buffer_index = 0;

    return samples;
}

void CdRomAudio::StartAudio(u32 lba, bool pause)
{
    s32 track = m_cdrom_media->GetTrackFromLBA(lba);

    if (track < 0)
        return;

    m_seek_cycles = m_cdrom_media->SeekTime(m_cdrom_media->GetCurrentSector(), lba);
    m_start_lba = lba;
    m_current_lba = lba;
    m_currunt_sample = 0;
    m_stop_lba = m_cdrom_media->GetLastSectorOfTrack(track);
    m_stop_event = CD_AUDIO_STOP_EVENT_STOP;
    m_sample_cycle_counter = 0;
    m_state = CD_AUDIO_STATE_PLAYING;//pause ? CD_AUDIO_STATE_PAUSED : CD_AUDIO_STATE_PLAYING;
}

void CdRomAudio::StopAudio()
{

}

void CdRomAudio::PauseAudio()
{

}

void CdRomAudio::SetStopLBA(u32 lba, CdAudioStopEvent event)
{

}

void CdRomAudio::SaveState(std::ostream& stream)
{
    
}

void CdRomAudio::LoadState(std::istream& stream)
{
    
}