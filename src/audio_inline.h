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

INLINE void Audio::Clock(u32 cycles)
{
    u32 total_cycles = m_cycle_counter + cycles;
    u32 psg_cycles = total_cycles / 6;
    m_psg->Clock(psg_cycles);
    m_cycle_counter = total_cycles % 6;
}

INLINE void Audio::WritePSG(u32 address, u8 value)
{
    m_psg->Write(address, value);
}

INLINE HuC6280PSG* Audio::GetPSG()
{
    return m_psg;
}

INLINE void Audio::Mute(bool mute)
{
    m_mute = mute;
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
