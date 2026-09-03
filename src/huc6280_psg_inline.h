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
    m_dc_offset = enabled ? 16 : 0;
    m_hpf_prev_input[0] = 0.0f;
    m_hpf_prev_input[1] = 0.0f;
    m_hpf_prev_output[0] = 0.0f;
    m_hpf_prev_output[1] = 0.0f;
}

INLINE HuC6280PSG::HuC6280PSG_State* HuC6280PSG::GetState()
{
    return &m_state;
}

INLINE u16 HuC6280PSG::GetLfoFrequency() const
{
    return m_lfo_frequency ? m_lfo_frequency : k_huc6280_psg_lfo_zero_divider;
}

INLINE bool HuC6280PSG::IsLfoConfigured() const
{
    return m_lfo_enabled && m_lfo_dest->enabled && m_lfo_src->enabled;
}

INLINE bool HuC6280PSG::IsLfoRunning() const
{
    return IsLfoConfigured() && IS_NOT_SET_BIT(m_lfo_control, 7);
}

INLINE u16 HuC6280PSG::CalculateLfoPeriod(u16 frequency, u8 data) const
{
    s32 modulation = ((s32)data - 16) * k_huc6280_psg_lfo_depth[m_lfo_control & 0x03];
    u16 period = (u16)((u32)((s32)frequency + modulation) & 0x0FFF);

    return period ? period : 0x1000;
}

INLINE s16 HuC6280PSG::GetWaveformSample(int channel, u16 frequency, u16 gain) const
{
    const HuC6280PSG_Channel* ch = &m_channels[channel];

    if (frequency <= k_huc6280_psg_analytic_max_period)
    {
        s32 numerator = (s32)m_wave_sum[channel] - ((s32)m_dc_offset * k_huc6280_psg_waveform_samples);

        return (s16)DivideRounded((s64)numerator * gain, k_huc6280_psg_waveform_samples);
    }

    return (s16)(((s32)ch->wave_data[ch->wave_index] - m_dc_offset) * gain);
}

INLINE s64 HuC6280PSG::DivideRounded(s64 value, s64 divisor) const
{
    if (value >= 0)
        return (value + (divisor >> 1)) / divisor;

    return (value - (divisor >> 1)) / divisor;
}

#endif /* HUC6280_PSG_INLINE_H */
