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
    InitPointer(m_huc6260);
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

void HuC6270::Init(HuC6260* huC6260)
{
    m_huc6260 = huC6260;
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
    m_vram_transfer_pending = 0;
    m_vram_transfer_src = 0;
    m_vram_transfer_dest = 0;
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
    m_h_state = HuC6270_HORIZONTAL_STATE_HDS;
    m_next_event = HuC6270_EVENT_NONE;
    m_lines_to_next_v_state = m_latched_vds + 2;
    m_clocks_to_next_h_state = 1;
    m_clocks_to_next_event = -1;
    m_vblank_triggered = false;
    m_active_line = false;
    m_burst_mode = false;
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
    // Low to high
    if (active)
    {
        EndOfLine();

        HUC6270_DEBUG("  HSW start (force)");

        m_h_state = HuC6270_HORIZONTAL_STATE_HSW;
        m_clocks_to_next_h_state = m_huc6260->GetClockDivider() == 3 ? 32 : 24;

        HSyncStart();
    }
}

void HuC6270::SetVSync(bool active)
{
    // High to low
    if (!active)
    {
        HUC6270_DEBUG("+++ VerticalSyncStart");

        m_latched_mwr = m_register[HUC6270_REG_MWR];
        m_latched_vds = HUC6270_VAR_VDS;
        m_latched_vdw = HUC6270_VAR_VDW;
        m_latched_vcr = HUC6270_VAR_VCR;
        m_latched_vsw = HUC6270_VAR_VSW;

        m_v_state = HuC6270_VERTICAL_STATE_VSW;
        m_lines_to_next_v_state = m_latched_vsw + 1;

        m_increment_bg_counter_y = false;
    }
}

u8 HuC6270::ReadRegister(u16 address)
{
    switch (address & 0x03)
    {
        // Status register
        case 0:
        {
            u8 ret = m_status_register & 0x7F;
            m_huc6280->AssertIRQ1(false);
            m_status_register &= 0x40;
            return ret;
        }
        // Data register (LSB)
        case 2:
        {
            if (m_address_register != HUC6270_REG_VRR)
            {
                Debug("[PC=%04X] HuC6270 invalid data register (LSB) read: %02X", m_huc6280->GetState()->PC->GetValue(), m_address_register);
            }
            return m_read_buffer & 0xFF;
        }
        // Data register (MSB)
        case 3:
        {
#if !defined(GG_DISABLE_DISASSEMBLER)
            m_huc6280->CheckMemoryBreakpoints(HuC6280::HuC6280_BREAKPOINT_TYPE_HUC6270_REGISTER, m_address_register, true);
#endif
            u8 ret = m_read_buffer >> 8;

            if (m_address_register == HUC6270_REG_VRR)
            {
#if !defined(GG_DISABLE_DISASSEMBLER)
                m_huc6280->CheckMemoryBreakpoints(HuC6280::HuC6280_BREAKPOINT_TYPE_VRAM, m_register[HUC6270_REG_MARR], true);
#endif
                m_read_buffer = m_vram[m_register[HUC6270_REG_MARR] & 0x7FFF];
                m_register[HUC6270_REG_MARR] += k_huc6270_read_write_increment[(m_register[HUC6270_REG_CR] >> 11) & 0x03];
            }
            else
            {
                Debug("[PC=%04X] HuC6270 invalid data register (MSB) read: %02X", m_huc6280->GetState()->PC->GetValue(), m_address_register);
            }

            return ret;
        }
        default:
        {
            Debug("[PC=%04X] HuC6270 invalid register read at %06X, reg=%d", m_huc6280->GetState()->PC->GetValue(), address, address & 0x03);
            return 0x00;
        }
    }
}

