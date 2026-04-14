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

#ifndef LIBRETRO_CORE_OPTIONS_H__
#define LIBRETRO_CORE_OPTIONS_H__

#include <stdlib.h>
#include <string.h>

#include "libretro.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TURBO_SPEED_VALUES \
    { "1",  NULL }, { "2",  NULL }, { "3",  NULL }, { "4",  NULL }, \
    { "5",  NULL }, { "6",  NULL }, { "7",  NULL }, { "8",  NULL }, \
    { "9",  NULL }, { "10", NULL }, { "11", NULL }, { "12", NULL }, \
    { "13", NULL }, { "14", NULL }, { "15", NULL }, { NULL, NULL }

/*
 ********************************
 * Core Option Definitions
 ********************************
 */

struct retro_core_option_v2_category option_cats_us[] = {
    {
        "system",
        "System",
        "Configure system type, backup RAM, netplay and other system-level settings."
    },
    {
        "video",
        "Video",
        "Configure aspect ratio, display cropping, scanline count, colors and video filter settings."
    },
    {
        "input",
        "Input",
        "Configure controller behavior, TurboTap, turbo buttons and other input settings."
    },
    {
        "audio",
        "Audio",
        "Configure audio chip and volume settings."
    },
    {
        "cdrom",
        "CD-ROM",
        "Configure CD-ROM system type, BIOS and preloading settings."
    },
    { NULL, NULL, NULL },
};

struct retro_core_option_v2_definition option_defs_us[] = {

    /* System */

    {
        "geargrafx_console_type",
        "System (restart)",
        NULL,
        "Select the console type to emulate. 'Auto' automatically detects the appropriate console type based on the loaded content. Many US games will not start if a Japanese system is detected.",
        NULL,
        "system",
        {
            { "Auto",              NULL },
            { "PC Engine (JAP)",   NULL },
            { "SuperGrafx (JAP)",  NULL },
            { "TurboGrafx-16 (USA)", NULL },
            { NULL, NULL },
        },
        "Auto"
    },
    {
        "geargrafx_backup_ram",
        "Backup RAM (restart)",
        NULL,
        "Enable or disable backup RAM. Disabling this is not recommended as it prevents games from saving data.",
        NULL,
        "system",
        {
            { "Enabled",  NULL },
            { "Disabled", NULL },
            { NULL, NULL },
        },
        "Enabled"
    },
    {
        "geargrafx_deterministic_netplay",
        "Deterministic Netplay",
        NULL,
        "When enabled, ensures deterministic emulation behavior for netplay by setting consistent reset values for memory and hardware registers. This helps prevent desyncs during netplay sessions.",
        NULL,
        "system",
        {
            { "Disabled", NULL },
            { "Enabled",  NULL },
            { NULL, NULL },
        },
        "Disabled"
    },
    {
        "geargrafx_safe_vdc_defaults",
        "Safe VDC Defaults (Homebrew)",
        NULL,
        "When enabled, sets safe default values for the VDC (Video Display Controller) registers. This can help some homebrew software run correctly.",
        NULL,
        "system",
        {
            { "Disabled", NULL },
            { "Enabled",  NULL },
            { NULL, NULL },
        },
        "Disabled"
    },

    /* Video */

