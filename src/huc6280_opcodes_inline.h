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

#ifndef HUC6280_OPCODES_INLINE_H
#define HUC6280_OPCODES_INLINE_H

#include "huc6280.h"

inline void HuC6280::OPCodes_ADC(u8 value)
{
    int result = m_A.GetValue() + value + (IsSetFlag(FLAG_CARRY) ? 1 : 0);
    u8 final_result = static_cast<u8> (result & 0xFF);
    SetZeroFlagFromResult(final_result);
    SetNegativeFlagFromResult(final_result);
    if ((result & 0x100) != 0)
        SetFlag(FLAG_CARRY);
    else
        ClearFlag(FLAG_CARRY);
    if ((((m_A.GetValue() ^ value) & 0x80) == 0) && (((m_A.GetValue() ^ result) & 0x80) != 0))
        SetFlag(FLAG_OVERFLOW);
    else
        ClearFlag(FLAG_OVERFLOW);
    m_A.SetValue(final_result);
}

inline void HuC6280::OPCodes_AND(u8 value)
{
    u8 result = m_A.GetValue() & value;
    m_A.SetValue(result);
    SetZeroFlagFromResult(result);
    SetNegativeFlagFromResult(result);
}

inline void HuC6280::OPCodes_ASL_Accumulator()
{
    u8 value = m_A.GetValue();
    u8 result = static_cast<u8>(value << 1);
    m_A.SetValue(result);
    SetZeroFlagFromResult(result);
    SetNegativeFlagFromResult(result);
    if ((value & 0x80) != 0)
        SetFlag(FLAG_CARRY);
    else
        ClearFlag(FLAG_CARRY);
}

inline void HuC6280::OPCodes_ASL_Memory(u16 address)
{
    u8 value = Read(address);
    u8 result = static_cast<u8>(value << 1);
    Write(address, result);
    SetZeroFlagFromResult(result);
    SetNegativeFlagFromResult(result);
    if ((value & 0x80) != 0)
        SetFlag(FLAG_CARRY);
    else
        ClearFlag(FLAG_CARRY);
}

inline void HuC6280::OPcodes_Branch(bool condition)
{
    if (condition)
    {
        s8 displacement = RelativeAddressing();
        u16 address = m_PC.GetValue();
        u16 result = static_cast<u16>(address + displacement);
        m_PC.SetValue(result);
        m_branch_taken = true;
        m_page_crossed = PageCrossed(address, result);
    }
    else
        m_PC.Increment();
}

inline void HuC6280::OPCodes_BIT(u16 address)
{
    u8 value = Read(address);
    u8 result = m_A.GetValue() & value;
    SetZeroFlagFromResult(result);
    SetOverflowFlagFromResult(value);
    SetNegativeFlagFromResult(value);
}

inline void HuC6280::OPCodes_BRK()
{
    StackPush16(m_PC.GetValue());
    SetFlag(FLAG_BRK);
    StackPush8(m_P.GetValue());
    SetFlag(FLAG_IRQ);
    m_PC.SetLow(Read(0xFFFE));
    m_PC.SetHigh(Read(0xFFFF));
}

inline void HuC6280::OPCodes_ClearFlag(u8 flag)
{
    ClearFlag(flag);
}

inline void HuC6280::OPCodes_SetFlag(u8 flag)
{
    SetFlag(flag);
}

inline void HuC6280::OPCodes_CMP(EightBitRegister* reg, u8 value)
{
    u8 reg_value = reg->GetValue();
    u8 result = reg_value - value;
    SetZeroFlagFromResult(result);
    SetNegativeFlagFromResult(result);
    if (reg_value >= value)
        SetFlag(FLAG_CARRY);
    else
        ClearFlag(FLAG_CARRY);
}

inline void HuC6280::OPCodes_DEC_Mem(u16 address)
{
    u8 value = Read(address);
    u8 result = value - 1;
    Write(address, result);
    SetZeroFlagFromResult(result);
    SetNegativeFlagFromResult(result);
}

inline void HuC6280::OPCodes_DEC_Reg(EightBitRegister* reg)
{
    u8 value = reg->GetValue();
    u8 result = value - 1;
    reg->SetValue(result);
    SetZeroFlagFromResult(result);
    SetNegativeFlagFromResult(result);
}

inline void HuC6280::OPCodes_EOR(u8 value)
{
    u8 result = m_A.GetValue() ^ value;
    m_A.SetValue(result);
    SetZeroFlagFromResult(result);
    SetNegativeFlagFromResult(result);
}

inline void HuC6280::OPCodes_INC_Mem(u16 address)
{
    u8 value = Read(address);
    u8 result = value + 1;
    Write(address, result);
    SetZeroFlagFromResult(result);
    SetNegativeFlagFromResult(result);
}

