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

#include "huc6260.h"

HuC6260::HuC6260(HuC6270* huc6270)
{
    m_huc6270 = huc6270;
    m_pixel_format = GG_PIXEL_RGB888;
    m_state.CR = &m_control_register;
    m_state.CTA = &m_color_table_address;
    m_state.HPOS = &m_hpos;
    m_state.VPOS = &m_vpos;
    m_state.PIXEL_INDEX = &m_pixel_index;
    m_state.HSYNC = &m_hsync;
    m_state.VSYNC = &m_vsync;
    InitPointer(m_color_table);
    InitPointer(m_frame_buffer);
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

        green = ((i >> 6) & 0x07) * 31 / 7;
        red = ((i >> 3) & 0x07) * 63 / 7;
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
    m_vpos = HUC6260_VSYNC_START_VPOS;
    m_pixel_index = 0;
    m_pixel_clock = 0;
    m_hsync = true;
    m_vsync = true;

    for (int i = 0; i < 512; i++)
    {
        m_color_table[i] = ((i ^ (i >> 3)) & 1) ? 0x000 : 0x1FF;
    }
}

bool HuC6260::Clock()
{
    bool frame_ready = false;

    if (m_pixel_clock == 0)
    {
        bool active = false;
        u16 pixel = m_huc6270->Clock(&active);

        if (active)
        {
            if ((pixel & 0x10F) == 0)
                pixel = 0;

            u16 color = m_color_table[pixel];

            u8 red = m_rgb888_palette[color][0];
            u8 green = m_rgb888_palette[color][1];
            u8 blue = m_rgb888_palette[color][2];

            if (m_pixel_index >= (256 * 240))
                Debug("Pixel Index: %d\n", m_pixel_index);
            else
            {
                m_frame_buffer[((m_pixel_index) * 3) + 0] = red;
                m_frame_buffer[((m_pixel_index) * 3) + 1] = green;
                m_frame_buffer[((m_pixel_index) * 3) + 2] = blue;
            }

            m_pixel_index++;
        }
    }

    m_pixel_clock = (m_pixel_clock + 1) % m_clock_divider;
    m_hpos = (m_hpos + 1) % HUC6260_LINE_LENGTH;

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
            if (m_vpos == 262)
            {
                m_pixel_index = 0;
                frame_ready = true;
            }
            m_vpos = (m_vpos + 1) % HUC6260_LINES;
            m_pixel_clock = 0;
            break;
        case HUC6260_VSYNC_HPOS:
            // Start of vertical sync
            if (m_vpos == HUC6260_VSYNC_START_VPOS)
            {
                m_vsync = false;
                m_huc6270->SetVSync(false);
            }
            // End of vertical sync
            else if (m_vpos == HUC6260_VSYNC_END_VPOS)
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
    return m_speed;
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