    {
        "geargrafx_aspect_ratio",
        "Aspect Ratio",
        NULL,
        "Select which aspect ratio will be presented by the core. '1:1 PAR' selects an aspect ratio that produces square pixels.",
        NULL,
        "video",
        {
            { "1:1 PAR",  NULL },
            { "4:3 DAR",  NULL },
            { "6:5 DAR",  NULL },
            { "16:9 DAR", NULL },
            { "16:10 DAR", NULL },
            { NULL, NULL },
        },
        "1:1 PAR"
    },
    {
        "geargrafx_overscan",
        "Overscan",
        NULL,
        "Enable or disable overscan (borders). Overscan width is dependent on the content.",
        NULL,
        "video",
        {
            { "Disabled", NULL },
            { "Enabled",  NULL },
            { NULL, NULL },
        },
        "Disabled"
    },
    {
        "geargrafx_scanline_count",
        "Scanline Count",
        NULL,
        "Select which scanline count will be used in emulation. '224p' forces 224 scanlines. '240p' forces 240 scanlines. 'Manual' lets you set the first and last scanline manually.",
        NULL,
        "video",
        {
            { "224p",   NULL },
            { "240p",   NULL },
            { "Manual", NULL },
            { NULL, NULL },
        },
        "224p"
    },
    {
        "geargrafx_scanline_start",
        "Scanline Start (Manual)",
        NULL,
        "Set the first scanline to be displayed. Scanline 0 is the first visible scanline. Only used when 'Scanline Count' is set to 'Manual'.",
        NULL,
        "video",
        {
            { "0",  NULL },
            { "1",  NULL },
            { "2",  NULL },
            { "3",  NULL },
            { "4",  NULL },
            { "5",  NULL },
            { "6",  NULL },
            { "7",  NULL },
            { "8",  NULL },
            { "9",  NULL },
            { "10", NULL },
            { "11", NULL },
            { "12", NULL },
            { "13", NULL },
            { "14", NULL },
            { "15", NULL },
            { "16", NULL },
            { "17", NULL },
            { "18", NULL },
            { "19", NULL },
            { "20", NULL },
            { "21", NULL },
            { "22", NULL },
            { "23", NULL },
            { "24", NULL },
            { "25", NULL },
            { "26", NULL },
            { "27", NULL },
            { "28", NULL },
            { "29", NULL },
            { "30", NULL },
            { NULL, NULL },
        },
        "3"
    },
    {
        "geargrafx_scanline_end",
        "Scanline End (Manual)",
        NULL,
        "Set the last scanline to be displayed. Scanline 241 is the last visible scanline. Only used when 'Scanline Count' is set to 'Manual'.",
        NULL,
        "video",
        {
            { "220", NULL },
            { "221", NULL },
            { "222", NULL },
            { "223", NULL },
            { "224", NULL },
            { "225", NULL },
            { "226", NULL },
            { "227", NULL },
            { "228", NULL },
            { "229", NULL },
            { "230", NULL },
            { "231", NULL },
            { "232", NULL },
            { "233", NULL },
            { "234", NULL },
            { "235", NULL },
            { "236", NULL },
            { "237", NULL },
            { "238", NULL },
            { "239", NULL },
            { "240", NULL },
            { "241", NULL },
            { NULL, NULL },
        },
        "241"
    },
    {
        "geargrafx_composite_colors",
        "Composite Colors",
        NULL,
        "If enabled, the core will use composite colors instead of RGB colors.",
        NULL,
        "video",
        {
            { "Disabled", NULL },
            { "Enabled",  NULL },
            { NULL, NULL },
        },
        "Disabled"
    },
    {
        "geargrafx_no_sprite_limit",
        "No Sprite Limit",
        NULL,
        "Remove the per-line sprite limit. This reduces flickering but may cause glitches in certain games. It's best to keep this option disabled.",
        NULL,
        "video",
        {
            { "Disabled", NULL },
            { "Enabled",  NULL },
            { NULL, NULL },
        },
        "Disabled"
    },
    {
        "geargrafx_lowpass_filter",
        "Video Low-Pass Filter",
        "Low-Pass Filter",
        "Enable a low-pass video filter to simulate the signal degradation of analog video output on CRT displays.",
        "Enable a low-pass video filter to simulate analog CRT signal.",
        "video",
        {
            { "Disabled", NULL },
            { "Enabled",  NULL },
            { NULL, NULL },
        },
        "Disabled"
    },
    {
        "geargrafx_lowpass_intensity",
        "Video LPF Intensity",
        "LPF Intensity",
        "Set the intensity of the video low-pass filter as a percentage from 0 to 100.",
        "Set the filter intensity (0-100%).",
        "video",
        {
            { "0",   NULL },
            { "10",  NULL },
            { "20",  NULL },
            { "30",  NULL },
            { "40",  NULL },
            { "50",  NULL },
            { "60",  NULL },
            { "70",  NULL },
            { "80",  NULL },
            { "90",  NULL },
            { "100", NULL },
            { NULL, NULL },
        },
        "100"
    },
    {
        "geargrafx_lowpass_cutoff",
        "Video LPF Cutoff",
        "LPF Cutoff",
        "Set the cutoff frequency of the video low-pass filter. Lower values produce a softer image.",
        "Set the cutoff frequency. Lower values produce a softer image.",
        "video",
        {
            { "3.0 MHz", NULL },
            { "3.5 MHz", NULL },
            { "4.0 MHz", NULL },
            { "4.5 MHz", NULL },
            { "5.0 MHz", NULL },
            { "5.5 MHz", NULL },
            { "6.0 MHz", NULL },
            { "6.5 MHz", NULL },
            { "7.0 MHz", NULL },
            { NULL, NULL },
        },
        "5.0 MHz"
    },
    {
        "geargrafx_lowpass_speed_536",
        "Video LPF HuC6270 5.36 MHz",
        "LPF HuC6270 5.36 MHz",
        "Apply the video low-pass filter when HuC6270 is running in 5.36 MHz dot clock mode (256px width).",
        "Apply the filter in 5.36 MHz dot clock mode (256px width).",
        "video",
        {
            { "Disabled", NULL },
            { "Enabled",  NULL },
            { NULL, NULL },
        },
        "Disabled"
    },
    {
        "geargrafx_lowpass_speed_716",
        "Video LPF HuC6270 7.16 MHz",
        "LPF HuC6270 7.16 MHz",
        "Apply the video low-pass filter when HuC6270 is running in 7.16 MHz dot clock mode (341px width).",
        "Apply the filter in 7.16 MHz dot clock mode (341px width).",
        "video",
        {
            { "Enabled",  NULL },
            { "Disabled", NULL },
            { NULL, NULL },
        },
        "Enabled"
    },
    {
        "geargrafx_lowpass_speed_108",
        "Video LPF HuC6270 10.8 MHz",
        "LPF HuC6270 10.8 MHz",
        "Apply the video low-pass filter when HuC6270 is running in 10.8 MHz dot clock mode (512px width).",
        "Apply the filter in 10.8 MHz dot clock mode (512px width).",
        "video",
        {
            { "Enabled",  NULL },
            { "Disabled", NULL },
            { NULL, NULL },
        },
        "Enabled"
    },

