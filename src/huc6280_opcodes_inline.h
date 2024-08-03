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
#include "huc6270.h"
#include "memory.h"
#include "huc6280_names.h"

inline void HuC6280::OPCodes_ADC(u8 value)
{
    // TODO
    int a;
    u8 final_result;
    u16 address;

    if (IsSetFlag(FLAG_TRANSFER))
    {
        address = ZeroPageX();
        a = m_memory->Read(address);
        m_cycles += 3;
    }
    else
        a = m_A.GetValue();

    if (IsSetFlag(FLAG_DECIMAL))
    {
        m_cycles++;
        int low_nibble = (a & 0x0F) + (value & 0x0F) + (IsSetFlag(FLAG_CARRY) ? 1 : 0);
        if (low_nibble > 9) low_nibble += 6;
        int high_nibble = (a >> 4) + (value >> 4) + (low_nibble > 15 ? 1 : 0);
        if (high_nibble > 9) high_nibble += 6;

        final_result = (low_nibble & 0x0F) | ((high_nibble & 0x0F) << 4);
        SetZNFlags(final_result);
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
        SetZNFlags(final_result);
        if ((result & 0x100) != 0)
            SetFlag(FLAG_CARRY);
        else
            ClearFlag(FLAG_CARRY);
        if ((((a ^ value) & 0x80) == 0) && (((a ^ result) & 0x80) != 0))
            SetFlag(FLAG_OVERFLOW);
        else
            ClearFlag(FLAG_OVERFLOW);
    }

    if (IsSetFlag(FLAG_TRANSFER))
        m_memory->Write(address, final_result);
    else
        m_A.SetValue(final_result);
}

inline void HuC6280::OPCodes_AND(u8 value)
{
    u8 result;
    if (IsSetFlag(FLAG_TRANSFER))
    {
        u16 address = ZeroPageX();
        u8 a = m_memory->Read(address);
        result = a & value;
        m_memory->Write(address, result);
        m_cycles += 3;
    }
    else
    {
        result = m_A.GetValue() & value;
        m_A.SetValue(result);
    }
    SetOrClearZNFlags(result);
}

inline void HuC6280::OPCodes_ASL_Accumulator()
{
    u8 value = m_A.GetValue();
    u8 result = static_cast<u8>(value << 1);
    m_A.SetValue(result);
    SetOrClearZNFlags(result);
    if ((value & 0x80) != 0)
        SetFlag(FLAG_CARRY);
    else
        ClearFlag(FLAG_CARRY);
}

inline void HuC6280::OPCodes_ASL_Memory(u16 address)
{
    u8 value = m_memory->Read(address);
    u8 result = static_cast<u8>(value << 1);
    m_memory->Write(address, result);
    SetOrClearZNFlags(result);
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
        m_cycles += 2;
    }
    else
        m_PC.Increment();
}

inline void HuC6280::OPCodes_BIT(u16 address)
{
    u8 value = m_memory->Read(address);
    u8 result = m_A.GetValue() & value;
    ClearFlag(FLAG_ZERO | FLAG_OVERFLOW | FLAG_NEGATIVE);
    u8 flags = m_P.GetValue();
    flags |= (m_zn_flags_lut[result] & FLAG_ZERO);
    flags |= (value & (FLAG_OVERFLOW | FLAG_NEGATIVE));
    m_P.SetValue(flags);
}

inline void HuC6280::OPCodes_BRK()
{
    m_PC.Increment();
    StackPush16(m_PC.GetValue());
    ClearFlag(FLAG_TRANSFER);
    SetFlag(FLAG_BREAK);
    StackPush8(m_P.GetValue());
    SetFlag(FLAG_INTERRUPT);
    ClearFlag(FLAG_DECIMAL);
    m_PC.SetLow(m_memory->Read(0xFFF6));
    m_PC.SetHigh(m_memory->Read(0xFFF7));
}

