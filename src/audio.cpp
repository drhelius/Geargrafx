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
    m_psg_volume = 1.0f;
    m_adpcm_volume = 1.0f;
    m_cdrom_volume = 1.0f;
    m_vgm_recording_enabled = false;
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
    memset(m_cdrom_buffer, 0, sizeof(m_cdrom_buffer));
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

        if (count_psg > GG_AUDIO_BUFFER_SIZE)
        {
            Error("PSG Audio buffer exceeded maximum size");
            count_psg = GG_AUDIO_BUFFER_SIZE;
        }
        if (count_adpcm > GG_AUDIO_BUFFER_SIZE)
        {
            Error("ADPCMM Audio buffer exceeded maximum size");
            count_adpcm = GG_AUDIO_BUFFER_SIZE;
        }
        if (count_cdrom > GG_AUDIO_BUFFER_SIZE)
        {
            Error("CDA Audio buffer exceeded maximum size");
            count_cdrom = GG_AUDIO_BUFFER_SIZE;
        }

        if (count_psg != count_adpcm || count_adpcm != count_cdrom)
        {
            Error("Audio buffers have different sample counts: PSG=%d, ADPCM=%d, CDROM=%d", count_psg, count_adpcm, count_cdrom);
        }

        int samples = count_psg;

        *sample_count = samples;

        if (m_mute)
            memset(sample_buffer, 0, sizeof(s16) * samples);
        else
        {
            for (int i = 0; i < samples; i++)
            {
                s32 mix = 0;
                mix += (s32)(m_psg_buffer[i] * m_psg_volume);
                mix += (s32)(m_adpcm_buffer[i] * m_adpcm_volume);
                mix += (s32)(m_cdrom_buffer[i] * m_cdrom_volume);

                if (mix > 32767)
                    mix = 32767;
                else if (mix < -32768)
                    mix = -32768;

                sample_buffer[i] = (s16)mix;
            }
        }
    }
    else
    {
        int samples = m_psg->EndFrame(m_psg_buffer);

        assert(samples <= GG_AUDIO_BUFFER_SIZE);

        if (samples > GG_AUDIO_BUFFER_SIZE)
        {
            Error("Audio buffer exceeded maximum size");
            samples = GG_AUDIO_BUFFER_SIZE;
        }

        *sample_count = samples;

        if (m_mute || (m_psg_volume <= 0.0f))
            memset(sample_buffer, 0, sizeof(s16) * samples);
        else
        {
            for (int i = 0; i < samples; i++)
            {
                s32 mix = (s32)(m_psg_buffer[i] * m_psg_volume);

                if (mix > 32767)
                    mix = 32767;
                else if (mix < -32768)
                    mix = -32768;

                sample_buffer[i] = (s16)mix;
            }
        }
    }

#ifndef GG_DISABLE_VGMRECORDER
    if (m_vgm_recording_enabled)
        m_vgm_recorder.UpdateTiming(*sample_count / 2);
#endif
}

void Audio::SaveState(std::ostream& stream)
{
    using namespace std;
    stream.write(reinterpret_cast<const char*> (&m_cycle_counter), sizeof(m_cycle_counter));
    m_psg->SaveState(stream);
}

void Audio::LoadState(std::istream& stream)
{
    using namespace std;
    stream.read(reinterpret_cast<char*> (&m_cycle_counter), sizeof(m_cycle_counter));
    m_psg->LoadState(stream);
}

bool Audio::StartVgmRecording(const char* file_path, int clock_rate)
{
    if (m_vgm_recording_enabled)
        return false;

    m_vgm_recorder.Start(file_path, clock_rate);
    m_vgm_recording_enabled = m_vgm_recorder.IsRecording();

    // Write initial state of all audio registers to VGM
    if (m_vgm_recording_enabled)
    {
        // Get PSG state
        HuC6280PSG::HuC6280PSG_State* psg_state = m_psg->GetState();

        // Write PSG registers (0x0800-0x0809)
        // 0x0800 - Channel select
        m_vgm_recorder.WriteHuC6280(0x0800, *psg_state->CHANNEL_SELECT);

        // 0x0801 - Main amplitude
        m_vgm_recorder.WriteHuC6280(0x0801, *psg_state->MAIN_AMPLITUDE);

        // For each channel, write frequency, control, amplitude, and waveform data
        for (int i = 0; i < 6; i++)
        {
            // Select channel
            m_vgm_recorder.WriteHuC6280(0x0800, i);

            // 0x0802 - Frequency low
            m_vgm_recorder.WriteHuC6280(0x0802, psg_state->CHANNELS[i].frequency & 0xFF);

            // 0x0803 - Frequency high
            m_vgm_recorder.WriteHuC6280(0x0803, (psg_state->CHANNELS[i].frequency >> 8) & 0x0F);

            // 0x0804 - Control
            m_vgm_recorder.WriteHuC6280(0x0804, psg_state->CHANNELS[i].control);

            // 0x0805 - Amplitude
            m_vgm_recorder.WriteHuC6280(0x0805, psg_state->CHANNELS[i].amplitude);

            // 0x0806 - Waveform data (32 writes)
            for (int j = 0; j < 32; j++)
            {
                m_vgm_recorder.WriteHuC6280(0x0806, psg_state->CHANNELS[i].wave_data[j]);
            }

            // 0x0807 - Noise control (channels 4 and 5 only)
            if (i >= 4)
            {
                m_vgm_recorder.WriteHuC6280(0x0807, psg_state->CHANNELS[i].noise_control);
            }
        }

        // 0x0808 - LFO frequency
        m_vgm_recorder.WriteHuC6280(0x0808, *psg_state->LFO_FREQUENCY & 0xFF);

        // 0x0809 - LFO control
        m_vgm_recorder.WriteHuC6280(0x0809, *psg_state->LFO_CONTROL);
    }

    return m_vgm_recording_enabled;
}

void Audio::StopVgmRecording()
{
    if (m_vgm_recording_enabled)
    {
        m_vgm_recorder.Stop();
        m_vgm_recording_enabled = false;
    }
}

bool Audio::IsVgmRecording() const
{
    return m_vgm_recording_enabled;
}