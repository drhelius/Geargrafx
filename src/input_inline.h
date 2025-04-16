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

#ifndef INPUT_INLINE_H
#define INPUT_INLINE_H

#include "input.h"

INLINE u8 Input::ReadK()
{
    return m_register;
}

INLINE void Input::WriteO(u8 value)
{
    m_sel = IS_SET_BIT(value, 0);
    m_clr = IS_SET_BIT(value, 1);
    UpdateRegister();
}

INLINE u8 Input::GetIORegister()
{
    return m_register;
}

INLINE bool Input::GetSel()
{
    return m_sel;
}

INLINE  bool Input::GetClr()
{
    return m_clr;
}

INLINE void Input::UpdateRegister()
{
    m_register = 0x30;

    if (m_pce_jap)
        m_register = SET_BIT(m_register, 6);

    if (!m_cdrom)
        m_register = SET_BIT(m_register, 7);

    //if (!m_clr)
    {
        if (m_sel)
            m_register |= (m_joypads[0] >> 4);
        else
            m_register |= (m_joypads[0] & 0x0F);
    }
}

#endif /* INPUT_INLINE_H */