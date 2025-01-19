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

#ifndef HUC6270_H
#define HUC6270_H

#include "huc6270_defines.h"
#include "common.h"

class HuC6280;

class HuC6270
{
public:
    enum HuC6270_Vertical_State
    {
        HuC6270_VERTICAL_STATE_VDS,
        HuC6270_VERTICAL_STATE_VDW,
        HuC6270_VERTICAL_STATE_VCR,
        HuC6270_VERTICAL_STATE_VSW,
        HuC6270_VERTICAL_STATE_COUNT
    };

    enum HuC6270_Horizontal_State
    {
        HuC6270_HORIZONTAL_STATE_HDS_1,
        HuC6270_HORIZONTAL_STATE_HDS_2,
        HuC6270_HORIZONTAL_STATE_HDS_3,
        HuC6270_HORIZONTAL_STATE_HDW_1,
        HuC6270_HORIZONTAL_STATE_HDW_2,
        HuC6270_HORIZONTAL_STATE_HDE,
        HuC6270_HORIZONTAL_STATE_HSW,
        HuC6270_HORIZONTAL_STATE_COUNT
    };

    struct HuC6270_State
    {
        u16* AR;
        u16* SR;
        u16* R;
        u16* READ_BUFFER;
        int* HPOS;
        int* VPOS;
        int* V_STATE;
        int* H_STATE;
    };

public:
    HuC6270(HuC6280* huC6280);
    ~HuC6270();
    void Init();
    void Reset();
    u16 Clock();
    void SetHSync(bool active);
    void SetVSync(bool active);
    u8 ReadRegister(u16 address);
    void WriteRegister(u16 address, u8 value);
    HuC6270_State* GetState();
    u16* GetVRAM();
    u16* GetSAT();
    void SetNoSpriteLimit(bool no_sprite_limit);

private:

    struct HuC6270_Sprite_Data
    {
        int index;
        u16 x;
        u16 flags;
        u8 palette;
        u16 data[4];
    };

private:
    HuC6280* m_huc6280;
    HuC6270_State m_state;
    u16* m_vram;
    u16 m_address_register;
    u16 m_status_register;
    u16 m_register[20];
    u16* m_sat;
    u16 m_read_buffer;
    bool m_trigger_sat_transfer;
    bool m_auto_sat_transfer;
    int m_sat_transfer_pending;
    int m_hpos;
    int m_vpos;
    int m_bg_offset_y;
    int m_bg_counter_y;
    bool m_increment_bg_counter_y;
    int m_raster_line;
    u16 m_latched_bxr;
    u16 m_latched_hds;
    u16 m_latched_hdw;
    u16 m_latched_hde;
    u16 m_latched_hsw;
    u16 m_latched_vds;
    u16 m_latched_vdw;
    u16 m_latched_vcr;
    u16 m_latched_vsw;
    u16 m_latched_mwr;
    u16 m_latched_cr;
    int m_v_state;
    int m_h_state;
    int m_lines_to_next_v_state;
    int m_clocks_to_next_h_state;
    bool m_vblank_triggered;
    bool m_active_line;
    u16 m_line_buffer[1024];
    u16 m_line_buffer_sprites[1024];
    int m_line_buffer_index;
    bool m_no_sprite_limit;
    int m_sprite_count;
    HuC6270_Sprite_Data m_sprites[128];

private:
    void NextVerticalState();
    void NextHorizontalState();
    void VBlankIRQ();
    void RCRIRQ();
    void OverflowIRQ();
    void SpriteCollisionIRQ();
    int ClocksToBYRLatch();
    int ClocksToBXRLatch();
    void RenderLine();
    void RenderBackground(int width);
    void RenderSprites(int width);
    void FetchSprites();
};

static const u16 k_register_mask[20] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000,
    0x1FFF, 0x03FF, 0x03FF, 0x01FF, 0x00FF,
    0x7F1F, 0x7F7F, 0xFF1F, 0x01FF, 0x00FF,
    0x001F, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF };

static const int k_huc6270_screen_size_x[8] = { 32, 64, 128, 128, 32, 64, 128, 128 };
static const int k_huc6270_screen_size_y[8] = { 32, 32, 32, 32, 64, 64, 64, 64 };
static const int k_huc6270_screen_size_x_pixels[8] = { 32 * 8, 64 * 8, 128 * 8, 128 * 8, 32 * 8, 64 * 8, 128 * 8, 128 * 8 };
static const int k_huc6270_screen_size_y_pixels[8] = { 32 * 8, 32 * 8, 32 * 8, 32 * 8, 64 * 8, 64 * 8, 64 * 8, 64 * 8 };
static const int k_huc6270_read_write_increment[4] = { 0x01, 0x20, 0x40, 0x80 };
static const int k_huc6270_sprite_width[2] = { 16, 32 };
static const int k_huc6270_sprite_height[4] = { 16, 32, 64, 64 };
static const int k_huc6270_sprite_mask_width[2] = { 0xFFFF, 0xFFFE };
static const int k_huc6270_sprite_mask_height[4] = { 0xFFFF, 0xFFFD, 0xFFF9, 0xFFF9 };

static const char* const k_register_names_aligned[32] = {
    "MAWR ", "MARR ", "VWR  ", "???  ", "???  ", "CR   ", "RCR  ", "BXR  ",
    "BYR  ", "MWR  ", "HSR  ", "HDR  ", "VSR  ", "VDR  ", "VCR  ", "DCR  ",
    "SOUR ", "DESR ", "LENR ", "DVSSR", "???  ", "???  ", "???  ", "???  ",
    "???  ", "???  ", "???  ", "???  ", "???  ", "???  ", "???  ", "???  " };

static const char* const k_register_names[32] = {
    "MAWR", "MARR", "VWR",  "???",   "???",  "CR",   "RCR", "BXR",
    "BYR",  "MWR",  "HSR",  "HDR",   "VSR",  "VDR",  "VCR", "DCR",
    "SOUR", "DESR", "LENR", "DVSSR", "???" , "???" , "???", "???",
    "???",  "???",  "???",  "???",   "???",  "???",  "???", "???" };

#include "huc6270_inline.h"

#endif /* HUC6270_H */