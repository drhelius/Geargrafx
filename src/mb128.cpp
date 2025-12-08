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

#include <string.h>
#include "mb128.h"

MB128::MB128()
{
    memset(m_ram, 0x00, sizeof(m_ram));
    m_connected = false;
    m_dirty = false;
    m_prev_data = 0;
    m_shiftreg = 0xFF;
    m_active = false;
    m_state = MODE_IDLE;
    m_bitnum = 0;
    m_cmd_wr_rd = false;
    m_address = 0;
    m_len_bits = 0;
    m_retval = 0;
}

void MB128::Reset()
{
    m_prev_data = 0;
    m_shiftreg = 0xFF;
    m_active = false;
    m_state = MODE_IDLE;
    m_bitnum = 0;
    m_cmd_wr_rd = false;
    m_address = 0;
    m_len_bits = 0;
    m_retval = 0;
}

void MB128::Connect(bool connected)
{
    m_connected = connected;
    if (!m_connected)
        Reset();
}

void MB128::Write(u8 value)
{
    if (!m_connected)
    {
        m_prev_data = value;
        return;
    }

    bool old_clr = IS_SET_BIT(m_prev_data, 1);
    bool new_sel = IS_SET_BIT(value, 0);
    bool new_clr = IS_SET_BIT(value, 1);

    // Rising edge of CLR
    if (!old_clr && new_clr)
    {
        if (m_active)
            SendBit(new_sel);
        else
        {
            // Detect the 0xA8 pattern.
            m_shiftreg = (m_shiftreg >> 1) | (new_sel ? 0x80 : 0x00);

            if (m_shiftreg == 0xA8)
            {
                m_state = MODE_A1;
                m_active = true;
            }
        }
    }

    m_prev_data = value;
}

u8 MB128::Read()
{
    if (!m_connected)
        return 0;

    if (!m_active)
        return 0;

    return m_retval & 0x0F;
}

void MB128::SendBit(bool sel_bit)
{
    u8 temp_byte;
    u8 temp_mask;

    switch (m_state)
    {
        case MODE_A1:
            m_state = MODE_A2;
            m_retval = sel_bit ? MB128_IDENT : 0;
            break;

        case MODE_A2:
            m_state = MODE_REQ;
            m_retval = sel_bit ? MB128_IDENT : 0;
            break;

        case MODE_REQ:
            // Command bit: 0 = write, 1 = read.
            m_cmd_wr_rd = sel_bit;
            m_state = MODE_ADDR;
            m_retval = 0;
            m_bitnum = 0;
            m_address = 0;
            break;

        case MODE_ADDR:
            m_address |= sel_bit ? (1u << (m_bitnum + 7)) : 0;
            m_retval = 0;
            m_bitnum += 1;

            if (m_bitnum == 10)
            {
                m_bitnum = 0;
                m_len_bits = 0;
                m_state = MODE_LENBITS;
            }
            break;

        case MODE_LENBITS:
            // Length in bits (20 bits)
            m_len_bits |= sel_bit ? (1u << m_bitnum) : 0;
            m_retval = 0;
            m_bitnum += 1;

            if (m_bitnum == 20)
            {
                m_bitnum = 0;
                if (m_cmd_wr_rd)
                {
                    // Read
                    m_state = (m_len_bits == 0) ? MODE_READTRAIL : MODE_READ;
                }
                else
                {
                    // Write
                    m_state = (m_len_bits == 0) ? MODE_WRITETRAIL : MODE_WRITE;
                }
            }
            break;

        case MODE_READ:
        {
            u32 addr = m_address & (kMB128Size - 1);
            temp_byte = m_ram[addr];
            m_retval = (temp_byte & (1u << m_bitnum)) ? 1 : 0;

            m_bitnum += 1;
            m_len_bits -= 1;

            if (m_len_bits == 0)
            {
                m_bitnum = 0;
                m_state = MODE_READTRAIL;
                break;
            }

            if (m_bitnum == 8)
            {
                m_bitnum = 0;
                m_address++;
            }
            break;
        }

        case MODE_WRITE:
        {
            u32 addr = m_address & (kMB128Size - 1);
            temp_mask = (u8)(1u << m_bitnum);
            temp_byte = m_ram[addr] & ~temp_mask;
            temp_byte |= sel_bit ? temp_mask : 0;
            m_ram[addr] = temp_byte;
            m_dirty = true;

            m_retval = 0;
            m_bitnum += 1;
            m_len_bits -= 1;

            if (m_len_bits == 0)
            {
                m_bitnum = 0;
                m_state = MODE_WRITETRAIL;
                break;
            }

            if (m_bitnum == 8)
            {
                m_bitnum = 0;
                m_address++;
            }
            break;
        }

        case MODE_WRITETRAIL:
            // Tail after write
            m_bitnum += 1;
            if (m_bitnum == 2)
            {
                m_retval = 0;
            }
            if (m_bitnum == 3)
            {
                m_bitnum = 0;
                m_state = MODE_READTRAIL;
            }
            break;

        case MODE_READTRAIL:
            // Tail after read
            m_bitnum += 1;
            if (m_bitnum == 2)
            {
                m_retval = 0;
            }
            if (m_bitnum == 4)
            {
                m_bitnum = 0;
                m_cmd_wr_rd = 0;
                m_address = 0;
                m_len_bits = 0;
                m_state = MODE_IDLE;
                m_active = false;
            }
            break;

        case MODE_IDLE:
        default:
            break;
    }
}

