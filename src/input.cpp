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
#include "common.h"

Input::Input(Media* media)
{
    m_media = media;
    m_turbo_tap = false;
    m_pce_jap = false;
    m_cdrom = true;
    m_sel = false;
    m_clr = false;
    m_register = 0;
    m_selected_pad = 0;
    m_selected_extra_buttons = false;

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

    bool mb128_included = m_mb128.IsConnected();
    stream.write(reinterpret_cast<const char*> (&mb128_included), sizeof(mb128_included));

    if (mb128_included)
        m_mb128.SaveState(stream);
}

void Input::LoadState(std::istream& stream)
{
    using namespace std;
    stream.read(reinterpret_cast<char*> (&m_clr), sizeof(m_clr));
    stream.read(reinterpret_cast<char*> (&m_sel), sizeof(m_sel));
    stream.read(reinterpret_cast<char*> (&m_register), sizeof(m_register));
    stream.read(reinterpret_cast<char*> (&m_selected_pad), sizeof(m_selected_pad));
    stream.read(reinterpret_cast<char*> (&m_selected_extra_buttons), sizeof(m_selected_extra_buttons));

    for (int i = 0; i < GG_MAX_GAMEPADS; i++)
        m_gamepads[i] = 0xFFFF;

    bool mb128_included = false;
    stream.read(reinterpret_cast<char*> (&mb128_included), sizeof(mb128_included));

    if (mb128_included)
        m_mb128.LoadState(stream);
}