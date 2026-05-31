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
#define GUI_DEBUG_CONSTANTS_H

#include "imgui.h"
#include "geargrafx.h"
#include "config.h"

struct GuiDebugColor
{
    ImVec4 dark;
    ImVec4 light;

    operator ImVec4() const
    {
        return (config_emulator.theme == config_Theme_Light) ? light : dark;
    }
};

struct GuiDebugTextColor
{
    const char* dark;
    const char* light;

    const char* c_str() const
    {
        return (config_emulator.theme == config_Theme_Light) ? light : dark;
    }

    operator const char*() const
    {
        return c_str();
    }
};

static const GuiDebugColor cyan =          { ImVec4(0.10f, 0.90f, 0.90f, 1.0f), ImVec4(0.00f, 0.49f, 0.60f, 1.0f) };
static const GuiDebugColor dark_cyan =     { ImVec4(0.00f, 0.30f, 0.30f, 1.0f), ImVec4(0.64f, 0.91f, 0.95f, 1.0f) };
static const GuiDebugColor magenta =       { ImVec4(1.00f, 0.50f, 0.96f, 1.0f), ImVec4(0.82f, 0.08f, 0.76f, 1.0f) };
static const GuiDebugColor dark_magenta =  { ImVec4(0.30f, 0.18f, 0.27f, 1.0f), ImVec4(0.93f, 0.74f, 0.92f, 1.0f) };
static const GuiDebugColor yellow =        { ImVec4(1.00f, 0.90f, 0.05f, 1.0f), ImVec4(0.64f, 0.48f, 0.00f, 1.0f) };
static const GuiDebugColor dark_yellow =   { ImVec4(0.30f, 0.25f, 0.00f, 1.0f), ImVec4(0.96f, 0.88f, 0.50f, 1.0f) };
static const GuiDebugColor orange =        { ImVec4(1.00f, 0.50f, 0.00f, 1.0f), ImVec4(0.84f, 0.28f, 0.00f, 1.0f) };
static const GuiDebugColor dark_orange =   { ImVec4(0.60f, 0.20f, 0.00f, 1.0f), ImVec4(0.98f, 0.76f, 0.58f, 1.0f) };
static const GuiDebugColor red =           { ImVec4(0.98f, 0.15f, 0.45f, 1.0f), ImVec4(0.86f, 0.00f, 0.26f, 1.0f) };
static const GuiDebugColor dark_red =      { ImVec4(0.30f, 0.04f, 0.16f, 1.0f), ImVec4(0.97f, 0.68f, 0.78f, 1.0f) };
static const GuiDebugColor green =         { ImVec4(0.10f, 0.90f, 0.10f, 1.0f), ImVec4(0.00f, 0.55f, 0.10f, 1.0f) };
static const GuiDebugColor dim_green =     { ImVec4(0.05f, 0.40f, 0.05f, 1.0f), ImVec4(0.32f, 0.60f, 0.28f, 1.0f) };
static const GuiDebugColor dark_green =    { ImVec4(0.03f, 0.20f, 0.02f, 1.0f), ImVec4(0.68f, 0.91f, 0.64f, 1.0f) };
static const GuiDebugColor violet =        { ImVec4(0.68f, 0.51f, 1.00f, 1.0f), ImVec4(0.46f, 0.24f, 0.82f, 1.0f) };
static const GuiDebugColor dark_violet =   { ImVec4(0.24f, 0.15f, 0.30f, 1.0f), ImVec4(0.80f, 0.70f, 0.94f, 1.0f) };
static const GuiDebugColor blue =          { ImVec4(0.20f, 0.40f, 1.00f, 1.0f), ImVec4(0.05f, 0.24f, 0.88f, 1.0f) };
static const GuiDebugColor dark_blue =     { ImVec4(0.07f, 0.10f, 0.30f, 1.0f), ImVec4(0.68f, 0.76f, 0.96f, 1.0f) };
static const GuiDebugColor white =         { ImVec4(1.00f, 1.00f, 1.00f, 1.0f), ImVec4(0.12f, 0.11f, 0.15f, 1.0f) };
static const GuiDebugColor gray =          { ImVec4(0.50f, 0.50f, 0.50f, 1.0f), ImVec4(0.45f, 0.43f, 0.50f, 1.0f) };
static const GuiDebugColor mid_gray =      { ImVec4(0.40f, 0.40f, 0.40f, 1.0f), ImVec4(0.62f, 0.59f, 0.67f, 1.0f) };
static const GuiDebugColor dark_gray =     { ImVec4(0.10f, 0.10f, 0.10f, 1.0f), ImVec4(0.64f, 0.61f, 0.69f, 1.0f) };
static const GuiDebugColor black =         { ImVec4(0.00f, 0.00f, 0.00f, 1.0f), ImVec4(1.00f, 1.00f, 1.00f, 1.0f) };
static const GuiDebugColor brown =         { ImVec4(0.68f, 0.50f, 0.36f, 1.0f), ImVec4(0.56f, 0.30f, 0.10f, 1.0f) };
static const GuiDebugColor dark_brown =    { ImVec4(0.38f, 0.20f, 0.06f, 1.0f), ImVec4(0.90f, 0.72f, 0.55f, 1.0f) };

