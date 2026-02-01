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

HuC6260::HuC6260(HuC6202* huc6202, HuC6280* huc6280)
{
    m_huc6280 = huc6280;
    m_huc6202 = huc6202;
    m_pixel_format = GG_PIXEL_RGBA8888;
    m_state.CR = &m_control_register;
    m_state.CTA = &m_color_table_address;
    m_state.HPOS = &m_hpos;
    m_state.VPOS = &m_vpos;
    m_state.HSYNC = &m_hsync;
    m_state.VSYNC = &m_vsync;
    m_overscan = 0;
    m_scanline_start = 0;
    m_scanline_end = 241;
    m_reset_value = -1;
    m_palette = 0;
    m_speed = HuC6260_SPEED_5_36_MHZ;
    InitPointer(m_frame_buffer);
    m_lowpass_enabled = false;
    m_lowpass_intensity = 1.0f;
    m_lowpass_cutoff_mhz = 5.0f;
    m_lowpass_speed[0] = false;
    m_lowpass_speed[1] = true;
    m_lowpass_speed[2] = true;

    CalculateScreenBounds();
}

HuC6260::~HuC6260()
{
}

void HuC6260::Init(GG_Pixel_Format pixel_format)
{
    m_pixel_format = pixel_format;
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
        m_rgba888_palette[0][i][0] = red;
        m_rgba888_palette[0][i][1] = green;
        m_rgba888_palette[0][i][2] = blue;
        m_rgba888_palette[0][i][3] = 255;

        // Custom palette defaults to standard RGB
        m_rgba888_palette[2][i][0] = red;
        m_rgba888_palette[2][i][1] = green;
        m_rgba888_palette[2][i][2] = blue;
        m_rgba888_palette[2][i][3] = 255;

        green = ((i >> 6) & 0x07) * 63 / 7;
        red = ((i >> 3) & 0x07) * 31 / 7;
        blue = (i & 0x07) * 31 / 7;
        u16 rgb565 = (red << 11) | (green << 5) | blue;
        m_rgb565_palette[0][i] = rgb565;
        m_rgb565_palette[2][i] = rgb565;

        m_rgba888_palette[1][i][0] = k_rgb888_palette_composite[i][0];
        m_rgba888_palette[1][i][1] = k_rgb888_palette_composite[i][1];
        m_rgba888_palette[1][i][2] = k_rgb888_palette_composite[i][2];
        m_rgba888_palette[1][i][3] = 255;

        green = k_rgb888_palette_composite[i][1] * 63 / 255;
        red = k_rgb888_palette_composite[i][0] * 31 / 255;
        blue = k_rgb888_palette_composite[i][2] * 31 / 255;
        rgb565 = (red << 11) | (green << 5) | blue;
        m_rgb565_palette[1][i] = rgb565;
    }
}

