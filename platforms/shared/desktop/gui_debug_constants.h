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

#ifndef GUI_DEBUG_CONSTANTS_H
#define	GUI_DEBUG_CONSTANTS_H

#include "imgui/imgui.h"
#include "geargrafx.h"

static const ImVec4 cyan =          ImVec4(0.10f, 0.90f, 0.90f, 1.0f);
static const ImVec4 dark_cyan =     ImVec4(0.00f, 0.30f, 0.30f, 1.0f);
static const ImVec4 magenta =       ImVec4(1.00f, 0.50f, 0.96f, 1.0f);
static const ImVec4 dark_magenta =  ImVec4(0.30f, 0.18f, 0.27f, 1.0f);
static const ImVec4 yellow =        ImVec4(1.00f, 0.90f, 0.05f, 1.0f);
static const ImVec4 dark_yellow =   ImVec4(0.30f, 0.25f, 0.00f, 1.0f);
static const ImVec4 orange =        ImVec4(1.00f, 0.50f, 0.00f, 1.0f);
static const ImVec4 dark_orange =   ImVec4(0.60f, 0.20f, 0.00f, 1.0f);
static const ImVec4 red =           ImVec4(0.98f, 0.15f, 0.45f, 1.0f);
static const ImVec4 dark_red =      ImVec4(0.30f, 0.04f, 0.16f, 1.0f);
static const ImVec4 green =         ImVec4(0.10f, 0.90f, 0.10f, 1.0f);
static const ImVec4 dark_green =    ImVec4(0.03f, 0.20f, 0.02f, 1.0f);
static const ImVec4 violet =        ImVec4(0.68f, 0.51f, 1.00f, 1.0f);
static const ImVec4 dark_violet =   ImVec4(0.24f, 0.15f, 0.30f, 1.0f);
static const ImVec4 blue =          ImVec4(0.20f, 0.40f, 1.00f, 1.0f);
static const ImVec4 dark_blue =     ImVec4(0.07f, 0.10f, 0.30f, 1.0f);
static const ImVec4 white =         ImVec4(1.00f, 1.00f, 1.00f, 1.0f);
static const ImVec4 gray =          ImVec4(0.50f, 0.50f, 0.50f, 1.0f);
static const ImVec4 mid_gray =      ImVec4(0.40f, 0.40f, 0.40f, 1.0f);
static const ImVec4 dark_gray =     ImVec4(0.10f, 0.10f, 0.10f, 1.0f);
static const ImVec4 black =         ImVec4(0.00f, 0.00f, 0.00f, 1.0f);
static const ImVec4 brown =         ImVec4(0.68f, 0.50f, 0.36f, 1.0f);
static const ImVec4 dark_brown =    ImVec4(0.38f, 0.20f, 0.06f, 1.0f);

static const char* const c_cyan = "{19E6E6}";
static const char* const c_dark_cyan = "{004C4C}";
static const char* const c_magenta = "{FF80F5}";
static const char* const c_dark_magenta = "{4C2E45}";
static const char* const c_yellow = "{FFE60D}";
static const char* const c_dark_yellow = "{4C4000}";
static const char* const c_orange = "{FF8000}";
static const char* const c_dark_orange = "{993300}";
static const char* const c_red = "{FA2673}";
static const char* const c_dark_red = "{4C0A29}";
static const char* const c_green = "{19E619}";
static const char* const c_dark_green = "{083305}";
static const char* const c_violet = "{AD82FF}";
static const char* const c_dark_violet = "{3D274D}";
static const char* const c_blue = "{3366FF}";
static const char* const c_dark_blue = "{12194D}";
static const char* const c_white = "{FFFFFF}";
static const char* const c_gray = "{808080}";
static const char* const c_mid_gray = "{666666}";
static const char* const c_dark_gray = "{1A1A1A}";
static const char* const c_black = "{000000}";
static const char* const c_brown = "{AD805C}";
static const char* const c_dark_brown = "{61330F}";

struct stDebugLabel
{
    u16 address;
    const char* label;
};

static const int k_debug_label_count = 23;
static const stDebugLabel k_debug_labels[k_debug_label_count] = 
{
    { 0x0000, "VDC_ADDRESS_" },
    { 0x0002, "VDC_DATA_LO_" },
    { 0x0003, "VDC_DATA_HI_" },
    { 0x0400, "VCE_CONTROL_" },
    { 0x0402, "VCE_ADDR_LO_" },
    { 0x0403, "VCE_ADDR_HI_" },
    { 0x0404, "VCE_DATA_LO_" },
    { 0x0405, "VCE_DATA_HI_" },
    { 0x0800, "PSG_CH_SELECT_" },
    { 0x0801, "PSG_MAIN_VOL_" },
    { 0x0802, "PSG_FREQ_LO_" },
    { 0x0803, "PSG_FREQ_HI_" },
    { 0x0804, "PSG_CH_CTRL_" },
    { 0x0805, "PSG_CH_VOL_" },
    { 0x0806, "PSG_CH_DATA_" },
    { 0x0807, "PSG_NOISE_" },
    { 0x0808, "PSG_LFO_FREQ_" },
    { 0x0809, "PSG_LFO_CTRL_" },
    { 0x0C00, "TIMER_COUNTER_" },
    { 0x0C01, "TIMER_CONTROL_" },
    { 0x1000, "JOYPAD_" },
    { 0x1402, "IRQ_DISABLE_" },
    { 0x1403, "IRQ_STATUS_" }
};

#endif	/* GUI_DEBUG_CONSTANTS_H */