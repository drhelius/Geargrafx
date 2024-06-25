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

#include <istream>
#include <ostream>
#include "audio.h"

Audio::Audio()
{
    m_elapsed_cycles = 0;
    m_sample_rate = 44100;
    m_mute = false;
}

Audio::~Audio()
{
}

void Audio::Init()
{
    
}

void Audio::Reset()
{
    m_elapsed_cycles = 0;
}

void Audio::Mute(bool mute)
{
    m_mute = mute;
}

void Audio::EndFrame(s16* sample_buffer, int* sample_count)
{
    *sample_count = 0;
    m_elapsed_cycles = 0;
}

// void Audio::SaveState(std::ostream& stream)
// {
//     stream.write(reinterpret_cast<const char*> (&m_elapsed_cycles), sizeof(m_ElapsedCycles));
// }

// void Audio::LoadState(std::istream& stream)
// {
//     stream.read(reinterpret_cast<char*> (&m_elapsed_cycles), sizeof(m_ElapsedCycles));
// }