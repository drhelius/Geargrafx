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
#include "huc6202.h"
#include "huc6280.h"

INLINE bool HuC6260::Clock(u32 cycles)
{
    bool frame_ready = false;

    while (cycles > 0)
    {
        u32 cycles_to_next_pixel = m_clock_divider - (m_hpos % m_clock_divider);
        if (cycles_to_next_pixel > cycles)
            cycles_to_next_pixel = cycles;

        u32 cycles_to_line_end = HUC6260_LINE_LENGTH - m_hpos;
        if (cycles_to_line_end > cycles)
            cycles_to_line_end = cycles;

        u32 cycles_to_hsync = (m_hpos < HUC6260_HSYNC_END_HPOS)
            ? (HUC6260_HSYNC_END_HPOS - m_hpos)
            : (HUC6260_LINE_LENGTH - m_hpos + HUC6260_HSYNC_END_HPOS);
        if (cycles_to_hsync > cycles)
            cycles_to_hsync = cycles;

        u32 step = cycles_to_next_pixel;
        if (cycles_to_line_end < step)
            step = cycles_to_line_end;
        if (cycles_to_hsync < step)
            step = cycles_to_hsync;

        m_hpos += step;
        cycles -= step;

        if ((m_hpos % m_clock_divider) == 0)
        {
            m_pixel_x++;
            if (m_pixel_x == k_huc6260_full_line_width[m_speed])
                m_pixel_x = 0;

            if (m_is_sgx)
            {
                u16 pixel_1, pixel_2;
                m_huc6202->ClockSGX(&pixel_1, &pixel_2);

                if (m_active_line && (m_pixel_x >= m_screen_start_x) && (m_pixel_x < m_screen_end_x))
                {
                    u16 win_1_width = m_huc6202->GetWindow1Width();
                    u16 win_2_width = m_huc6202->GetWindow2Width();

                    int in_win_1 = (win_1_width >= 0x40) && (m_pixel_x < win_1_width);
                    int in_win_2 = ((win_2_width >= 0x40) && (m_pixel_x < win_2_width)) << 1;
                    int win_mode = in_win_1 | in_win_2;

                    HuC6202::HuC6202_Window_Priority* priorities = m_huc6202->GetWindowPriorities();
                    HuC6202::HuC6202_Window_Priority* priority = &priorities[win_mode];

                    int vdc_1_enabled = priority->vdc_1_enabled;
                    int vdc_2_enabled = priority->vdc_2_enabled << 1;
                    int vdcs_enabled = vdc_1_enabled | vdc_2_enabled;

                    u16 final_pixel;

                    if (vdcs_enabled == 0)
                        final_pixel = 0;
                    else if (vdcs_enabled == 1)
                        final_pixel = pixel_1;
                    else if (vdcs_enabled == 2)
                        final_pixel = pixel_2;
                    else
                    {
                        bool is_pixel_1_transparent = (pixel_1 & 0x0F) == 0;
                        bool is_vdc_1_sprite = pixel_1 & 0x100;
                        bool is_vdc_2_sprite = pixel_2 & 0x100;

                        switch (priority->priority_mode)
                        {
                            case HuC6202::HuC6270_PRIORITY_DEFAULT:
                                final_pixel = is_pixel_1_transparent ? pixel_2 : pixel_1;
                                break;
                            case HuC6202::HuC6270_PRIORITY_SPRITES_2_ABOVE_BG_1:
                                if (is_pixel_1_transparent || (is_vdc_2_sprite && !is_vdc_1_sprite))
                                    final_pixel = pixel_2;
                                else
                                    final_pixel = pixel_1;
                                break;
                            case HuC6202::HuC6270_PRIORITY_SPRITES_1_BELOW_BG_2:
                            {
                                bool is_pixel_2_transparent = (pixel_2 & 0x0F) == 0;
                                if (is_pixel_1_transparent || (is_vdc_1_sprite && !is_vdc_2_sprite && !is_pixel_2_transparent))
                                    final_pixel = pixel_2;
                                else
                                    final_pixel = pixel_1;
                                break;
                            }
                        }
                    }

                    m_vce_buffer[m_pixel_index] = m_color_table[final_pixel];
                    m_pixel_index++;
                }
            }
            else
            {
                u16 pixel = m_huc6202->Clock();
                if (m_active_line && (m_pixel_x >= m_screen_start_x) && (m_pixel_x < m_screen_end_x))
                {
                    m_vce_buffer[m_pixel_index] = m_color_table[pixel];
                    m_pixel_index++;
                }
            }
        }

        // End of line
        if (m_hpos >= HUC6260_LINE_LENGTH)
        {
            m_hpos = 0;
            m_pixel_x = 0;
        }

        if (m_hpos == HUC6260_HSYNC_END_HPOS)
        {
            m_hsync = true;
            m_huc6202->SetHSyncHigh();

            // Start of vertical sync
            if (m_vpos == (k_huc6260_total_lines[m_blur] - 4))
            {
                m_vsync = false;
                m_huc6202->SetVSyncLow();
            }
            // End of vertical sync
            else if (m_vpos == (k_huc6260_total_lines[m_blur] - 1))
            {
                RenderFrame();
                m_vsync = true;
                m_pixel_index = 0;
                frame_ready = true;
                m_scaled_width = m_multiple_speeds;
                if (m_multiple_speeds)
                {
                    m_multiple_speeds = false;
                    AdjustForMultipleDividers();
                }
            }

            if(m_vpos >= 14 && m_vpos < 256)
                m_line_speed[m_vpos - 14] = m_speed;
            m_vpos++;
            if (m_vpos == k_huc6260_total_lines[m_blur])
                m_vpos = 0;

            m_active_line = (m_vpos >= m_screen_start_y) && (m_vpos < m_screen_end_y);
        }
        else if (m_hpos == HUC6260_HSYNC_START_HPOS)
        {
            m_hsync = false;
        }
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

INLINE int HuC6260::GetTotalLines()
{
    return k_huc6260_total_lines[m_blur];
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

INLINE int HuC6260::GetCurrentWidth()
{
    if (m_scaled_width)
        return k_huc6260_scaling_width[m_overscan];
    else
        return k_huc6260_line_width[m_overscan][m_speed];
}

INLINE int HuC6260::GetCurrentHeight()
{
    return CLAMP(HUC6270_LINES_ACTIVE - m_scanline_start - ((HUC6270_LINES_ACTIVE - 1) - m_scanline_end), 1, HUC6270_LINES_ACTIVE);
}

INLINE int HuC6260::GetWidthScale()
{
    return m_scaled_width ? 3 : 1;
}

INLINE void HuC6260::SetScanlineStart(int scanline_start)
{
    m_scanline_start = CLAMP(scanline_start, 0, HUC6270_LINES_ACTIVE - 1);
    m_screen_start_y = m_scanline_start + HUC6270_LINES_TOP_BLANKING;
}

INLINE void HuC6260::SetScanlineEnd(int scanline_end)
{
    m_scanline_end = CLAMP(scanline_end, 0, HUC6270_LINES_ACTIVE - 1);
    m_screen_end_y = m_scanline_end + HUC6270_LINES_TOP_BLANKING + 1;
}

INLINE void HuC6260::SetOverscan(bool overscan)
{
    m_overscan = overscan ? 1 : 0;
    m_screen_start_x = k_huc6260_line_start[m_overscan][m_speed];
    m_screen_end_x = k_huc6260_line_end[m_overscan][m_speed];
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