void HuC6270::WriteRegister(u16 address, u8 value)
{
    switch (address & 0x03)
    {
        // Address register
        case 0:
            m_address_register = value & 0x1F;
            break;
        // Data register (LSB)
        case 2:
        // Data register (MSB)
        case 3:
        {
#if !defined(GG_DISABLE_DISASSEMBLER)
            m_huc6280->CheckMemoryBreakpoints(HuC6280::HuC6280_BREAKPOINT_TYPE_HUC6270_REGISTER, m_address_register, false);
#endif

            bool msb = address & 0x01;

            if (m_address_register > 0x13)
            {
                Debug("[PC=%04X] HuC6270 INVALID write to data register (%s) %02X: %04X", m_huc6280->GetState()->PC->GetValue(), msb ? "MSB" : "LSB", value, m_address_register);
                return;
            }

            if (msb)
                m_register[m_address_register] = (m_register[m_address_register] & 0x00FF) | (value << 8);
            else
                m_register[m_address_register] = (m_register[m_address_register] & 0xFF00) | value;

            m_register[m_address_register] &= k_register_mask[m_address_register];

            switch (m_address_register)
            {
                // 0x01
                case HUC6270_REG_MARR:
                    if (msb)
                    {
                        m_read_buffer = m_vram[m_register[HUC6270_REG_MARR] & 0x7FFF];
                        m_register[HUC6270_REG_MARR] += k_huc6270_read_write_increment[(m_register[HUC6270_REG_CR] >> 11) & 0x03];
                    }
                    break;
                // 0x02
                case HUC6270_REG_VWR:
                    if (msb)
                    {
                        if (m_register[HUC6270_REG_MAWR] >= 0x8000)
                        {
                            Debug("[PC=%04X] HuC6270 ignoring write VWR out of bounds (%s) %04X: %02X", m_huc6280->GetState()->PC->GetValue(), msb ? "MSB" : "LSB", m_register[HUC6270_REG_MAWR], value);
                        }
                        else
                        {
#if !defined(GG_DISABLE_DISASSEMBLER)
                            m_huc6280->CheckMemoryBreakpoints(HuC6280::HuC6280_BREAKPOINT_TYPE_VRAM, m_register[HUC6270_REG_MAWR], false);
#endif
                            m_vram[m_register[HUC6270_REG_MAWR] & 0x7FFF] = m_register[HUC6270_REG_VWR];
                        }

                        m_register[HUC6270_REG_MAWR] += k_huc6270_read_write_increment[(m_register[HUC6270_REG_CR] >> 11) & 0x03];
                    }
                    break;
                // 0x07
                case HUC6270_REG_BXR:
                    //HUC6270_DEBUG("**** BXR Set");
                    break;
                // 0x08
                case HUC6270_REG_BYR:
                    m_bg_counter_y = m_register[HUC6270_REG_BYR];
                    //HUC6270_DEBUG("**** BYR Set");
                    break;
                // 0x12
                case HUC6270_REG_LENR:
                    if (msb)
                    {
                        m_vram_transfer_pending = 4 * (m_register[HUC6270_REG_LENR] + 1);
                        m_vram_transfer_src = m_register[HUC6270_REG_SOUR];
                        m_vram_transfer_dest = m_register[HUC6270_REG_DESR];
                        //m_status_register |= HUC6270_STATUS_BUSY;
                    }
                    break;
                // 0x13
                case HUC6270_REG_DVSSR:
                    m_trigger_sat_transfer = true;
                    break;
            }
            break;
        }
        default:
            Debug("[PC=%04X] HuC6270 invalid write at %06X, value=%02X", m_huc6280->GetState()->PC->GetValue(), address, value);
            break;
    }
}
void HuC6270::EndOfLine()
{
    m_hpos = 0;
}

void HuC6270::LineEvents()
{
    m_clocks_to_next_event--;

    if (m_clocks_to_next_event == 0)
    {
        switch (m_next_event)
        {
            case HuC6270_EVENT_BYR:
                HUC6270_DEBUG("  [?] LatchScrollY");
                m_next_event = HuC6270_EVENT_BXR;
                m_clocks_to_next_event = 2;

                if (m_increment_bg_counter_y)
                {
                    m_increment_bg_counter_y = false;
                    if(m_raster_line == 0)
                        m_bg_counter_y = m_register[HUC6270_REG_BYR];
                    else
                        m_bg_counter_y++;
                }
                m_bg_offset_y = m_bg_counter_y;

                break;
            case HuC6270_EVENT_BXR:
                HUC6270_DEBUG("  [?] LatchScrollX");
                m_next_event = HuC6270_EVENT_HDS;
                m_clocks_to_next_event = 6;

                m_latched_bxr = m_register[HUC6270_REG_BXR];

                break;
            case HuC6270_EVENT_HDS:
                HUC6270_DEBUG("  [?] HdsIrqTrigger");
                m_next_event = HuC6270_EVENT_NONE;
                m_clocks_to_next_event = -1;

                if ((m_v_state != HuC6270_VERTICAL_STATE_VDW) && !m_vblank_triggered)
                {
                    m_vblank_triggered = true;
                    VBlankIRQ();
                }
                if (m_v_state == HuC6270_VERTICAL_STATE_VDW)
                    RenderLine();

                break;
            case HuC6270_EVENT_RCR:
                HUC6270_DEBUG("  [?] IncRcrCounter");
                m_next_event = HuC6270_EVENT_NONE;
                m_clocks_to_next_event = -1;

                m_raster_line++;
                m_increment_bg_counter_y = true;

                m_lines_to_next_v_state--;
                while (m_lines_to_next_v_state <= 0)
                    NextVerticalState();

                if ((m_v_state == HuC6270_VERTICAL_STATE_VDW) && !m_burst_mode)
                    FetchSprites();

                RCRIRQ();

                break;
            default:
                HUC6270_DEBUG("HuC6270 invalid event %d", m_next_event);
                break;
        }
    }
}

