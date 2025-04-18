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

u8 HuC6260::ReadRegister(u16 address)
{
#if !defined(GG_DISABLE_DISASSEMBLER)
            m_huc6280->CheckMemoryBreakpoints(HuC6280::HuC6280_BREAKPOINT_TYPE_HUC6260_REGISTER, address & 0x07, true);
#endif

    u8 ret = 0xFF;

    switch (address & 0x07)
    {
        case 4:
            // Color table data LSB
            ret = m_color_table[m_color_table_address] & 0xFF;
            break;
        case 5:
            // Color table data MSB
#if !defined(GG_DISABLE_DISASSEMBLER)
            m_huc6280->CheckMemoryBreakpoints(HuC6280::HuC6280_BREAKPOINT_TYPE_PALETTE_RAM, m_color_table_address, true);
#endif
            ret = 0xFE | ((m_color_table[m_color_table_address] >> 8) & 0x01);
            m_color_table_address = (m_color_table_address + 1) & 0x01FF;
            break;
    }

    return ret;
}

void HuC6260::WriteRegister(u16 address, u8 value)
{
#if !defined(GG_DISABLE_DISASSEMBLER)
            m_huc6280->CheckMemoryBreakpoints(HuC6280::HuC6280_BREAKPOINT_TYPE_HUC6260_REGISTER, address & 0x07, false);
#endif

    switch (address & 0x07)
    {
        case 0:
            // Control register
            m_control_register = value;
            m_speed = m_control_register & 0x03;
            m_blur = (m_control_register >> 2) & 0x01;
            m_black_and_white = (m_control_register >> 7) & 0x01;

            switch (m_speed)
            {
                case 0:
                    Debug("HuC6260 Speed: 5.36 MHz");
                    m_clock_divider = 4;
                    break;
                case 1:
                    Debug("HuC6260 Speed: 7.16 MHz");
                    m_clock_divider = 3;
                    break;
                default:
                    Debug("HuC6260 Speed: 10.8 MHz");
                    m_clock_divider = 2;
                    break;
            }
            break;
        case 2:
            // Color table address LSB
            m_color_table_address = (m_color_table_address & 0x0100) | value;
            break;
        case 3:
            // Color table address MSB
            m_color_table_address = (m_color_table_address & 0x00FF) | ((value & 0x01) << 8);
            break;
        case 4:
            // Color table data LSB
            m_color_table[m_color_table_address] = (m_color_table[m_color_table_address] & 0x0100) | value;
            break;
        case 5:
            // Color table data MSB
#if !defined(GG_DISABLE_DISASSEMBLER)
            m_huc6280->CheckMemoryBreakpoints(HuC6280::HuC6280_BREAKPOINT_TYPE_PALETTE_RAM, m_color_table_address, false);
#endif
            m_color_table[m_color_table_address] = (m_color_table[m_color_table_address] & 0x00FF) | ((value & 0x01) << 8);
            m_color_table_address = (m_color_table_address + 1) & 0x01FF;
            break;
        default:
            // Not used
            Debug("HuC6260 Write unused register");
            break;
    }
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