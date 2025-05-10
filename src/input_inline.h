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
#include "cartridge.h"

INLINE void Input::KeyPressed(GG_Controllers controller, GG_Keys key)
{
    m_gamepads[controller] &= ~key;

    if (m_controller_type[controller] == GG_CONTROLLER_AVENUE_PAD_3)
    {
        GG_Keys iii_button = m_avenue_pad_3_button[controller];
        if (iii_button == GG_KEY_NONE)
            iii_button = m_cartridge->GetAvenuePad3Button();

        if ((key == iii_button) || (key == GG_KEY_III))
        {
            m_avenue_pad_3_state[controller] &= ~key;
            m_gamepads[controller] &= ~iii_button;
        }
    }
}

INLINE void Input::KeyReleased(GG_Controllers controller, GG_Keys key)
{
    m_gamepads[controller] |= key;

    if (m_controller_type[controller] == GG_CONTROLLER_AVENUE_PAD_3)
    {
        GG_Keys iii_button = m_avenue_pad_3_button[controller];
        if (iii_button == GG_KEY_NONE)
            iii_button = m_cartridge->GetAvenuePad3Button();

        if ((key == iii_button) || (key == GG_KEY_III))
        {
            m_avenue_pad_3_state[controller] |= key;
            if ((m_avenue_pad_3_state[controller] & iii_button) && (m_avenue_pad_3_state[controller] & GG_KEY_III))
            {
                m_gamepads[controller] |= iii_button;
            }
        }
    }
}

INLINE u8 Input::ReadK()
{
    return m_register;
}

INLINE void Input::WriteO(u8 value)
{
    UpdateRegister(value);
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

INLINE void Input::EnablePCEJap(bool enable)
{
    m_pce_jap = enable;
}

INLINE void Input::EnableCDROM(bool enable)
{
    m_cdrom = enable;
}

INLINE void Input::EnableTurboTap(bool enabled)
{
    m_turbo_tap = enabled;
}

INLINE void Input::SetControllerType(GG_Controllers controller, GG_Controller_Type type)
{
    m_controller_type[controller] = type;
}

INLINE void Input::SetAvenuePad3Button(GG_Controllers controller, GG_Keys button)
{
    m_avenue_pad_3_button[controller] = button;
}

INLINE void Input::UpdateRegister(u8 value)
{
    bool prev_sel = m_sel;
    bool prev_clr = m_clr;
    m_sel = IS_SET_BIT(value, 0);
    m_clr = IS_SET_BIT(value, 1);
    m_register = 0x30;

    if (m_pce_jap)
        m_register = SET_BIT(m_register, 6);
    if (!m_cdrom)
        m_register = SET_BIT(m_register, 7);

    if (m_turbo_tap)
    {
        if(!m_clr && !prev_sel && m_sel && m_selected_pad < GG_MAX_GAMEPADS)
            m_selected_pad++;

        if(m_sel && !prev_clr && m_clr)
            m_selected_pad = 0;

        if (m_selected_pad >= GG_MAX_GAMEPADS)
        {
            m_register |= 0x0F;
            return;
        }
    }
    else
        m_selected_pad = 0;

    if (prev_clr && !m_clr)
        m_selected_extra_buttons = !m_selected_extra_buttons;

    if (!m_clr)
    {
        if ((m_controller_type[m_selected_pad] == GG_CONTROLLER_AVENUE_PAD_6) && m_selected_extra_buttons)
        {
            if (!m_sel)
                m_register |= ((m_gamepads[m_selected_pad] >> 8) & 0x0F);
        }
        else
        {
            if (m_sel)
                m_register |= ((m_gamepads[m_selected_pad] >> 4) & 0x0F);
            else
                m_register |= (m_gamepads[m_selected_pad] & 0x0F);
        }
    }
}

#endif /* INPUT_INLINE_H */