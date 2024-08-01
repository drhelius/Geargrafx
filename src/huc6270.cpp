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

#include <stdlib.h>
#include "huc6270.h"
#include "huc6260.h"

HuC6270::HuC6270(HuC6280* huC6280)
{
    m_huc6280 = huC6280;
    InitPointer(m_huc6260); 
    InitPointer(m_vram);
    InitPointer(m_sat);
    m_address_register = 0;
    m_status_register = 0;
    m_read_buffer = 0;
    m_hpos = 0;
    m_vpos = 0;
    m_scanline_section = 0;
    m_trigger_sat_transfer = false;
    m_auto_sat_transfer = false;
    m_pixel_format = GG_PIXEL_RGB888;
    InitPointer(m_frame_buffer);

    for (int i = 0; i < 20; i++)
    {
        m_register[i] = 0;
    }

    m_state.AR = &m_address_register;
    m_state.SR = &m_status_register;
    m_state.R = m_register;
    m_state.READ_BUFFER = &m_read_buffer;
    m_state.HPOS = &m_hpos;
    m_state.VPOS = &m_vpos;
    m_state.SCANLINE_SECTION = &m_scanline_section;
}

HuC6270::~HuC6270()
{
    SafeDeleteArray(m_vram);
    SafeDeleteArray(m_sat);
}

void HuC6270::Init(HuC6260* huc6260, GG_Pixel_Format pixel_format)
{
    m_huc6260 = huc6260;
    m_pixel_format = pixel_format;
    m_vram = new u16[HUC6270_VRAM_SIZE];
    m_sat = new u16[HUC6270_SAT_SIZE];
    Reset();
}

void HuC6270::Reset()
{
    m_address_register = 0;
    m_status_register = 0;
    m_read_buffer = 0xFFFF;
    m_hpos = 0;
    m_vpos = 0;
    m_scanline_section = 0;
    m_trigger_sat_transfer = false;
    m_auto_sat_transfer = false;

    for (int i = 0; i < 20; i++)
    {
        m_register[i] = 0;
    }

    for (int i = 0; i < HUC6270_VRAM_SIZE; i++)
    {
        m_vram[i] = rand() & 0xFFFF;
    }

    for (int i = 0; i < HUC6270_SAT_SIZE; i++)
    {
        m_sat[i] = rand() & 0xFFFF;
    }
}

HuC6270::HuC6270_State* HuC6270::GetState()
{
    return &m_state;
}

u16* HuC6270::GetVRAM()
{
    return m_vram;
}

u16* HuC6270::GetSAT()
{
    return m_sat;
}

void HuC6270::RenderLine(int y)
{
    int screen_reg = (m_register[HUC6270_REG_MWR] >> 4) & 0x07;
    int screen_size_x = k_scren_size_x[screen_reg];
    int screen_size_x_pixels = screen_size_x * 8;
    int screen_size_y = k_scren_size_y[screen_reg];
    int screen_size_y_pixels = screen_size_y * 8;
    int buffer_line_offset = y * GG_MAX_RESOLUTION_WIDTH;
   
    int scroll_x = m_register[HUC6270_REG_BXR];
    int scroll_y = m_register[HUC6270_REG_BYR];

    int bg_y = y + scroll_y;
    bg_y %= screen_size_y_pixels;
    int bat_offset = (bg_y / 8) * screen_size_x;

    for (int line_x = 0; line_x < GG_MAX_RESOLUTION_WIDTH; line_x++)
    {
        int bg_x = line_x + scroll_x;
        bg_x %= screen_size_x_pixels;

        u16 bat_entry = m_vram[bat_offset + (bg_x / 8)];
        int tile_index = bat_entry & 0x07FF;
        int color_table = (bat_entry >> 12) & 0x0F;
        int tile_data = tile_index * 16;
        int tile_x =  (bg_x % 8);
        int tile_y = (bg_y % 8);
        int line_start_a = (tile_data + tile_y);
        int line_start_b = (tile_data + tile_y + 8);
        u8 byte1 = m_vram[line_start_a] & 0xFF;
        u8 byte2 = m_vram[line_start_a] >> 8;
        u8 byte3 = m_vram[line_start_b] & 0xFF;
        u8 byte4 = m_vram[line_start_b] >> 8;

        int color = ((byte1 >> (7 - tile_x)) & 0x01) | (((byte2 >> (7 - tile_x)) & 0x01) << 1) | (((byte3 >> (7 - tile_x)) & 0x01) << 2) | (((byte4 >> (7 - tile_x)) & 0x01) << 3);

        u16 color_value = m_huc6260->GetColorTable()[(color_table * 16) + color];
        int blue = (color_value & 0x07) * 255 / 7;
        int red = ((color_value >> 3) & 0x07) * 255 / 7;
        int green = ((color_value >> 6) & 0x07) * 255 / 7;

        int index = (buffer_line_offset + line_x) * 3;
        m_frame_buffer[index] = red;
        m_frame_buffer[index + 1] = green;
        m_frame_buffer[index + 2] = blue;
    }
}