static const GuiDebugTextColor c_cyan = { "{19E6E6}", "{007D99}" };
static const GuiDebugTextColor c_dark_cyan = { "{004C4C}", "{A3E8F2}" };
static const GuiDebugTextColor c_magenta = { "{FF80F5}", "{D114C2}" };
static const GuiDebugTextColor c_dark_magenta = { "{4C2E45}", "{EDBDEB}" };
static const GuiDebugTextColor c_yellow = { "{FFE60D}", "{A37A00}" };
static const GuiDebugTextColor c_dark_yellow = { "{4C4000}", "{F5E080}" };
static const GuiDebugTextColor c_orange = { "{FF8000}", "{D64700}" };
static const GuiDebugTextColor c_dark_orange = { "{993300}", "{FAC294}" };
static const GuiDebugTextColor c_red = { "{FA2673}", "{DB0042}" };
static const GuiDebugTextColor c_dark_red = { "{4C0A29}", "{F7ADC7}" };
static const GuiDebugTextColor c_green = { "{19E619}", "{008C1A}" };
static const GuiDebugTextColor c_dim_green = { "{0D660D}", "{529947}" };
static const GuiDebugTextColor c_dark_green = { "{083305}", "{ADE8A3}" };
static const GuiDebugTextColor c_violet = { "{AD82FF}", "{753DD1}" };
static const GuiDebugTextColor c_dark_violet = { "{3D274D}", "{CCB3F0}" };
static const GuiDebugTextColor c_blue = { "{3366FF}", "{0D3DE0}" };
static const GuiDebugTextColor c_dark_blue = { "{12194D}", "{ADC2F5}" };
static const GuiDebugTextColor c_white = { "{FFFFFF}", "{1F1D26}" };
static const GuiDebugTextColor c_gray = { "{808080}", "{736E80}" };
static const GuiDebugTextColor c_mid_gray = { "{666666}", "{9E96AB}" };
static const GuiDebugTextColor c_dark_gray = { "{1A1A1A}", "{A39CB0}" };
static const GuiDebugTextColor c_black = { "{000000}", "{000000}" };
static const GuiDebugTextColor c_brown = { "{AD805C}", "{8F4D1A}" };
static const GuiDebugTextColor c_dark_brown = { "{61330F}", "{E6B88C}" };

static inline ImVec4 gui_debug_lerp_color(const ImVec4& a, const ImVec4& b, float t)
{
    return ImVec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t);
}

struct stDebugLabel
{
    u16 address;
    const char* label;
};

static const int k_debug_label_count = 43;
static const stDebugLabel k_debug_labels[k_debug_label_count] = 
{
    { 0x0000, "VDC_ADDRESS" },
    { 0x0002, "VDC_DATA_LO" },
    { 0x0003, "VDC_DATA_HI" },
    { 0x0400, "VCE_CONTROL" },
    { 0x0402, "VCE_ADDR_LO" },
    { 0x0403, "VCE_ADDR_HI" },
    { 0x0404, "VCE_DATA_LO" },
    { 0x0405, "VCE_DATA_HI" },
    { 0x0800, "PSG_CH_SELECT" },
    { 0x0801, "PSG_MAIN_VOL" },
    { 0x0802, "PSG_FREQ_LO" },
    { 0x0803, "PSG_FREQ_HI" },
    { 0x0804, "PSG_CH_CTRL" },
    { 0x0805, "PSG_CH_VOL" },
    { 0x0806, "PSG_CH_DATA" },
    { 0x0807, "PSG_NOISE" },
    { 0x0808, "PSG_LFO_FREQ" },
    { 0x0809, "PSG_LFO_CTRL" },
    { 0x0C00, "TIMER_COUNTER" },
    { 0x0C01, "TIMER_CONTROL" },
    { 0x1000, "JOYPAD" },
    { 0x1402, "IRQ_DISABLE" },
    { 0x1403, "IRQ_STATUS" },
    { 0x1800, "CD_STATUS" },
    { 0x1801, "CD_DATA_BUS" },
    { 0x1802, "CD_ENABLED_IRQS" },
    { 0x1803, "CD_ACTIVE_IRQS" },
    { 0x1804, "CD_RESET" },
    { 0x1805, "CD_PCM_LSB" },
    { 0x1806, "CD_PCM_MSB" },
    { 0x1807, "CD_BRAM_UNLOCK" },
    { 0x1808, "CD_DATA_ACK_ADPCM_LSB" },
    { 0x1809, "CD_ADPCM_MSB" },
    { 0x180A, "CD_ADPCM_DATA" },
    { 0x180B, "CD_ADPCM_DMA" },
    { 0x180C, "CD_ADPCM_STATUS" },
    { 0x180D, "CD_ADPCM_CONTROL" },
    { 0x180E, "CD_ADPCM_RATE" },
    { 0x180F, "CD_AUDIO_FADER" },
    { 0x18C0, "CD_SIGNATURE0" },
    { 0x18C1, "CD_SIGNATURE1" },
    { 0x18C2, "CD_SIGNATURE2" },
    { 0x18C3, "CD_SIGNATURE3" }
};

