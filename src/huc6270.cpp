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

#include <cassert>
#include <stdlib.h>
#include "huc6270.h"
#include "huc6260.h"

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

    m_register[HUC6270_REG_HSR] = 0x0202;
    m_register[HUC6270_REG_HDR] = 0x041f;
    m_register[HUC6270_REG_VSR] = 0x0f02;
    m_register[HUC6270_REG_VDR] = 0x00ef;
    m_register[HUC6270_REG_VCR] = 0x0004;

    m_address_register = 0;
    m_status_register = 0;
    m_read_buffer = 0xFFFF;
    m_trigger_sat_transfer = false;
    m_auto_sat_transfer = false;
    m_hpos = 0;
    m_vpos = 0;
    m_bg_offset_y = 0;
    m_bg_counter_y = 0;
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
    m_lines_to_next_v_state = 1;
    m_clocks_to_next_h_state = 1;
    m_vblank_triggered = false;
    m_active_line = false;

    for (int i = 0; i < HUC6270_VRAM_SIZE; i++)
    {
        m_vram[i] = rand() & 0xFFFF;
    }

    for (int i = 0; i < HUC6270_SAT_SIZE; i++)
    {
        m_sat[i] = rand() & 0xFFFF;
    }
}

u16 HuC6270::Clock(bool* active)
{
    *active = false;
    u16 pixel = 0;

    if (m_active_line &&
        (m_h_state == HuC6270_HORIZONTAL_STATE_HDW_1 || m_h_state == HuC6270_HORIZONTAL_STATE_HDW_2))
    {
        *active = true;

        int screen_reg = (m_register[HUC6270_REG_MWR] >> 4) & 0x07;
        int screen_size_x = k_scren_size_x[screen_reg];
        int screen_size_x_pixels = screen_size_x * 8;
        int screen_size_y = k_scren_size_y[screen_reg];
        int screen_size_y_pixels = screen_size_y * 8;

        int bg_y = m_bg_offset_y;
        bg_y %= screen_size_y_pixels;
        int bat_offset = (bg_y / 8) * screen_size_x;

        int bg_x = m_latched_bxr;
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

        pixel = color_table << 4;

        pixel |= ((byte1 >> (7 - tile_x)) & 0x01) | (((byte2 >> (7 - tile_x)) & 0x01) << 1) | (((byte3 >> (7 - tile_x)) & 0x01) << 2) | (((byte4 >> (7 - tile_x)) & 0x01) << 3);

        m_latched_bxr++;
    }

    m_clocks_to_next_h_state--;
    while (m_clocks_to_next_h_state == 0)
        NextHorizontalState();

    return pixel;
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
            m_hpos = 0;
            m_active_line = (m_raster_line < 240);
            m_latched_hds = HUC6270_VAR_HDS;
            m_latched_hdw = HUC6270_VAR_HDW;
            m_latched_hde = HUC6270_VAR_HDE;
            m_latched_hsw = HUC6270_VAR_HSW;
            m_clocks_to_next_h_state = ClocksToBYRLatch();
            HUC6270_DEBUG(">>>\nHDS Start!  HSW: %d, HDW: %d, HDW: %d, HDE: %d", m_latched_hsw, m_latched_hds, m_latched_hdw, m_latched_hde);
            HUC6270_DEBUG("HDS 1");
            break;
        case HuC6270_HORIZONTAL_STATE_HDS_2:
            m_clocks_to_next_h_state = ClocksToBXRLatch();

            if(m_raster_line == 0)
                m_bg_counter_y = m_register[HUC6270_REG_BYR];
            else
                m_bg_counter_y++;
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
            if (m_v_state == HuC6270_VERTICAL_STATE_VCR && !m_vblank_triggered)
            {
                m_vblank_triggered = true;
                VBlankIRQ();
            }
            HUC6270_DEBUG("HDW 1");
            break;
        case HuC6270_HORIZONTAL_STATE_HDW_2:
            m_clocks_to_next_h_state = HUC6270_RCR_IRQ_CYCLES_BEFORE_HDE;

            m_raster_line++;

            m_lines_to_next_v_state--;
            if (m_lines_to_next_v_state <= 0)
                NextVerticalState();

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

        u16 satb = m_register[HUC6270_REG_DVSSR] & 0x7FFF;

        for (int i = 0; i < HUC6270_SAT_SIZE; i++)
        {
            m_sat[i] = m_vram[satb + i] & 0x7FFF;
        }

        m_status_register |= HUC6270_STATUS_SAT_END;
        if (m_register[HUC6270_REG_DCR] & 0x01)
        {
            m_huc6280->AssertIRQ1(true);
        }
    }
}
