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

#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint64_t u64;
typedef int64_t s64;

struct GG_Runtime_Info
{
    int screen_width;
    int screen_height;
    int width_scale;
};

enum GG_Console_Type
{
    GG_CONSOLE_AUTO = 0,
    GG_CONSOLE_PCE,
    GG_CONSOLE_SGX,
    GG_CONSOLE_TG16
};

enum GG_CDROM_Type
{
    GG_CDROM_AUTO = 0,
    GG_CDROM_STANDARD,
    GG_CDROM_SUPER_CDROM,
    GG_CDROM_ARCADE_CARD
};

struct GG_Color
{
    u8 red;
    u8 green;
    u8 blue;
};

enum GG_Pixel_Format
{
    GG_PIXEL_RGB565,
    GG_PIXEL_RGBA8888,
};

enum GG_Keys
{
    GG_KEY_NONE = 0x00,
    GG_KEY_I = 0x01,
    GG_KEY_II = 0x02,
    GG_KEY_SELECT = 0x04,
    GG_KEY_RUN = 0x08,
    GG_KEY_UP = 0x10,
    GG_KEY_RIGHT = 0x20,
    GG_KEY_DOWN = 0x40,
    GG_KEY_LEFT = 0x80,
    GG_KEY_III = 0x100,
    GG_KEY_IV = 0x200,
    GG_KEY_V = 0x400,
    GG_KEY_VI = 0x800,
};

enum GG_Controller_Type
{
    GG_CONTROLLER_STANDARD = 0,
    GG_CONTROLLER_AVENUE_PAD_3,
    GG_CONTROLLER_AVENUE_PAD_6
};

enum GG_Controllers
{
    GG_CONTROLLER_1 = 0,
    GG_CONTROLLER_2 = 1,
    GG_CONTROLLER_3 = 2,
    GG_CONTROLLER_4 = 3,
    GG_CONTROLLER_5 = 4
};

struct GG_SaveState_Header
{
    u32 magic;
    u32 version;
    u32 size;
    s64 timestamp;
    char rom_name[128];
    u32 rom_crc;
    u32 screenshot_size;
    u16 screenshot_width;
    u16 screenshot_height;
    u8 screenshot_width_scale;
    char emu_build[32];
};

struct GG_SaveState_Header_Libretro
{
    u32 magic;
    u32 version;
};

struct GG_SaveState_Screenshot
{
    u32 width;
    u32 height;
    u32 size;
    u8* data;
    u8 width_scale;
};

struct GG_Disassembler_Record
{
    u32 address;
    u8 bank;
    char name[64];
    char bytes[25];
    char segment[8];
    u8 opcodes[7];
    int size;
    bool jump;
    u16 jump_address;
    u8 jump_bank;
    bool subroutine;
    int irq;
};

#endif /* TYPES_H */