inline void HuC6280::OPCodes_Subroutine()
{
    StackPush16(m_PC.GetValue());
    s8 displacement = RelativeAddressing();
    m_PC.SetValue(static_cast<u16>(m_PC.GetValue() + displacement));
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
    SetOrClearZNFlags(result);
    if (reg_value >= value)
        SetFlag(FLAG_CARRY);
    else
        ClearFlag(FLAG_CARRY);
}

inline void HuC6280::OPCodes_DEC_Mem(u16 address)
{
    u8 value = m_memory->Read(address);
    u8 result = value - 1;
    m_memory->Write(address, result);
    SetOrClearZNFlags(result);
}

inline void HuC6280::OPCodes_DEC_Reg(EightBitRegister* reg)
{
    reg->Decrement();
    SetOrClearZNFlags(reg->GetValue());
}

inline void HuC6280::OPCodes_EOR(u8 value)
{
    u8 result;
    if (IsSetFlag(FLAG_TRANSFER))
    {
        u16 address = ZeroPageX();
        u8 a = m_memory->Read(address);
        result = a ^ value;
        m_memory->Write(address, result);
        m_cycles += 3;
    }
    else
    {
        result = m_A.GetValue() ^ value;
        m_A.SetValue(result);
    }
    SetOrClearZNFlags(result);
}

inline void HuC6280::OPCodes_INC_Mem(u16 address)
{
    u8 value = m_memory->Read(address);
    u8 result = value + 1;
    m_memory->Write(address, result);
    SetOrClearZNFlags(result);
}

inline void HuC6280::OPCodes_INC_Reg(EightBitRegister* reg)
{
    reg->Increment();
    SetOrClearZNFlags(reg->GetValue());
}

inline void HuC6280::OPCodes_LD(EightBitRegister* reg, u8 value)
{
    reg->SetValue(value);
    SetOrClearZNFlags(value);
}

inline void HuC6280::OPCodes_LSR_Accumulator()
{
    u8 value = m_A.GetValue();
    u8 result = value >> 1;
    m_A.SetValue(result);
    SetZNFlags(result);
    if ((value & 0x01) != 0)
        SetFlag(FLAG_CARRY);
    else
        ClearFlag(FLAG_CARRY);
}

inline void HuC6280::OPCodes_LSR_Memory(u16 address)
{
    u8 value = m_memory->Read(address);
    u8 result = value >> 1;
    m_memory->Write(address, result);
    SetZNFlags(result);
    if ((value & 0x01) != 0)
        SetFlag(FLAG_CARRY);
    else
        ClearFlag(FLAG_CARRY);
}

inline void HuC6280::OPCodes_ORA(u8 value)
{
    u8 result;
    if (IsSetFlag(FLAG_TRANSFER))
    {
        u16 address = ZeroPageX();
        u8 a = m_memory->Read(address);
        result = a | value;
        m_memory->Write(address, result);
        m_cycles += 3;
    }
    else
    {
        result = m_A.GetValue() | value;
        m_A.SetValue(result);
    }
    SetOrClearZNFlags(result);
}

inline void HuC6280::OPCodes_RMB(u8 bit, u16 address)
{
    u8 result = UnsetBit(m_memory->Read(address), bit);
    m_memory->Write(address, result);
}

inline void HuC6280::OPCodes_ROL_Accumulator()
{
    u8 value = m_A.GetValue();
    u8 result = static_cast<u8>(value << 1);
    result |= IsSetFlag(FLAG_CARRY) ? 0x01 : 0x00;
    m_A.SetValue(result);
    SetZNFlags(result);
    if ((value & 0x80) != 0)
        SetFlag(FLAG_CARRY);
    else
        ClearFlag(FLAG_CARRY);
}

inline void HuC6280::OPCodes_ROL_Memory(u16 address)
{
    u8 value = m_memory->Read(address);
    u8 result = static_cast<u8>(value << 1);
    result |= IsSetFlag(FLAG_CARRY) ? 0x01 : 0x00;
    m_memory->Write(address, result);
    SetZNFlags(result);
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
    SetZNFlags(result);
    if ((value & 0x01) != 0)
        SetFlag(FLAG_CARRY);
    else
        ClearFlag(FLAG_CARRY);
}