void HuC6270::HSyncStart()
{
    HUC6270_DEBUG("--- HorizSyncStart");

    m_latched_hds = HUC6270_VAR_HDS;
    m_latched_hdw = HUC6270_VAR_HDW;
    m_latched_hde = HUC6270_VAR_HDE;
    m_latched_hsw = HUC6270_VAR_HSW;
    m_latched_cr = HUC6270_VAR_CR;

    m_next_event = HuC6270_EVENT_NONE;
    m_clocks_to_next_event = -1;

    s32 display_start = m_hpos + m_clocks_to_next_h_state + ((m_latched_hds + 1) << 3);

    HUC6270_DEBUG("  ** displayStart: %d, hc: %d\t", display_start, m_clocks_to_next_h_state);

    s32 event_clocks;
    if (m_v_state == HuC6270_VERTICAL_STATE_VDW)
    {
        m_next_event = HuC6270_EVENT_BYR;
        event_clocks = 33;
    }
    else
    {
        m_next_event = HuC6270_EVENT_HDS;
        event_clocks = 24;
    }

    m_clocks_to_next_event = display_start - event_clocks - m_hpos;
    HUC6270_DEBUG("  ** _nextEventCounter: %d, ec: %d\t", m_clocks_to_next_event, event_clocks);
}

void HuC6270::SATTransfer()
{
    m_sat_transfer_pending--;

    if ((m_sat_transfer_pending & 3) == 0)
    {
        u16 satb = m_register[HUC6270_REG_DVSSR];
        int i = 255 - (m_sat_transfer_pending >> 2);
        m_sat[i] = m_vram[(satb + i) & 0x7FFF];

        if (m_sat_transfer_pending == 0)
        {
            m_status_register &= ~HUC6270_STATUS_BUSY;

            if (m_register[HUC6270_REG_DCR] & 0x01)
            {
                m_status_register |= HUC6270_STATUS_SAT_END;
                m_huc6280->AssertIRQ1(true);
            }
        }
    }
}

void HuC6270::VRAMTransfer()
{
    m_vram_transfer_pending--;

    if ((m_vram_transfer_pending & 3) == 0)
    {
        if (m_vram_transfer_dest < 0x8000)
        {
            m_vram[m_vram_transfer_dest & 0x7FFF] = m_vram[m_vram_transfer_src & 0x7FFF];
        }
        else
        {
            Debug("[PC=%04X] HuC6270 ignoring write VRAM-DMA out of bounds: %04X", m_huc6280->GetState()->PC->GetValue(), m_register[HUC6270_REG_DESR]);
        }

        s8 src_increment = IS_SET_BIT(m_register[HUC6270_REG_DCR], 2) ? -1 : 1;
        s8 dest_increment = IS_SET_BIT(m_register[HUC6270_REG_DCR], 3) ? -1 : 1;
        m_vram_transfer_src += src_increment;
        m_vram_transfer_dest += dest_increment;

        if (m_vram_transfer_pending == 0)
        {
            m_status_register &= ~HUC6270_STATUS_BUSY;

            if (m_register[HUC6270_REG_DCR] & 0x02)
            {
                m_status_register |= HUC6270_STATUS_VRAM_END;
                m_huc6280->AssertIRQ1(true);
            }
        }
    }
}

void HuC6270::NextVerticalState()
{
    m_v_state = (m_v_state + 1) % HuC6270_VERTICAL_STATE_COUNT;

    switch (m_v_state)
    {
        case HuC6270_VERTICAL_STATE_VDS:
            HUC6270_DEBUG(" >> VDS start\t");
            m_lines_to_next_v_state = m_latched_vds + 2;
            break;
        case HuC6270_VERTICAL_STATE_VDW:
            m_lines_to_next_v_state = m_latched_vdw + 1;
            m_raster_line = 0;
            m_vblank_triggered = false;

            HUC6270_DEBUG(" >> VDW start\t");
            break;
        case HuC6270_VERTICAL_STATE_VCR:
            HUC6270_DEBUG(" >> VDE start\t");
            m_lines_to_next_v_state = m_latched_vcr;
            break;
        case HuC6270_VERTICAL_STATE_VSW:
            HUC6270_DEBUG(" >> VSW start\t");
            m_lines_to_next_v_state = m_latched_vsw + 1;
            m_latched_mwr = m_register[HUC6270_REG_MWR];
            m_latched_vds = HUC6270_VAR_VDS;
            m_latched_vdw = HUC6270_VAR_VDW;
            m_latched_vcr = HUC6270_VAR_VCR;
            m_latched_vsw = HUC6270_VAR_VSW;
            break;
    }
}

