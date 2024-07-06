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
    GG_PIXEL_RGB555,
    GG_PIXEL_RGB888,
    GG_PIXEL_BGR565,
    GG_PIXEL_BGR555,
    GG_PIXEL_BGR888
};

enum GG_Keys
{
    GG_KEY_UP,
    GG_KEY_DOWN,
    GG_KEY_LEFT,
    GG_KEY_RIGHT,
    GG_KEY_1,
    GG_KEY_2,
    GG_KEY_RUN,
    GG_KEY_SELECT
};

enum GG_Controllers
{
    GG_CONTROLLER_1,
    GG_CONTROLLER_2
};

enum GG_OPCode_Type
{
    GG_OPCode_Type_Implied,
    GG_OPCode_Type_1b,
    GG_OPCode_Type_1b_1b,
    GG_OPCode_Type_1b_2b,
    GG_OPCode_Type_2b,
    GG_OPCode_Type_2b_2b_2b,
    GG_OPCode_Type_1b_Relative,
    GG_OPCode_Type_1b_1b_Relative
};

struct GG_OPCode_Info
{
    const char* name;
    GG_OPCode_Type type;
};

enum GG_Debugger_Command
{
    GG_Debugger_Command_Continue,
    GG_Debugger_Command_StepInto,
    GG_Debugger_Command_StepOver,
    GG_Debugger_Command_StepOut
};

#endif /* TYPES_H */