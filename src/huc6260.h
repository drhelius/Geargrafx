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

#define HUC6260_LINE_LENGTH 1365
#define HUC6260_LINES 263
#define HUC6260_HSYNC_LENGTH 237
#define HUC6260_HSYNC_START_HPOS (HUC6260_LINE_LENGTH - HUC6260_HSYNC_LENGTH)
#define HUC6260_HSYNC_END_HPOS 0
#define HUC6260_VSYNC_HPOS (HUC6260_HSYNC_START_HPOS + 30)
#define HUC6260_VSYNC_START_VPOS (HUC6260_LINES - 4)
#define HUC6260_VSYNC_END_VPOS (HUC6260_LINES - 1)

class HuC6270;

class HuC6260
{
public:
    struct HuC6260_State
    {
        u8* CR;
        u16* CTA;
        int* HPOS;
        int* VPOS;
        int* PIXEL_INDEX;
        bool* HSYNC;
        bool* VSYNC;
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
    void Init(GG_Pixel_Format pixel_format = GG_PIXEL_RGB888);
    void InitPalettes();
    void Reset();
    bool Clock();
    u8 ReadRegister(u32 address);
    void WriteRegister(u32 address, u8 value);
    HuC6260_State* GetState();
    HuC6260_Speed GetSpeed();
    int GetClockDivider();
    u16* GetColorTable();
    void SetBuffer(u8* frame_buffer);

private:
    HuC6270* m_huc6270;
    HuC6260_State m_state;
    u8 m_control_register;
    u16 m_color_table_address;
    HuC6260_Speed m_speed;
    int m_clock_divider;
    u16* m_color_table;
    u8* m_frame_buffer;
    int m_hpos;
    int m_vpos;
    int m_pixel_index;
    int m_pixel_clock;
    bool m_hsync;
    bool m_vsync;
    GG_Pixel_Format m_pixel_format;
    u8 m_rgb888_palette[512][3];
    u8 m_bgr888_palette[512][3];
    u16 m_rgb565_palette[512];
    u16 m_bgr565_palette[512];
    u16 m_rgb555_palette[512];
    u16 m_bgr555_palette[512];
};

static const int k_huc6260_line_width[4] = { 256, 341, 512, 512 };

#include "huc6260_inline.h"

#endif /* HUC6260_H */