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

#ifndef MB128_H
#define MB128_H

#include <iostream>
#include "common.h"

class MB128
{
public:
    MB128();
    void Reset();
    void Connect(bool connected);
    bool IsConnected() const;
    bool IsActive() const;
    void Write(u8 value);
    u8 Read();
    u8*       GetRAM();
    const u8* GetRAM() const;
    bool IsDirty() const { return m_dirty; }
    void ClearDirty() { m_dirty = false; }
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);

private:
    enum Mode
    {
        MODE_IDLE = 0,
        MODE_A1,
        MODE_A2,
        MODE_REQ,
        MODE_ADDR,
        MODE_LENBITS,
        MODE_READ,
        MODE_READTRAIL,
        MODE_WRITE,
        MODE_WRITETRAIL,
    };

    void SendBit(bool sel_bit);

private:
    static const u32 kMB128Size = 0x20000; // 128 KiB
    static const u8 MB128_IDENT = 0x04;  // Ident bit in return value (bit 2)

    u8 m_ram[kMB128Size];
    bool m_connected;
    u8 m_prev_data;
    u8 m_shiftreg;
    bool m_active;
    u8 m_state;
    u8 m_bitnum;
    bool m_cmd_wr_rd;
    u32 m_address;
    u32 m_len_bits;
    u8 m_retval;
    bool m_dirty;
};

#include "mb128_inline.h"

#endif /* MB128_H */
