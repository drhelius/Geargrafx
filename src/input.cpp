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

#include "input.h"
#include "media.h"
#include "geargrafx_core.h"
#include "common.h"
#include "trace_logger.h"

Input::Input(Media* media, GeargrafxCore* core)
{
    m_media = media;
    m_core = core;
    InitPointer(m_trace_logger);
    m_turbo_tap = false;
    m_pce_jap = false;
    m_cdrom = true;
    m_sel = false;
    m_clr = false;
    m_register = 0;
    m_selected_pad = 0;
    m_selected_extra_buttons = false;
    m_mouse_x = 0;
    m_mouse_y = 0;
    m_mouse_shifter = 0;
    m_mouse_latched = false;
    m_mouse_last_latch_cycles = 0;

    for (int i = 0; i < GG_MAX_GAMEPADS; i++)
    {
        m_gamepads[i] = 0xFFFF;
        m_controller_type[i] = GG_CONTROLLER_STANDARD;
        m_avenue_pad_3_button[i] = GG_KEY_NONE;
        m_avenue_pad_3_state[i] = 0xFFFF;

        for (int j = 0; j < 2; j++)
        {
            m_turbo_enabled[i][j] = false;
            m_turbo_state[i][j] = false;
            m_turbo_counter[i][j] = 0;
            m_turbo_speed[i][j] = 4;
        }
    }

    m_mb128.Connect(false);
    m_mb128.Reset();
}

void Input::SetTraceLogger(TraceLogger* trace_logger)
{
    m_trace_logger = trace_logger;
}

void Input::LogInputEvent(u8 event, u8 value)
{
#if !defined(GG_DISABLE_DISASSEMBLER)
    GG_Trace_Entry e = {};
    e.type = TRACE_INPUT;
    e.input.event = event;
    e.input.value = value;
    e.input.port = (u8)m_selected_pad;
    if (event == TRACE_INPUT_READ)
    {
        if (m_turbolink.IsCableConnected() && m_clr)
            e.input.source = TRACE_INPUT_SOURCE_TURBOLINK;
        else if (m_mb128.IsConnected() && m_mb128.IsActive())
            e.input.source = TRACE_INPUT_SOURCE_MB128;
        else if (m_selected_pad >= GG_MAX_GAMEPADS)
            e.input.source = TRACE_INPUT_SOURCE_NONE;
        else if (m_controller_type[m_selected_pad] == GG_CONTROLLER_MOUSE)
            e.input.source = TRACE_INPUT_SOURCE_MOUSE;
        else
            e.input.source = TRACE_INPUT_SOURCE_GAMEPAD;
    }
    e.input.state = (m_sel ? 0x01 : 0x00) |
                    (m_clr ? 0x02 : 0x00) |
                    (m_selected_extra_buttons ? 0x04 : 0x00);
    m_trace_logger->TraceLog(e);
#else
    UNUSED(event);
    UNUSED(value);
#endif
}

void Input::LogTurboLinkEvent(u8 event, u8 value, u64 tick, u8 lines)
{
#if !defined(GG_DISABLE_DISASSEMBLER)
    GG_Trace_Entry e = {};
    GG_TurboLink_Drive drive = m_turbolink.GetDrive();
    e.type = TRACE_INPUT;
    e.input.event = event;
    e.input.source = TRACE_INPUT_SOURCE_TURBOLINK;
    e.input.value = value;
    e.input.port = (u8)m_selected_pad;
    e.input.state = (m_sel ? 0x01 : 0x00) | (m_clr ? 0x02 : 0x00);
    e.input.link_tick = tick;
    e.input.pull_low_mask = drive.drive_mask;
    e.input.lines = lines & GG_TURBOLINK_LINE_MASK;
    m_trace_logger->TraceLog(e);
#else
    UNUSED(event);
    UNUSED(value);
    UNUSED(tick);
    UNUSED(lines);
#endif
}

void Input::Init()
{
    Reset();
}

void Input::Reset()
{
    m_sel = true;
    m_clr = true;
    m_register = 0;
    m_selected_pad = 0;
    m_selected_extra_buttons = false;
    m_mouse_x = 0;
    m_mouse_y = 0;
    m_mouse_shifter = 0;
    m_mouse_latched = false;
    m_mouse_last_latch_cycles = 0;

    for (int i = 0; i < GG_MAX_GAMEPADS; i++)
    {
        m_gamepads[i] = 0xFFFF;
        m_avenue_pad_3_state[i] = 0xFFFF;

        for (int j = 0; j < 2; j++)
        {
            m_turbo_state[i][j] = false;
            m_turbo_counter[i][j] = 0;
        }
    }

    m_mb128.Reset();
    m_turbolink.Reset(m_core->GetTurboLinkCycle());

    WriteO(0xFF);
}

