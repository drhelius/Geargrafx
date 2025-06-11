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
    else
    {
        m_sample_cycle_counter += cycles;

        if (m_sample_cycle_counter >= GG_CDAUDIO_CYCLES_PER_SAMPLE)
        {
            m_sample_cycle_counter -= GG_CDAUDIO_CYCLES_PER_SAMPLE;

            m_left_sample = 0;
            m_right_sample = 0;

            if (m_state == CD_AUDIO_STATE_PLAYING)
            {
                GenerateSamples();
            }

            m_buffer[m_buffer_index + 0] = m_left_sample;
            m_buffer[m_buffer_index + 1] = m_right_sample;

            m_buffer_index += 2;

            if (m_buffer_index >= GG_AUDIO_BUFFER_SIZE)
            {
                Log("ERROR: CD AUDIO buffer overflow");
                m_buffer_index = 0;
            }
        }
    }
}

INLINE CdRomAudio::CdAudioState CdRomAudio::GetAudioState()
{
    return m_state;
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
    m_currunt_sample = 0;
    m_stop_lba = m_cdrom_media->GetLastSectorOfTrack(track);
    m_stop_event = CD_AUDIO_STOP_EVENT_STOP;
    m_sample_cycle_counter = 0;
    m_state = pause ? CD_AUDIO_STATE_PAUSED : CD_AUDIO_STATE_PLAYING;

    Debug("CD AUDIO: Start audio at LBA %d, track %d, current lba %d, seek cycles %d",
          lba, track, current_lba, m_seek_cycles);

    m_cdrom_media->PreloadTrackChunks(m_cdrom_media->GetTrackFromLBA(m_start_lba));
}

INLINE void CdRomAudio::StopAudio()
{
    m_state = CD_AUDIO_STATE_STOPPED;
}

INLINE void CdRomAudio::PauseAudio()
{
    m_state = CD_AUDIO_STATE_PAUSED;
}

INLINE void CdRomAudio::SetIdle()
{
    m_state = CD_AUDIO_STATE_IDLE;
}

INLINE void CdRomAudio::SetStopLBA(u32 lba, CdAudioStopEvent event)
{
    if (lba >= m_cdrom_media->GetSectorCount())
    {
        Debug("ERROR: Invalid stop LBA %d", lba);
        lba = m_cdrom_media->GetSectorCount() - 1;
    }

    m_stop_lba = lba;
    m_stop_event = event;
    m_state = CD_AUDIO_STATE_PLAYING;
}

INLINE void CdRomAudio::GenerateSamples()
{
    u8 buffer[4] = {0};
    m_cdrom_media->ReadBytes(m_current_lba, m_currunt_sample * 4, buffer, 4);
    m_left_sample = (s16)((buffer[1] << 8) | buffer[0]);
    m_right_sample = (s16)((buffer[3] << 8) | buffer[2]);

    m_currunt_sample++;
    if (m_currunt_sample == 588)
    {
        m_currunt_sample = 0;
        m_current_lba++;

        if (m_current_lba > m_stop_lba)
        {
            if (m_current_lba >= m_cdrom_media->GetSectorCount())
                m_current_lba = m_cdrom_media->GetSectorCount() - 1;

            switch (m_stop_event)
            {
                case CD_AUDIO_STOP_EVENT_STOP:
                    m_state = CD_AUDIO_STATE_STOPPED;
                    break;
                case CD_AUDIO_STOP_EVENT_LOOP:
                    m_current_lba = m_start_lba;
                    break;
                case CD_AUDIO_STOP_EVENT_IRQ:
                    m_state = CD_AUDIO_STATE_STOPPED;
                    m_scsi_controller->StartStatus(ScsiController::SCSI_STATUS_GOOD);
                    break;
                default:
                    Log("ERROR: Unknown CD audio stop event");
                    break;
            }
        }
    }
}

INLINE void CdRomAudio::WriteFader(u8 fader)
{
    Debug("CDROM Write Fader %02X", fader);
    m_fader = fader;
}

INLINE u8 CdRomAudio::ReadFader()
{
    return m_fader;
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