static const int k_cdrom_bios_symbol_count = 76;

static const stDebugLabel k_cdrom_bios_symbols[k_cdrom_bios_symbol_count] = 
{
    // CD commands
    { 0xE000, "CD_BOOT"     },
    { 0xE003, "CD_RESET"    },
    { 0xE006, "CD_BASE"     },
    { 0xE009, "CD_READ"     },
    { 0xE00C, "CD_SEEK"     },
    { 0xE00F, "CD_EXEC"     },
    { 0xE012, "CD_PLAY"     },
    { 0xE015, "CD_SEARCH"   },
    { 0xE018, "CD_PAUSE"    },
    { 0xE01B, "CD_STAT"     },
    { 0xE01E, "CD_SUBQ"     },
    { 0xE021, "CD_DINFO"    },
    { 0xE024, "CD_CONTENTS" },
    { 0xE027, "CD_SUBRD"    },
    { 0xE02A, "CD_PCMRD"    },
    { 0xE02D, "CD_FADE"     },

    // ADPCM commands
    { 0xE030, "AD_RESET"    },
    { 0xE033, "AD_TRANS"    },
    { 0xE036, "AD_READ"     },
    { 0xE039, "AD_WRITE"    },
    { 0xE03C, "AD_PLAY"     },
    { 0xE03F, "AD_CPLAY"    },
    { 0xE042, "AD_STOP"     },
    { 0xE045, "AD_STAT"     },

    // Block manager
    { 0xE048, "BM_FORMAT"   },
    { 0xE04B, "BM_FREE"     },
    { 0xE04E, "BM_READ"     },
    { 0xE051, "BM_WRITE"    },
    { 0xE054, "BM_DELETE"   },
    { 0xE057, "BM_FILES"    },

    // System extensions (I)
    { 0xE05A, "EX_GETVER"   },
    { 0xE05D, "EX_SETVEC"   },
    { 0xE060, "EX_GETFNT"   },
    { 0xE063, "EX_JOYSNS"   },
    { 0xE066, "EX_JOYREP"   },
    { 0xE069, "EX_SCRSIZ"   },

    // System extensions (II)
    { 0xE06C, "EX_DOTMOD"   },
    { 0xE06F, "EX_SCRMOD"   },
    { 0xE072, "EX_IMODE"    },
    { 0xE075, "EX_VMODE"    },
    { 0xE078, "EX_HMODE"    },
    { 0xE07B, "EX_VSYNC"    },
    { 0xE07E, "EX_RCRON"    },
    { 0xE081, "EX_RCROFF"   },
    { 0xE084, "EX_IRQON"    },
    { 0xE087, "EX_IRQOFF"   },
    { 0xE08A, "EX_BGON"     },
    { 0xE08D, "EX_BGOFF"    },
    { 0xE090, "EX_SPRON"    },
    { 0xE093, "EX_SPROFF"   },
    { 0xE096, "EX_DSPON"    },
    { 0xE099, "EX_DSPOFF"   },
    { 0xE09C, "EX_DMAMOD"   },
    { 0xE09F, "EX_SPRDMA"   },
    { 0xE0A2, "EX_SATCLR"   },
    { 0xE0A5, "EX_SPRPUT"   },
    { 0xE0A8, "EX_SETRCR"   },
    { 0xE0AB, "EX_SETRED"   },
    { 0xE0AE, "EX_SETWRT"   },
    { 0xE0B1, "EX_SETDMA"   },
    { 0xE0B4, "EX_COLORCMD" },
    { 0xE0B7, "EX_BINBCD"   },
    { 0xE0BA, "EX_BCDBIN"   },
    { 0xE0BD, "EX_RND"      },

    // Maths
    { 0xE0C0, "MA_MUL8U"    },
    { 0xE0C3, "MA_MUL8S"    },
    { 0xE0C6, "MA_MUL16U"   },
    { 0xE0C9, "MA_DIV16S"   },
    { 0xE0CC, "MA_DIV16U"   },
    { 0xE0CF, "MA_SQRT"     },
    { 0xE0D2, "MA_SIN"      },
    { 0xE0D5, "MA_COS"      },
    { 0xE0D8, "MA_ATNI"     },

    // PSG
    { 0xE0DB, "PSG_BIOS"    },
    { 0xE0DE, "GRP_BIOS"    },
    { 0xE0E1, "PSG_DRIVE"   }
};

#endif /* GUI_DEBUG_CONSTANTS_H */