    /* Audio */

    {
        "geargrafx_psg_huc6280a",
        "HuC6280A Audio Chip",
        NULL,
        "Enable the HuC6280A audio chip, as found in the SuperGrafx and CoreGrafx I. When disabled, the original HuC6280 chip from the PC Engine is used instead.",
        NULL,
        "audio",
        {
            { "Enabled",  NULL },
            { "Disabled", NULL },
            { NULL, NULL },
        },
        "Enabled"
    },
    {
        "geargrafx_psg_volume",
        "PSG Volume",
        NULL,
        "Set the volume of the PSG (Programmable Sound Generator). The value is a percentage from 0 to 200, where 100 is the default volume.",
        NULL,
        "audio",
        {
            { "0",   NULL },
            { "10",  NULL },
            { "20",  NULL },
            { "30",  NULL },
            { "40",  NULL },
            { "50",  NULL },
            { "60",  NULL },
            { "70",  NULL },
            { "80",  NULL },
            { "90",  NULL },
            { "100", NULL },
            { "110", NULL },
            { "120", NULL },
            { "130", NULL },
            { "140", NULL },
            { "150", NULL },
            { "160", NULL },
            { "170", NULL },
            { "180", NULL },
            { "190", NULL },
            { "200", NULL },
            { NULL, NULL },
        },
        "100"
    },
    {
        "geargrafx_cdrom_volume",
        "CD-ROM Volume",
        NULL,
        "Set the volume of the CD-ROM audio, used for music in CD-ROM games. The value is a percentage from 0 to 200, where 100 is the default volume.",
        NULL,
        "audio",
        {
            { "0",   NULL },
            { "10",  NULL },
            { "20",  NULL },
            { "30",  NULL },
            { "40",  NULL },
            { "50",  NULL },
            { "60",  NULL },
            { "70",  NULL },
            { "80",  NULL },
            { "90",  NULL },
            { "100", NULL },
            { "110", NULL },
            { "120", NULL },
            { "130", NULL },
            { "140", NULL },
            { "150", NULL },
            { "160", NULL },
            { "170", NULL },
            { "180", NULL },
            { "190", NULL },
            { "200", NULL },
            { NULL, NULL },
        },
        "100"
    },
    {
        "geargrafx_adpcm_volume",
        "ADPCM Volume",
        NULL,
        "Set the volume of the ADPCM audio, typically used for speech in CD-ROM games. The value is a percentage from 0 to 200, where 100 is the default volume.",
        NULL,
        "audio",
        {
            { "0",   NULL },
            { "10",  NULL },
            { "20",  NULL },
            { "30",  NULL },
            { "40",  NULL },
            { "50",  NULL },
            { "60",  NULL },
            { "70",  NULL },
            { "80",  NULL },
            { "90",  NULL },
            { "100", NULL },
            { "110", NULL },
            { "120", NULL },
            { "130", NULL },
            { "140", NULL },
            { "150", NULL },
            { "160", NULL },
            { "170", NULL },
            { "180", NULL },
            { "190", NULL },
            { "200", NULL },
            { NULL, NULL },
        },
        "100"
    },

