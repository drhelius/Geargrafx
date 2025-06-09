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

CdRomAudio::CdRomAudio()
{
   m_sample_cycle_counter = 0;
   m_buffer_index = 0;
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
}

int CdRomAudio::EndFrame(s16* sample_buffer)
{
    int samples = 0;

    if (IsValidPointer(sample_buffer))
    {
        samples = m_buffer_index;
        memcpy(sample_buffer, m_buffer, samples * sizeof(s16));
    }

    m_buffer_index = 0;

    return samples;
}

void CdRomAudio::SaveState(std::ostream& stream)
{
    
}

void CdRomAudio::LoadState(std::istream& stream)
{
    
}