void MB128::SaveState(std::ostream& stream)
{
    using namespace std;

    stream.write(reinterpret_cast<const char*>(m_ram), sizeof(m_ram));
    stream.write(reinterpret_cast<const char*>(&m_connected), sizeof(m_connected));
    stream.write(reinterpret_cast<const char*>(&m_prev_data), sizeof(m_prev_data));
    stream.write(reinterpret_cast<const char*>(&m_shiftreg), sizeof(m_shiftreg));
    stream.write(reinterpret_cast<const char*>(&m_active), sizeof(m_active));
    stream.write(reinterpret_cast<const char*>(&m_state), sizeof(m_state));
    stream.write(reinterpret_cast<const char*>(&m_bitnum), sizeof(m_bitnum));
    stream.write(reinterpret_cast<const char*>(&m_cmd_wr_rd), sizeof(m_cmd_wr_rd));
    stream.write(reinterpret_cast<const char*>(&m_address), sizeof(m_address));
    stream.write(reinterpret_cast<const char*>(&m_len_bits), sizeof(m_len_bits));
    stream.write(reinterpret_cast<const char*>(&m_retval), sizeof(m_retval));
    stream.write(reinterpret_cast<const char*>(&m_dirty), sizeof(m_dirty));
}

void MB128::LoadState(std::istream& stream)
{
    using namespace std;

    stream.read(reinterpret_cast<char*>(m_ram), sizeof(m_ram));
    stream.read(reinterpret_cast<char*>(&m_connected), sizeof(m_connected));
    stream.read(reinterpret_cast<char*>(&m_prev_data), sizeof(m_prev_data));
    stream.read(reinterpret_cast<char*>(&m_shiftreg), sizeof(m_shiftreg));
    stream.read(reinterpret_cast<char*>(&m_active), sizeof(m_active));
    stream.read(reinterpret_cast<char*>(&m_state), sizeof(m_state));
    stream.read(reinterpret_cast<char*>(&m_bitnum), sizeof(m_bitnum));
    stream.read(reinterpret_cast<char*>(&m_cmd_wr_rd), sizeof(m_cmd_wr_rd));
    stream.read(reinterpret_cast<char*>(&m_address), sizeof(m_address));
    stream.read(reinterpret_cast<char*>(&m_len_bits), sizeof(m_len_bits));
    stream.read(reinterpret_cast<char*>(&m_retval), sizeof(m_retval));
    stream.read(reinterpret_cast<char*>(&m_dirty), sizeof(m_dirty));
}
