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
#include <assert.h>
#include "huc6270.h"

HuC6270::HuC6270(HuC6280* huC6280)
{
    m_huc6280 = huC6280;
    InitPointer(m_vram);
    InitPointer(m_sat);
    m_state.AR = &m_address_register;
    m_state.SR = &m_status_register;
    m_state.R = m_register;
    m_state.READ_BUFFER = &m_read_buffer;
    m_state.HPOS = &m_hpos;
    m_state.VPOS = &m_vpos;
    m_state.V_STATE = &m_v_state;
    m_state.H_STATE = &m_h_state;
}

HuC6270::~HuC6270()
{
    SafeDeleteArray(m_vram);
    SafeDeleteArray(m_sat);
}

void HuC6270::Init()
{
    m_vram = new u16[HUC6270_VRAM_SIZE];
    m_sat = new u16[HUC6270_SAT_SIZE];
    Reset();
}

void HuC6270::Reset()
{
    for (int i = 0; i < 20; i++)
    {
        m_register[i] = 0;
    }

    m_address_register = 0;
    m_status_register = 0;
    m_read_buffer = 0xFFFF;
    m_trigger_sat_transfer = false;
    m_sat_transfer_pending = 0;
    m_hpos = 0;
    m_vpos = 0;
    m_bg_offset_y = 0;
    m_bg_counter_y = 0;
    m_increment_bg_counter_y = false;
    m_raster_line = 0;
    m_latched_bxr = 0;
    m_latched_hds = HUC6270_VAR_HDS;
    m_latched_hdw = HUC6270_VAR_HDW;
    m_latched_hde = HUC6270_VAR_HDE;
    m_latched_hsw = HUC6270_VAR_HSW;
    m_latched_vds = HUC6270_VAR_VDS;
    m_latched_vdw = HUC6270_VAR_VDW;
    m_latched_vcr = HUC6270_VAR_VCR;
    m_latched_vsw = HUC6270_VAR_VSW;
    m_latched_mwr = 0;
    m_v_state = HuC6270_VERTICAL_STATE_VDS;
    m_h_state = HuC6270_HORIZONTAL_STATE_HDS_1;
    m_lines_to_next_v_state = m_latched_vds + 2;
    m_clocks_to_next_h_state = 1;
    m_vblank_triggered = false;
    m_active_line = false;
    m_line_buffer_index = 0;
    m_no_sprite_limit = false;
    m_sprite_count = 0;

    for (int i = 0; i < HUC6270_VRAM_SIZE; i++)
    {
        m_vram[i] = 0;
    }

    for (int i = 0; i < HUC6270_SAT_SIZE; i++)
    {
        m_sat[i] = 0;
    }

    for (int i = 0; i < 1024; i++)
    {
        m_line_buffer[i] = 0;
        m_line_buffer_sprites[i] = 0;
    }

    for (int i = 0; i < 128; i++)
    {
        m_sprites[i].x = 0;
        m_sprites[i].flags = 0;
        m_sprites[i].palette = 0;
        for (int j = 0; j < 4; j++)
        {
            m_sprites[i].data[j] = 0;
        }
    }
}

