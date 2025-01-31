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
    m_hsync = false;
    m_vsync = false;
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

bool HuC6260::Clock()
{
    bool frame_ready = false;

    if (m_pixel_clock == 0)
    {
        u16 pixel = m_huc6270->Clock();

#if defined(HUC6260_DEBUG)
        int start_x = 0;
        int end_x = k_huc6260_full_line_width[m_speed];
        int start_y = 0;
        int end_y = 263;
#else
        int start_x = k_huc6260_line_offset[m_overscan][m_speed];
        int end_x = start_x + k_huc6260_line_width[m_overscan][m_speed];
        int start_y = m_scanline_start + 14;
        int end_y = m_scanline_end + 14 + 1;
#endif
        if ((m_pixel_x >= start_x) && (m_pixel_x < end_x) && (m_vpos >= start_y) && (m_vpos < end_y))
        {
            if ((pixel & 0x10F) == 0)
                pixel = 0;

            WritePixel(pixel);
        }

        m_pixel_x = (m_pixel_x + 1) % k_huc6260_full_line_width[m_speed];
    }

    m_pixel_clock = (m_pixel_clock + 1) % m_clock_divider;
    m_hpos = (m_hpos + 1) % HUC6260_LINE_LENGTH;

    if (m_hpos == 0)
        m_pixel_x = 0;

    switch (m_hpos)
    {
        case HUC6260_HSYNC_START_HPOS:
            // Start of horizontal sync
            m_hsync = false;
            m_huc6270->SetHSync(false);
            break;
        case HUC6260_HSYNC_END_HPOS:
            // End of horizontal sync
            m_hsync = true;
            m_huc6270->SetHSync(true);
            if (m_vpos == (k_huc6260_total_lines[m_blur] - 1))
            {
                m_pixel_index = 0;
                frame_ready = true;
            }
            m_vpos = (m_vpos + 1) % k_huc6260_total_lines[m_blur];
            m_pixel_clock = 0;
            break;
        case HUC6260_VSYNC_HPOS:
            // Start of vertical sync
            if (m_vpos == (k_huc6260_total_lines[m_blur] - 4))
            {
                m_vsync = false;
                m_huc6270->SetVSync(false);
            }
            // End of vertical sync
            else if (m_vpos == (k_huc6260_total_lines[m_blur] - 1))
            {
                m_vsync = true;
                m_huc6270->SetVSync(true);
            }
            break;
    }

    return frame_ready;
}

HuC6260::HuC6260_State* HuC6260::GetState()
{
    return &m_state;
}

HuC6260::HuC6260_Speed HuC6260::GetSpeed()
{
    return k_huc6260_speed[m_speed];
}

int HuC6260::GetClockDivider()
{
    return m_clock_divider;
}

u16* HuC6260::GetColorTable()
{
    return m_color_table;
}

void HuC6260::SetBuffer(u8* frame_buffer)
{
    m_frame_buffer = frame_buffer;
}

u8* HuC6260::GetBuffer()
{
    return m_frame_buffer;
}

int HuC6260::GetCurrentLineWidth()
{
#if defined(HUC6260_DEBUG)
    return k_huc6260_full_line_width[m_speed];
#else
    return k_huc6260_line_width[m_overscan][m_speed];
#endif
}

int HuC6260::GetCurrentHeight()
{
#if defined(HUC6260_DEBUG)
    return 263;
#else
    return std::min(240, std::max(1, 240 - m_scanline_start - (239 - m_scanline_end)));
#endif
}

void HuC6260::SetScanlineStart(int scanline_start)
{
    m_scanline_start = std::max(0, std::min(239, scanline_start));
}

void HuC6260::SetScanlineEnd(int scanline_end)
{
    m_scanline_end = std::max(0, std::min(239, scanline_end));
}

void HuC6260::SetOverscan(bool overscan)
{
    m_overscan = overscan ? 1 : 0;
}

GG_Pixel_Format HuC6260::GetPixelFormat()
{
    return m_pixel_format;
}

void HuC6260::SetResetValue(int value)
{
    m_reset_value = value;
}

void HuC6260::WritePixel(u16 pixel)
{
    assert(pixel < 512);

    if (pixel >= 512)
    {
        Debug("HuC6260: Invalid pixel value %04X\n", pixel);
        pixel = 0;
    }

    u16 color = m_color_table[pixel];

    assert(color < 512);

    if (color >= 512)
    {
        Debug("HuC6260: Invalid color value %04X\n", color);
        color = 0;
    }

    switch (m_pixel_format)
    {
        case GG_PIXEL_RGB565:
            {
                int byte = m_pixel_index * 2;
                u16 color_16 = m_rgb565_palette[color];
                m_frame_buffer[byte + 0] = color_16 & 0xFF;
                m_frame_buffer[byte + 1] = (color_16 >> 8) & 0xFF;
            }
            break;
        case GG_PIXEL_RGBA8888:
            {
                int byte = m_pixel_index * 4;
                u8 red = m_rgb888_palette[color][0];
                u8 green = m_rgb888_palette[color][1];
                u8 blue = m_rgb888_palette[color][2];
                m_frame_buffer[byte + 0] = red;
                m_frame_buffer[byte + 1] = green;
                m_frame_buffer[byte + 2] = blue;
                m_frame_buffer[byte + 3] = 255;
            }
            break;
        case GG_PIXEL_RGB555:
            {
                int byte = m_pixel_index * 2;
                u16 color_16 = m_rgb555_palette[color];
                m_frame_buffer[byte + 0] = color_16 & 0xFF;
                m_frame_buffer[byte + 1] = (color_16 >> 8) & 0xFF;
            }
            break;
        case GG_PIXEL_BGR565:
            {
                int byte = m_pixel_index * 2;
                u16 color_16 = m_bgr565_palette[color];
                m_frame_buffer[byte + 0] = color_16 & 0xFF;
                m_frame_buffer[byte + 1] = (color_16 >> 8) & 0xFF;
            }
            break;
        case GG_PIXEL_BGR555:
            {
                int byte = m_pixel_index * 2;
                u16 color_16 = m_bgr555_palette[color];
                m_frame_buffer[byte + 0] = color_16 & 0xFF;
                m_frame_buffer[byte + 1] = (color_16 >> 8) & 0xFF;
            }
            break;
        case GG_PIXEL_BGRA8888:
            {
                int byte = m_pixel_index * 4;
                u8 blue = m_bgr888_palette[color][0];
                u8 green = m_bgr888_palette[color][1];
                u8 red = m_bgr888_palette[color][2];
                m_frame_buffer[byte + 0] = blue;
                m_frame_buffer[byte + 1] = green;
                m_frame_buffer[byte + 2] = red;
                m_frame_buffer[byte + 3] = 255;
            }
            break;
    }

    m_pixel_index++;
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