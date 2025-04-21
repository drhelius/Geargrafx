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

INLINE void HuC6280::OPCodes_ADC(u8 value)
{
    u8 a;

#if !defined(GG_TESTING)
    u16 address = 0;
    if (IsSetFlag(FLAG_TRANSFER))
    {
        address = ZeroPageX();
        a = MemoryRead(address);
        m_cycles += 3;
    }
    else
#endif
        a = m_A.GetValue();

    u16 result = 0;
    if (IsSetFlag(FLAG_DECIMAL))
    {
        m_cycles++;

        result = (u16)(a & 0x0F) + (u16)(value & 0x0F) + (IsSetFlag(FLAG_CARRY) ? 1 : 0);
        if(result > 0x09)
            result += 0x06;
        result = (u16)(a & 0xF0) + (u16)(value & 0xF0) + (result > 0x0F ? 0x10 : 0) + (result & 0x0F);

#if defined(GG_TESTING)
        if(~(a ^ value) & ((result & 0xFF) ^ a) & 0x80)
            SetFlag(FLAG_OVERFLOW);
        else
            ClearFlag(FLAG_OVERFLOW);
#endif

        if(result > 0x9F)
            result += 0x60;
    }
    else
    {
        result = a + value + (IsSetFlag(FLAG_CARRY) ? 1 : 0);

        if(~(a ^ value) & (a ^ result) & 0x80)
            SetFlag(FLAG_OVERFLOW);
        else
            ClearFlag(FLAG_OVERFLOW);
    }

    u8 final_result = static_cast<u8>(result & 0xFF);

    ClearFlag(FLAG_ZERO | FLAG_CARRY | FLAG_NEGATIVE);
    SetZNFlags(final_result);

    if(result > 0xFF) {
        SetFlag(FLAG_CARRY);
    }

#if !defined(GG_TESTING)
    if (IsSetFlag(FLAG_TRANSFER))
        MemoryWrite(address, final_result);
    else
#endif
        m_A.SetValue(final_result);
}

INLINE void HuC6280::OPCodes_AND(u8 value)
{
    u8 result;
#if !defined(GG_TESTING)
    if (IsSetFlag(FLAG_TRANSFER))
    {
        u16 address = ZeroPageX();
        u8 a = MemoryRead(address);
        result = a & value;
        MemoryWrite(address, result);
        m_cycles += 3;
    }
    else
#endif
    {
        result = m_A.GetValue() & value;
        m_A.SetValue(result);
    }
    SetOrClearZNFlags(result);
}

INLINE void HuC6280::OPCodes_ASL_Accumulator()
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

INLINE void HuC6280::OPCodes_ASL_Memory(u16 address)
{
    u8 value = MemoryRead(address);
    u8 result = static_cast<u8>(value << 1);
    MemoryWrite(address, result);
    SetOrClearZNFlags(result);
    if ((value & 0x80) != 0)
        SetFlag(FLAG_CARRY);
    else
        ClearFlag(FLAG_CARRY);
}

INLINE void HuC6280::OPcodes_Branch(bool condition)
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

INLINE void HuC6280::OPCodes_BIT(u16 address)
{
    u8 value = MemoryRead(address);
    u8 result = m_A.GetValue() & value;
    ClearFlag(FLAG_ZERO | FLAG_OVERFLOW | FLAG_NEGATIVE);
    u8 flags = m_P.GetValue();
    flags |= (m_zn_flags_lut[result] & FLAG_ZERO);
    flags |= (value & (FLAG_OVERFLOW | FLAG_NEGATIVE));
    m_P.SetValue(flags);
}

INLINE void HuC6280::OPCodes_BIT_Immediate(u16 address)
{
    u8 value = MemoryRead(address);
    u8 result = m_A.GetValue() & value;
    ClearFlag(FLAG_ZERO);
    u8 flags = m_P.GetValue();
    flags |= (m_zn_flags_lut[result] & FLAG_ZERO);
    m_P.SetValue(flags);
}