void HuC6270::RenderBackground(int width)
{
    for (int i = 0; i < width; i++)
    {
        int screen_reg = (m_latched_mwr >> 4) & 0x07;
        int screen_size_x = k_huc6270_screen_size_x[screen_reg];
        int screen_size_x_pixels = k_huc6270_screen_size_x_pixels[screen_reg];
        int screen_size_y_pixels = k_huc6270_screen_size_y_pixels[screen_reg];

        int bg_y = m_bg_offset_y;
        bg_y %= screen_size_y_pixels;
        int bat_offset = (bg_y >> 3) * screen_size_x;

        int bg_x = m_latched_bxr + i;
        bg_x %= screen_size_x_pixels;

        u16 bat_entry = m_vram[bat_offset + (bg_x >> 3)];
        int tile_index = bat_entry & 0x07FF;
        int color_table = (bat_entry >> 12) & 0x0F;
        int tile_data = tile_index << 4;
        int tile_x = 7 - (bg_x & 7);
        int tile_y = (bg_y & 7);
        int line_start_a = (tile_data + tile_y);
        int line_start_b = (tile_data + tile_y + 8);
        u8 byte1 = m_vram[line_start_a] & 0xFF;
        u8 byte2 = m_vram[line_start_a] >> 8;
        u8 byte3 = m_vram[line_start_b] & 0xFF;
        u8 byte4 = m_vram[line_start_b] >> 8;

        m_line_buffer[i] = color_table << 4;

        m_line_buffer[i] |= ((byte1 >> tile_x) & 0x01) | (((byte2 >> tile_x) & 0x01) << 1) | (((byte3 >> tile_x) & 0x01) << 2) | (((byte4 >> tile_x) & 0x01) << 3);
    }
}

void HuC6270::RenderSprites(int width)
{
    for (int i = 0; i < width; i++)
    {
        m_line_buffer_sprites[i] = 0;
    }

    for(int i = (m_sprite_count - 1) ; i >= 0; i--)
    {
        int pos = m_sprites[i].x - 0x20;
        u16 plane1 = m_sprites[i].data[0];
        u16 plane2 = m_sprites[i].data[1];
        u16 plane3 = m_sprites[i].data[2];
        u16 plane4 = m_sprites[i].data[3];

        for(int x = 0; x < 16; x++)
        {
            int pixel_x;
            if (m_sprites[i].flags & 0x0800)
                pixel_x = x & 0xF;
            else
                pixel_x = 15 - (x & 0xF);

            u16 pixel = ((plane1 >> pixel_x) & 0x01) | (((plane2 >> pixel_x) & 0x01) << 1) | (((plane3 >> pixel_x) & 0x01) << 2) | (((plane4 >> pixel_x) & 0x01) << 3);

            if(pixel & 0x0F)
            {
                int x_in_screen = pos + x;
                if((x_in_screen < 0) || (x_in_screen >= width))
                    continue;

                bool priority = (m_sprites[i].flags & 0x0080);

                if (!priority && (m_line_buffer[x_in_screen] & 0x0F))
                    pixel = 0;
                else
                    pixel |= m_sprites[i].palette;

                pixel |= 0x100;

                if ((m_sprites[i].index == 0) && (m_line_buffer_sprites[x_in_screen] & 0x0F))
                    SpriteCollisionIRQ();

                m_line_buffer_sprites[x_in_screen] = pixel;
            }
        }
    }

    for (int i = 0; i < width; i++)
    {
        if(m_line_buffer_sprites[i] & 0x0F)
            m_line_buffer[i] = m_line_buffer_sprites[i];
    }
}

