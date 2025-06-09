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

INLINE void CdRomAudio::Clock(u32 cycles)
{
    m_elapsed_cycles += cycles;
}

INLINE void CdRomAudio::Sync()
{
    int remaining_cycles = m_elapsed_cycles;
    m_elapsed_cycles = 0;

    while (remaining_cycles > 0)
    {
        int batch_size = MIN(remaining_cycles, GG_CDAUDIO_CYCLES_PER_SAMPLE - m_sample_cycle_counter);

        m_sample_cycle_counter += batch_size;
        remaining_cycles -= batch_size;

        if (m_sample_cycle_counter >= GG_CDAUDIO_CYCLES_PER_SAMPLE)
        {
            m_sample_cycle_counter -= GG_CDAUDIO_CYCLES_PER_SAMPLE;

            s16 left_sample = 0;
            s16 right_sample = 0;

            if (m_state == CD_AUDIO_STATE_PLAYING)
            {
                u8 buffer[4] = {0};
                m_cdrom_media->ReadBytes(m_current_lba, m_currunt_sample * 4, buffer, 4);
                left_sample = (s16)((buffer[1] << 8) | buffer[0]);
                right_sample = (s16)((buffer[3] << 8) | buffer[2]);

                Debug("CD AUDIO: LBA %d, Sample %d, Left: %d, Right: %d", m_current_lba, m_currunt_sample, left_sample, right_sample);

                m_currunt_sample++;
                if (m_currunt_sample == 588)
                {
                    m_currunt_sample = 0;
                    m_current_lba++;

                    if (m_current_lba > m_stop_lba)
                    {
                        if (m_current_lba >= m_cdrom_media->GetCdRomLengthLba())
                        {
                           m_current_lba = m_cdrom_media->GetCdRomLengthLba() - 1;
                        }
                        switch (m_stop_event)
                        {
                            case CD_AUDIO_STOP_EVENT_STOP:
                                m_state = CD_AUDIO_STATE_STOPPED;
                                break;

                            case CD_AUDIO_STOP_EVENT_LOOP:
                                m_current_lba = m_start_lba;
                                break;

                            case CD_AUDIO_STOP_EVENT_IRQ:
                                // TODO
                                break;

                            default:
                                Log("ERROR: Unknown CD audio stop event");
                                break;
                        }
                    }
                   
                }

            }

            m_buffer[m_buffer_index + 0] = left_sample;
            m_buffer[m_buffer_index + 1] = right_sample;

            m_buffer_index += 2;

            if (m_buffer_index >= GG_AUDIO_BUFFER_SIZE)
            {
                Log("ERROR: CD AUDIO buffer overflow");
                m_buffer_index = 0;
            }
        }
    }
}

#endif /* CDROM_AUDIO_INLINE_H */