void HuC6270::NextHorizontalState()
{
    m_h_state = (m_h_state + 1) % HuC6270_HORIZONTAL_STATE_COUNT;

    switch (m_h_state)
    {
        case HuC6270_HORIZONTAL_STATE_HDS:
            m_clocks_to_next_h_state = (m_latched_hds + 1) << 3;

            m_line_buffer_index = 0;

            //m_hpos = 0;
            m_vpos = (m_vpos + 1) % HUC6270_LINES;
            m_active_line = (m_raster_line < HUC6270_LINES_ACTIVE);


            if (m_vpos == 0)
                m_burst_mode = ((m_latched_cr & 0x00C0) == 0);

            HUC6270_DEBUG("  HDS start\t");
            break;
        case HuC6270_HORIZONTAL_STATE_HDW:
            HUC6270_DEBUG("  HDW start\t");
            m_clocks_to_next_h_state = (m_latched_hdw + 1) << 3;
            m_next_event = HuC6270_EVENT_RCR;
            m_clocks_to_next_event = ((m_latched_hdw - 1) << 3) + 1;
            break;
        case HuC6270_HORIZONTAL_STATE_HDE:
            HUC6270_DEBUG("  HDE start\t");
            m_clocks_to_next_h_state = (m_latched_hde + 1) << 3;
            break;
        case HuC6270_HORIZONTAL_STATE_HSW:
            //m_hpos = 0;
            HUC6270_DEBUG("  HSW start\t");
            m_clocks_to_next_h_state = (m_latched_hsw + 1) << 3;
            HSyncStart();
            break;
    }
}

void HuC6270::VBlankIRQ()
{
    HUC6270_DEBUG("  [!] VBLANK IRQ");
    if (m_register[HUC6270_REG_CR] & HUC6270_CONTROL_VBLANK)
    {
        m_status_register |= HUC6270_STATUS_VBLANK;
        m_huc6280->AssertIRQ1(true);
    }

    if (m_trigger_sat_transfer || (m_register[HUC6270_REG_DCR] & 0x10))
    {
        m_trigger_sat_transfer = false;

        m_sat_transfer_pending = 1024;
        //m_status_register |= HUC6270_STATUS_BUSY;
    }
}

void HuC6270::RenderLine()
{
    int width = MIN(1024, (m_latched_hdw + 1) << 3);

    if((m_latched_cr & 0x80) == 0)
    {
        u16 color = 0x100;

        for (int i = 0; i < width; i++)
        {
            m_line_buffer[i] = color;
        }
    }

    if (!m_burst_mode)
    {
        if((m_latched_cr & 0x80) != 0)
        {
            RenderBackground(width);
        }

        if((m_latched_cr & 0x40) != 0)
        {
            RenderSprites(width);
        }
    }
}

