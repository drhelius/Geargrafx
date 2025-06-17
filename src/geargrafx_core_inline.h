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

#ifndef GEARGRAFX_CORE_INLINE_H
#define GEARGRAFX_CORE_INLINE_H

#include "geargrafx_core.h"
#include "media.h"
#include "huc6260.h"
#include "huc6280.h"
#include "audio.h"
#include "cdrom.h"
#include "cdrom_audio.h"
#include "adpcm.h"

INLINE bool GeargrafxCore::RunToVBlank(u8* frame_buffer, s16* sample_buffer, int* sample_count, GG_Debug_Run* debug)
{
    if (m_paused || !m_media->IsReady())
        return false;

    const bool is_cdrom = m_media->IsCDROM();
    const bool is_sgx = m_media->IsSGX();
#if defined(GG_DISABLE_DISASSEMBLER)
    const bool debugger = false;
#else
    const bool debugger = true;
#endif

    if (debugger)
    {
        if (is_cdrom && is_sgx)
            return RunToVBlankTemplate<true, true, true>(frame_buffer, sample_buffer, sample_count, debug);
        else if (is_cdrom && !is_sgx)
            return RunToVBlankTemplate<true, true, false>(frame_buffer, sample_buffer, sample_count, debug);
        else if (!is_cdrom && is_sgx)
            return RunToVBlankTemplate<true, false, true>(frame_buffer, sample_buffer, sample_count, debug);
        else
            return RunToVBlankTemplate<true, false, false>(frame_buffer, sample_buffer, sample_count, debug);
    }
    else
    {
        if (is_cdrom && is_sgx)
            return RunToVBlankTemplate<false, true, true>(frame_buffer, sample_buffer, sample_count, debug);
        else if (is_cdrom && !is_sgx)
            return RunToVBlankTemplate<false, true, false>(frame_buffer, sample_buffer, sample_count, debug);
        else if (!is_cdrom && is_sgx)
            return RunToVBlankTemplate<false, false, true>(frame_buffer, sample_buffer, sample_count, debug);
        else
            return RunToVBlankTemplate<false, false, false>(frame_buffer, sample_buffer, sample_count, debug);
    }
}

template<bool debugger, bool is_cdrom, bool is_sgx>
bool GeargrafxCore::RunToVBlankTemplate(u8* frame_buffer, s16* sample_buffer, int* sample_count, GG_Debug_Run* debug)
{
    if (debugger)
    {
        bool debug_enable = false;
        bool instruction_completed = false;
        if (IsValidPointer(debug))
        {
            debug_enable = true;
            m_huc6280->EnableBreakpoints(debug->stop_on_breakpoint, debug->stop_on_irq);
        }

        m_huc6260->SetBuffer(frame_buffer);
        bool stop = false;

        do
        {
            if (debug_enable && (IsValidPointer(m_debug_callback)))
                m_debug_callback();

            u32 cycles = m_huc6280->RunInstruction(&instruction_completed);
            m_master_clock_cycles += cycles;
            m_huc6280->ClockTimer(cycles);
            stop = m_huc6260->Clock<is_sgx>(cycles);
            if (is_cdrom)
            {
                m_cdrom->Clock(cycles);
                m_adpcm->Clock(cycles);
                m_cdrom_audio->Clock(cycles);
            }
            m_audio->Clock(cycles);

            if (debug_enable)
            {
                if (debug->step_debugger)
                    stop = instruction_completed;

                if (instruction_completed)
                {
                    if (m_huc6280->BreakpointHit())
                        stop = true;

                    if (debug->stop_on_run_to_breakpoint && m_huc6280->RunToBreakpointHit())
                        stop = true;
                }
            }
        }
        while (!stop);

        m_audio->EndFrame(sample_buffer, sample_count);

        return m_huc6280->BreakpointHit() || m_huc6280->RunToBreakpointHit();
    }
    else
    {
        UNUSED(debug);
        m_huc6260->SetBuffer(frame_buffer);
        bool stop = false;

        do
        {
            u32 cycles = m_huc6280->RunInstruction();
            m_master_clock_cycles += cycles;
            m_huc6280->ClockTimer(cycles);
            stop = m_huc6260->Clock<is_sgx>(cycles);
            if (is_cdrom)
            {
                m_cdrom->Clock(cycles);
                m_adpcm->Clock(cycles);
                m_cdrom_audio->Clock(cycles);
            }
            m_audio->Clock(cycles);
        }
        while (!stop);

        m_audio->EndFrame(sample_buffer, sample_count);
        return false;
    }
}

INLINE Memory* GeargrafxCore::GetMemory()
{
    return m_memory;
}

INLINE Media* GeargrafxCore::GetMedia()
{
    return m_media;
}

INLINE HuC6202* GeargrafxCore::GetHuC6202()
{
    return m_huc6202;
}

INLINE HuC6260* GeargrafxCore::GetHuC6260()
{
    return m_huc6260;
}

INLINE HuC6270* GeargrafxCore::GetHuC6270_1()
{
    return m_huc6270_1;
}

INLINE HuC6270* GeargrafxCore::GetHuC6270_2()
{
    return m_huc6270_2;
}

INLINE HuC6280* GeargrafxCore::GetHuC6280()
{
    return m_huc6280;
}

INLINE CdRom* GeargrafxCore::GetCDROM()
{
    return m_cdrom;
}

INLINE CdRomMedia* GeargrafxCore::GetCDROMMedia()
{
    return m_cdrom_media;
}

INLINE CdRomAudio* GeargrafxCore::GetCDROMAudio()
{
    return m_cdrom_audio;
}

INLINE ScsiController* GeargrafxCore::GetScsiController()
{
    return m_scsi_controller;
}

INLINE Adpcm* GeargrafxCore::GetAdpcm()
{
    return m_adpcm;
}

INLINE Audio* GeargrafxCore::GetAudio()
{
    return m_audio;
}

INLINE Input* GeargrafxCore::GetInput()
{
    return m_input;
}

INLINE u64 GeargrafxCore::GetMasterClockCycles()
{
    return m_master_clock_cycles;
}

#endif /* GEARGRAFX_CORE_INLINE_H */