void Input::EndFrame()
{
    for (int i = 0; i < GG_MAX_GAMEPADS; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            if (m_turbo_enabled[i][j])
            {
                m_turbo_counter[i][j]++;
                if (m_turbo_counter[i][j] == m_turbo_speed[i][j])
                {
                    m_turbo_counter[i][j] = 0;
                    m_turbo_state[i][j] = !m_turbo_state[i][j];
                }
            }
            else
            {
                m_turbo_counter[i][j] = 0;
                m_turbo_state[i][j] = false;
            }
        }
    }
}

void Input::SaveState(std::ostream& stream)
{
    using namespace std;
    stream.write(reinterpret_cast<const char*> (&m_clr), sizeof(m_clr));
    stream.write(reinterpret_cast<const char*> (&m_sel), sizeof(m_sel));
    stream.write(reinterpret_cast<const char*> (&m_register), sizeof(m_register));
    stream.write(reinterpret_cast<const char*> (&m_selected_pad), sizeof(m_selected_pad));
    stream.write(reinterpret_cast<const char*> (&m_selected_extra_buttons), sizeof(m_selected_extra_buttons));
    stream.write(reinterpret_cast<const char*> (m_gamepads), sizeof(m_gamepads));
    stream.write(reinterpret_cast<const char*> (&m_mouse_x), sizeof(m_mouse_x));
    stream.write(reinterpret_cast<const char*> (&m_mouse_y), sizeof(m_mouse_y));
    stream.write(reinterpret_cast<const char*> (&m_mouse_shifter), sizeof(m_mouse_shifter));
    stream.write(reinterpret_cast<const char*> (&m_mouse_latched), sizeof(m_mouse_latched));
    stream.write(reinterpret_cast<const char*> (&m_mouse_last_latch_cycles), sizeof(m_mouse_last_latch_cycles));

    bool mb128_included = m_mb128.IsConnected();
    stream.write(reinterpret_cast<const char*> (&mb128_included), sizeof(mb128_included));

    if (mb128_included)
        m_mb128.SaveState(stream);

    m_turbolink.SaveState(stream);
}

void Input::LoadState(std::istream& stream, int version)
{
    using namespace std;
    stream.read(reinterpret_cast<char*> (&m_clr), sizeof(m_clr));
    stream.read(reinterpret_cast<char*> (&m_sel), sizeof(m_sel));
    stream.read(reinterpret_cast<char*> (&m_register), sizeof(m_register));
    stream.read(reinterpret_cast<char*> (&m_selected_pad), sizeof(m_selected_pad));
    stream.read(reinterpret_cast<char*> (&m_selected_extra_buttons), sizeof(m_selected_extra_buttons));

    m_selected_pad = MAX(m_selected_pad, 0);

    if (version >= 25)
        stream.read(reinterpret_cast<char*> (m_gamepads), sizeof(m_gamepads));
    else
    {
        for (int i = 0; i < GG_MAX_GAMEPADS; i++)
            m_gamepads[i] = 0xFFFF;
    }

    if (version >= 26)
    {
        stream.read(reinterpret_cast<char*> (&m_mouse_x), sizeof(m_mouse_x));
        stream.read(reinterpret_cast<char*> (&m_mouse_y), sizeof(m_mouse_y));
        stream.read(reinterpret_cast<char*> (&m_mouse_shifter), sizeof(m_mouse_shifter));
        stream.read(reinterpret_cast<char*> (&m_mouse_latched), sizeof(m_mouse_latched));

        if (version >= 27)
            stream.read(reinterpret_cast<char*> (&m_mouse_last_latch_cycles), sizeof(m_mouse_last_latch_cycles));
        else
            m_mouse_last_latch_cycles = 0;
    }
    else
    {
        m_mouse_x = 0;
        m_mouse_y = 0;
        m_mouse_shifter = 0;
        m_mouse_latched = false;
        m_mouse_last_latch_cycles = 0;
    }

    bool mb128_included = false;
    stream.read(reinterpret_cast<char*> (&mb128_included), sizeof(mb128_included));

    if (mb128_included)
        m_mb128.LoadState(stream);

    if (version >= 35)
        m_turbolink.LoadState(stream);
    else
        m_turbolink.RestoreControl(m_sel, m_clr);
}

u64 Input::GetMasterClockCycles()
{
    return m_core->GetMasterClockCycles();
}

u64 Input::GetTurboLinkCycles()
{
    return m_core->GetTurboLinkCycle();
}
