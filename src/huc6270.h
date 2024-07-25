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

#include "huc6270_defines.h"
#include "common.h"

class HuC6280;

class HuC6270
{
public:
    struct HuC6270_State
    {
        u8* AR;
        u8* SR;
        u16* R;
        u16* READ_BUFFER;
        int* HPOS;
        int* VPOS;
    };

public:
    HuC6270(HuC6280* HuC6280);
    ~HuC6270();
    void Init();
    void Reset();
    bool Clock();
    u8 ReadRegister(u32 address);
    void WriteRegister(u32 address, u8 value);
    HuC6270_State* GetState();
    u16* GetVRAM();
    u16* GetSAT();

private:
    HuC6280* m_huc6280;
    HuC6270_State m_state;
    u16* m_vram;
    u8 m_address_register;
    u8 m_status_register;
    u16 m_register[20];
    u16* m_sat;
    u16 m_read_buffer;
    int m_hpos;
    int m_vpos;

private:
    u8 ReadDataRegister(bool msb);
    void WriteDataRegister(u8 value, bool msb);
};

static const u16 k_register_mask[20] = { 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000,
                                         0x1FFF, 0x03FF, 0x03FF, 0x01FF, 0x00FF,
                                         0x7F1F, 0x7F7F, 0xFF1F, 0x01FF, 0x00FF,
                                         0x001F, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF };

static const int k_scren_size_x[8] = { 32, 64, 128, 128, 32, 64, 128, 128 };
static const int k_scren_size_y[8] = { 32, 32, 32, 32, 64, 64, 64, 64 };
static const int k_read_write_increment[4] = { 0x01, 0x20, 0x40, 0x80 };

#include "huc6270_inline.h"

#endif /* HUC6270_H */