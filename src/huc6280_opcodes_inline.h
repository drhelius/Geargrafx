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
#include "memory.h"

inline void HuC6280::OPCodes_ADC(u8 value)
{
    // TODO
    int a;
    u8 final_result;
    u16 address;

    if (IsSetFlag(FLAG_MEMORY))
    {
        address = ZeroPageX();
        a = m_memory->Read(address);
        m_t_states += 3;
    }
    else
        a = m_A.GetValue();

    if (IsSetFlag(FLAG_DECIMAL))
    {
        int low_nibble = (a & 0x0F) + (value & 0x0F) + (IsSetFlag(FLAG_CARRY) ? 1 : 0);
        if (low_nibble > 9) low_nibble += 6;
        int high_nibble = (a >> 4) + (value >> 4) + (low_nibble > 15 ? 1 : 0);
        if (high_nibble > 9) high_nibble += 6;

        final_result = (low_nibble & 0x0F) | ((high_nibble & 0x0F) << 4);
        SetZeroFlagFromResult(final_result);
        SetNegativeFlagFromResult(final_result);
        if (high_nibble > 15)
            SetFlag(FLAG_CARRY);
        else
            ClearFlag(FLAG_CARRY);

        if ((((a ^ value) & 0x80) == 0) && (((a ^ final_result) & 0x80) != 0))
            SetFlag(FLAG_OVERFLOW);
        else
            ClearFlag(FLAG_OVERFLOW);
    }
    else
    {
        int result = a + value + (IsSetFlag(FLAG_CARRY) ? 1 : 0);
        final_result = static_cast<u8> (result & 0xFF);
        SetZeroFlagFromResult(final_result);
        SetNegativeFlagFromResult(final_result);
        if ((result & 0x100) != 0)
            SetFlag(FLAG_CARRY);
        else
            ClearFlag(FLAG_CARRY);
        if ((((a ^ value) & 0x80) == 0) && (((a ^ result) & 0x80) != 0))
            SetFlag(FLAG_OVERFLOW);
        else
            ClearFlag(FLAG_OVERFLOW);
    }

    if (IsSetFlag(FLAG_MEMORY))
    {
        m_memory->Write(address, final_result);
        ClearFlag(FLAG_MEMORY);
    }
    else
        m_A.SetValue(final_result);
}

inline void HuC6280::OPCodes_AND(u8 value)
{
    u8 result;
    if (IsSetFlag(FLAG_MEMORY))
    {
        u16 address = ZeroPageX();
        u8 a = m_memory->Read(address);
        result = a & value;
        m_memory->Write(address, result);
        m_t_states += 3;
        ClearFlag(FLAG_MEMORY);
    }
    else
    {
        result = m_A.GetValue() & value;
        m_A.SetValue(result);
    }
    SetZeroFlagFromResult(result);
    SetNegativeFlagFromResult(result);
}

inline void HuC6280::OPCodes_ASL_Accumulator()
{
    ClearFlag(FLAG_MEMORY);
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
    ClearFlag(FLAG_MEMORY);
    u8 value = m_memory->Read(address);
    u8 result = static_cast<u8>(value << 1);
    m_memory->Write(address, result);
    SetZeroFlagFromResult(result);
    SetNegativeFlagFromResult(result);
    if ((value & 0x80) != 0)
        SetFlag(FLAG_CARRY);
    else
        ClearFlag(FLAG_CARRY);
}

inline void HuC6280::OPcodes_Branch(bool condition)
{
    ClearFlag(FLAG_MEMORY);
    if (condition)
    {
        s8 displacement = RelativeAddressing();
        u16 address = m_PC.GetValue();
        u16 result = static_cast<u16>(address + displacement);
        m_PC.SetValue(result);
        m_branch_taken = true;
    }
    else
        m_PC.Increment();
}

inline void HuC6280::OPCodes_BIT(u16 address)
{
    ClearFlag(FLAG_MEMORY);
    u8 value = m_memory->Read(address);
    u8 result = m_A.GetValue() & value;
    SetZeroFlagFromResult(result);
    SetOverflowFlagFromResult(value);
    SetNegativeFlagFromResult(value);
}

