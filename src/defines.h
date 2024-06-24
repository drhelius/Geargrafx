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

#ifndef DEFINES_H
#define DEFINES_H

#ifndef EMULATOR_BUILD
#define EMULATOR_BUILD "undefined"
#endif

#define GEARGRAFX_TITLE "Geargrafx"
#define GEARGRAFX_VERSION EMULATOR_BUILD
#define GEARGRAFX_TITLE_ASCII "" \
"   ____                                 __      \n" \
"  / ___| ___  __ _ _ __ __ _ _ __ __ _ / _|_  __\n" \
" | |  _ / _ \\/ _` | '__/ _` | '__/ _` | |_\\ \\/ /\n" \
" | |_| |  __/ (_| | | | (_| | | | (_| |  _|>  < \n" \
"  \\____|\\___|\\__,_|_|  \\__, |_|  \\__,_|_| /_/\\_\\\n" \
"                       |___/                    \n"

#ifdef DEBUG
#define GEARGRAFX_DEBUG 1
#endif

#ifndef NULL
#define NULL 0
#endif

#define SafeDelete(pointer) if(pointer != NULL) {delete pointer; pointer = NULL;}
#define SafeDeleteArray(pointer) if(pointer != NULL) {delete [] pointer; pointer = NULL;}

#define InitPointer(pointer) ((pointer) = NULL)
#define IsValidPointer(pointer) ((pointer) != NULL)

#if defined(MSB_FIRST) || defined(__BIG_ENDIAN__) || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define IS_BIG_ENDIAN
#else
#define IS_LITTLE_ENDIAN
#endif

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY_PATTERN_SPACED "%c%c%c%c %c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 

#endif /* DEFINES_H */