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

#ifndef HUC6280_INLINE_H
#define HUC6280_INLINE_H

#include "huc6280.h"
#include "memory.h"

inline void HuC6280::AssertIRQ(bool asserted)
{
    m_interrupt_asserted = asserted;
}

inline void HuC6280::RequestNMI()
{
    m_nmi_interrupt_requested = true;
}

void HuC6280::SerHighSpeed(bool high_speed)
{
    m_high_speed = high_speed;
}

bool HuC6280::IsHighSpeed()
{
    return m_high_speed;
}

inline u8 HuC6280::Fetch8()
{
    u8 value = m_memory->Read(m_PC.GetValue());
    m_PC.Increment();
    return value;
}

inline u16 HuC6280::Fetch16()
{
    u16 pc = m_PC.GetValue();
    u8 l = m_memory->Read(pc);
    u8 h = m_memory->Read(pc + 1);
    m_PC.SetValue(pc + 2);
    return Address16(h , l);
}

inline u16 HuC6280::Address16(u8 high, u8 low)
{
    return static_cast<u16>(high << 8 ) | low;
}

inline bool HuC6280::PageCrossed(u16 old_address, u16 new_address)
{
    return (old_address ^ new_address) > 0x00FF;
}

inline u16 HuC6280::ZeroPageX()
{
    return 0x2000 | m_X.GetValue();
}

inline void HuC6280::SetZeroFlagFromResult(u8 result)
{
    if (result == 0)
        SetFlag(FLAG_ZERO);
    else
        ClearFlag(FLAG_ZERO);
}

inline void HuC6280::SetOverflowFlagFromResult(u8 result)
{
    m_P.SetValue((m_P.GetValue() & 0xBF) | (result & 0x40));
}

inline void HuC6280::SetNegativeFlagFromResult(u8 result)
{
    m_P.SetValue((m_P.GetValue() & 0x7F) | (result & 0x80));
}

inline void HuC6280::SetFlag(u8 flag)
{
    m_P.SetValue(m_P.GetValue() | flag);
}

inline void HuC6280::ClearFlag(u8 flag)
{
    m_P.SetValue(m_P.GetValue() & (~flag));
}

inline bool HuC6280::IsSetFlag(u8 flag)
{
    return (m_P.GetValue() & flag) != 0;
}

inline void HuC6280::StackPush16(u16 value)
{
    m_memory->Write(0x2100 | m_S.GetValue(), static_cast<u8>((value >> 8) & 0x00FF));
    m_S.Decrement();
    m_memory->Write(0x2100 | m_S.GetValue(), static_cast<u8>(value & 0x00FF));
    m_S.Decrement();
}

inline void HuC6280::StackPush8(u8 value)
{
    m_memory->Write(0x2100 | m_S.GetValue(), value);
    m_S.Decrement();
}

inline u16 HuC6280::StackPop16()
{
    m_S.Increment();
    u8 l = m_memory->Read(0x2100 | m_S.GetValue());
    m_S.Increment();
    u8 h = m_memory->Read(0x2100 | m_S.GetValue());
    return Address16(h , l);
}

inline u8 HuC6280::StackPop8()
{
    m_S.Increment();
    u8 result = m_memory->Read(0x2100 | m_S.GetValue());
    return result;
}

inline u8 HuC6280::ImmediateAddressing()
{
    return Fetch8();
}

inline u16 HuC6280::ZeroPageAddressing()
{
    return 0x2000 | Fetch8();
}

inline u16 HuC6280::ZeroPageAddressing(EightBitRegister* reg)
{
    return (0x2000 | (Fetch8() + reg->GetValue())) & 0x20FF;
}

inline u16 HuC6280::ZeroPageRelativeAddressing()
{
    u16 address = ZeroPageAddressing();
    s8 offset = static_cast<s8>(Fetch8());
    return address + offset;
}

inline u16 HuC6280::ZeroPageIndirectAddressing()
{
    u16 address = ZeroPageAddressing();
    u8 l = m_memory->Read(address);
    u8 h = m_memory->Read(address + 1);
    return Address16(h, l);
}

inline u16 HuC6280::ZeroPageIndexedIndirectAddressing()
{
    u16 address = (ZeroPageAddressing() + m_X.GetValue()) & 0x20FF;
    u8 l = m_memory->Read(address);
    u8 h = m_memory->Read((address + 1) & 0x20FF);
    return Address16(h, l);
}

inline u16 HuC6280::ZeroPageIndirectIndexedAddressing()
{
    u16 address = ZeroPageAddressing();
    u8 l = m_memory->Read(address);
    u8 h = m_memory->Read((address + 1) & 0x20FF);
    address = Address16(h, l);
    u16 result = address + m_Y.GetValue();
    return result;
}

inline s8 HuC6280::RelativeAddressing()
{
    return static_cast<s8>(Fetch8());
}

inline u16 HuC6280::AbsoluteAddressing()
{
    return Fetch16();
}

inline u16 HuC6280::AbsoluteAddressing(EightBitRegister* reg)
{
    u16 address = Fetch16();
    u16 result = address + reg->GetValue();
    return result;
}

inline u16 HuC6280::AbsoluteIndirectAddressing()
{
    u16 address = Fetch16();
    u8 l = m_memory->Read(address);
    u8 h = m_memory->Read(address + 1);
    return Address16(h, l);
}

inline u16 HuC6280::AbsoluteIndexedIndirectAddressing()
{
    u16 address = Fetch16() + m_X.GetValue();
    u8 l = m_memory->Read(address);
    u8 h = m_memory->Read(address + 1);
    return Address16(h, l);
}

#endif /* HUC6280_INLINE_H */