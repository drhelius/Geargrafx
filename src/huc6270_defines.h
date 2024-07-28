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

#ifndef HUC6270_DEFINES_H
#define HUC6270_DEFINES_H

#define GG_MAX_RESOLUTION_WIDTH 256
#define GG_MAX_RESOLUTION_HEIGHT 224

#define GG_MAX_BACKGROUND_WIDTH 1024
#define GG_MAX_BACKGROUND_HEIGHT 512

#define HUC6270_STATUS_COLLISION 0x01
#define HUC6270_STATUS_OVERFLOW  0x02
#define HUC6270_STATUS_SCANLINE  0x04
#define HUC6270_STATUS_SAT_END   0x08
#define HUC6270_STATUS_VRAM_END  0x10
#define HUC6270_STATUS_VBLANK    0x20
#define HUC6270_STATUS_BUSY      0x40

#define HUC6270_CONTROL_COLLISION 0x01
#define HUC6270_CONTROL_OVERFLOW  0x02
#define HUC6270_CONTROL_SCANLINE  0x04
#define HUC6270_CONTROL_VBLANK    0x08

#define HUC6270_VRAM_SIZE 0x8000
#define HUC6270_SAT_SIZE 0x100

#define HUC6270_SPRITES 64

#define HUC6270_LINES 263
#define HUC6270_LINES_TOP_BLANKING 14
#define HUC6270_LINES_ACTIVE 242
#define HUC6270_LINES_BOTTOM_BLANKING 4
#define HUC6270_LINES_SYNC 3

#define HUC6270_TOP_BLANKING_START 0
#define HUC6270_ACTIVE_DISPLAY_START 14
#define HUC6270_BOTTOM_BLANKING_START 256
#define HUC6270_SYNC_START 260

#define HUC6270_REG_MAWR  0x00
#define HUC6270_REG_MARR  0x01
#define HUC6270_REG_VWR   0x02
#define HUC6270_REG_VRR   0x02
#define HUC6270_REG_CR    0x05
#define HUC6270_REG_RCR   0x06
#define HUC6270_REG_BXR   0x07
#define HUC6270_REG_BYR   0x08
#define HUC6270_REG_MWR   0x09
#define HUC6270_REG_HSR   0x0A
#define HUC6270_REG_HDR   0x0B
#define HUC6270_REG_VPR   0x0C
#define HUC6270_REG_VDR   0x0D
#define HUC6270_REG_VCR   0x0E
#define HUC6270_REG_DCR   0x0F
#define HUC6270_REG_SOUR  0x10
#define HUC6270_REG_DESR  0x11
#define HUC6270_REG_LENR  0x12
#define HUC6270_REG_DVSSR 0x13

#endif /* HUC6270_DEFINES_H */