inline void HuC6280::OPCodes_BRK()
{
    ClearFlag(FLAG_MEMORY);
    m_PC.Increment();
    StackPush16(m_PC.GetValue());
    SetFlag(FLAG_BRK);
    StackPush8(m_P.GetValue());
    SetFlag(FLAG_IRQ);
    m_PC.SetLow(m_memory->Read(0xFFF6));
    m_PC.SetHigh(m_memory->Read(0xFFF7));
}

inline void HuC6280::OPCodes_Subroutine()
{
    ClearFlag(FLAG_MEMORY);
    StackPush16(m_PC.GetValue());
    s8 displacement = RelativeAddressing();
    m_PC.SetValue(static_cast<u16>(m_PC.GetValue() + displacement));
}

inline void HuC6280::OPCodes_ClearFlag(u8 flag)
{
    ClearFlag(flag | FLAG_MEMORY);
}

inline void HuC6280::OPCodes_SetFlag(u8 flag)
{
    ClearFlag(FLAG_MEMORY);
    SetFlag(flag);
}

inline void HuC6280::OPCodes_Swap(EightBitRegister* reg1, EightBitRegister* reg2)
{
    ClearFlag(FLAG_MEMORY);
    u8 temp = reg1->GetValue();
    reg1->SetValue(reg2->GetValue());
    reg2->SetValue(temp);
}

inline void HuC6280::OPCodes_CMP(EightBitRegister* reg, u8 value)
{
    ClearFlag(FLAG_MEMORY);
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
    ClearFlag(FLAG_MEMORY);
    u8 value = m_memory->Read(address);
    u8 result = value - 1;
    m_memory->Write(address, result);
    SetZeroFlagFromResult(result);
    SetNegativeFlagFromResult(result);
}

inline void HuC6280::OPCodes_DEC_Reg(EightBitRegister* reg)
{
    ClearFlag(FLAG_MEMORY);
    u8 value = reg->GetValue();
    u8 result = value - 1;
    reg->SetValue(result);
    SetZeroFlagFromResult(result);
    SetNegativeFlagFromResult(result);
}

inline void HuC6280::OPCodes_EOR(u8 value)
{
    u8 result;
    if (IsSetFlag(FLAG_MEMORY))
    {
        u16 address = ZeroPageX();
        u8 a = m_memory->Read(address);
        result = a ^ value;
        m_memory->Write(address, result);
        m_t_states += 3;
        ClearFlag(FLAG_MEMORY);
    }
    else
    {
        result = m_A.GetValue() ^ value;
        m_A.SetValue(result);
    }
    SetZeroFlagFromResult(result);
    SetNegativeFlagFromResult(result);
}

inline void HuC6280::OPCodes_INC_Mem(u16 address)
{
    ClearFlag(FLAG_MEMORY);
    u8 value = m_memory->Read(address);
    u8 result = value + 1;
    m_memory->Write(address, result);
    SetZeroFlagFromResult(result);
    SetNegativeFlagFromResult(result);
}

inline void HuC6280::OPCodes_INC_Reg(EightBitRegister* reg)
{
    ClearFlag(FLAG_MEMORY);
    u8 value = reg->GetValue();
    u8 result = value + 1;
    reg->SetValue(result);
    SetZeroFlagFromResult(result);
    SetNegativeFlagFromResult(result);
}

inline void HuC6280::OPCodes_LD(EightBitRegister* reg, u8 value)
{
    ClearFlag(FLAG_MEMORY);
    reg->SetValue(value);
    SetZeroFlagFromResult(value);
    SetNegativeFlagFromResult(value);
}

inline void HuC6280::OPCodes_LSR_Accumulator()
{
    ClearFlag(FLAG_MEMORY);
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
    ClearFlag(FLAG_MEMORY);
    u8 value = m_memory->Read(address);
    u8 result = value >> 1;
    m_memory->Write(address, result);
    SetZeroFlagFromResult(result);
    SetNegativeFlagFromResult(result);
    if ((value & 0x01) != 0)
        SetFlag(FLAG_CARRY);
    else
        ClearFlag(FLAG_CARRY);
}

