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

#ifndef CDROM_INLINE_H
#define CDROM_INLINE_H

#include "cdrom.h"
#include "scsi_controller.h"
#include "huc6280.h"
#include "cdrom_audio.h"

INLINE void CdRom::Clock(u32 cycles)
{
    m_scsi_controller->Clock(cycles);
}

INLINE void CdRom::SetIRQ(u8 value)
{
    if (m_active_irqs & value)
        return;

    m_active_irqs |= value;
    AssertIRQ2();
}

INLINE void CdRom::ClearIRQ(u8 value)
{
    if ((m_active_irqs & value) == 0)
        return;

    m_active_irqs &= ~value;
    AssertIRQ2();
}

INLINE void CdRom::AssertIRQ2()
{
    bool asserted = (m_enabled_irqs & m_active_irqs);
    m_huc6280->AssertIRQ2(asserted);
}

INLINE void CdRom::LatchCdAudioSample()
{
    u64 current_clock = m_core->GetMasterClockCycles();

    if (current_clock - m_cdaudio_sample_last_clock >= 700)
    {
        m_cdaudio_sample_last_clock = current_clock;
        m_cdaudio_sample_toggle = !m_cdaudio_sample_toggle;

        if (m_cdaudio_sample_toggle)
            m_cdaudio_sample = m_cdrom_audio->GetRightSample();
        else
            m_cdaudio_sample = m_cdrom_audio->GetLeftSample();
    }
}

INLINE CdRom::CdRom_State* CdRom::GetState()
{
    return &m_state;
}

INLINE bool CdRom::IsFaderEnabled(bool adpcm)
{
    return (m_fader_enabled && (m_fader_adpcm == adpcm));
}

INLINE double CdRom::GetFaderValue()
{
    u64 elapsed_cycles = m_core->GetMasterClockCycles() - m_fader_start_cycles;

    if (elapsed_cycles >= m_fader_cycles)
        return 0.0;

    double completed = double(elapsed_cycles) / double(m_fader_cycles);
    return (1.0 - completed);
}

inline void CdRom::WriteFader(u8 value)
{
    m_fader = value;
    m_fader_enabled = IS_SET_BIT(value, 3);
    m_fader_adpcm = IS_SET_BIT(value, 1);
    m_fader_fast = IS_SET_BIT(value, 2);
    m_fader_start_cycles = m_core->GetMasterClockCycles();

    double fader_seconds = m_fader_fast ? CDROM_FAST_FADE : CDROM_SLOW_FADE;
    m_fader_cycles = (u64)(fader_seconds * GG_MASTER_CLOCK_RATE);

    Debug("CDROM Fader: %02X, enabled: %d, adpcm: %d, fast: %d, cycles: %llu",
          value, m_fader_enabled, m_fader_adpcm, m_fader_fast, m_fader_cycles);
}

#endif /* CDROM_INLINE_H */