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

#ifndef MB12_INLINE_H
#define MB12_INLINE_H

#include "mb128.h"

INLINE bool MB128::IsConnected() const
{
    return m_connected;
}

INLINE bool MB128::IsActive() const
{
    return m_active;
}

INLINE u8* MB128::GetRAM()
{
    return m_ram;
}

INLINE const u8* MB128::GetRAM() const
{
    return m_ram;
}

INLINE void MB128::Connect(bool connected)
{
    m_connected = connected;
    if (!m_connected)
        Reset();
}

INLINE void MB128::Write(u8 value)
{
    if (!m_connected)
    {
        m_prev_data = value;
        return;
    }

    bool old_clr = IS_SET_BIT(m_prev_data, 1);
    bool new_sel = IS_SET_BIT(value, 0);
    bool new_clr = IS_SET_BIT(value, 1);

    // Rising edge of CLR
    if (!old_clr && new_clr)
    {
        if (m_active)
            SendBit(new_sel);
        else
        {
            // Detect the 0xA8 pattern.
            m_shiftreg = (m_shiftreg >> 1) | (new_sel ? 0x80 : 0x00);

            if (m_shiftreg == 0xA8)
            {
                m_state = MODE_A1;
                m_active = true;
            }
        }
    }

    m_prev_data = value;
}

INLINE u8 MB128::Read()
{
    if (!m_connected)
        return 0;

    if (!m_active)
        return 0;

    return m_retval & 0x0F;
}

#endif /* MB12_INLINE_H */