void HuC6270::FetchSprites()
{
    m_sprite_count = 0;

    for (int i = 0; i < 64; i++)
    {
        int sprite_offset = i << 2;
        int sprite_y = (m_sat[sprite_offset + 0] & 0x3FF) - 64;
        u16 flags = m_sat[sprite_offset + 3];
        int cgy = (flags >> 12) & 0x03;
        u16 height = k_huc6270_sprite_height[cgy];

        if ((sprite_y <= m_raster_line) && ((sprite_y + height) > m_raster_line))
        {
            int y = m_raster_line - sprite_y;
            if (y >= height)
                continue;

            if (m_sprite_count >= 16)
            {
                OverflowIRQ();
                if (!m_no_sprite_limit)
                    break;
            }

            int cgx = (flags >> 8) & 0x01;
            u16 width = k_huc6270_sprite_width[cgx];
            u16 sprite_x = m_sat[sprite_offset + 1] & 0x3FF;
            u16 pattern = (m_sat[sprite_offset + 2] >> 1) & 0x3FF;
            pattern &= k_huc6270_sprite_mask_width[cgx];
            pattern &= k_huc6270_sprite_mask_height[cgy];
            u16 sprite_address = pattern << 6;
            u8 palette = (flags & 0x0F) << 4;
            bool x_flip = (flags & 0x0800);

            if(flags & 0x8000)
                y = height - 1 - y;

            int tile_y = y >> 4;
            int tile_line_offset = tile_y * 128;
            int offset_y = y & 0xF;

            if (width == 16)
            {
                u16 line_start = sprite_address + tile_line_offset + offset_y;
                m_sprites[m_sprite_count].index = i;
                m_sprites[m_sprite_count].x = sprite_x;
                m_sprites[m_sprite_count].flags = flags;
                m_sprites[m_sprite_count].palette = palette;
                m_sprites[m_sprite_count].data[0] = m_vram[line_start + 0];
                m_sprites[m_sprite_count].data[1] = m_vram[line_start + 16];
                m_sprites[m_sprite_count].data[2] = m_vram[line_start + 32];
                m_sprites[m_sprite_count].data[3] = m_vram[line_start + 48];
            }
            else
            {
                u16 line_start = sprite_address + tile_line_offset + offset_y;
                u16 line = line_start + (x_flip ? 64 : 0);
                m_sprites[m_sprite_count].index = i;
                m_sprites[m_sprite_count].x = sprite_x;
                m_sprites[m_sprite_count].flags = flags;
                m_sprites[m_sprite_count].palette = palette;
                m_sprites[m_sprite_count].data[0] = m_vram[line + 0];
                m_sprites[m_sprite_count].data[1] = m_vram[line + 16];
                m_sprites[m_sprite_count].data[2] = m_vram[line + 32];
                m_sprites[m_sprite_count].data[3] = m_vram[line + 48];

                m_sprite_count++;

                if (m_sprite_count >= 16)
                {
                    OverflowIRQ();
                    if (!m_no_sprite_limit)
                        break;
                }

                line = line_start + (x_flip ? 0 : 64);
                m_sprites[m_sprite_count].index = i;
                m_sprites[m_sprite_count].x = sprite_x + 16;
                m_sprites[m_sprite_count].flags = flags;
                m_sprites[m_sprite_count].palette = palette;
                m_sprites[m_sprite_count].data[0] = m_vram[line + 0];
                m_sprites[m_sprite_count].data[1] = m_vram[line + 16];
                m_sprites[m_sprite_count].data[2] = m_vram[line + 32];
                m_sprites[m_sprite_count].data[3] = m_vram[line + 48];
            }

            m_sprite_count++;
        }
    }
}