    /* CD-ROM */

    {
        "geargrafx_cdrom_type",
        "CD-ROM (restart)",
        NULL,
        "Select the CD-ROM system type. 'Auto' automatically selects the appropriate CD-ROM system based on the loaded content.",
        NULL,
        "cdrom",
        {
            { "Auto",        NULL },
            { "Standard",    NULL },
            { "Super CD-ROM", NULL },
            { "Arcade CD-ROM", NULL },
            { NULL, NULL },
        },
        "Auto"
    },
    {
        "geargrafx_cdrom_bios",
        "CD-ROM Bios (restart)",
        NULL,
        "Specify the BIOS file to use for CD-ROM emulation. 'Auto' automatically selects the appropriate BIOS based on the loaded content. You can also manually choose one for compatibility with specific games.",
        NULL,
        "cdrom",
        {
            { "Auto",          NULL },
            { "System Card 1", NULL },
            { "System Card 2", NULL },
            { "System Card 3", NULL },
            { "Game Express",  NULL },
            { NULL, NULL },
        },
        "Auto"
    },
    {
        "geargrafx_cdrom_preload",
        "Preload CD-ROM (restart)",
        NULL,
        "Preload all CD-ROM tracks into RAM. This increases memory usage but may improve performance.",
        NULL,
        "cdrom",
        {
            { "Disabled", NULL },
            { "Enabled",  NULL },
            { NULL, NULL },
        },
        "Disabled"
    },

    /* Input */

