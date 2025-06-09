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

INLINE void CdRomAudio::Clock(u32 cycles)
{
    m_sample_cycle_counter += cycles;

    if (m_sample_cycle_counter >= GG_CDAUDIO_CYCLES_PER_SAMPLE)
    {
        m_sample_cycle_counter -= GG_CDAUDIO_CYCLES_PER_SAMPLE;

        m_buffer[m_buffer_index + 0] = 0; // Placeholder for left channel
        m_buffer[m_buffer_index + 1] = 0; // Placeholder for right channel

        m_buffer_index += 2;

        if (m_buffer_index >= GG_AUDIO_BUFFER_SIZE)
        {
            Log("ERROR: CD AUDIO buffer overflow");
            m_buffer_index = 0;
        }
    }
}

#endif /* CDROM_AUDIO_INLINE_H */