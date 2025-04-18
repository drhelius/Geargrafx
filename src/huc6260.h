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

#include <iostream>
#include <fstream>
#include "common.h"

#define HUC6260_LINE_LENGTH 1365
#define HUC6260_LINES 263
#define HUC6260_HSYNC_LENGTH 237
#define HUC6260_HSYNC_START_HPOS (HUC6260_LINE_LENGTH - HUC6260_HSYNC_LENGTH)
#define HUC6260_HSYNC_END_HPOS 0
#define HUC6260_VSYNC_HPOS (HUC6260_HSYNC_START_HPOS + 30)
//#define HUC6260_DEBUG

class HuC6270;
class HuC6280;

class HuC6260
{
public:
    struct HuC6260_State
    {
        u8* CR;
        u16* CTA;
        s32* HPOS;
        s32* VPOS;
        s32* PIXEL_INDEX;
        bool* HSYNC;
        bool* VSYNC;
    };

    enum HuC6260_Speed
    {
        HuC6260_SPEED_5_36_MHZ,
        HuC6260_SPEED_7_16_MHZ,
        HuC6260_SPEED_10_8_MHZ,
    };

public:
    HuC6260(HuC6270* huc6270, HuC6280* huc6280);
    ~HuC6260();
    void Init(GG_Pixel_Format pixel_format = GG_PIXEL_RGBA8888);
    void InitPalettes();
    void Reset();
    bool Clock();
    NO_INLINE u8 ReadRegister(u16 address);
    NO_INLINE void WriteRegister(u16 address, u8 value);
    HuC6260_State* GetState();
    HuC6260_Speed GetSpeed();
    int GetClockDivider();
    u16* GetColorTable();
    void SetBuffer(u8* frame_buffer);
    u8* GetBuffer();
    int GetCurrentLineWidth();
    int GetCurrentHeight();
    void SetScanlineStart(int scanline_start);
    void SetScanlineEnd(int scanline_end);
    void SetOverscan(bool overscan);
    GG_Pixel_Format GetPixelFormat();
    void SetResetValue(int value);
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);

private:
    NO_INLINE void WritePixel(u16 pixel);

private:
    HuC6270* m_huc6270;
    HuC6280* m_huc6280;
    HuC6260_State m_state;
    u8 m_control_register;
    u16 m_color_table_address;
    s32 m_speed;
    s32 m_clock_divider;
    u16* m_color_table;
    u8* m_frame_buffer;
    s32 m_hpos;
    s32 m_vpos;
    s32 m_pixel_index;
    s32 m_pixel_clock;
    s32 m_pixel_x;
    bool m_hsync;
    bool m_vsync;
    s32 m_blur;
    u32 m_black_and_white;
    int m_overscan;
    int m_scanline_start;
    int m_scanline_end;
    GG_Pixel_Format m_pixel_format;
    u8 m_rgb888_palette[512][3];
    u8 m_bgr888_palette[512][3];
    u16 m_rgb565_palette[512];
    u16 m_bgr565_palette[512];
    u16 m_rgb555_palette[512];
    u16 m_bgr555_palette[512];
    int m_reset_value;
};

static const HuC6260::HuC6260_Speed k_huc6260_speed[4] = {
    HuC6260::HuC6260_SPEED_5_36_MHZ, HuC6260::HuC6260_SPEED_7_16_MHZ,
    HuC6260::HuC6260_SPEED_10_8_MHZ, HuC6260::HuC6260_SPEED_10_8_MHZ };

static const int k_huc6260_total_lines[2] = { 262, 263 };
static const int k_huc6260_full_line_width[4] = { 342, 455, 683, 683 };
static const int k_huc6260_line_width[2][4] = {
    { 256, 341, 512, 512 },
    { 256 + 24, 341 + 32, 512 + 48, 512 + 48 } };
static const int k_huc6260_line_offset[2][4] = {
    { 8 + 24, 8 + 38, 8 + 96, 8 + 96 },
    { 8 + 24 - 12, 8 + 38 - 16, 8 + 96 - 24, 8 + 96 - 24 } };

#include "huc6260_inline.h"

#endif /* HUC6260_H */