    {
        "geargrafx_up_down_allowed",
        "Allow Up+Down / Left+Right",
        NULL,
        "Allow pressing, quickly alternating, or holding both left and right (or up and down) directions at the same time. This may cause movement based glitches in certain games.",
        NULL,
        "input",
        {
            { "Disabled", NULL },
            { "Enabled",  NULL },
            { NULL, NULL },
        },
        "Disabled"
    },
    {
        "geargrafx_soft_reset",
        "Allow Soft Reset",
        NULL,
        "Pressing RUN and SELECT simultaneously on the PCE gamepad will soft reset the console. This is the default hardware behavior. Disable this if you want the soft reset functionality turned off.",
        NULL,
        "input",
        {
            { "Enabled",  NULL },
            { "Disabled", NULL },
            { NULL, NULL },
        },
        "Enabled"
    },
    {
        "geargrafx_turbotap",
        "TurboTap",
        NULL,
        "Enable or disable TurboTap support (up to 5 players).",
        NULL,
        "input",
        {
            { "Disabled", NULL },
            { "Enabled",  NULL },
            { NULL, NULL },
        },
        "Disabled"
    },
    {
        "geargrafx_mb128",
        "MB128 Backup Memory",
        NULL,
        "Enable or disable MB128 backup memory support. MB128 is an external memory card device that can be used to save game data across multiple games.",
        NULL,
        "input",
        {
            { "Auto",     NULL },
            { "Enabled",  NULL },
            { "Disabled", NULL },
            { NULL, NULL },
        },
        "Auto"
    },
    {
        "geargrafx_mouse_sensitivity",
        "Mouse Sensitivity",
        NULL,
        "Adjust the sensitivity of the PC Engine Mouse. Higher values result in faster cursor movement.",
        NULL,
        "input",
        {
            { "1",  NULL }, { "2",  NULL }, { "3",  NULL }, { "4",  NULL },
            { "5",  NULL }, { "6",  NULL }, { "7",  NULL }, { "8",  NULL },
            { "9",  NULL }, { "10", NULL }, { "11", NULL }, { "12", NULL },
            { "13", NULL }, { "14", NULL }, { "15", NULL }, { NULL, NULL }
        },
        "5"
    },
    {
        "geargrafx_avenue_pad_3_switch",
        "Avenue Pad 3 Switch",
        NULL,
        "Configure the button mapping for the Avenue Pad 3 controller's third button (III). 'Auto' automatically selects the appropriate mapping based on the game.",
        NULL,
        "input",
        {
            { "Auto",   NULL },
            { "SELECT", NULL },
            { "RUN",    NULL },
            { NULL, NULL },
        },
        "Auto"
    },
    {
        "geargrafx_turbo_toggle_hotkey",
        "Turbo Toggle Hotkey (R2/L2)",
        NULL,
        "When enabled, R2 and L2 act as shortcuts to toggle Turbo I and Turbo II respectively for each player. Toggling updates the corresponding core option below. When disabled, L2 and R2 are ignored.",
        NULL,
        "input",
        {
            { "Disabled", NULL },
            { "Enabled",  NULL },
            { NULL, NULL },
        },
        "Disabled"
    },
    {
        "geargrafx_turbo_p1_i",
        "P1 Turbo I",
        NULL,
        "When enabled, holding button I will rapidly toggle it on and off (auto-fire) for Player 1. Can also be toggled in-game with R2 if the turbo toggle hotkey is enabled.",
        NULL,
        "input",
        {
            { "Disabled", NULL },
            { "Enabled",  NULL },
            { NULL, NULL },
        },
        "Disabled"
    },
    {
        "geargrafx_turbo_speed_p1_i",
        "P1 Turbo I Speed",
        NULL,
        "Sets the number of frames between each auto-fire toggle for button I on Player 1. Lower values fire faster.",
        NULL,
        "input",
        {
            TURBO_SPEED_VALUES
        },
        "4"
    },
    {
        "geargrafx_turbo_p1_ii",
        "P1 Turbo II",
        NULL,
        "When enabled, holding button II will rapidly toggle it on and off (auto-fire) for Player 1. Can also be toggled in-game with L2 if the turbo toggle hotkey is enabled.",
        NULL,
        "input",
        {
            { "Disabled", NULL },
            { "Enabled",  NULL },
            { NULL, NULL },
        },
        "Disabled"
    },
    {
        "geargrafx_turbo_speed_p1_ii",
        "P1 Turbo II Speed",
        NULL,
        "Sets the number of frames between each auto-fire toggle for button II on Player 1. Lower values fire faster.",
        NULL,
        "input",
        {
            TURBO_SPEED_VALUES
        },
        "4"
    },
    {
        "geargrafx_turbo_p2_i",
        "P2 Turbo I",
        NULL,
        "When enabled, holding button I will rapidly toggle it on and off (auto-fire) for Player 2. Can also be toggled in-game with R2 if the turbo toggle hotkey is enabled.",
        NULL,
        "input",
        {
            { "Disabled", NULL },
            { "Enabled",  NULL },
            { NULL, NULL },
        },
        "Disabled"
    },
    {
        "geargrafx_turbo_speed_p2_i",
        "P2 Turbo I Speed",
        NULL,
        "Sets the number of frames between each auto-fire toggle for button I on Player 2. Lower values fire faster.",
        NULL,
        "input",
        {
            TURBO_SPEED_VALUES
        },
        "4"
    },
    {
        "geargrafx_turbo_p2_ii",
        "P2 Turbo II",
        NULL,
        "When enabled, holding button II will rapidly toggle it on and off (auto-fire) for Player 2. Can also be toggled in-game with L2 if the turbo toggle hotkey is enabled.",
        NULL,
        "input",
        {
            { "Disabled", NULL },
            { "Enabled",  NULL },
            { NULL, NULL },
        },
        "Disabled"
    },
    {
        "geargrafx_turbo_speed_p2_ii",
        "P2 Turbo II Speed",
        NULL,
        "Sets the number of frames between each auto-fire toggle for button II on Player 2. Lower values fire faster.",
        NULL,
        "input",
        {
            TURBO_SPEED_VALUES
        },
        "4"
    },
    {
        "geargrafx_turbo_p3_i",
        "P3 Turbo I",
        NULL,
        "When enabled, holding button I will rapidly toggle it on and off (auto-fire) for Player 3. Can also be toggled in-game with R2 if the turbo toggle hotkey is enabled.",
        NULL,
        "input",
        {
            { "Disabled", NULL },
            { "Enabled",  NULL },
            { NULL, NULL },
        },
        "Disabled"
    },
    {
        "geargrafx_turbo_speed_p3_i",
        "P3 Turbo I Speed",
        NULL,
        "Sets the number of frames between each auto-fire toggle for button I on Player 3. Lower values fire faster.",
        NULL,
        "input",
        {
            TURBO_SPEED_VALUES
        },
        "4"
    },
    {
        "geargrafx_turbo_p3_ii",
        "P3 Turbo II",
        NULL,
        "When enabled, holding button II will rapidly toggle it on and off (auto-fire) for Player 3. Can also be toggled in-game with L2 if the turbo toggle hotkey is enabled.",
        NULL,
        "input",
        {
            { "Disabled", NULL },
            { "Enabled",  NULL },
            { NULL, NULL },
        },
        "Disabled"
    },
    {
        "geargrafx_turbo_speed_p3_ii",
        "P3 Turbo II Speed",
        NULL,
        "Sets the number of frames between each auto-fire toggle for button II on Player 3. Lower values fire faster.",
        NULL,
        "input",
        {
            TURBO_SPEED_VALUES
        },
        "4"
    },
    {
        "geargrafx_turbo_p4_i",
        "P4 Turbo I",
        NULL,
        "When enabled, holding button I will rapidly toggle it on and off (auto-fire) for Player 4. Can also be toggled in-game with R2 if the turbo toggle hotkey is enabled.",
        NULL,
        "input",
        {
            { "Disabled", NULL },
            { "Enabled",  NULL },
            { NULL, NULL },
        },
        "Disabled"
    },
    {
        "geargrafx_turbo_speed_p4_i",
        "P4 Turbo I Speed",
        NULL,
        "Sets the number of frames between each auto-fire toggle for button I on Player 4. Lower values fire faster.",
        NULL,
        "input",
        {
            TURBO_SPEED_VALUES
        },
        "4"
    },
    {
        "geargrafx_turbo_p4_ii",
        "P4 Turbo II",
        NULL,
        "When enabled, holding button II will rapidly toggle it on and off (auto-fire) for Player 4. Can also be toggled in-game with L2 if the turbo toggle hotkey is enabled.",
        NULL,
        "input",
        {
            { "Disabled", NULL },
            { "Enabled",  NULL },
            { NULL, NULL },
        },
        "Disabled"
    },
    {
        "geargrafx_turbo_speed_p4_ii",
        "P4 Turbo II Speed",
        NULL,
        "Sets the number of frames between each auto-fire toggle for button II on Player 4. Lower values fire faster.",
        NULL,
        "input",
        {
            TURBO_SPEED_VALUES
        },
        "4"
    },
    {
        "geargrafx_turbo_p5_i",
        "P5 Turbo I",
        NULL,
        "When enabled, holding button I will rapidly toggle it on and off (auto-fire) for Player 5. Can also be toggled in-game with R2 if the turbo toggle hotkey is enabled.",
        NULL,
        "input",
        {
            { "Disabled", NULL },
            { "Enabled",  NULL },
            { NULL, NULL },
        },
        "Disabled"
    },
    {
        "geargrafx_turbo_speed_p5_i",
        "P5 Turbo I Speed",
        NULL,
        "Sets the number of frames between each auto-fire toggle for button I on Player 5. Lower values fire faster.",
        NULL,
        "input",
        {
            TURBO_SPEED_VALUES
        },
        "4"
    },
    {
        "geargrafx_turbo_p5_ii",
        "P5 Turbo II",
        NULL,
        "When enabled, holding button II will rapidly toggle it on and off (auto-fire) for Player 5. Can also be toggled in-game with L2 if the turbo toggle hotkey is enabled.",
        NULL,
        "input",
        {
            { "Disabled", NULL },
            { "Enabled",  NULL },
            { NULL, NULL },
        },
        "Disabled"
    },
    {
        "geargrafx_turbo_speed_p5_ii",
        "P5 Turbo II Speed",
        NULL,
        "Sets the number of frames between each auto-fire toggle for button II on Player 5. Lower values fire faster.",
        NULL,
        "input",
        {
            TURBO_SPEED_VALUES
        },
        "4"
    },

    { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};

struct retro_core_options_v2 options_us = {
    option_cats_us,
    option_defs_us
};

/*
 ********************************
 * Functions
 ********************************
 */

static void libretro_set_core_options(retro_environment_t environ_cb,
        bool *categories_supported)
{
    unsigned version = 0;

    if (!environ_cb || !categories_supported)
        return;

    *categories_supported = false;

    if (!environ_cb(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version))
        version = 0;

    if (version >= 2)
    {
        *categories_supported = environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2,
                &options_us);
    }
    else
    {
        size_t i, j;
        size_t option_index  = 0;
        size_t num_options   = 0;
        struct retro_core_option_definition *option_v1_defs_us = NULL;
        struct retro_variable *variables   = NULL;
        char **values_buf                  = NULL;

        while (true)
        {
            if (option_defs_us[num_options].key)
                num_options++;
            else
                break;
        }

        if (version >= 1)
        {
            option_v1_defs_us = (struct retro_core_option_definition *)
                    calloc(num_options + 1, sizeof(struct retro_core_option_definition));

            for (i = 0; i < num_options; i++)
            {
                struct retro_core_option_v2_definition *option_def_us = &option_defs_us[i];
                struct retro_core_option_value *option_values         = option_def_us->values;
                struct retro_core_option_definition *option_v1_def_us = &option_v1_defs_us[i];
                struct retro_core_option_value *option_v1_values      = option_v1_def_us->values;

                option_v1_def_us->key           = option_def_us->key;
                option_v1_def_us->desc          = option_def_us->desc;
                option_v1_def_us->info          = option_def_us->info;
                option_v1_def_us->default_value = option_def_us->default_value;

                while (option_values->value)
                {
                    option_v1_values->value = option_values->value;
                    option_v1_values->label = option_values->label;

                    option_values++;
                    option_v1_values++;
                }
            }

            environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS, option_v1_defs_us);
        }
        else
        {
            variables  = (struct retro_variable *)calloc(num_options + 1,
                    sizeof(struct retro_variable));
            values_buf = (char **)calloc(num_options, sizeof(char *));

            if (!variables || !values_buf)
                goto error;

            for (i = 0; i < num_options; i++)
            {
                const char *key                        = option_defs_us[i].key;
                const char *desc                       = option_defs_us[i].desc;
                const char *default_value              = option_defs_us[i].default_value;
                struct retro_core_option_value *values  = option_defs_us[i].values;
                size_t buf_len                         = 3;
                size_t default_index                   = 0;

                values_buf[i] = NULL;

                if (desc)
                {
                    size_t num_values = 0;

                    while (true)
                    {
                        if (values[num_values].value)
                        {
                            if (default_value)
                                if (strcmp(values[num_values].value, default_value) == 0)
                                    default_index = num_values;

                            buf_len += strlen(values[num_values].value);
                            num_values++;
                        }
                        else
                            break;
                    }

                    if (num_values > 0)
                    {
                        buf_len += num_values - 1;
                        buf_len += strlen(desc);

                        values_buf[i] = (char *)calloc(buf_len, sizeof(char));
                        if (!values_buf[i])
                            goto error;

                        strcpy(values_buf[i], desc);
                        strcat(values_buf[i], "; ");

                        strcat(values_buf[i], values[default_index].value);

                        for (j = 0; j < num_values; j++)
                        {
                            if (j != default_index)
                            {
                                strcat(values_buf[i], "|");
                                strcat(values_buf[i], values[j].value);
                            }
                        }
                    }
                }

                variables[option_index].key   = key;
                variables[option_index].value = values_buf[i];
                option_index++;
            }

            environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, variables);
        }

error:
        if (option_v1_defs_us)
        {
            free(option_v1_defs_us);
            option_v1_defs_us = NULL;
        }

        if (values_buf)
        {
            for (i = 0; i < num_options; i++)
            {
                if (values_buf[i])
                {
                    free(values_buf[i]);
                    values_buf[i] = NULL;
                }
            }

            free(values_buf);
            values_buf = NULL;
        }

        if (variables)
        {
            free(variables);
            variables = NULL;
        }
    }
}

#ifdef __cplusplus
}
#endif

#endif
