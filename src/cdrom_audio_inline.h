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

#ifndef CDROM_AUDIO_INLINE_H
#define CDROM_AUDIO_INLINE_H

#include "cdrom_audio.h"
#include "cdrom.h"
#include "cdrom_media.h"
#include "scsi_controller.h"

INLINE void CdRomAudio::Clock(u32 cycles)
{
    if (m_seek_cycles > 0)
    {
        m_seek_cycles -= cycles;

        if (m_seek_cycles <= 0)
        {
            m_seek_cycles = 0;
            m_scsi_controller->StartStatus(ScsiController::SCSI_STATUS_GOOD);
        }
    }

    m_sample_cycle_counter += cycles;

    if (m_sample_cycle_counter >= GG_CDAUDIO_CYCLES_PER_SAMPLE)
    {
        m_sample_cycle_counter -= GG_CDAUDIO_CYCLES_PER_SAMPLE;

        m_left_sample = 0;
        m_right_sample = 0;

        if ((m_current_state == CD_AUDIO_STATE_PLAYING) && (m_seek_cycles == 0))
        {
            GenerateSamples();
        }

        m_buffer[m_buffer_index + 0] = m_left_sample;
        m_buffer[m_buffer_index + 1] = m_right_sample;

        m_buffer_index += 2;

        if (m_buffer_index >= GG_AUDIO_BUFFER_SIZE)
        {
            Error("CD AUDIO buffer overflow");
            m_buffer_index = 0;
        }
    }
}

INLINE CdRomAudio::CdAudioState CdRomAudio::GetCurrentState()
{
    return m_current_state;
}

INLINE CdRomAudio::CdRomAudio_State* CdRomAudio::GetState()
{
    return &m_state;
}

INLINE void CdRomAudio::StartAudio(u32 lba, bool pause)
{
    s32 track = m_cdrom_media->GetTrackFromLBA(lba);

    if (track < 0)
        return;

    u32 current_lba = m_cdrom_media->GetCurrentSector();
    u32 seek_time = m_cdrom_media->SeekTime(current_lba, lba);
    m_seek_cycles = TimeToCycles(seek_time * 1000);
    m_start_lba = lba;
    m_current_lba = lba;
    m_current_sample = 0;
    m_stop_lba = m_cdrom_media->GetLastSectorOfTrack(track);
    m_stop_event = CD_AUDIO_STOP_EVENT_STOP;
    m_current_state = pause ? CD_AUDIO_STATE_PAUSED : CD_AUDIO_STATE_PLAYING;

    Debug("CD AUDIO: Start audio at LBA %d, track %d, current lba %d, seek cycles %d",
          lba, track, current_lba, m_seek_cycles);

    m_cdrom_media->PreloadTrack(m_cdrom_media->GetTrackFromLBA(m_start_lba));
}

INLINE void CdRomAudio::StopAudio()
{
    m_current_state = CD_AUDIO_STATE_STOPPED;
}

INLINE void CdRomAudio::PauseAudio()
{
    m_current_state = CD_AUDIO_STATE_PAUSED;
}

INLINE void CdRomAudio::SetIdle()
{
    m_current_state = CD_AUDIO_STATE_IDLE;
}

INLINE void CdRomAudio::SetStopLBA(u32 lba, CdAudioStopEvent event)
{
    if (lba >= m_cdrom_media->GetSectorCount())
    {
        Error("Invalid stop LBA %d", lba);
        lba = m_cdrom_media->GetSectorCount() - 1;
    }

    m_stop_lba = lba;
    m_stop_event = event;
    m_current_state = CD_AUDIO_STATE_PLAYING;
}

INLINE void CdRomAudio::GenerateSamples()
{
    s16 buffer[2] = { };
    m_cdrom_media->ReadSamples(m_current_lba, m_current_sample * 4, buffer, 2);
    m_left_sample = buffer[0];
    m_right_sample = buffer[1];

    if (m_cdrom->IsFaderEnabled(false))
    {
        double fader_value = m_cdrom->GetFaderValue();
        m_left_sample = (s16)(m_left_sample * fader_value);
        m_right_sample = (s16)(m_right_sample * fader_value);
    }

    m_current_sample++;
    if (m_current_sample == (2352 / 4))
    {
        m_current_sample = 0;
        m_current_lba++;

        if (m_current_lba > m_stop_lba)
        {
            if (m_current_lba >= m_cdrom_media->GetSectorCount())
                m_current_lba = m_cdrom_media->GetSectorCount() - 1;

            switch (m_stop_event)
            {
                case CD_AUDIO_STOP_EVENT_STOP:
                    m_current_state = CD_AUDIO_STATE_STOPPED;
                    break;
                case CD_AUDIO_STOP_EVENT_LOOP:
                    m_current_lba = m_start_lba;
                    break;
                case CD_AUDIO_STOP_EVENT_IRQ:
                    m_current_state = CD_AUDIO_STATE_STOPPED;
                    m_scsi_controller->StartStatus(ScsiController::SCSI_STATUS_GOOD);
                    break;
                default:
                    Error("Unknown CD audio stop event");
                    break;
            }
        }
    }
}

INLINE s16 CdRomAudio::GetLeftSample()
{
    return m_left_sample;
}

INLINE s16 CdRomAudio::GetRightSample()
{
    return m_right_sample;
}

#endif /* CDROM_AUDIO_INLINE_H */