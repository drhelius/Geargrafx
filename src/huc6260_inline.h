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

#ifndef HUC6260_INLINE_H
#define HUC6260_INLINE_H

#include "huc6260.h"
#include "huc6270.h"
#include "huc6280.h"

INLINE u8 HuC6260::ReadRegister(u16 address)
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

INLINE void HuC6260::WriteRegister(u16 address, u8 value)
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

INLINE bool HuC6260::Clock()
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

INLINE HuC6260::HuC6260_State* HuC6260::GetState()
{
    return &m_state;
}

INLINE HuC6260::HuC6260_Speed HuC6260::GetSpeed()
{
    return k_huc6260_speed[m_speed];
}

INLINE int HuC6260::GetClockDivider()
{
    return m_clock_divider;
}

INLINE u16* HuC6260::GetColorTable()
{
    return m_color_table;
}

INLINE void HuC6260::SetBuffer(u8* frame_buffer)
{
    m_frame_buffer = frame_buffer;
}

INLINE u8* HuC6260::GetBuffer()
{
    return m_frame_buffer;
}

INLINE int HuC6260::GetCurrentLineWidth()
{
#if defined(HUC6260_DEBUG)
    return k_huc6260_full_line_width[m_speed];
#else
    return k_huc6260_line_width[m_overscan][m_speed];
#endif
}

INLINE int HuC6260::GetCurrentHeight()
{
#if defined(HUC6260_DEBUG)
    return 263;
#else
    return CLAMP(240 - m_scanline_start - (239 - m_scanline_end), 1, 240);
#endif
}

INLINE void HuC6260::SetScanlineStart(int scanline_start)
{
    m_scanline_start = CLAMP(scanline_start, 0, 239);
}

INLINE void HuC6260::SetScanlineEnd(int scanline_end)
{
    m_scanline_end = CLAMP(scanline_end, 0, 239);
}

INLINE void HuC6260::SetOverscan(bool overscan)
{
    m_overscan = overscan ? 1 : 0;
}

INLINE GG_Pixel_Format HuC6260::GetPixelFormat()
{
    return m_pixel_format;
}

INLINE void HuC6260::SetResetValue(int value)
{
    m_reset_value = value;
}

INLINE void HuC6260::WritePixel(u16 pixel)
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

#endif /* HUC6260_INLINE_H */