void HuC6270::SaveState(std::ostream& stream)
{
    using namespace std;
    stream.write(reinterpret_cast<const char*> (m_vram), sizeof(u16) * HUC6270_VRAM_SIZE);
    stream.write(reinterpret_cast<const char*> (&m_address_register), sizeof(m_address_register));
    stream.write(reinterpret_cast<const char*> (&m_status_register), sizeof(m_status_register));
    stream.write(reinterpret_cast<const char*> (m_register), sizeof(m_register));
    stream.write(reinterpret_cast<const char*> (m_sat), sizeof(u16) * HUC6270_SAT_SIZE);
    stream.write(reinterpret_cast<const char*> (&m_read_buffer), sizeof(m_read_buffer));
    stream.write(reinterpret_cast<const char*> (&m_trigger_sat_transfer), sizeof(m_trigger_sat_transfer));
    stream.write(reinterpret_cast<const char*> (&m_sat_transfer_pending), sizeof(m_sat_transfer_pending));
    stream.write(reinterpret_cast<const char*> (&m_hpos), sizeof(m_hpos));
    stream.write(reinterpret_cast<const char*> (&m_vpos), sizeof(m_vpos));
    stream.write(reinterpret_cast<const char*> (&m_bg_offset_y), sizeof(m_bg_offset_y));
    stream.write(reinterpret_cast<const char*> (&m_bg_counter_y), sizeof(m_bg_counter_y));
    stream.write(reinterpret_cast<const char*> (&m_increment_bg_counter_y), sizeof(m_increment_bg_counter_y));
    stream.write(reinterpret_cast<const char*> (&m_raster_line), sizeof(m_raster_line));
    stream.write(reinterpret_cast<const char*> (&m_latched_bxr), sizeof(m_latched_bxr));
    stream.write(reinterpret_cast<const char*> (&m_latched_hds), sizeof(m_latched_hds));
    stream.write(reinterpret_cast<const char*> (&m_latched_hdw), sizeof(m_latched_hdw));
    stream.write(reinterpret_cast<const char*> (&m_latched_hde), sizeof(m_latched_hde));
    stream.write(reinterpret_cast<const char*> (&m_latched_hsw), sizeof(m_latched_hsw));
    stream.write(reinterpret_cast<const char*> (&m_latched_vds), sizeof(m_latched_vds));
    stream.write(reinterpret_cast<const char*> (&m_latched_vdw), sizeof(m_latched_vdw));
    stream.write(reinterpret_cast<const char*> (&m_latched_vcr), sizeof(m_latched_vcr));
    stream.write(reinterpret_cast<const char*> (&m_latched_vsw), sizeof(m_latched_vsw));
    stream.write(reinterpret_cast<const char*> (&m_latched_mwr), sizeof(m_latched_mwr));
    stream.write(reinterpret_cast<const char*> (&m_latched_cr), sizeof(m_latched_cr));
    stream.write(reinterpret_cast<const char*> (&m_v_state), sizeof(m_v_state));
    stream.write(reinterpret_cast<const char*> (&m_h_state), sizeof(m_h_state));
    stream.write(reinterpret_cast<const char*> (&m_lines_to_next_v_state), sizeof(m_lines_to_next_v_state));
    stream.write(reinterpret_cast<const char*> (&m_clocks_to_next_h_state), sizeof(m_clocks_to_next_h_state));
    stream.write(reinterpret_cast<const char*> (&m_vblank_triggered), sizeof(m_vblank_triggered));
    stream.write(reinterpret_cast<const char*> (&m_active_line), sizeof(m_active_line));
    stream.write(reinterpret_cast<const char*> (m_line_buffer), sizeof(u16) * 1024);
    stream.write(reinterpret_cast<const char*> (m_line_buffer_sprites), sizeof(u16) * 1024);
    stream.write(reinterpret_cast<const char*> (&m_line_buffer_index), sizeof(m_line_buffer_index));
    stream.write(reinterpret_cast<const char*> (&m_no_sprite_limit), sizeof(m_no_sprite_limit));
    stream.write(reinterpret_cast<const char*> (&m_sprite_count), sizeof(m_sprite_count));

    for (int i = 0; i < 128; i++)
    {
        stream.write(reinterpret_cast<const char*> (&m_sprites[i].index), sizeof(m_sprites[i].index));
        stream.write(reinterpret_cast<const char*> (&m_sprites[i].x), sizeof(m_sprites[i].x));
        stream.write(reinterpret_cast<const char*> (&m_sprites[i].flags), sizeof(m_sprites[i].flags));
        stream.write(reinterpret_cast<const char*> (&m_sprites[i].palette), sizeof(m_sprites[i].palette));
        stream.write(reinterpret_cast<const char*> (m_sprites[i].data), sizeof(m_sprites[i].data));
    }
}

