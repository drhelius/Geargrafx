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
#define AUDIO_INLINE_H

#include "audio.h"
#include "huc6280_psg.h"
#include "adpcm.h"
#include "cdrom_audio.h"
#include "trace_logger.h"

INLINE void Audio::Clock(u32 cycles)
{
    while (cycles > 0)
    {
        u32 step = cycles;
        u64 cycles_to_sample = (GG_MASTER_CLOCK_RATE - m_sample_clock_counter + GG_AUDIO_SAMPLE_RATE - 1) / GG_AUDIO_SAMPLE_RATE;

        if ((cycles_to_sample > 0) && (cycles_to_sample < step))
            step = (u32)cycles_to_sample;

        ClockSources(step);

        m_sample_clock_counter += (u64)step * GG_AUDIO_SAMPLE_RATE;
        cycles -= step;

        while (m_sample_clock_counter >= GG_MASTER_CLOCK_RATE)
        {
            m_sample_clock_counter -= GG_MASTER_CLOCK_RATE;
            SampleSources();
        }
    }
}

INLINE void Audio::ClockSources(u32 cycles)
{
    u32 total_cycles = m_cycle_counter + cycles;
    u32 psg_cycles = total_cycles / 6;
    m_psg->Clock(psg_cycles);
    m_cycle_counter = total_cycles % 6;

    if (m_is_cdrom)
    {
        m_adpcm->Clock(cycles);
        m_cdrom_audio->Clock(cycles);
    }
}

INLINE void Audio::SampleSources()
{
    m_psg->Sample();

    if (m_is_cdrom)
    {
        m_adpcm->Sample();
        m_cdrom_audio->Sample();
    }
}

INLINE void Audio::WritePSG(u32 address, u8 value)
{
#if !defined(GG_DISABLE_DISASSEMBLER)
    if (m_trace_logger->IsEnabled(TRACE_PSG))
    {
        GG_Trace_Entry e = {};
        e.type = TRACE_PSG;
        e.psg.channel = *m_psg->GetState()->CHANNEL_SELECT;
        e.psg.reg = (u8)(address & 0x0F);
        e.psg.value = value;
        m_trace_logger->TraceLog(e);
    }
#endif
    m_psg->Write(address, value);
#ifndef GG_DISABLE_VGMRECORDER
    if (m_vgm_recording_enabled)
        m_vgm_recorder.WriteHuC6280(address, value);
#endif
}

INLINE HuC6280PSG* Audio::GetPSG()
{
    return m_psg;
}

INLINE void Audio::Mute(bool mute)
{
    m_mute = mute;
}

INLINE void Audio::SetMasterVolume(float volume)
{
    m_master_volume = CLAMP(volume, 0.0f, 2.0f);
}

INLINE void Audio::SetPSGVolume(float volume)
{
    m_psg_volume = CLAMP(volume, 0.0f, 2.0f);
}

INLINE void Audio::SetADPCMVolume(float volume)
{
    m_adpcm_volume = CLAMP(volume, 0.0f, 2.0f);
}

INLINE void Audio::SetCDROMVolume(float volume)
{
    m_cdrom_volume = CLAMP(volume, 0.0f, 2.0f);
}

#endif /* AUDIO_INLINE_H */