void HuC6270::RenderBackground(int width)
{
    int screen_reg = (m_latched_mwr >> 4) & 0x07;
    int screen_size_x = k_huc6270_screen_size_x[screen_reg];
    int bg_y = m_bg_offset_y;
    bg_y &= k_huc6270_screen_size_y_pixels_mask[screen_reg];
    int tile_y = (bg_y & 7);
    int bat_offset = (bg_y >> 3) * screen_size_x;

    u8 byte1 = 0, byte2 = 0, byte3 = 0, byte4 = 0;
    int prev_tile_col = -1;
    u16 bat_entry = 0;
    int tile_index = 0;
    int color_table = 0;
    int tile_data = 0;

    for (int i = 0; i < width; i++)
    {
        int bg_x = m_latched_bxr + i;
        bg_x &= k_huc6270_screen_size_x_pixels_mask[screen_reg];
        int tile_col = bg_x >> 3;

        if (tile_col != prev_tile_col)
        {
            bat_entry = m_vram[bat_offset + tile_col];
            tile_index = bat_entry & 0x07FF;
            color_table = (bat_entry >> 12) & 0x0F;
            tile_data = tile_index << 4;
            int line_start_a = (tile_data + tile_y);
            int line_start_b = (line_start_a + 8);
            byte1 = m_vram[line_start_a] & 0xFF;
            byte2 = m_vram[line_start_a] >> 8;
            byte3 = m_vram[line_start_b] & 0xFF;
            byte4 = m_vram[line_start_b] >> 8;
            prev_tile_col = tile_col;
        }

        int tile_x = 7 - (bg_x & 7);
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

        if ((pos + 15) < 0 || pos >= width)
            continue;

        bool priority = (m_sprites[i].flags & 0x0080);
        u16 plane1 = m_sprites[i].data[0];
        u16 plane2 = m_sprites[i].data[1];
        u16 plane3 = m_sprites[i].data[2];
        u16 plane4 = m_sprites[i].data[3];


        int start_x = (pos < 0) ? -pos : 0;
        int end_x = (pos + 15 >= width) ? (width - pos - 1) : 15;

        for(int x = start_x; x <= end_x; x++)
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
    bool mode1 = ((m_latched_mwr >> 2) & 0x03) == 1;

    for (int i = 0; i < HUC6270_SPRITES; i++)
    {
        int sprite_offset = i << 2;
        u16 sat0 = m_sat[sprite_offset + 0];
        u16 sat3 = m_sat[sprite_offset + 3];
        int sprite_y = (sat0 & 0x3FF) - 64;
        u16 flags = sat3;
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

            u16 sat1 = m_sat[sprite_offset + 1];
            u16 sat2 = m_sat[sprite_offset + 2];
            int mode1_offset = mode1 ? (sat2 & 1) << 5 : 0;
            int cgx = (flags >> 8) & 0x01;
            u16 width = k_huc6270_sprite_width[cgx];
            u16 sprite_x = sat1 & 0x3FF;
            u16 pattern = (sat2 >> 1) & 0x3FF;
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
                u16 line_start = sprite_address + tile_line_offset + offset_y + mode1_offset;
                m_sprites[m_sprite_count].index = i;
                m_sprites[m_sprite_count].x = sprite_x;
                m_sprites[m_sprite_count].flags = flags;
                m_sprites[m_sprite_count].palette = palette;
                m_sprites[m_sprite_count].data[0] = m_vram[line_start + 0];
                m_sprites[m_sprite_count].data[1] = m_vram[line_start + 16];
                m_sprites[m_sprite_count].data[2] = mode1 ? 0 : m_vram[line_start + 32];
                m_sprites[m_sprite_count].data[3] = mode1 ? 0 : m_vram[line_start + 48];
            }
            else
            {
                u16 line_start = sprite_address + tile_line_offset + offset_y + mode1_offset;
                u16 line = line_start + (x_flip ? 64 : 0);
                m_sprites[m_sprite_count].index = i;
                m_sprites[m_sprite_count].x = sprite_x;
                m_sprites[m_sprite_count].flags = flags;
                m_sprites[m_sprite_count].palette = palette;
                m_sprites[m_sprite_count].data[0] = m_vram[line + 0];
                m_sprites[m_sprite_count].data[1] = m_vram[line + 16];
                m_sprites[m_sprite_count].data[2] = mode1 ? 0 : m_vram[line + 32];
                m_sprites[m_sprite_count].data[3] = mode1 ? 0 : m_vram[line + 48];

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
                m_sprites[m_sprite_count].data[2] = mode1 ? 0 : m_vram[line + 32];
                m_sprites[m_sprite_count].data[3] = mode1 ? 0 : m_vram[line + 48];
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
    stream.write(reinterpret_cast<const char*> (&m_vram_transfer_pending), sizeof(m_vram_transfer_pending));
    stream.write(reinterpret_cast<const char*> (&m_vram_transfer_src), sizeof(m_vram_transfer_src));
    stream.write(reinterpret_cast<const char*> (&m_vram_transfer_dest), sizeof(m_vram_transfer_dest));
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
    stream.write(reinterpret_cast<const char*> (&m_burst_mode), sizeof(m_burst_mode));
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
    stream.read(reinterpret_cast<char*> (&m_vram_transfer_pending), sizeof(m_vram_transfer_pending));
    stream.read(reinterpret_cast<char*> (&m_vram_transfer_src), sizeof(m_vram_transfer_src));
    stream.read(reinterpret_cast<char*> (&m_vram_transfer_dest), sizeof(m_vram_transfer_dest));
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
    stream.read(reinterpret_cast<char*> (&m_burst_mode), sizeof(m_burst_mode));
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
