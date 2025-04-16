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

#include <assert.h>
#include <stdlib.h>
#include "huc6260.h"

HuC6260::HuC6260(HuC6270* huc6270, HuC6280* huc6280)
{
    m_huc6280 = huc6280;
    m_huc6270 = huc6270;
    m_pixel_format = GG_PIXEL_RGBA8888;
    m_state.CR = &m_control_register;
    m_state.CTA = &m_color_table_address;
    m_state.HPOS = &m_hpos;
    m_state.VPOS = &m_vpos;
    m_state.PIXEL_INDEX = &m_pixel_index;
    m_state.HSYNC = &m_hsync;
    m_state.VSYNC = &m_vsync;
    InitPointer(m_color_table);
    InitPointer(m_frame_buffer);

    m_overscan = 0;
    m_scanline_start = 0;
    m_scanline_end = 239;
    m_reset_value = -1;
}

HuC6260::~HuC6260()
{
    SafeDeleteArray(m_color_table);
}

void HuC6260::Init(GG_Pixel_Format pixel_format)
{
    m_pixel_format = pixel_format;
    m_color_table = new u16[512];

    InitPalettes();
    Reset();
}

void HuC6260::InitPalettes()
{
    for (int i = 0; i < 512; i++)
    {
        u8 green = ((i >> 6) & 0x07) * 255 / 7;
        u8 red = ((i >> 3) & 0x07) * 255 / 7;
        u8 blue = (i & 0x07) * 255 / 7;
        m_rgb888_palette[i][0] = red;
        m_rgb888_palette[i][1] = green;
        m_rgb888_palette[i][2] = blue;
        m_bgr888_palette[i][0] = blue;
        m_bgr888_palette[i][1] = green;
        m_bgr888_palette[i][2] = red;

        green = ((i >> 6) & 0x07) * 63 / 7;
        red = ((i >> 3) & 0x07) * 31 / 7;
        blue = (i & 0x07) * 31 / 7;
        m_rgb565_palette[i] = (red << 11) | (green << 5) | blue;
        m_bgr565_palette[i] = (blue << 11) | (green << 5) | red;

        green = ((i >> 6) & 0x07) * 31 / 7;
        red = ((i >> 3) & 0x07) * 31 / 7;
        blue = (i & 0x07) * 31 / 7;
        m_rgb555_palette[i] = (red << 10) | (green << 5) | blue;
        m_bgr555_palette[i] = (blue << 10) | (green << 5) | red;
    }
}

void HuC6260::Reset()
{
    m_control_register = 0;
    m_color_table_address = 0;
    m_speed = HuC6260_SPEED_5_36_MHZ;
    m_clock_divider = 4;
    m_hpos = 0;
    m_vpos = 0;
    m_pixel_index = 0;
    m_pixel_clock = 0;
    m_pixel_x = 0;
    m_hsync = true;
    m_vsync = true;
    m_blur = 0;
    m_black_and_white = 0;

    for (int i = 0; i < 512; i++)
    {
        if (m_reset_value < 0)
            m_color_table[i] = rand() & 0x1FF;
        else
            m_color_table[i] = m_reset_value & 0x1FF;
    }
}


void HuC6260::SaveState(std::ostream& stream)
{
    using namespace std;
    stream.write(reinterpret_cast<const char*> (&m_control_register), sizeof(m_control_register));
    stream.write(reinterpret_cast<const char*> (&m_color_table_address), sizeof(m_color_table_address));
    stream.write(reinterpret_cast<const char*> (&m_speed), sizeof(m_speed));
    stream.write(reinterpret_cast<const char*> (&m_clock_divider), sizeof(m_clock_divider));
    stream.write(reinterpret_cast<const char*> (m_color_table), sizeof(u16) * 512);
    stream.write(reinterpret_cast<const char*> (&m_hpos), sizeof(m_hpos));
    stream.write(reinterpret_cast<const char*> (&m_vpos), sizeof(m_vpos));
    stream.write(reinterpret_cast<const char*> (&m_pixel_index), sizeof(m_pixel_index));
    stream.write(reinterpret_cast<const char*> (&m_pixel_clock), sizeof(m_pixel_clock));
    stream.write(reinterpret_cast<const char*> (&m_pixel_x), sizeof(m_pixel_x));
    stream.write(reinterpret_cast<const char*> (&m_hsync), sizeof(m_hsync));
    stream.write(reinterpret_cast<const char*> (&m_vsync), sizeof(m_vsync));
    stream.write(reinterpret_cast<const char*> (&m_blur), sizeof(m_blur));
    stream.write(reinterpret_cast<const char*> (&m_black_and_white), sizeof(m_black_and_white));
}

void HuC6260::LoadState(std::istream& stream)
{
    using namespace std;
    stream.read(reinterpret_cast<char*> (&m_control_register), sizeof(m_control_register));
    stream.read(reinterpret_cast<char*> (&m_color_table_address), sizeof(m_color_table_address));
    stream.read(reinterpret_cast<char*> (&m_speed), sizeof(m_speed));
    stream.read(reinterpret_cast<char*> (&m_clock_divider), sizeof(m_clock_divider));
    stream.read(reinterpret_cast<char*> (m_color_table), sizeof(u16) * 512);
    stream.read(reinterpret_cast<char*> (&m_hpos), sizeof(m_hpos));
    stream.read(reinterpret_cast<char*> (&m_vpos), sizeof(m_vpos));
    stream.read(reinterpret_cast<char*> (&m_pixel_index), sizeof(m_pixel_index));
    stream.read(reinterpret_cast<char*> (&m_pixel_clock), sizeof(m_pixel_clock));
    stream.read(reinterpret_cast<char*> (&m_pixel_x), sizeof(m_pixel_x));
    stream.read(reinterpret_cast<char*> (&m_hsync), sizeof(m_hsync));
    stream.read(reinterpret_cast<char*> (&m_vsync), sizeof(m_vsync));
    stream.read(reinterpret_cast<char*> (&m_blur), sizeof(m_blur));
    stream.read(reinterpret_cast<char*> (&m_black_and_white), sizeof(m_black_and_white));
}