inline void HuC6280::OPCodes_ORA(u8 value)
{
    u8 result;
    if (IsSetFlag(FLAG_MEMORY))
    {
        u16 address = ZeroPageX();
        u8 a = m_memory->Read(address);
        result = a | value;
        m_memory->Write(address, result);
        m_t_states += 3;
        ClearFlag(FLAG_MEMORY);
    }
    else
    {
        result = m_A.GetValue() | value;
        m_A.SetValue(result);
    }
    SetZeroFlagFromResult(result);
    SetNegativeFlagFromResult(result);
}

inline void HuC6280::OPCodes_RMB(int bit, u16 address)
{
    ClearFlag(FLAG_MEMORY);
    u8 result = UnsetBit(m_memory->Read(address), bit);
    m_memory->Write(address, result);
}

inline void HuC6280::OPCodes_ROL_Accumulator()
{
    ClearFlag(FLAG_MEMORY);
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
    ClearFlag(FLAG_MEMORY);
    u8 value = m_memory->Read(address);
    u8 result = static_cast<u8>(value << 1);
    result |= IsSetFlag(FLAG_CARRY) ? 0x01 : 0x00;
    m_memory->Write(address, result);
    SetZeroFlagFromResult(result);
    SetNegativeFlagFromResult(result);
    if ((value & 0x80) != 0)
        SetFlag(FLAG_CARRY);
    else
        ClearFlag(FLAG_CARRY);
}

inline void HuC6280::OPCodes_ROR_Accumulator()
{
    ClearFlag(FLAG_MEMORY);
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
    ClearFlag(FLAG_MEMORY);
    u8 value = m_memory->Read(address);
    u8 result = value >> 1;
    result |= IsSetFlag(FLAG_CARRY) ? 0x80 : 0x00;
    m_memory->Write(address, result);
    SetZeroFlagFromResult(result);
    SetNegativeFlagFromResult(result);
    if ((value & 0x01) != 0)
        SetFlag(FLAG_CARRY);
    else
        ClearFlag(FLAG_CARRY);
}

inline void HuC6280::OPCodes_SBC(u8 value)
{
    // TODO
    if (IsSetFlag(FLAG_DECIMAL))
    {
        int carry = IsSetFlag(FLAG_CARRY) ? 0 : 1;
        int low_nibble = (m_A.GetValue() & 0x0F) - (value & 0x0F) - carry;
        int high_nibble = (m_A.GetValue() >> 4) - (value >> 4) - ((low_nibble < 0) ? 1 : 0);

        if (low_nibble < 0) {
            low_nibble += 10;
        }
        if (high_nibble < 0) {
            high_nibble += 10;
        }

        u8 final_result = (high_nibble << 4) | (low_nibble & 0x0F);        
        int signed_result = m_A.GetValue() - value - carry;

        if ((signed_result < -99) || (signed_result > 99)) {
            SetFlag(FLAG_OVERFLOW);
        } else {
            ClearFlag(FLAG_OVERFLOW);
        }
        SetZeroFlagFromResult(final_result);
        SetNegativeFlagFromResult(final_result);

        if (high_nibble & 0xF0) {
            ClearFlag(FLAG_CARRY);
        } else {
            SetFlag(FLAG_CARRY);
        }

        m_A.SetValue(final_result);
    }
    else
    {
        ClearFlag(FLAG_MEMORY);
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
}

inline void HuC6280::OPCodes_SMB(int bit, u16 address)
{
    ClearFlag(FLAG_MEMORY);
    u8 result = SetBit(m_memory->Read(address), bit);
    m_memory->Write(address, result);
}

inline void HuC6280::OPCodes_Store(EightBitRegister* reg, u16 address)
{
    ClearFlag(FLAG_MEMORY);
    u8 value = reg->GetValue();
    m_memory->Write(address, value);
}

inline void HuC6280::OPCodes_Transfer(EightBitRegister* reg, EightBitRegister* target)
{
    ClearFlag(FLAG_MEMORY);
    u8 value = reg->GetValue();
    target->SetValue(value);
    SetZeroFlagFromResult(value);
    SetNegativeFlagFromResult(value);
}

#endif /* HUC6280_OPCODES_INLINE_H */