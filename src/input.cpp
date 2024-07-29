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
#include "common.h"

Input::Input()
{
    m_pce_jap = false;
    m_cdrom = false;
    m_sel = false;
    m_clr = false;
    m_register = 0;
    m_joypads[0] = 0xFF;
    m_joypads[1] = 0xFF;
}

void Input::Init()
{
    Reset();
}

void Input::Reset()
{
    m_pce_jap = false;
    m_cdrom = false;
    m_sel = true;
    m_clr = true;
    m_register = 0;
    m_joypads[0] = 0xFF;
    m_joypads[1] = 0XFF;
    UpdateRegister();
}

void Input::KeyPressed(GG_Controllers controller, GG_Keys key)
{
    m_joypads[controller] &= ~key;
}

void Input::KeyReleased(GG_Controllers controller, GG_Keys key)
{
    m_joypads[controller] |= key;
}

// void Input::SaveState(std::ostream& stream)
// {
// }

// void Input::LoadState(std::istream& stream)
// {
// }