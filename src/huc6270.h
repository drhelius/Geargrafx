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
        HuC6270_VERTICAL_STATE_VSW,
        HuC6270_VERTICAL_STATE_VDS,
        HuC6270_VERTICAL_STATE_VDW,
        HuC6270_VERTICAL_STATE_VCR
    };

    enum HuC6270_Horizontal_State
    {
        HuC6270_HORIZONTAL_STATE_HDS,
        HuC6270_HORIZONTAL_STATE_HDW,
        HuC6270_HORIZONTAL_STATE_HDE,
        HuC6270_HORIZONTAL_STATE_HSW
    };

    struct HuC6270_State
    {
        u8* AR;
        u8* SR;
        u16* R;
        u16* READ_BUFFER;
        int* HPOS;
        int* VPOS;
        bool* HSYNC;
        bool* VSYNC;
        HuC6270_Vertical_State* V_STATE;
        HuC6270_Horizontal_State* H_STATE;
    };

public:
    HuC6270(HuC6280* huC6280);
    ~HuC6270();
    void Init();
    void Reset();
    u16 Clock(bool* active);
    void SetHSync(bool active);
    void SetVSync(bool active);
    u8 ReadRegister(u32 address);
    void WriteRegister(u32 address, u8 value);
    HuC6270_State* GetState();
    u16* GetVRAM();
    u16* GetSAT();

private:
    HuC6280* m_huc6280;
    HuC6270_State m_state;
    u16* m_vram;
    u8 m_address_register;
    u8 m_status_register;
    u16 m_register[20];
    u16* m_sat;
    u16 m_read_buffer;
    int m_hpos;
    int m_vpos;
    int m_raster_line;
    bool m_trigger_sat_transfer;
    bool m_auto_sat_transfer;
    u8 m_latched_byr;
    bool m_hsync;
    bool m_vsync;
    HuC6270_Vertical_State m_v_state;
    HuC6270_Horizontal_State m_h_state;
    int m_clocks_to_next_v_state;
    int m_clocks_to_next_h_state;
    int m_hds_clocks;

private:
    void NextVerticalState();
    void NextHorizontalState();
};

static const u16 k_register_mask[20] = {
    0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000,
    0x1FFF, 0x03FF, 0x03FF, 0x01FF, 0x00FF,
    0x7F1F, 0x7F7F, 0xFF1F, 0x01FF, 0x00FF,
    0x001F, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF };

static const int k_scren_size_x[8] = { 32, 64, 128, 128, 32, 64, 128, 128 };
static const int k_scren_size_y[8] = { 32, 32, 32, 32, 64, 64, 64, 64 };
static const int k_read_write_increment[4] = { 0x01, 0x20, 0x40, 0x80 };

static const char* const k_register_names_aligned[20] = {
    "MAWR ", "MARR ", "VWR  ", "???  ", "???  ",
    "CR   ", "RCR  ", "BXR  ", "BYR  ", "MWR  ",
    "HSR  ", "HDR  ", "VPR  ", "VDR  ", "VCR  ",
    "DCR  ", "SOUR ", "DESR ", "LENR ", "DVSSR" };

static const char* const k_register_names[20] = {
    "MAWR", "MARR", "VWR",  "???",  "???",
    "CR",   "RCR",  "BXR",  "BYR",  "MWR",
    "HSR",  "HDR",  "VPR",  "VDR",  "VCR",
    "DCR",  "SOUR", "DESR", "LENR", "DVSSR" };

#include "huc6270_inline.h"

#endif /* HUC6270_H */