void HuC6260::Reset()
{
    m_control_register = 0;
    m_color_table_address = 0;
    m_speed = HuC6260_SPEED_5_36_MHZ;
    m_clock_divider = 4;
    m_multiple_speeds = false;
    m_scaled_width = false;
    m_hpos = 0;
    m_vpos = 0;
    m_pixel_index = 0;
    m_pixel_x = 0;
    m_hsync = true;
    m_vsync = true;
    m_blur = 0;
    m_black_and_white = 0;
    m_active_line = false;

    for (int i = 0; i < 512; i++)
    {
        if (m_reset_value < 0)
        {
            bool random = (rand() & 0x1);
            u16 and_value = random ? 0x1D0 : 0x1F1;
            u16 or_value = random ? 0x1C0 : 0x1FC;
            m_color_table[i] = (((rand() & and_value) ) | or_value);
            m_color_table[i] = MIN(m_color_table[i] + ((i & 0xFF) >> 2), 0x1FF);
        }
        else
            m_color_table[i] = m_reset_value & 0x1FF;
    }

    CalculateScreenBounds();
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
        {
            // Control register
            m_control_register = value;

            m_blur = (m_control_register >> 2) & 0x01;
            m_black_and_white = (m_control_register >> 7) & 0x01;

            s32 old_speed = m_speed;
            m_speed = m_control_register & 0x03;

            if (old_speed != m_speed)
            {
                m_screen_start_x = k_huc6260_line_start[m_overscan][m_speed];
                m_screen_end_x = k_huc6260_line_end[m_overscan][m_speed];
                m_multiple_speeds = true;
            }

            switch (m_speed)
            {
                case 0:
                    //Debug("HuC6260 Speed: 5.36 MHz");
                    m_clock_divider = 4;
                    break;
                case 1:
                    //Debug("HuC6260 Speed: 7.16 MHz");
                    m_clock_divider = 3;
                    break;
                default:
                    //Debug("HuC6260 Speed: 10.8 MHz");
                    m_clock_divider = 2;
                    break;
            }
            break;
        }
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

void HuC6260::CalculateScreenBounds()
{
    m_screen_start_x = k_huc6260_line_start[m_overscan][m_speed];
    m_screen_end_x = k_huc6260_line_end[m_overscan][m_speed];
    m_screen_start_y = m_scanline_start + HUC6270_LINES_TOP_BLANKING;
    m_screen_end_y = m_scanline_end + HUC6270_LINES_TOP_BLANKING + 1;
}

void HuC6260::AdjustForMultipleDividers()
{
    int bytes_per_pixel = 2;
    if (m_pixel_format == GG_PIXEL_RGBA8888)
        bytes_per_pixel = 4;

    int dominant_width = k_huc6260_scaling_width[m_overscan];
    u8* src_ptr = m_frame_buffer;
    int end_line = 242 - m_scanline_start;

    for (int line = 0; line < end_line; line++)
    {
        s32 original_line_speed = m_line_speed[line + m_scanline_start];
        int original_line_width = k_huc6260_line_width[m_overscan][original_line_speed];

        u8* dest_line = m_scale_buffer + ((line * dominant_width) * bytes_per_pixel);

        switch (original_line_speed)
        {
        case 0:
            for (int pixel = 0; pixel < original_line_width; pixel++)
            {
                for (int s = 0; s < 4; s++)
                {
                    memcpy(dest_line + (((pixel * 4) + s) * bytes_per_pixel),
                           src_ptr + (pixel * bytes_per_pixel),
                           bytes_per_pixel);
                }
            }
            break;
        case 1:
            for (int pixel = 0; pixel < original_line_width; pixel++)
            {
                for (int s = 0; s < 3; s++)
                {
                    memcpy(dest_line + (((pixel * 3) + s) * bytes_per_pixel),
                           src_ptr + (pixel * bytes_per_pixel),
                           bytes_per_pixel);
                }
            }
            memcpy(dest_line + ((original_line_width * 3) * bytes_per_pixel),
                   src_ptr + (original_line_width * bytes_per_pixel),
                   bytes_per_pixel);
            break;
        default:
            for (int pixel = 0; pixel < original_line_width; pixel++)
            {
                for (int s = 0; s < 2; s++)
                {
                    memcpy(dest_line + (((pixel * 2) + s) * bytes_per_pixel),
                           src_ptr + (pixel * bytes_per_pixel),
                           bytes_per_pixel);
                }
            }
            break;
        }

        src_ptr += (original_line_width * bytes_per_pixel);
    }

    memcpy(m_frame_buffer, m_scale_buffer, dominant_width * 242 * bytes_per_pixel);
}

void HuC6260::SetCustomPalette(const u8* data)
{
    for (int i = 0; i < 512; i++)
    {
        u8 red = data[i * 3];
        u8 green = data[i * 3 + 1];
        u8 blue = data[i * 3 + 2];

        m_rgba888_palette[2][i][0] = red;
        m_rgba888_palette[2][i][1] = green;
        m_rgba888_palette[2][i][2] = blue;
        m_rgba888_palette[2][i][3] = 255;

        u8 green565 = green * 63 / 255;
        u8 red565 = red * 31 / 255;
        u8 blue565 = blue * 31 / 255;
        u16 rgb565 = (red565 << 11) | (green565 << 5) | blue565;
        m_rgb565_palette[2][i] = rgb565;
    }
}

void HuC6260::SetLowPassFilter(bool enabled, float intensity, float cutoff_mhz, bool speed_5_36, bool speed_7_16, bool speed_10_8)
{
    m_lowpass_enabled = enabled;
    m_lowpass_intensity = intensity;
    m_lowpass_cutoff_mhz = cutoff_mhz;
    m_lowpass_speed[0] = speed_5_36;
    m_lowpass_speed[1] = speed_7_16;
    m_lowpass_speed[2] = speed_10_8;
}

template <int bytes_per_pixel>
void HuC6260::ApplyLowPassFilter()
{
    static const float k_speed_mhz[4] = { 5.36f, 7.16f, 10.8f, 10.8f };

    int speed_index = MIN(m_speed, 2);
    if (!m_lowpass_speed[speed_index])
        return;

    float dot_clock = k_speed_mhz[m_speed];
    float base_alpha = m_lowpass_cutoff_mhz / dot_clock;
    base_alpha = CLAMP(base_alpha, 0.3f, 1.0f);
    float alpha = base_alpha + (1.0f - base_alpha) * (1.0f - m_lowpass_intensity);

    if (alpha >= 1.0f)
        return;

    int width = m_scaled_width ? k_huc6260_scaling_width[m_overscan] : k_huc6260_line_width[m_overscan][m_speed];
    int height = GetCurrentHeight();

    for (int y = 0; y < height; y++)
    {
        int line_offset = y * width * bytes_per_pixel;
        float r_prev, g_prev, b_prev;

        if (bytes_per_pixel == 2)
        {
            u16 first_pixel = *reinterpret_cast<u16*>(m_frame_buffer + line_offset);
            r_prev = ((first_pixel >> 11) & 0x1F) * 255.0f / 31.0f;
            g_prev = ((first_pixel >> 5) & 0x3F) * 255.0f / 63.0f;
            b_prev = (first_pixel & 0x1F) * 255.0f / 31.0f;
        }
        else
        {
            r_prev = m_frame_buffer[line_offset + 0];
            g_prev = m_frame_buffer[line_offset + 1];
            b_prev = m_frame_buffer[line_offset + 2];
        }

        for (int x = 0; x < width; x++)
        {
            int idx = line_offset + x * bytes_per_pixel;
            float r, g, b;

            if (bytes_per_pixel == 2)
            {
                u16 pixel = *reinterpret_cast<u16*>(m_frame_buffer + idx);
                r = ((pixel >> 11) & 0x1F) * 255.0f / 31.0f;
                g = ((pixel >> 5) & 0x3F) * 255.0f / 63.0f;
                b = (pixel & 0x1F) * 255.0f / 31.0f;
            }
            else
            {
                r = m_frame_buffer[idx + 0];
                g = m_frame_buffer[idx + 1];
                b = m_frame_buffer[idx + 2];
            }

            r = alpha * r + (1.0f - alpha) * r_prev;
            g = alpha * g + (1.0f - alpha) * g_prev;
            b = alpha * b + (1.0f - alpha) * b_prev;

            if (bytes_per_pixel == 2)
            {
                u8 r8 = (u8)r;
                u8 g8 = (u8)g;
                u8 b8 = (u8)b;
                u16 pixel = ((r8 * 31 / 255) << 11) | ((g8 * 63 / 255) << 5) | (b8 * 31 / 255);
                *reinterpret_cast<u16*>(m_frame_buffer + idx) = pixel;
            }
            else
            {
                m_frame_buffer[idx + 0] = (u8)r;
                m_frame_buffer[idx + 1] = (u8)g;
                m_frame_buffer[idx + 2] = (u8)b;
            }

            r_prev = r; g_prev = g; b_prev = b;
        }
    }
}

template void HuC6260::ApplyLowPassFilter<2>();
template void HuC6260::ApplyLowPassFilter<4>();

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
    stream.write(reinterpret_cast<const char*> (&m_pixel_x), sizeof(m_pixel_x));
    stream.write(reinterpret_cast<const char*> (&m_hsync), sizeof(m_hsync));
    stream.write(reinterpret_cast<const char*> (&m_vsync), sizeof(m_vsync));
    stream.write(reinterpret_cast<const char*> (&m_blur), sizeof(m_blur));
    stream.write(reinterpret_cast<const char*> (&m_black_and_white), sizeof(m_black_and_white));
    stream.write(reinterpret_cast<const char*> (&m_multiple_speeds), sizeof(m_multiple_speeds));
    stream.write(reinterpret_cast<const char*> (&m_active_line), sizeof(m_active_line));
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
    stream.read(reinterpret_cast<char*> (&m_pixel_x), sizeof(m_pixel_x));
    stream.read(reinterpret_cast<char*> (&m_hsync), sizeof(m_hsync));
    stream.read(reinterpret_cast<char*> (&m_vsync), sizeof(m_vsync));
    stream.read(reinterpret_cast<char*> (&m_blur), sizeof(m_blur));
    stream.read(reinterpret_cast<char*> (&m_black_and_white), sizeof(m_black_and_white));
    stream.read(reinterpret_cast<char*> (&m_multiple_speeds), sizeof(m_multiple_speeds));
    stream.read(reinterpret_cast<char*> (&m_active_line), sizeof(m_active_line));

    CalculateScreenBounds();
}