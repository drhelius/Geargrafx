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
#include "huc6280_psg.h"

Audio::Audio()
{
    InitPointer(m_psg);
    m_mute = false;
}

Audio::~Audio()
{
    SafeDelete(m_psg);
}

void Audio::Init()
{
    m_psg = new HuC6280PSG();
    m_psg->Init();
}

void Audio::Reset()
{
    m_cycle_counter = 0;
    m_psg->Reset();

    memset(m_psg_buffer, 0, sizeof(m_psg_buffer));
}

void Audio::Mute(bool mute)
{
    m_mute = mute;
}

void Audio::EndFrame(s16* sample_buffer, int* sample_count)
{
    *sample_count = 0;

    int count = m_psg->EndFrame(m_psg_buffer);

    if (IsValidPointer(sample_buffer) && IsValidPointer(sample_count))
    {
        *sample_count = count;

        for (int i = 0; i < count; i++)
        {
            if (m_mute)
                sample_buffer[i] = 0;
            else
                sample_buffer[i] = m_psg_buffer[i];
        }
    }
}

HuC6280PSG* Audio::GetPSG()
{
    return m_psg;
}

void Audio::SaveState(std::ostream& stream)
{
    using namespace std;
    stream.write(reinterpret_cast<const char*> (m_psg_buffer), sizeof(s16) * GG_AUDIO_BUFFER_SIZE);
    m_psg->SaveState(stream);
    stream.write(reinterpret_cast<const char*> (&m_cycle_counter), sizeof(m_cycle_counter));
}

void Audio::LoadState(std::istream& stream)
{
    using namespace std;
    stream.read(reinterpret_cast<char*> (m_psg_buffer), sizeof(s16) * GG_AUDIO_BUFFER_SIZE);
    m_psg->LoadState(stream);
    stream.read(reinterpret_cast<char*> (&m_cycle_counter), sizeof(m_cycle_counter));
}