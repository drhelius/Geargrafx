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
        m_cdaudio_sample_right = !m_cdaudio_sample_right;

        if (m_cdaudio_sample_right)
            m_cdaudio_sample = m_cdrom_audio->GetRightSample();
        else
            m_cdaudio_sample = m_cdrom_audio->GetLeftSample();
    }
}

INLINE CdRom::CdRom_State* CdRom::GetState()
{
    return &m_state;
}

#endif /* CDROM_INLINE_H */