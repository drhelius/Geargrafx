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

#ifndef INPUT_H
#define INPUT_H

#include "common.h"

class Input
{
public:
    Input();
    void Init();
    void Reset();
    void KeyPressed(GG_Controllers controller, GG_Keys key);
    void KeyReleased(GG_Controllers controller, GG_Keys key);
    u8 ReadK();
    void WriteO(u8 value);
    u8 GetIORegister();
    bool GetSel();
    bool GetClr();
    // void SaveState(std::ostream& stream);
    // void LoadState(std::istream& stream);

private:
    bool m_clr;
    bool m_sel;
    u8 m_joypads[2];
    u8 m_register;
    bool m_pce_jap;
    bool m_cdrom;

private:
    void UpdateRegister();

};

#include "input_inline.h"

#endif /* INPUT_H */