inline void HuC6280::OPCodes_INC_Reg(EightBitRegister* reg)
{
    u8 value = reg->GetValue();
    u8 result = value + 1;
    reg->SetValue(result);
    SetZeroFlagFromResult(result);
    SetNegativeFlagFromResult(result);
}

inline void HuC6280::OPCodes_LD(EightBitRegister* reg, u8 value)
{
    reg->SetValue(value);
    SetZeroFlagFromResult(value);
    SetNegativeFlagFromResult(value);
}

inline void HuC6280::OPCodes_LSR_Accumulator()
{
    u8 value = m_A.GetValue();
    u8 result = value >> 1;
    m_A.SetValue(result);
    SetZeroFlagFromResult(result);
    SetNegativeFlagFromResult(result);
    if ((value & 0x01) != 0)
        SetFlag(FLAG_CARRY);
    else
        ClearFlag(FLAG_CARRY);
}

inline void HuC6280::OPCodes_LSR_Memory(u16 address)
{
    u8 value = Read(address);
    u8 result = value >> 1;
    Write(address, result);
    SetZeroFlagFromResult(result);
    SetNegativeFlagFromResult(result);
    if ((value & 0x01) != 0)
        SetFlag(FLAG_CARRY);
    else
        ClearFlag(FLAG_CARRY);
}

inline void HuC6280::OPCodes_ORA(u8 value)
{
    u8 result = m_A.GetValue() | value;
    m_A.SetValue(result);
    SetZeroFlagFromResult(result);
    SetNegativeFlagFromResult(result);
}

inline void HuC6280::OPCodes_ROL_Accumulator()
{
    u8 value = m_A.GetValue();
    u8 result = static_cast<u8>(value << 1);
    result |= IsSetFlag(FLAG_CARRY) ? 0x01 : 0x00;
    m_A.SetValue(result);
    SetZeroFlagFromResult(result);
    SetNegativeFlagFromResult(result);
    if ((value & 0x80) != 0)
        SetFlag(FLAG_CARRY);
    else
        ClearFlag(FLAG_CARRY);
}

inline void HuC6280::OPCodes_ROL_Memory(u16 address)
{
    u8 value = Read(address);
    u8 result = static_cast<u8>(value << 1);
    result |= IsSetFlag(FLAG_CARRY) ? 0x01 : 0x00;
    Write(address, result);
    SetZeroFlagFromResult(result);
    SetNegativeFlagFromResult(result);
    if ((value & 0x80) != 0)
        SetFlag(FLAG_CARRY);
    else
        ClearFlag(FLAG_CARRY);
}

inline void HuC6280::OPCodes_ROR_Accumulator()
{
    u8 value = m_A.GetValue();
    u8 result = value >> 1;
    result |= IsSetFlag(FLAG_CARRY) ? 0x80 : 0x00;
    m_A.SetValue(result);
    SetZeroFlagFromResult(result);
    SetNegativeFlagFromResult(result);
    if ((value & 0x01) != 0)
        SetFlag(FLAG_CARRY);
    else
        ClearFlag(FLAG_CARRY);
}

inline void HuC6280::OPCodes_ROR_Memory(u16 address)
{
    u8 value = Read(address);
    u8 result = value >> 1;
    result |= IsSetFlag(FLAG_CARRY) ? 0x80 : 0x00;
    Write(address, result);
    SetZeroFlagFromResult(result);
    SetNegativeFlagFromResult(result);
    if ((value & 0x01) != 0)
        SetFlag(FLAG_CARRY);
    else
        ClearFlag(FLAG_CARRY);
}

inline void HuC6280::OPCodes_SBC(u8 value)
{
    int result = m_A.GetValue() - value - (IsSetFlag(FLAG_CARRY) ? 0x00 : 0x01);
    u8 final_result = static_cast<u8> (result & 0xFF);
    SetZeroFlagFromResult(final_result);
    SetNegativeFlagFromResult(final_result);
    if ((result & 0x100) == 0)
        SetFlag(FLAG_CARRY);
    else
        ClearFlag(FLAG_CARRY);
    if ((((m_A.GetValue() ^ value) & 0x80) != 0) && (((m_A.GetValue() ^ result) & 0x80) != 0))
        SetFlag(FLAG_OVERFLOW);
    else
        ClearFlag(FLAG_OVERFLOW);
    m_A.SetValue(final_result);
}

inline void HuC6280::OPCodes_Store(EightBitRegister* reg, u16 address)
{
    u8 value = reg->GetValue();
    Write(address, value);
}

inline void HuC6280::OPCodes_Transfer(EightBitRegister* reg, EightBitRegister* target)
{
    u8 value = reg->GetValue();
    target->SetValue(value);
    SetZeroFlagFromResult(value);
    SetNegativeFlagFromResult(value);
}

#endif /* HUC6280_OPCODES_INLINE_H */