INLINE void HuC6280::OPCodes_BRK()
{
    u16 pc = m_PC.GetValue();
    StackPush16(pc + 1);

#if !defined(GG_TESTING)
    ClearFlag(FLAG_TRANSFER);
#endif

    StackPush8(m_P.GetValue() | FLAG_BREAK);
    ClearFlag(FLAG_DECIMAL);
    SetFlag(FLAG_INTERRUPT);

#if defined(GG_TESTING)
    m_PC.SetLow(MemoryRead(0xFFFE));
    m_PC.SetHigh(MemoryRead(0xFFFF));
#else
    m_PC.SetLow(MemoryRead(0xFFF6));
    m_PC.SetHigh(MemoryRead(0xFFF7));
#endif

#if !defined(GG_DISABLE_DISASSEMBLER)
    u16 dest = m_PC.GetValue();
    PushCallStack(pc - 1, dest, pc + 1);
#endif
}

INLINE void HuC6280::OPCodes_Subroutine()
{
    u16 pc = m_PC.GetValue();
    s8 displacement = RelativeAddressing();
    u16 dest = static_cast<u16>(m_PC.GetValue() + displacement);

    StackPush16(pc);
    m_PC.SetValue(dest);

#if !defined(GG_DISABLE_DISASSEMBLER)
    PushCallStack(pc - 1, dest, pc);
#endif

}

INLINE void HuC6280::OPCodes_CMP(EightBitRegister* reg, u8 value)
{
    u8 reg_value = reg->GetValue();
    u8 result = reg_value - value;
    SetOrClearZNFlags(result);
    if (reg_value >= value)
        SetFlag(FLAG_CARRY);
    else
        ClearFlag(FLAG_CARRY);
}

INLINE void HuC6280::OPCodes_DEC_Mem(u16 address)
{
    u8 value = MemoryRead(address);
    u8 result = value - 1;
    MemoryWrite(address, result);
    SetOrClearZNFlags(result);
}

INLINE void HuC6280::OPCodes_DEC_Reg(EightBitRegister* reg)
{
    reg->Decrement();
    SetOrClearZNFlags(reg->GetValue());
}

INLINE void HuC6280::OPCodes_EOR(u8 value)
{
    u8 result;
#if !defined(GG_TESTING)
    if (IsSetFlag(FLAG_TRANSFER))
    {
        u16 address = ZeroPageX();
        u8 a = MemoryRead(address);
        result = a ^ value;
        MemoryWrite(address, result);
        m_cycles += 3;
    }
    else
#endif
    {
        result = m_A.GetValue() ^ value;
        m_A.SetValue(result);
    }
    SetOrClearZNFlags(result);
}

INLINE void HuC6280::OPCodes_INC_Mem(u16 address)
{
    u8 value = MemoryRead(address);
    u8 result = value + 1;
    MemoryWrite(address, result);
    SetOrClearZNFlags(result);
}

INLINE void HuC6280::OPCodes_INC_Reg(EightBitRegister* reg)
{
    reg->Increment();
    SetOrClearZNFlags(reg->GetValue());
}

INLINE void HuC6280::OPCodes_LD(EightBitRegister* reg, u8 value)
{
    reg->SetValue(value);
    SetOrClearZNFlags(value);
}

INLINE void HuC6280::OPCodes_LSR_Accumulator()
{
    u8 value = m_A.GetValue();
    u8 result = value >> 1;
    m_A.SetValue(result);
    SetOrClearZNFlags(result);
    if ((value & 0x01) != 0)
        SetFlag(FLAG_CARRY);
    else
        ClearFlag(FLAG_CARRY);
}

INLINE void HuC6280::OPCodes_LSR_Memory(u16 address)
{
    u8 value = MemoryRead(address);
    u8 result = value >> 1;
    MemoryWrite(address, result);
    SetOrClearZNFlags(result);
    if ((value & 0x01) != 0)
        SetFlag(FLAG_CARRY);
    else
        ClearFlag(FLAG_CARRY);
}

