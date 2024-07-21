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

#ifndef HUC6270_H
#define HUC6270_H

#include "common.h"

#define GG_MAX_RESOLUTION_WIDTH 256
#define GG_MAX_RESOLUTION_HEIGHT 192

class HuC6270
{
public:
    struct HuC6270_State
    {

    };

public:
    HuC6270();
    ~HuC6270();
    void Init();
    void Reset();
    u8 ReadRegister(u32 address);
    void WriteRegister(u32 address, u8 value);
    HuC6270_State* GetState();

private:
    HuC6270_State m_state;
};

#include "huc6270_inline.h"

#endif /* HUC6270_H */