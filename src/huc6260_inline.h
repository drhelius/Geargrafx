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
    return CLAMP(242 - m_scanline_start - (241 - m_scanline_end), 1, 242);
#endif
}

INLINE void HuC6260::SetScanlineStart(int scanline_start)
{
    m_scanline_start = CLAMP(scanline_start, 0, 241);
}

INLINE void HuC6260::SetScanlineEnd(int scanline_end)
{
    m_scanline_end = CLAMP(scanline_end, 0, 241);
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

INLINE void HuC6260::SetCompositePalette(bool enable)
{
    m_palette = enable ? 1 : 0;
}

#endif /* HUC6260_INLINE_H */