INLINE void HuC6280::OPCodes_ORA(u8 value)
{
    u8 result;

#if !defined(GG_TESTING)
    if (IsSetFlag(FLAG_TRANSFER))
    {
        u16 address = ZeroPageX();
        u8 a = MemoryRead(address);
        result = a | value;
        MemoryWrite(address, result);
        m_cycles += 3;
    }
    else
#endif
    {
        result = m_A.GetValue() | value;
        m_A.SetValue(result);
    }
    SetOrClearZNFlags(result);
}

INLINE void HuC6280::OPCodes_RMB(u8 bit, u16 address)
{
    u8 result = UNSET_BIT(MemoryRead(address), bit);
    MemoryWrite(address, result);
}

INLINE void HuC6280::OPCodes_ROL_Accumulator()
{
    u8 value = m_A.GetValue();
    u8 result = static_cast<u8>(value << 1);
    result |= IsSetFlag(FLAG_CARRY) ? 0x01 : 0x00;
    m_A.SetValue(result);
    SetOrClearZNFlags(result);
    if ((value & 0x80) != 0)
        SetFlag(FLAG_CARRY);
    else
        ClearFlag(FLAG_CARRY);
}

INLINE void HuC6280::OPCodes_ROL_Memory(u16 address)
{
    u8 value = MemoryRead(address);
    u8 result = static_cast<u8>(value << 1);
    result |= IsSetFlag(FLAG_CARRY) ? 0x01 : 0x00;
    MemoryWrite(address, result);
    SetOrClearZNFlags(result);
    if ((value & 0x80) != 0)
        SetFlag(FLAG_CARRY);
    else
        ClearFlag(FLAG_CARRY);
}

INLINE void HuC6280::OPCodes_ROR_Accumulator()
{
    u8 value = m_A.GetValue();
    u8 result = value >> 1;
    result |= IsSetFlag(FLAG_CARRY) ? 0x80 : 0x00;
    m_A.SetValue(result);
    SetOrClearZNFlags(result);
    if ((value & 0x01) != 0)
        SetFlag(FLAG_CARRY);
    else
        ClearFlag(FLAG_CARRY);
}

INLINE void HuC6280::OPCodes_ROR_Memory(u16 address)
{
    u8 value = MemoryRead(address);
    u8 result = value >> 1;
    result |= IsSetFlag(FLAG_CARRY) ? 0x80 : 0x00;
    MemoryWrite(address, result);
    SetOrClearZNFlags(result);
    if ((value & 0x01) != 0)
        SetFlag(FLAG_CARRY);
    else
        ClearFlag(FLAG_CARRY);
}

INLINE void HuC6280::OPCodes_SBC(u8 value)
{
    u16 result = 0;

    if (IsSetFlag(FLAG_DECIMAL))
    {
        m_cycles++;

        u16 tmp = (m_A.GetValue() & 0x0f) - (value & 0x0f) - (IsSetFlag(FLAG_CARRY) ? 0 : 1);
        result = m_A.GetValue() - value - (IsSetFlag(FLAG_CARRY) ? 0 : 1);

        if (result & 0x8000)
            result -= 0x60;

        if (tmp & 0x8000)
            result -= 0x06;

#if defined(GG_TESTING)
        u16 bin_result = m_A.GetValue() + ~value + (IsSetFlag(FLAG_CARRY) ? 1 : 0);
        if ((m_A.GetValue() ^ bin_result) & (~value ^ bin_result) & 0x80)
            SetFlag(FLAG_OVERFLOW);
        else
            ClearFlag(FLAG_OVERFLOW);
#endif

        if ((u16)result <= (u16)m_A.GetValue() || (result & 0xff0) == 0xff0)
            SetFlag(FLAG_CARRY);
        else
            ClearFlag(FLAG_CARRY);
    }
    else
    {
        value = ~value;
        result = (u16)m_A.GetValue() + (u16)value + (IsSetFlag(FLAG_CARRY) ? 1 : 0);

        if(~(m_A.GetValue() ^ value) & (m_A.GetValue() ^ result) & 0x80)
            SetFlag(FLAG_OVERFLOW);
        else
            ClearFlag(FLAG_OVERFLOW);

        if(result > 0xFF)
            SetFlag(FLAG_CARRY);
        else
            ClearFlag(FLAG_CARRY);
    }

    SetOrClearZNFlags((u8)result);
    m_A.SetValue((u8)result);
}

