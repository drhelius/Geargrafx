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
#include "gui_colors.h"

struct stDebugLabel
{
    u16 address;
    const char* label;
};

static const stDebugLabel k_debug_laseractive_labels[] =
{
    { 0x18C0, "PAC_SRAM_ENABLE" },
    { 0x18C1, "PAC_ID_AA" },
    { 0x18C2, "PAC_ID_55" },
    { 0x18C3, "PAC_SRAM_SIZE" },
    { 0x1920, "PD_IN_CONTROL" },
    { 0x1921, "PD_IN_MIXING" },
    { 0x1922, "PD_IN_DRIVE" },
    { 0x1923, "PD_IN_PLAYBACK" },
    { 0x1924, "PD_IN_IN_04" },
    { 0x1925, "PD_IN_TRACK_INFO" },
    { 0x1926, "PD_IN_SEEK_MODE" },
    { 0x1927, "PD_IN_SEEK_TRACK" },
    { 0x1928, "PD_IN_SEEK_HIGH" },
    { 0x1929, "PD_IN_SEEK_MID" },
    { 0x192A, "PD_IN_SEEK_LOW" },
    { 0x192B, "PD_IN_SEEK_FRAME" },
    { 0x192C, "PD_IN_VIDEO" },
    { 0x192D, "PD_IN_DIGITAL_AUDIO" },
    { 0x192E, "PD_IN_ANALOG_AUDIO" },
    { 0x192F, "PD_IN_DIGITAL_VOLUME" },
    { 0x1930, "PD_IN_IN_10" },
    { 0x1931, "PD_IN_IN_11" },
    { 0x1932, "PD_IN_IN_12" },
    { 0x1933, "PD_IN_IN_13" },
    { 0x1934, "PD_IN_IN_14" },
    { 0x1935, "PD_IN_IN_15" },
    { 0x1936, "PD_IN_IN_16" },
    { 0x1937, "PD_IN_IN_17" },
    { 0x1938, "PD_IN_IN_18" },
    { 0x1939, "PD_IN_TRANSPARENCY" },
    { 0x193A, "PD_IN_SPRITE_FADER" },
    { 0x193B, "PD_IN_BACKGROUND_FADER" },
    { 0x193C, "PD_IN_BACKDROP_FADER" },
    { 0x193D, "PD_IN_BLANKING_FADER" },
    { 0x193E, "PD_IN_ANALOG_CONTROL" },
    { 0x193F, "PD_IN_ANALOG_ATTENUATION" },
    { 0x1940, "PD_OUT_CONTROL" },
    { 0x1941, "PD_OUT_MODEL" },
    { 0x1942, "PD_OUT_DISC_TYPE" },
    { 0x1943, "PD_OUT_DISC_SIDE" },
    { 0x1944, "PD_OUT_AUDIO" },
    { 0x1945, "PD_OUT_BUTTON" },
    { 0x1946, "PD_OUT_DRIVE" },
    { 0x1947, "PD_OUT_PLAYBACK" },
    { 0x1948, "PD_OUT_DISC_STATUS" },
    { 0x1949, "PD_OUT_ERROR" },
    { 0x194A, "PD_OUT_SEEK_MODE" },
    { 0x194B, "PD_OUT_SEEK_TRACK" },
    { 0x194C, "PD_OUT_SEEK_HIGH" },
    { 0x194D, "PD_OUT_SEEK_MID" },
    { 0x194E, "PD_OUT_SEEK_LOW" },
    { 0x194F, "PD_OUT_SEEK_FRAME" },
    { 0x1950, "PD_OUT_TRACK_INFO" },
    { 0x1951, "PD_OUT_TOC_CONTROL" },
    { 0x1952, "PD_OUT_TOC_MINUTE" },
    { 0x1953, "PD_OUT_TOC_SECOND" },
    { 0x1954, "PD_OUT_TOC_FRAME" },
    { 0x1955, "PD_OUT_TRACK" },
    { 0x1956, "PD_OUT_POSITION_HIGH" },
    { 0x1957, "PD_OUT_POSITION_MID" },
    { 0x1958, "PD_OUT_POSITION_LOW" },
    { 0x1959, "PD_OUT_POSITION_FRAME" },
    { 0x195A, "PD_OUT_STOP_TRACK" },
    { 0x195B, "PD_OUT_STOP_FRAME" },
    { 0x195C, "PD_OUT_STOP_LOW" },
    { 0x195D, "PD_OUT_STOP_MID" },
    { 0x195E, "PD_OUT_STOP_HIGH" },
    { 0x195F, "PD_OUT_STOP_STATUS" },
};

static const stDebugLabel k_debug_labels[] =
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

static const stDebugLabel k_cdrom_bios_symbols[] =
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

static const int k_debug_laseractive_label_count = sizeof(k_debug_laseractive_labels) / sizeof(k_debug_laseractive_labels[0]);
static const int k_cdrom_bios_symbol_count = sizeof(k_cdrom_bios_symbols) / sizeof(k_cdrom_bios_symbols[0]);
static const int k_debug_label_count = sizeof(k_debug_labels) / sizeof(k_debug_labels[0]);

#endif /* GUI_DEBUG_CONSTANTS_H */
