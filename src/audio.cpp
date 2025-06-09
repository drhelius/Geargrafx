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
#include <assert.h>
#include "audio.h"
#include "huc6280_psg.h"
#include "adpcm.h"
#include "cdrom_audio.h"

Audio::Audio(Adpcm* adpcm, CdRomAudio* cdrom_audio)
{
    m_adpcm = adpcm;
    m_cdrom_audio = cdrom_audio;
    InitPointer(m_psg);
    m_mute = false;
    m_is_cdrom = false;
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

void Audio::Reset(bool cdrom)
{
    m_is_cdrom = cdrom;
    m_cycle_counter = 0;
    m_psg->Reset();

    memset(m_psg_buffer, 0, sizeof(m_psg_buffer));
    memset(m_adpcm_buffer, 0, sizeof(m_adpcm_buffer));
}

void Audio::Mute(bool mute)
{
    m_mute = mute;
}

void Audio::EndFrame(s16* sample_buffer, int* sample_count)
{
    *sample_count = 0;

    if (!IsValidPointer(sample_buffer) || !IsValidPointer(sample_count))
        return;

    if (m_is_cdrom)
    {
        int count_psg = m_psg->EndFrame(m_psg_buffer);
        int count_adpcm = m_adpcm->EndFrame(m_adpcm_buffer);
        int count_cdrom = m_cdrom_audio->EndFrame(m_cdrom_buffer);
        assert(count_psg <= GG_AUDIO_BUFFER_SIZE);
        assert(count_adpcm <= GG_AUDIO_BUFFER_SIZE);
        assert(count_cdrom <= GG_AUDIO_BUFFER_SIZE);

        *sample_count = count_cdrom;

        if (m_mute)
            memset(sample_buffer, 0, sizeof(s16) * count_cdrom);
        else
        {
            for (int i = 0; i < count_cdrom; i++)
            {
                //sample_buffer[i] = m_psg_buffer[i];
                //sample_buffer[i] += m_adpcm_buffer[i];
                sample_buffer[i] = m_cdrom_buffer[i];
            }
        }
    }
    else
    {
        int count_psg = m_psg->EndFrame(m_psg_buffer);
        assert(count_psg <= GG_AUDIO_BUFFER_SIZE);
        *sample_count = count_psg;

        if (m_mute)
            memset(sample_buffer, 0, sizeof(s16) * count_psg);
        else
            memcpy(sample_buffer, m_psg_buffer, sizeof(s16) * count_psg);
    }
}

void Audio::SaveState(std::ostream& stream)
{
    using namespace std;
    stream.write(reinterpret_cast<const char*> (m_psg_buffer), sizeof(s16) * GG_AUDIO_BUFFER_SIZE);
    stream.write(reinterpret_cast<const char*> (m_adpcm_buffer), sizeof(s16) * GG_AUDIO_BUFFER_SIZE);
    stream.write(reinterpret_cast<const char*> (&m_cycle_counter), sizeof(m_cycle_counter));
    m_psg->SaveState(stream);
}

void Audio::LoadState(std::istream& stream)
{
    using namespace std;
    stream.read(reinterpret_cast<char*> (m_psg_buffer), sizeof(s16) * GG_AUDIO_BUFFER_SIZE);
    stream.read(reinterpret_cast<char*> (m_adpcm_buffer), sizeof(s16) * GG_AUDIO_BUFFER_SIZE);
    stream.read(reinterpret_cast<char*> (&m_cycle_counter), sizeof(m_cycle_counter));
    m_psg->LoadState(stream);
}