INLINE void HuC6280::OPCodes_SMB(u8 bit, u16 address)
{
    u8 result = SET_BIT(MemoryRead(address), bit);
    MemoryWrite(address, result);
}

INLINE void HuC6280::OPCodes_Store(EightBitRegister* reg, u16 address)
{
    u8 value = reg->GetValue();
    MemoryWrite(address, value);
}

INLINE void HuC6280::OPCodes_STZ(u16 address)
{
    MemoryWrite(address, 0x00);
}

INLINE void HuC6280::OPCodes_Swap(EightBitRegister* reg1, EightBitRegister* reg2)
{
    u8 temp = reg1->GetValue();
    reg1->SetValue(reg2->GetValue());
    reg2->SetValue(temp);
}

INLINE void HuC6280::OPCodes_TAM()
{
    u8 bits = Fetch8();
    m_memory->SetMprTAM(bits, m_A.GetValue());
}

INLINE void HuC6280::OPCodes_TMA()
{
    u8 bits = Fetch8();
    m_A.SetValue(m_memory->GetMprTMA(bits));
}

INLINE void HuC6280::OPCodes_Transfer(EightBitRegister* source, EightBitRegister* dest)
{
    u8 value = source->GetValue();
    dest->SetValue(value);
    SetOrClearZNFlags(value);
}

INLINE void HuC6280::OPCodes_TRB(u16 address)
{
    u8 value = MemoryRead(address);
    u8 result = ~m_A.GetValue() & value;
    MemoryWrite(address, result);
#if defined(GG_TESTING)
    ClearFlag(FLAG_ZERO);
#else
    ClearFlag(FLAG_ZERO | FLAG_OVERFLOW | FLAG_NEGATIVE);
#endif
    u8 flags = m_P.GetValue();
    flags |= (m_zn_flags_lut[m_A.GetValue() & value] & FLAG_ZERO);
#if !defined(GG_TESTING)
    flags |= (value & (FLAG_OVERFLOW | FLAG_NEGATIVE));
#endif
    m_P.SetValue(flags);
}

INLINE void HuC6280::OPCodes_TSB(u16 address)
{
    u8 value = MemoryRead(address);
    u8 result = m_A.GetValue() | value;
    MemoryWrite(address, result);
#if defined(GG_TESTING)
    ClearFlag(FLAG_ZERO);
#else
    ClearFlag(FLAG_ZERO | FLAG_OVERFLOW | FLAG_NEGATIVE);
#endif
    u8 flags = m_P.GetValue();
    flags |= (m_zn_flags_lut[m_A.GetValue() & value] & FLAG_ZERO);
#if !defined(GG_TESTING)
    flags |= (value & (FLAG_OVERFLOW | FLAG_NEGATIVE));
#endif
    m_P.SetValue(flags);
}

INLINE void HuC6280::OPCodes_TST(u8 value, u16 address)
{
    u8 mem = MemoryRead(address);
    ClearFlag(FLAG_ZERO | FLAG_OVERFLOW | FLAG_NEGATIVE);
    u8 flags = m_P.GetValue();
    flags |= ((value & mem) ? 0 : FLAG_ZERO);
    flags |= (mem & (FLAG_OVERFLOW | FLAG_NEGATIVE));
    m_P.SetValue(flags);
}

INLINE void HuC6280::OPCodes_TAI()
{
    if (m_transfer_state == 0)
    {
        OPCodes_TransferStart();
        return;
    }

    if (m_transfer_state == 2)
    {
        MemoryWrite(m_transfer_dest, MemoryRead(m_transfer_source, true));
        m_transfer_source += (m_transfer_count & 1) ? -1 : 1;
        m_transfer_dest++;
        m_transfer_count++;
        m_transfer_length--;
        m_cycles += 6;

        if (m_transfer_length == 0)
            m_transfer_state = 1;

        m_PC.Decrement();
        return;
    }

    if (m_transfer_state == 1)
    {
        OPCodes_TransferEnd();
        return;
    }
}

