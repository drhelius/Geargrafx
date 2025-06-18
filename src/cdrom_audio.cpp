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
#include "cdrom_media.h"

CdRomAudio::CdRomAudio(CdRomMedia* cdrom_media)
{
    m_cdrom_media = cdrom_media;
    InitPointer(m_cdrom);
    InitPointer(m_scsi_controller);
    m_sample_cycle_counter = 0;
    m_buffer_index = 0;
    m_frame_samples = 0;
    m_current_state = CD_AUDIO_STATE_STOPPED;
    m_start_lba = 0;
    m_stop_lba = 0;
    m_current_lba = 0;
    m_current_sample = 0;
    m_stop_event = CD_AUDIO_STOP_EVENT_STOP;
    m_seek_cycles = 0;
    m_left_sample = 0;
    m_right_sample = 0;

    m_state.CURRENT_STATE = &m_current_state;
    m_state.START_LBA = &m_start_lba;
    m_state.STOP_LBA = &m_stop_lba;
    m_state.CURRENT_LBA = &m_current_lba;
    m_state.STOP_EVENT = &m_stop_event;
    m_state.SEEK_CYCLES = &m_seek_cycles;
    m_state.FRAME_SAMPLES = &m_frame_samples;
    m_state.BUFFER = m_buffer;
}

CdRomAudio::~CdRomAudio()
{

}

void CdRomAudio::Init(CdRom* cdrom, ScsiController* scsi_controller)
{
    m_cdrom = cdrom;
    m_scsi_controller = scsi_controller;
    Reset();
}

void CdRomAudio::Reset()
{
    m_sample_cycle_counter = 0;
    m_buffer_index = 0;
    m_frame_samples = 0;
    m_current_state = CD_AUDIO_STATE_IDLE;
    m_start_lba = 0;
    m_stop_lba = 0;
    m_current_lba = 0;
    m_current_sample = 0;
    m_stop_event = CD_AUDIO_STOP_EVENT_STOP;
    m_seek_cycles = 0;
    m_left_sample = 0;
    m_right_sample = 0;
}

int CdRomAudio::EndFrame(s16* sample_buffer)
{
    int samples = 0;

    if (IsValidPointer(sample_buffer))
    {
        samples = m_buffer_index;
        m_frame_samples = m_buffer_index;
        memcpy(sample_buffer, m_buffer, samples * sizeof(s16));
    }

    m_buffer_index = 0;

    return samples;
}

void CdRomAudio::SaveState(std::ostream& stream)
{
    using namespace std;

    stream.write(reinterpret_cast<const char*> (&m_sample_cycle_counter), sizeof(m_sample_cycle_counter));
    stream.write(reinterpret_cast<const char*> (&m_current_state), sizeof(m_current_state));
    stream.write(reinterpret_cast<const char*> (&m_start_lba), sizeof(m_start_lba));
    stream.write(reinterpret_cast<const char*> (&m_stop_lba), sizeof(m_stop_lba));
    stream.write(reinterpret_cast<const char*> (&m_current_lba), sizeof(m_current_lba));
    stream.write(reinterpret_cast<const char*> (&m_current_sample), sizeof(m_current_sample));
    stream.write(reinterpret_cast<const char*> (&m_stop_event), sizeof(m_stop_event));
    stream.write(reinterpret_cast<const char*> (&m_seek_cycles), sizeof(m_seek_cycles));
    stream.write(reinterpret_cast<const char*> (&m_left_sample), sizeof(m_left_sample));
    stream.write(reinterpret_cast<const char*> (&m_right_sample), sizeof(m_right_sample));
}

void CdRomAudio::LoadState(std::istream& stream)
{
    using namespace std;

    stream.read(reinterpret_cast<char*> (&m_sample_cycle_counter), sizeof(m_sample_cycle_counter));
    stream.read(reinterpret_cast<char*> (&m_current_state), sizeof(m_current_state));
    stream.read(reinterpret_cast<char*> (&m_start_lba), sizeof(m_start_lba));
    stream.read(reinterpret_cast<char*> (&m_stop_lba), sizeof(m_stop_lba));
    stream.read(reinterpret_cast<char*> (&m_current_lba), sizeof(m_current_lba));
    stream.read(reinterpret_cast<char*> (&m_current_sample), sizeof(m_current_sample));
    stream.read(reinterpret_cast<char*> (&m_stop_event), sizeof(m_stop_event));
    stream.read(reinterpret_cast<char*> (&m_seek_cycles), sizeof(m_seek_cycles));
    stream.read(reinterpret_cast<char*> (&m_left_sample), sizeof(m_left_sample));
    stream.read(reinterpret_cast<char*> (&m_right_sample), sizeof(m_right_sample));
}