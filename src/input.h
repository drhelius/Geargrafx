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

#include <iostream>
#include <fstream>
#include "common.h"

class Media;

class Input
{
public:
    Input(Media* media);
    void Init();
    void Reset();
    void KeyPressed(GG_Controllers controller, GG_Keys key);
    void KeyReleased(GG_Controllers controller, GG_Keys key);
    u8 ReadK();
    void WriteO(u8 value);
    u8 GetIORegister();
    bool GetSel();
    bool GetClr();
    void EndFrame();
    void EnablePCEJap(bool enable);
    void EnableCDROM(bool enable);
    void EnableTurboTap(bool enabled);
    void EnableTurbo(GG_Controllers controller, GG_Keys key, bool enabled);
    bool IsTurboEnabled(GG_Controllers controller, GG_Keys key);
    void SetTurboSpeed(GG_Controllers controller, GG_Keys key, u8 speed);
    void SetControllerType(GG_Controllers controller, GG_Controller_Type type);
    void SetAvenuePad3Button(GG_Controllers controller, GG_Keys button);
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);

private:
    Media* m_media;
    bool m_clr;
    bool m_sel;
    u16 m_gamepads[GG_MAX_GAMEPADS];
    u8 m_register;
    bool m_pce_jap;
    bool m_cdrom;
    bool m_turbo_tap;
    GG_Controller_Type m_controller_type[GG_MAX_GAMEPADS];
    int m_selected_pad;
    bool m_selected_extra_buttons;
    GG_Keys m_avenue_pad_3_button[GG_MAX_GAMEPADS];
    u16 m_avenue_pad_3_state[GG_MAX_GAMEPADS];
    bool m_turbo_enabled[GG_MAX_GAMEPADS][2];
    bool m_turbo_state[GG_MAX_GAMEPADS][2];
    u8 m_turbo_counter[GG_MAX_GAMEPADS][2];
    u8 m_turbo_speed[GG_MAX_GAMEPADS][2];
};

#include "input_inline.h"

#endif /* INPUT_H */