INLINE void HuC6280::OPCodes_TDD()
{
    if (m_transfer_state == 0)
    {
        OPCodes_TransferStart();
        return;
    }

    if (m_transfer_state == 2)
    {
        MemoryWrite(m_transfer_dest, MemoryRead(m_transfer_source, true));
        m_transfer_source--;
        m_transfer_dest--;
        m_transfer_length--;
        m_cycles += 6;

        if (m_transfer_length == 0)
            m_transfer_state = 1;

        m_PC.Decrement();
        return;
    }

    if (m_transfer_state == 1)
    {
        OPCodes_TransferEnd();
        return;
    }
}

INLINE void HuC6280::OPCodes_TIA()
{
    if (m_transfer_state == 0)
    {
        OPCodes_TransferStart();
        return;
    }

    if (m_transfer_state == 2)
    {
        MemoryWrite(m_transfer_dest, MemoryRead(m_transfer_source, true));
        m_transfer_source++;
        m_transfer_dest += (m_transfer_count & 1) ? -1 : 1;
        m_transfer_count++;
        m_transfer_length--;
        m_cycles += 6;

        if (m_transfer_length == 0)
            m_transfer_state = 1;

        m_PC.Decrement();
        return;
    }

    if (m_transfer_state == 1)
    {
        OPCodes_TransferEnd();
        return;
    }
}

INLINE void HuC6280::OPCodes_TII()
{
    if (m_transfer_state == 0)
    {
        OPCodes_TransferStart();
        return;
    }

    if (m_transfer_state == 2)
    {
        MemoryWrite(m_transfer_dest, MemoryRead(m_transfer_source, true));
        m_transfer_source++;
        m_transfer_dest++;
        m_transfer_length--;
        m_cycles += 6;

        if (m_transfer_length == 0)
            m_transfer_state = 1;

        m_PC.Decrement();
        return;
    }

    if (m_transfer_state == 1)
    {
        OPCodes_TransferEnd();
        return;
    }
}

INLINE void HuC6280::OPCodes_TIN()
{
    if (m_transfer_state == 0)
    {
        OPCodes_TransferStart();
        return;
    }

    if (m_transfer_state == 2)
    {
        MemoryWrite(m_transfer_dest, MemoryRead(m_transfer_source, true));
        m_transfer_source++;
        m_transfer_length--;
        m_cycles += 6;

        if (m_transfer_length == 0)
            m_transfer_state = 1;

        m_PC.Decrement();
        return;
    }

    if (m_transfer_state == 1)
    {
        OPCodes_TransferEnd();
        return;
    }
}

INLINE void HuC6280::OPCodes_TransferStart()
{
    m_transfer_state = 2;

    StackPush8(m_Y.GetValue());
    StackPush8(m_X.GetValue());
    StackPush8(m_A.GetValue());

    m_transfer_source = Fetch16();
    m_transfer_dest = Fetch16();
    m_transfer_length = Fetch16();
    m_transfer_count = 0;

    m_PC.SetValue(m_PC.GetValue() - 7);
    m_cycles += 14;
}

INLINE void HuC6280::OPCodes_TransferEnd()
{
    m_transfer_state = 0;
    m_A.SetValue(StackPop8());
    m_X.SetValue(StackPop8());
    m_Y.SetValue(StackPop8());
    m_PC.SetValue(m_PC.GetValue() + 6);
    m_cycles += 3;
}

INLINE void HuC6280::UnofficialOPCode()
{
#if defined(GG_DEBUG)
    u16 opcode_address = m_PC.GetValue() - 1;
    u8 opcode = m_memory->Read(opcode_address);
    Debug("** HuC6280 --> UNOFFICIAL OP Code (%02X) at $%.4X -- %s", opcode, opcode_address, k_huc6280_opcode_names[opcode]);
#endif
}

#endif /* HUC6280_OPCODES_INLINE_H */