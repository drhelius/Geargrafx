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
    m_auto_sat_transfer = false;
    m_sat_transfer_pending = -1;
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

void HuC6270::SetHSync(bool active)
{
    // High to low
    if (!active)
    {
        HUC6270_DEBUG("HSYNC H to L");
    }
    // Low to high
    else
    {
        m_h_state = HuC6270_HORIZONTAL_STATE_HSW;
        m_clocks_to_next_h_state = 8;

        HUC6270_DEBUG("HSYNC L to H");
    }
}

void HuC6270::SetVSync(bool active)
{
    // High to low
    if (!active)
    {
        m_latched_mwr = m_register[HUC6270_REG_MWR];
        m_latched_vds = HUC6270_VAR_VDS;
        m_latched_vdw = HUC6270_VAR_VDW;
        m_latched_vcr = HUC6270_VAR_VCR;
        m_latched_vsw = HUC6270_VAR_VSW;

        m_v_state = HuC6270_VERTICAL_STATE_VSW;
        m_lines_to_next_v_state = m_latched_vsw + 1;

        m_increment_bg_counter_y = false;

        HUC6270_DEBUG("+++ VSYNC H to L");
    }
    // Low to high
    else
    {
        HUC6270_DEBUG("+++ VSYNC L to H");
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

void HuC6270::SetNoSpriteLimit(bool no_sprite_limit)
{
    m_no_sprite_limit = no_sprite_limit;
}

void HuC6270::NextVerticalState()
{
    m_v_state = (m_v_state + 1) % HuC6270_VERTICAL_STATE_COUNT;

    switch (m_v_state)
    {
        case HuC6270_VERTICAL_STATE_VDS:
            m_lines_to_next_v_state = m_latched_vds + 2;
            HUC6270_DEBUG("+ VDS");
            break;
        case HuC6270_VERTICAL_STATE_VDW:
            m_lines_to_next_v_state = m_latched_vdw + 1;
            m_raster_line = 0;
            m_vblank_triggered = false;
            HUC6270_DEBUG("+ VDW");
            break;
        case HuC6270_VERTICAL_STATE_VCR:
            m_lines_to_next_v_state = m_latched_vcr;
            HUC6270_DEBUG("+ VCR");
            break;
        case HuC6270_VERTICAL_STATE_VSW:
            m_lines_to_next_v_state = m_latched_vsw + 1;
            m_latched_mwr = m_register[HUC6270_REG_MWR];
            m_latched_vds = HUC6270_VAR_VDS;
            m_latched_vdw = HUC6270_VAR_VDW;
            m_latched_vcr = HUC6270_VAR_VCR;
            m_latched_vsw = HUC6270_VAR_VSW;
            HUC6270_DEBUG(">>>\nVSW Start!  VSW: %d, VDS: %d, VDW: %d, VCR: %d", m_latched_vsw, m_latched_vds, m_latched_vdw, m_latched_vcr);
            break;
    }
}

void HuC6270::NextHorizontalState()
{
    m_h_state = (m_h_state + 1) % HuC6270_HORIZONTAL_STATE_COUNT;

    switch (m_h_state)
    {
        case HuC6270_HORIZONTAL_STATE_HDS_1:
            m_line_buffer_index = 0;
            //HUC6270_DEBUG("------ hpos reset: %d", m_hpos);
            m_hpos = 0;
            m_vpos = (m_vpos + 1) % 263;
            m_active_line = (m_raster_line < 240);
            m_latched_hds = HUC6270_VAR_HDS;
            m_latched_hdw = HUC6270_VAR_HDW;
            m_latched_hde = HUC6270_VAR_HDE;
            m_latched_hsw = HUC6270_VAR_HSW;
            m_latched_cr = HUC6270_VAR_CR;
            m_clocks_to_next_h_state = ClocksToBYRLatch();
            HUC6270_DEBUG(">>>\nHDS Start!  HSW: %d, HDW: %d, HDW: %d, HDE: %d", m_latched_hsw, m_latched_hds, m_latched_hdw, m_latched_hde);
            HUC6270_DEBUG("HDS 1");
            break;
        case HuC6270_HORIZONTAL_STATE_HDS_2:
            m_clocks_to_next_h_state = ClocksToBXRLatch();

            if (m_increment_bg_counter_y)
            {
                m_increment_bg_counter_y = false;
                if(m_raster_line == 0)
                    m_bg_counter_y = m_register[HUC6270_REG_BYR];
                else
                    m_bg_counter_y++;
            }
            m_bg_offset_y = m_bg_counter_y;

            HUC6270_DEBUG("HDS 2");
            break;
        case HuC6270_HORIZONTAL_STATE_HDS_3:
            m_clocks_to_next_h_state = ((m_latched_hds + 1) << 3) - ClocksToBYRLatch() - ClocksToBXRLatch();
            assert(m_clocks_to_next_h_state > 0);
            m_latched_bxr = m_register[HUC6270_REG_BXR];
            HUC6270_DEBUG("HDS 3");
            break;
        case HuC6270_HORIZONTAL_STATE_HDW_1:
            m_clocks_to_next_h_state = ((m_latched_hdw + 1) << 3) - HUC6270_RCR_IRQ_CYCLES_BEFORE_HDE;
            if ((m_v_state != HuC6270_VERTICAL_STATE_VDW) && !m_vblank_triggered)
            {
                m_vblank_triggered = true;
                VBlankIRQ();
            }
            if (m_v_state == HuC6270_VERTICAL_STATE_VDW)
                RenderLine();
            HUC6270_DEBUG("HDW 1");
            break;
        case HuC6270_HORIZONTAL_STATE_HDW_2:
            m_clocks_to_next_h_state = HUC6270_RCR_IRQ_CYCLES_BEFORE_HDE;

            m_raster_line++;
            m_increment_bg_counter_y = true;

            m_lines_to_next_v_state--;
            while (m_lines_to_next_v_state <= 0)
                NextVerticalState();

            if (m_v_state == HuC6270_VERTICAL_STATE_VDW)
                FetchSprites();

            RCRIRQ();

            HUC6270_DEBUG("HDW 2");
            break;
        case HuC6270_HORIZONTAL_STATE_HDE:
            m_clocks_to_next_h_state = (m_latched_hde + 1) << 3;
            HUC6270_DEBUG("HDE");
            break;
        case HuC6270_HORIZONTAL_STATE_HSW:
            m_clocks_to_next_h_state = (m_latched_hsw + 1) << 3;
            HUC6270_DEBUG("HSW");
            break;
    }
}

void HuC6270::VBlankIRQ()
{
    if (m_register[HUC6270_REG_CR] & HUC6270_CONTROL_VBLANK)
    {
        HUC6270_DEBUG("VBlank IRQ");
        m_status_register |= HUC6270_STATUS_VBLANK;
        m_huc6280->AssertIRQ1(true);
    }

    if (m_trigger_sat_transfer || m_auto_sat_transfer)
    {
        m_trigger_sat_transfer = false;
        m_auto_sat_transfer = m_register[HUC6270_REG_DCR] & 0x10;

        u16 satb = m_register[HUC6270_REG_DVSSR];

        for (int i = 0; i < HUC6270_SAT_SIZE; i++)
        {
            m_sat[i] = m_vram[(satb + i) & 0x7FFF];
        }

        m_sat_transfer_pending = 1024;
    }
}

void HuC6270::RenderLine()
{
    int width = std::min(1024, (m_latched_hdw + 1) << 3);

    if((m_latched_cr & 0x80) == 0)
    {
        u16 color = 0x100;

        for (int i = 0; i < width; i++)
        {
            m_line_buffer[i] = color;
        }
    }

    if((m_latched_cr & 0x80) != 0)
    {
        RenderBackground(width);
    }

    if((m_latched_cr & 0x40) != 0)
    {
        RenderSprites(width);
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