void HuC6270::LoadState(std::istream& stream)
{
    using namespace std;
    stream.read(reinterpret_cast<char*> (m_vram), sizeof(u16) * HUC6270_VRAM_SIZE);
    stream.read(reinterpret_cast<char*> (&m_address_register), sizeof(m_address_register));
    stream.read(reinterpret_cast<char*> (&m_status_register), sizeof(m_status_register));
    stream.read(reinterpret_cast<char*> (m_register), sizeof(m_register));
    stream.read(reinterpret_cast<char*> (m_sat), sizeof(u16) * HUC6270_SAT_SIZE);
    stream.read(reinterpret_cast<char*> (&m_read_buffer), sizeof(m_read_buffer));
    stream.read(reinterpret_cast<char*> (&m_trigger_sat_transfer), sizeof(m_trigger_sat_transfer));
    stream.read(reinterpret_cast<char*> (&m_sat_transfer_pending), sizeof(m_sat_transfer_pending));
    stream.read(reinterpret_cast<char*> (&m_hpos), sizeof(m_hpos));
    stream.read(reinterpret_cast<char*> (&m_vpos), sizeof(m_vpos));
    stream.read(reinterpret_cast<char*> (&m_bg_offset_y), sizeof(m_bg_offset_y));
    stream.read(reinterpret_cast<char*> (&m_bg_counter_y), sizeof(m_bg_counter_y));
    stream.read(reinterpret_cast<char*> (&m_increment_bg_counter_y), sizeof(m_increment_bg_counter_y));
    stream.read(reinterpret_cast<char*> (&m_raster_line), sizeof(m_raster_line));
    stream.read(reinterpret_cast<char*> (&m_latched_bxr), sizeof(m_latched_bxr));
    stream.read(reinterpret_cast<char*> (&m_latched_hds), sizeof(m_latched_hds));
    stream.read(reinterpret_cast<char*> (&m_latched_hdw), sizeof(m_latched_hdw));
    stream.read(reinterpret_cast<char*> (&m_latched_hde), sizeof(m_latched_hde));
    stream.read(reinterpret_cast<char*> (&m_latched_hsw), sizeof(m_latched_hsw));
    stream.read(reinterpret_cast<char*> (&m_latched_vds), sizeof(m_latched_vds));
    stream.read(reinterpret_cast<char*> (&m_latched_vdw), sizeof(m_latched_vdw));
    stream.read(reinterpret_cast<char*> (&m_latched_vcr), sizeof(m_latched_vcr));
    stream.read(reinterpret_cast<char*> (&m_latched_vsw), sizeof(m_latched_vsw));
    stream.read(reinterpret_cast<char*> (&m_latched_mwr), sizeof(m_latched_mwr));
    stream.read(reinterpret_cast<char*> (&m_latched_cr), sizeof(m_latched_cr));
    stream.read(reinterpret_cast<char*> (&m_v_state), sizeof(m_v_state));
    stream.read(reinterpret_cast<char*> (&m_h_state), sizeof(m_h_state));
    stream.read(reinterpret_cast<char*> (&m_lines_to_next_v_state), sizeof(m_lines_to_next_v_state));
    stream.read(reinterpret_cast<char*> (&m_clocks_to_next_h_state), sizeof(m_clocks_to_next_h_state));
    stream.read(reinterpret_cast<char*> (&m_vblank_triggered), sizeof(m_vblank_triggered));
    stream.read(reinterpret_cast<char*> (&m_active_line), sizeof(m_active_line));
    stream.read(reinterpret_cast<char*> (m_line_buffer), sizeof(u16) * 1024);
    stream.read(reinterpret_cast<char*> (m_line_buffer_sprites), sizeof(u16) * 1024);
    stream.read(reinterpret_cast<char*> (&m_line_buffer_index), sizeof(m_line_buffer_index));
    stream.read(reinterpret_cast<char*> (&m_no_sprite_limit), sizeof(m_no_sprite_limit));
    stream.read(reinterpret_cast<char*> (&m_sprite_count), sizeof(m_sprite_count));

    for (int i = 0; i < 128; i++)
    {
        stream.read(reinterpret_cast<char*> (&m_sprites[i].index), sizeof(m_sprites[i].index));
        stream.read(reinterpret_cast<char*> (&m_sprites[i].x), sizeof(m_sprites[i].x));
        stream.read(reinterpret_cast<char*> (&m_sprites[i].flags), sizeof(m_sprites[i].flags));
        stream.read(reinterpret_cast<char*> (&m_sprites[i].palette), sizeof(m_sprites[i].palette));
        stream.read(reinterpret_cast<char*> (m_sprites[i].data), sizeof(m_sprites[i].data));
    }
}
