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
#ifndef AUDIO_INLINE_H
#define	AUDIO_INLINE_H

#include "audio.h"
#include "huc6280_psg.h"

INLINE void Audio::Clock(u32 cycles)
{
    for (u32 i = 0; i < cycles; i++)
    {
        if (m_cycle_counter == 0)
        {
            m_cycle_counter = 6;
            m_psg->Clock();
        }
        m_cycle_counter--;
    }
}

INLINE void Audio::WritePSG(u32 address, u8 value)
{
    m_psg->Write(address, value);
}

#endif /* AUDIO_INLINE_H */
