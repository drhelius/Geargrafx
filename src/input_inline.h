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
#include "trace_logger.h"

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

INLINE bool Input::IsKeyPressed(GG_Controllers controller, GG_Keys key) const
{
    return !(m_gamepads[controller] & key);
}

INLINE u8 Input::ReadK()
{
    u8 result;
    if (m_turbolink.IsCableConnected() && m_clr)
    {
        u64 cycles = GetTurboLinkCycles();
        result = m_turbolink.ReadPort(m_register, cycles);
        TraceTurboLinkEvent(TRACE_INPUT_TURBOLINK_SAMPLE, result,
            turbolink_make_tick(cycles, GG_TURBOLINK_TICK_BEFORE_PORT_ACCESS),
            m_turbolink.GetLastSampledLines());
    }
    else if (m_mb128.IsConnected() && m_mb128.IsActive())
    {
        u8 low  = m_mb128.Read() & 0x0F;
        u8 high = m_register & 0xF0;
        result = high | low;
    }
    else
    {
        result = m_register;
    }

    TraceInputEvent(TRACE_INPUT_READ, result);

    return result;
}

INLINE void Input::WriteO(u8 value)
{
    static const u64 mouse_latch_delay_cycles = 10000 * 3;

    if (m_mb128.IsConnected())
        m_mb128.Write(value);

    bool prev_sel = m_sel;
    bool prev_clr = m_clr;
    m_sel = IS_SET_BIT(value, 0);
    m_clr = IS_SET_BIT(value, 1);

    u64 turbolink_cycles = GetTurboLinkCycles();
    bool turbolink_connected = m_turbolink.IsCableConnected();
    bool turbolink_drive_changed = m_turbolink.WriteControl(m_sel, m_clr, turbolink_cycles);

    if (turbolink_connected)
    {
        TraceTurboLinkEvent(TRACE_INPUT_TURBOLINK_CONTROL_WRITE, value,
            turbolink_make_tick(turbolink_cycles, GG_TURBOLINK_TICK_BEFORE_PORT_ACCESS),
            GG_TURBOLINK_LINE_MASK);

        if (turbolink_drive_changed)
        {
            TraceTurboLinkEvent(TRACE_INPUT_TURBOLINK_DRIVE_CHANGE, value,
                turbolink_make_tick(turbolink_cycles, GG_TURBOLINK_TICK_AFTER_PORT_ACCESS),
                GG_TURBOLINK_LINE_MASK);
        }
    }
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
            TraceInputEvent(TRACE_INPUT_WRITE, value);
            return;
        }
    }
    else
        m_selected_pad = 0;

    if (m_controller_type[m_selected_pad] == GG_CONTROLLER_MOUSE)
    {
        if (!prev_clr && m_clr)
        {
            u64 current_cycles = GetMasterClockCycles();

            if (!m_mouse_latched || ((current_cycles - m_mouse_last_latch_cycles) > mouse_latch_delay_cycles))
            {
                m_mouse_latched = true;
                m_mouse_last_latch_cycles = current_cycles;

                s32 rel_x = CLAMP(-m_mouse_x, -127, 127);
                s32 rel_y = CLAMP(-m_mouse_y, -127, 127);

                m_mouse_shifter = ((rel_x & 0xF0) >> 4) | ((rel_x & 0x0F) << 4);
                m_mouse_shifter |= (((rel_y & 0xF0) >> 4) | ((rel_y & 0x0F) << 4)) << 8;

                m_mouse_x += rel_x;
                m_mouse_y += rel_y;
            }
            else
            {
                m_mouse_shifter >>= 4;
            }
        }

        if (m_sel)
            m_register |= (m_mouse_shifter & 0x0F);
        else
            m_register |= (m_gamepads[m_selected_pad] & 0x0F);

        TraceInputEvent(TRACE_INPUT_WRITE, value);
        return;
    }

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

    TraceInputEvent(TRACE_INPUT_WRITE, value);
}

INLINE void Input::TraceInputEvent(u8 event, u8 value)
{
    if (IsValidPointer(m_trace_logger) && m_trace_logger->IsEventEnabled(TRACE_INPUT, event))
        LogInputEvent(event, value);
}

INLINE void Input::TraceTurboLinkEvent(u8 event, u8 value, u64 tick, u8 lines)
{
    if (IsValidPointer(m_trace_logger) && m_trace_logger->IsEventEnabled(TRACE_INPUT, event))
    {
        LogTurboLinkEvent(event, value, tick, lines);
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

INLINE void Input::SetTurboLinkCallbacks(
    GG_TurboLink_Publish_Callback publish_callback, GG_TurboLink_Sample_Callback sample_callback,
    GG_TurboLink_Sync_Callback sync_callback, void* user_data)
{
    m_turbolink.SetCallbacks(publish_callback, sample_callback, sync_callback, user_data);
}

INLINE void Input::SetTurboLinkCableConnected(bool connected)
{
    bool changed = m_turbolink.IsCableConnected() != connected;
    u64 cycles = GetTurboLinkCycles();
    m_turbolink.SetCableConnected(connected, cycles);

    if (changed)
    {
        TraceTurboLinkEvent(TRACE_INPUT_TURBOLINK_CABLE,
            connected ? 1 : 0,
            turbolink_make_tick(cycles, GG_TURBOLINK_TICK_BEFORE_PORT_ACCESS),
            m_turbolink.GetLastSampledLines());
    }
}

INLINE bool Input::IsTurboLinkCableConnected() const
{
    return m_turbolink.IsCableConnected();
}

INLINE void Input::InvalidateTurboLinkSample()
{
    m_turbolink.InvalidateSample();
}

INLINE void Input::SynchronizeTurboLink(u64 cycles)
{
    m_turbolink.Synchronize(cycles);
}

INLINE GG_TurboLink_Drive Input::GetTurboLinkDrive() const
{
    return m_turbolink.GetDrive();
}

INLINE bool Input::HasTurboLinkSample() const
{
    return m_turbolink.HasLastSample();
}

INLINE u8 Input::GetTurboLinkLastSampledLines() const
{
    return m_turbolink.GetLastSampledLines();
}

INLINE u8 Input::GetTurboLinkLastPortResult() const
{
    return m_turbolink.GetLastPortResult();
}

INLINE u8 Input::GetTurboLinkLastSamplePullLowMask() const
{
    return m_turbolink.GetLastSamplePullLowMask();
}

INLINE bool Input::GetTurboLinkLastSampleSel() const
{
    return m_turbolink.GetLastSampleSel();
}

INLINE bool Input::GetTurboLinkLastSampleClr() const
{
    return m_turbolink.GetLastSampleClr();
}

INLINE u64 Input::GetTurboLinkLastSampleTick() const
{
    return m_turbolink.GetLastSampleTick();
}

INLINE u64 Input::GetTurboLinkLastControlTick() const
{
    return m_turbolink.GetLastControlTick();
}

INLINE u64 Input::GetTurboLinkLastDriveTick() const
{
    return m_turbolink.GetLastDriveTick();
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

INLINE void Input::SetMouseDelta(s32 x, s32 y)
{
    m_mouse_x += x;
    m_mouse_y += y;
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
