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
#include "cartridge.h"
#include "common.h"

Input::Input(Cartridge* cartridge)
{
    m_cartridge = cartridge;
    m_turbo_tap = false;
    m_pce_jap = false;
    m_cdrom = true;
    m_sel = false;
    m_clr = false;
    m_register = 0;
    m_selected_pad = 0;

    for (int i = 0; i < GG_MAX_GAMEPADS; i++)
    {
        m_controller_type[i] = GG_CONTROLLER_STANDARD;
        m_avenue_pad_3_button[i] = GG_KEY_NONE;
        m_avenue_pad_3_state[i] = 0xFFFF;
        m_gamepads[i] = 0xFFFF;
        m_selected_extra_buttons = false;
    }
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

    for (int i = 0; i < GG_MAX_GAMEPADS; i++)
    {
        m_avenue_pad_3_state[i] = 0xFFFF;
        m_gamepads[i] = 0xFFFF;
        m_selected_extra_buttons = false;
    }

    UpdateRegister(0xFF);
}

void Input::SaveState(std::ostream& stream)
{
    using namespace std;
    stream.write(reinterpret_cast<const char*> (&m_clr), sizeof(m_clr));
    stream.write(reinterpret_cast<const char*> (&m_sel), sizeof(m_sel));
    stream.write(reinterpret_cast<const char*> (&m_register), sizeof(m_register));
    stream.write(reinterpret_cast<const char*> (&m_selected_pad), sizeof(m_selected_pad));
    stream.write(reinterpret_cast<const char*> (&m_selected_extra_buttons), sizeof(m_selected_extra_buttons));
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
}