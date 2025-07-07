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

#ifndef HUC6280_PSG_INLINE_H
#define HUC6280_PSG_INLINE_H

#include "huc6280_psg.h"

INLINE void HuC6280PSG::Clock(u32 cycles)
{
    m_elapsed_cycles += cycles;
}

INLINE void HuC6280PSG::EnableHuC6280A(bool enabled)
{
    m_huc6280a = enabled;
    m_dc_offset = enabled ? 16 : 0;
    m_hpf_prev_input = 0.0f;
    m_hpf_prev_output = 0.0f;
}

INLINE HuC6280PSG::HuC6280PSG_State* HuC6280PSG::GetState()
{
    return &m_state;
}

#endif /* HUC6280_PSG_INLINE_H */