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

#ifndef HUC6260_H
#define HUC6260_H

#include "common.h"

class HuC6270;

class HuC6260
{
public:
    struct HuC6260_State
    {
        u8* CR;
        u16* CTA;
    };

    enum HuC6260_Speed
    {
        HuC6260_SPEED_10_8_MHZ = 0,
        HuC6260_SPEED_7_16_MHZ = 1,
        HuC6260_SPEED_5_36_MHZ = 2
    };

public:
    HuC6260(HuC6270* huc6270);
    ~HuC6260();
    void Init();
    void Reset();
    void Clock();
    u8 ReadRegister(u32 address);
    void WriteRegister(u32 address, u8 value);
    HuC6260_State* GetState();
    HuC6260_Speed GetSpeed();
    int GetClockDivider();
    u16* GetColorTable();

private:
    HuC6270* m_huc6270;
    HuC6260_State m_state;
    u8 m_control_register;
    u16 m_color_table_address;
    HuC6260_Speed m_speed;
    int m_clock_divider;
    u16* m_color_table;
};

#include "huc6260_inline.h"

#endif /* HUC6260_H */