inline void HuC6280::OPCodes_ROR_Memory(u16 address)
{
    u8 value = m_memory->Read(address);
    u8 result = value >> 1;
    result |= IsSetFlag(FLAG_CARRY) ? 0x80 : 0x00;
    m_memory->Write(address, result);
    SetZNFlags(result);
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
        m_cycles++;
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
        SetZNFlags(final_result);

        if (high_nibble & 0xF0) {
            ClearFlag(FLAG_CARRY);
        } else {
            SetFlag(FLAG_CARRY);
        }

        m_A.SetValue(final_result);
    }
    else
    {
        int result = m_A.GetValue() - value - (IsSetFlag(FLAG_CARRY) ? 0x00 : 0x01);
        u8 final_result = static_cast<u8> (result & 0xFF);
        SetZNFlags(final_result);
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

inline void HuC6280::OPCodes_SMB(u8 bit, u16 address)
{
    u8 result = SetBit(m_memory->Read(address), bit);
    m_memory->Write(address, result);
}

inline void HuC6280::OPCodes_Store(EightBitRegister* reg, u16 address)
{
    u8 value = reg->GetValue();
    m_memory->Write(address, value);
}

inline void HuC6280::OPCodes_STN(u8 reg, u8 value)
{
    m_huc6270->WriteRegister(0x1FE000 | reg, value);
}

inline void HuC6280::OPCodes_STZ(u16 address)
{
    m_memory->Write(address, 0x00);
}

inline void HuC6280::OPCodes_Swap(EightBitRegister* reg1, EightBitRegister* reg2)
{
    u8 temp = reg1->GetValue();
    reg1->SetValue(reg2->GetValue());
    reg2->SetValue(temp);
}

inline void HuC6280::OPCodes_TAM()
{
    u8 bits = Fetch8();

    if ((bits == 0) || (bits & (bits - 1)))
    {
        Debug("Invalid TAM bit: %02X", bits);
    }

    for (int i = 0; i < 8; i++)
    {
        if ((bits & (0x01 << i)) != 0)
        {
            m_memory->SetMpr(i, m_A.GetValue());
        }
    }
}

inline void HuC6280::OPCodes_TMA()
{
    u8 bits = Fetch8();

    if ((bits == 0) || (bits & (bits - 1)))
    {
        Debug("Invalid TMA bit: %02X", bits);
    }

    for (int i = 0; i < 8; i++)
    {
        if ((bits & (0x01 << i)) != 0)
        {
            m_A.SetValue(m_memory->GetMpr(i));
        }
    }
}

inline void HuC6280::OPCodes_Transfer(EightBitRegister* source, EightBitRegister* dest)
{
    u8 value = source->GetValue();
    dest->SetValue(value);
    SetOrClearZNFlags(value);
}

inline void HuC6280::OPCodes_TRB(u16 address)
{
    u8 value = m_memory->Read(address);
    u8 result = ~m_A.GetValue() & value;
    m_memory->Write(address, result);
    ClearFlag(FLAG_ZERO | FLAG_OVERFLOW | FLAG_NEGATIVE);
    u8 flags = m_P.GetValue();
    flags |= (m_zn_flags_lut[result] & FLAG_ZERO);
    flags |= (value & (FLAG_OVERFLOW | FLAG_NEGATIVE));
    m_P.SetValue(flags);
}

inline void HuC6280::OPCodes_TSB(u16 address)
{
    u8 value = m_memory->Read(address);
    u8 result = m_A.GetValue() | value;
    m_memory->Write(address, result);
    ClearFlag(FLAG_ZERO | FLAG_OVERFLOW | FLAG_NEGATIVE);
    u8 flags = m_P.GetValue();
    flags |= (m_zn_flags_lut[result] & FLAG_ZERO);
    flags |= (value & (FLAG_OVERFLOW | FLAG_NEGATIVE));
    m_P.SetValue(flags);
}

inline void HuC6280::OPCodes_TST(u8 value, u16 address)
{
    u8 mem = m_memory->Read(address);
    u8 result = value & mem;
    ClearFlag(FLAG_ZERO | FLAG_OVERFLOW | FLAG_NEGATIVE);
    u8 flags = m_P.GetValue();
    flags |= (m_zn_flags_lut[result] & FLAG_ZERO);
    flags |= (value & (FLAG_OVERFLOW | FLAG_NEGATIVE));
    m_P.SetValue(flags);
}

inline void HuC6280::OPCodes_TAI()
{
    StackPush8(m_Y.GetValue());
    StackPush8(m_A.GetValue());
    StackPush8(m_X.GetValue());

    u16 source = Fetch16();
    u16 dest = Fetch16();
    u16 length = Fetch16();
    u16 alternate = 0;
    do
    {
        m_memory->Write(dest, m_memory->Read(source + alternate, true));
        alternate ^= 1;
        dest++;
        length--;
        m_cycles += 6;
    }
    while (length);

    m_X.SetValue(StackPop8());
    m_A.SetValue(StackPop8());
    m_Y.SetValue(StackPop8());
}

inline void HuC6280::OPCodes_TDD()
{
    StackPush8(m_Y.GetValue());
    StackPush8(m_A.GetValue());
    StackPush8(m_X.GetValue());

    u16 source = Fetch16();
    u16 dest = Fetch16();
    u16 length = Fetch16();
    do
    {
        m_memory->Write(dest, m_memory->Read(source, true));
        source--;
        dest--;
        length--;
        m_cycles += 6;
    }
    while (length);

    m_X.SetValue(StackPop8());
    m_A.SetValue(StackPop8());
    m_Y.SetValue(StackPop8());
}

inline void HuC6280::OPCodes_TIA()
{
    StackPush8(m_Y.GetValue());
    StackPush8(m_A.GetValue());
    StackPush8(m_X.GetValue());

    u16 source = Fetch16();
    u16 dest = Fetch16();
    u16 length = Fetch16();
    u16 alternate = 0;
    do
    {
        m_memory->Write(dest + alternate, m_memory->Read(source, true));
        source++;
        alternate ^= 1;
        length--;
        m_cycles += 6;
    }
    while (length);

    m_X.SetValue(StackPop8());
    m_A.SetValue(StackPop8());
    m_Y.SetValue(StackPop8());
}

inline void HuC6280::OPCodes_TII()
{
    StackPush8(m_Y.GetValue());
    StackPush8(m_A.GetValue());
    StackPush8(m_X.GetValue());

    u16 source = Fetch16();
    u16 dest = Fetch16();
    u16 length = Fetch16();
    do
    {
        m_memory->Write(dest, m_memory->Read(source, true));
        source++;
        dest++;
        length--;
        m_cycles += 6;
    }
    while (length);

    m_X.SetValue(StackPop8());
    m_A.SetValue(StackPop8());
    m_Y.SetValue(StackPop8());
}

inline void HuC6280::OPCodes_TIN()
{
    StackPush8(m_Y.GetValue());
    StackPush8(m_A.GetValue());
    StackPush8(m_X.GetValue());

    u16 source = Fetch16();
    u16 dest = Fetch16();
    u16 length = Fetch16();
    do
    {
        m_memory->Write(dest, m_memory->Read(source, true));
        source++;
        length--;
        m_cycles += 6;
    }
    while (length);

    m_X.SetValue(StackPop8());
    m_A.SetValue(StackPop8());
    m_Y.SetValue(StackPop8());
}

inline void HuC6280::UnofficialOPCode()
{
    u16 opcode_address = m_PC.GetValue() - 1;
    u8 opcode = m_memory->Read(opcode_address);
    Debug("** HuC6280 --> UNOFFICIAL OP Code (%02X) at $%.4X -- %s", opcode, opcode_address, k_opcode_names[opcode]);
}

#endif /* HUC6280_OPCODES_INLINE_H */