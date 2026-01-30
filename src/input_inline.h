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
#include "media.h"

INLINE void Input::KeyPressed(GG_Controllers controller, GG_Keys key)
{
    m_gamepads[controller] &= ~key;

    if (m_controller_type[controller] == GG_CONTROLLER_AVENUE_PAD_3)
    {
        GG_Keys iii_button = m_avenue_pad_3_button[controller];
        if (iii_button == GG_KEY_NONE)
            iii_button = m_media->GetAvenuePad3Button();

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
            iii_button = m_media->GetAvenuePad3Button();

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
    if (m_mb128.IsConnected() && m_mb128.IsActive())
    {
        u8 low  = m_mb128.Read() & 0x0F;   // MB128 drives only low nibble
        u8 high = m_register & 0xF0;
        return high | low;
    }

    return m_register;
}

INLINE void Input::WriteO(u8 value)
{
    if (m_mb128.IsConnected())
        m_mb128.Write(value);

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

    u16 raw_gamepad = m_gamepads[m_selected_pad];

    if (m_turbo_enabled[m_selected_pad][0] && !(raw_gamepad & GG_KEY_I))
    {
        if (m_turbo_state[m_selected_pad][0])
            raw_gamepad &= ~GG_KEY_I;
        else
            raw_gamepad |= GG_KEY_I;
    }

    if (m_turbo_enabled[m_selected_pad][1] && !(raw_gamepad & GG_KEY_II))
    {
        if (m_turbo_state[m_selected_pad][1])
            raw_gamepad &= ~GG_KEY_II;
        else
            raw_gamepad |= GG_KEY_II;
    }

    if (!m_clr)
    {
        if ((m_controller_type[m_selected_pad] == GG_CONTROLLER_AVENUE_PAD_6) && m_selected_extra_buttons)
        {
            if (!m_sel)
                m_register |= ((raw_gamepad >> 8) & 0x0F);
        }
        else
        {
            if (m_sel)
                m_register |= ((raw_gamepad >> 4) & 0x0F);
            else
                m_register |= (raw_gamepad & 0x0F);
        }
    }
}

INLINE u8 Input::GetIORegister()
{
    return m_register;
}

INLINE void Input::SetIORegister(u8 value)
{
    m_register = value;
}

INLINE bool Input::GetSel()
{
    return m_sel;
}

INLINE bool Input::GetClr()
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

INLINE void Input::EnableTurbo(GG_Controllers controller, GG_Keys key, bool enabled)
{
    if (key < GG_KEY_I || key > GG_KEY_II)
        return;

    int index = key - 1;
    m_turbo_enabled[controller][index] = enabled;
}

INLINE bool Input::IsTurboEnabled(GG_Controllers controller, GG_Keys key)
{
    if (key < GG_KEY_I || key > GG_KEY_II)
        return false;

    int index = key - 1;
    return m_turbo_enabled[controller][index];
}

INLINE void Input::SetTurboSpeed(GG_Controllers controller, GG_Keys key, u8 speed)
{
    if (key < GG_KEY_I || key > GG_KEY_II)
        return;

    if (speed == 0)
        speed = 1;

    int index = key - 1;
    m_turbo_speed[controller][index] = speed;
}

INLINE void Input::SetControllerType(GG_Controllers controller, GG_Controller_Type type)
{
    m_controller_type[controller] = type;
}

INLINE GG_Controller_Type Input::GetControllerType(GG_Controllers controller)
{
    return m_controller_type[controller];
}

INLINE void Input::SetAvenuePad3Button(GG_Controllers controller, GG_Keys button)
{
    m_avenue_pad_3_button[controller] = button;
}

INLINE void Input::EnableMB128(bool enable)
{
    m_mb128.Connect(enable);
}

INLINE MB128* Input::GetMB128()
{
    return &m_mb128;
}

#endif /* INPUT_INLINE_H */