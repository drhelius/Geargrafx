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

#include "huc6280.h"
#include "memory.h"

void HuC6280::OPCode0x00()
{
    // OK
    // BRK
    OPCodes_BRK();
}

void HuC6280::OPCode0x01()
{
    // OK
    // ORA (ZZ,X)
    OPCodes_ORA(m_memory->Read(ZeroPageIndexedIndirectAddressing()));
}

void HuC6280::OPCode0x02()
{
    // SXY
    OPCodes_Swap(&m_X, &m_Y);
}

void HuC6280::OPCode0x03()
{
    // TODO
    // ST1 #nn
    UnofficialOPCode();
}

void HuC6280::OPCode0x04()
{
    // UNOFFICIAL
    // NOP $n
    UnofficialOPCode();
}

void HuC6280::OPCode0x05()
{
    // OK
    // ORA ZZ
    OPCodes_ORA(m_memory->Read(ZeroPageAddressing()));
}

void HuC6280::OPCode0x06()
{
    // OK
    // ASL ZZ
    OPCodes_ASL_Memory(ZeroPageAddressing());
}

void HuC6280::OPCode0x07()
{
    // OK
    // RMB0 ZZ
    OPCodes_RMB(0, ZeroPageAddressing());
}

void HuC6280::OPCode0x08()
{
    // OK
    // PHP
    ClearFlag(FLAG_MEMORY);
    // TODO
    // SetFlag(FLAG_BRK);
    StackPush8(m_P.GetValue());
}

void HuC6280::OPCode0x09()
{
    // OK
    // ORA #nn
    OPCodes_ORA(ImmediateAddressing());
}

void HuC6280::OPCode0x0A()
{
    // OK
    // ASL A
    OPCodes_ASL_Accumulator();
}

void HuC6280::OPCode0x0B()
{
    // UNOFFICIAL
    // ANC #$n
    UnofficialOPCode();
}

void HuC6280::OPCode0x0C()
{
    // UNOFFICIAL
    // NOP $nn
    UnofficialOPCode();
}

void HuC6280::OPCode0x0D()
{
    // OK
    // ORA hhll
    OPCodes_ORA(m_memory->Read(AbsoluteAddressing()));
}

void HuC6280::OPCode0x0E()
{
    // OK
    // ASL hhll
    OPCodes_ASL_Memory(AbsoluteAddressing());
}

void HuC6280::OPCode0x0F()
{
    // OK
    // BBR0 ZZ,hhll
    OPcodes_Branch(!IsSetBit(0, m_memory->Read(ZeroPageAddressing())));
}

void HuC6280::OPCode0x10()
{
    // OK
    // BPL hhll
    OPcodes_Branch(!IsSetFlag(FLAG_NEGATIVE));
}

void HuC6280::OPCode0x11()
{
    // OK
    // ORA (ZZ),Y
    OPCodes_ORA(m_memory->Read(ZeroPageIndirectIndexedAddressing()));
}

void HuC6280::OPCode0x12()
{
    // OK
    // ORA (ZZ)
    OPCodes_ORA(m_memory->Read(ZeroPageIndirectAddressing()));
}

void HuC6280::OPCode0x13()
{
    // UNOFFICIAL
    // SLO ($n),Y
    UnofficialOPCode();
}

void HuC6280::OPCode0x14()
{
    // UNOFFICIAL
    // NOP $n,X
    UnofficialOPCode();
}

void HuC6280::OPCode0x15()
{
    // ORA ZZ,X
    OPCodes_ORA(m_memory->Read(ZeroPageAddressing(&m_X)));
}

void HuC6280::OPCode0x16()
{
    // OK
    // ASL ZZ,X
    OPCodes_ASL_Memory(ZeroPageAddressing(&m_X));
}

void HuC6280::OPCode0x17()
{
    // OK
    // RMB1 ZZ
    OPCodes_RMB(1, ZeroPageAddressing());
}

void HuC6280::OPCode0x18()
{
    // OK
    // CLC
    OPCodes_ClearFlag(FLAG_CARRY);
}

void HuC6280::OPCode0x19()
{
    // OK
    // ORA hhll,Y
    OPCodes_ORA(m_memory->Read(AbsoluteAddressing(&m_Y)));
}

void HuC6280::OPCode0x1A()
{
    // OK
    // INC A
    OPCodes_INC_Reg(&m_A);
}

void HuC6280::OPCode0x1B()
{
    // UNOFFICIAL
    // SLO $nn,Y
    UnofficialOPCode();
}

void HuC6280::OPCode0x1C()
{
    // UNOFFICIAL
    // NOP $nn,X
    UnofficialOPCode();
}

void HuC6280::OPCode0x1D()
{
    // OK
    // ORA hhll,X
    OPCodes_ORA(m_memory->Read(AbsoluteAddressing(&m_X)));
}

void HuC6280::OPCode0x1E()
{
    // OK
    // ASL hhll,X
    OPCodes_ASL_Memory(AbsoluteAddressing(&m_X));
}

void HuC6280::OPCode0x1F()
{
    // OK
    // BBR1 ZZ,hhll
    OPcodes_Branch(!IsSetBit(1, m_memory->Read(ZeroPageAddressing())));
}

void HuC6280::OPCode0x20()
{
    // OK
    // JSR $nn
    ClearFlag(FLAG_MEMORY);
    u16 target = AbsoluteAddressing();
    StackPush16(m_PC.GetValue() - 1);
    m_PC.SetValue(target);
}

void HuC6280::OPCode0x21()
{
    // OK
    // AND (ZZ,X)
    OPCodes_AND(m_memory->Read(ZeroPageIndexedIndirectAddressing()));
}

void HuC6280::OPCode0x22()
{
    // OK
    // SAX
    OPCodes_Swap(&m_A, &m_X);
}

void HuC6280::OPCode0x23()
{
    // UNOFFICIAL
    // RLA $(nn,X)
    UnofficialOPCode();
}

void HuC6280::OPCode0x24()
{
    // OK
    // BIT ZZ
    OPCodes_BIT(ZeroPageAddressing());
}

void HuC6280::OPCode0x25()
{
    // OK
    // AND ZZ
    OPCodes_AND(m_memory->Read(ZeroPageAddressing()));
}

void HuC6280::OPCode0x26()
{
    // OK
    // ROL ZZ
    OPCodes_ROL_Memory(ZeroPageAddressing());
}

void HuC6280::OPCode0x27()
{
    // OK
    // RMB2 ZZ
    OPCodes_RMB(2, ZeroPageAddressing());
}

void HuC6280::OPCode0x28()
{
    // OK
    // PLP
    // TODO
    //m_P.SetValue((StackPop8() & 0xCF) | (m_P.GetValue() & 0x30));
    m_P.SetValue(StackPop8());
}

void HuC6280::OPCode0x29()
{
    // OK
    // AND #nn
    OPCodes_AND(ImmediateAddressing());
}

void HuC6280::OPCode0x2A()
{
    // OK
    // ROL A
    OPCodes_ROL_Accumulator();
}

void HuC6280::OPCode0x2B()
{
    // UNOFFICIAL
    // ANC #$n
    UnofficialOPCode();
}

void HuC6280::OPCode0x2C()
{
    // OK
    // BIT hhll
    OPCodes_BIT(AbsoluteAddressing());
}

void HuC6280::OPCode0x2D()
{
    // OK
    // AND hhll
    OPCodes_AND(m_memory->Read(AbsoluteAddressing()));
}

void HuC6280::OPCode0x2E()
{
    // OK
    // ROL hhll
    OPCodes_ROL_Memory(AbsoluteAddressing());
}

void HuC6280::OPCode0x2F()
{
    // OK
    // BBR2 ZZ,hhll
    OPcodes_Branch(!IsSetBit(2, m_memory->Read(ZeroPageAddressing())));
}

void HuC6280::OPCode0x30()
{
    // OKI
    // BMI hhll
    OPcodes_Branch(IsSetFlag(FLAG_NEGATIVE));
}

void HuC6280::OPCode0x31()
{
    // OK
    // AND (ZZ),Y
    OPCodes_AND(m_memory->Read(ZeroPageIndirectIndexedAddressing()));
}

void HuC6280::OPCode0x32()
{
    // OK
    // AND (ZZ)
    OPCodes_AND(m_memory->Read(ZeroPageIndirectAddressing()));
}

void HuC6280::OPCode0x33()
{
    // UNOFFICIAL
    // RLA ($n),Y
    UnofficialOPCode();
}

void HuC6280::OPCode0x34()
{
    // OK
    // BIT ZZ,X
    OPCodes_BIT(ZeroPageAddressing(&m_X));
}

void HuC6280::OPCode0x35()
{
    // OK
    // AND ZZ,X
    OPCodes_AND(m_memory->Read(ZeroPageAddressing(&m_X)));
}

void HuC6280::OPCode0x36()
{
    // OK
    // ROL ZZ,X
    OPCodes_ROL_Memory(ZeroPageAddressing(&m_X));
}

void HuC6280::OPCode0x37()
{
    // OK
    // RMB3 ZZ
    OPCodes_RMB(3, ZeroPageAddressing());
}

void HuC6280::OPCode0x38()
{
    // SEC
    OPCodes_SetFlag(FLAG_CARRY);
}

void HuC6280::OPCode0x39()
{
    // OK
    // AND hhll,Y
    OPCodes_AND(m_memory->Read(AbsoluteAddressing(&m_Y)));
}

void HuC6280::OPCode0x3A()
{
    // OK
    // DEC A
    OPCodes_DEC_Reg(&m_A);
}

void HuC6280::OPCode0x3B()
{
    // UNOFFICIAL
    // RLA $nn,Y
    UnofficialOPCode();
}

void HuC6280::OPCode0x3C()
{
    // OK
    // BIT hhll,X
    OPCodes_BIT(AbsoluteAddressing(&m_X));
}

void HuC6280::OPCode0x3D()
{
    // OK
    // AND hhll,X
    OPCodes_AND(m_memory->Read(AbsoluteAddressing(&m_X)));
}

void HuC6280::OPCode0x3E()
{
    // OK
    // ROL hhll,X
    OPCodes_ROL_Memory(AbsoluteAddressing(&m_X));
}

void HuC6280::OPCode0x3F()
{
    // OK
    // BBR3 ZZ,hhll
    OPcodes_Branch(!IsSetBit(3, m_memory->Read(ZeroPageAddressing())));
}

void HuC6280::OPCode0x40()
{
    // OK
    // RTI
    // TODO
    // m_P.SetValue((StackPop8() & 0xCF) | (m_P.GetValue() & 0x30));
    m_P.SetValue(StackPop8());
    m_PC.SetValue(StackPop16());
}

void HuC6280::OPCode0x41()
{
    // OK
    // EOR (ZZ,X)
    OPCodes_EOR(m_memory->Read(ZeroPageIndexedIndirectAddressing()));
}

void HuC6280::OPCode0x42()
{
    // OK
    // SAY
    OPCodes_Swap(&m_A, &m_Y);
}

void HuC6280::OPCode0x43()
{
    // UNOFFICIAL
    // SRE $(nn,X);
    UnofficialOPCode();
}

void HuC6280::OPCode0x44()
{
    // OK
    // BSR hhll
    OPCodes_Subroutine();
}

void HuC6280::OPCode0x45()
{
    // OK
    // EOR ZZ
    OPCodes_EOR(m_memory->Read(ZeroPageAddressing()));
}

void HuC6280::OPCode0x46()
{
    // OK
    // LSR ZZ
    OPCodes_LSR_Memory(ZeroPageAddressing());
}

void HuC6280::OPCode0x47()
{
    // OK
    // RMB4 ZZ
    OPCodes_RMB(4, ZeroPageAddressing());
}

void HuC6280::OPCode0x48()
{
    // OK
    // PHA
    StackPush8(m_A.GetValue());
}

void HuC6280::OPCode0x49()
{
    // OK
    // EOR #nn
    OPCodes_EOR(ImmediateAddressing());
}

void HuC6280::OPCode0x4A()
{
    // OK
    // LSR A
    OPCodes_LSR_Accumulator();
}

void HuC6280::OPCode0x4B()
{
    // UNOFFICIAL
    // ALR #$n
    UnofficialOPCode();
}

void HuC6280::OPCode0x4C()
{
    // OK
    // JMP hhll
    ClearFlag(FLAG_MEMORY);
    m_PC.SetValue(AbsoluteAddressing());
}

void HuC6280::OPCode0x4D()
{
    // OK
    // EOR hhll
    OPCodes_EOR(m_memory->Read(AbsoluteAddressing()));
}

void HuC6280::OPCode0x4E()
{
    // OK
    // LSR hhll
    OPCodes_LSR_Memory(AbsoluteAddressing());
}

void HuC6280::OPCode0x4F()
{
    // OK
    // BBR4 ZZ,hhll
    OPcodes_Branch(!IsSetBit(4, m_memory->Read(ZeroPageAddressing())));
}

void HuC6280::OPCode0x50()
{
    // OK
    // BVC hhll
    OPcodes_Branch(!IsSetFlag(FLAG_OVERFLOW));
}

void HuC6280::OPCode0x51()
{
    // OK
    // EOR (ZZ),Y
    OPCodes_EOR(m_memory->Read(ZeroPageIndirectIndexedAddressing()));
}

void HuC6280::OPCode0x52()
{
    // OK
    // EOR (ZZ)
    OPCodes_EOR(m_memory->Read(ZeroPageIndirectAddressing()));
}

void HuC6280::OPCode0x53()
{
    // UNOFFICIAL
    // SRE ($n),Y
    UnofficialOPCode();
}

void HuC6280::OPCode0x54()
{
    // UNOFFICIAL
    // NOP $n,X
    UnofficialOPCode();
}

void HuC6280::OPCode0x55()
{
    // OK
    // EOR ZZ,X
    OPCodes_EOR(m_memory->Read(ZeroPageAddressing(&m_X)));
}

void HuC6280::OPCode0x56()
{
    // OK
    // LSR ZZ,X
    OPCodes_LSR_Memory(ZeroPageAddressing(&m_X));
}

void HuC6280::OPCode0x57()
{
    // OK
    // RMB5 ZZ
    OPCodes_RMB(5, ZeroPageAddressing());
}

void HuC6280::OPCode0x58()
{
    // OK
    // CLI
    OPCodes_ClearFlag(FLAG_IRQ);
}

void HuC6280::OPCode0x59()
{
    // OK
    // EOR hhll,Y
    OPCodes_EOR(m_memory->Read(AbsoluteAddressing(&m_Y)));
}

void HuC6280::OPCode0x5A()
{
    // OK
    // PHY
    ClearFlag(FLAG_MEMORY);
    StackPush8(m_Y.GetValue());
}

void HuC6280::OPCode0x5B()
{
    // UNOFFICIAL
    // SRE $nn,Y
    UnofficialOPCode();
}

void HuC6280::OPCode0x5C()
{
    // UNOFFICIAL
    // NOP $nn,X
    UnofficialOPCode();
}

void HuC6280::OPCode0x5D()
{
    // OK
    // EOR hhll,X
    OPCodes_EOR(m_memory->Read(AbsoluteAddressing(&m_X)));
}

void HuC6280::OPCode0x5E()
{
    // OK
    // LSR hhll,X
    OPCodes_LSR_Memory(AbsoluteAddressing(&m_X));
}

void HuC6280::OPCode0x5F()
{
    // OK
    // BBR5 ZZ,hhll
    OPcodes_Branch(!IsSetBit(5, m_memory->Read(ZeroPageAddressing())));
}

void HuC6280::OPCode0x60()
{
    // OK
    // RTS
    ClearFlag(FLAG_MEMORY);
    m_PC.SetValue(StackPop16() + 1);
}

void HuC6280::OPCode0x61()
{
    // OK
    // ADC (ZZ,X)
    OPCodes_ADC(m_memory->Read(ZeroPageIndexedIndirectAddressing()));
}

void HuC6280::OPCode0x62()
{
    // OK
    // CLA
    ClearFlag(FLAG_MEMORY);
    m_A.SetValue(0x00);
}

void HuC6280::OPCode0x63()
{
    // UNOFFICIAL
    // RRA $(nn,X)
    UnofficialOPCode();
}

void HuC6280::OPCode0x64()
{
    // UNOFFICIAL
    // NOP $n
    UnofficialOPCode();
}

void HuC6280::OPCode0x65()
{
    // OK
    // ADC ZZ
    OPCodes_ADC(m_memory->Read(ZeroPageAddressing()));
}

void HuC6280::OPCode0x66()
{
    // OK
    // ROR ZZ
    OPCodes_ROR_Memory(ZeroPageAddressing());
}

void HuC6280::OPCode0x67()
{
    // OK
    // RMB6 ZZ
    OPCodes_RMB(6, ZeroPageAddressing());
}

void HuC6280::OPCode0x68()
{
    // OK
    // PLA
    ClearFlag(FLAG_MEMORY);
    u8 result = StackPop8();
    m_A.SetValue(result);
    SetZeroFlagFromResult(result);
    SetNegativeFlagFromResult(result);
}

void HuC6280::OPCode0x69()
{
    // OK
    // ADC #nn
    OPCodes_ADC(ImmediateAddressing());
}

void HuC6280::OPCode0x6A()
{
    // OK
    // ROR A
    OPCodes_ROR_Accumulator();
}

void HuC6280::OPCode0x6B()
{
    // UNOFFICIAL
    // ARR #$n
    UnofficialOPCode();
}

void HuC6280::OPCode0x6C()
{
    // OK
    // JMP (hhll)
    ClearFlag(FLAG_MEMORY);
    m_PC.SetValue(AbsoluteIndirectAddressing());
}

void HuC6280::OPCode0x6D()
{
    // OK
    // ADC (hhll)
    OPCodes_ADC(m_memory->Read(AbsoluteAddressing()));
}

void HuC6280::OPCode0x6E()
{
    // OK
    // ROR hhll
    OPCodes_ROR_Memory(AbsoluteAddressing());
}

void HuC6280::OPCode0x6F()
{
    // OK
    // BBR6 ZZ,hhll
    OPcodes_Branch(!IsSetBit(6, m_memory->Read(ZeroPageAddressing())));
}

void HuC6280::OPCode0x70()
{
    // OK
    // BVS hhll
    OPcodes_Branch(IsSetFlag(FLAG_OVERFLOW));
}

void HuC6280::OPCode0x71()
{
    // OK
    // ADC (ZZ),Y
    OPCodes_ADC(m_memory->Read(ZeroPageIndirectIndexedAddressing()));
}

void HuC6280::OPCode0x72()
{
    // OK
    // ADC (ZZ)
    OPCodes_ADC(m_memory->Read(ZeroPageIndirectAddressing()));
}

void HuC6280::OPCode0x73()
{
    // UNOFFICIAL
    // RRA ($n),Y
    UnofficialOPCode();
}

void HuC6280::OPCode0x74()
{
    // UNOFFICIAL
    // NOP $n,X
    UnofficialOPCode();
}

void HuC6280::OPCode0x75()
{
    // OK
    // ADC ZZ,X
    OPCodes_ADC(m_memory->Read(ZeroPageAddressing(&m_X)));
}

void HuC6280::OPCode0x76()
{
    // OK
    // ROR ZZ,X
    OPCodes_ROR_Memory(ZeroPageAddressing(&m_X));
}

void HuC6280::OPCode0x77()
{
    // OK
    // RMB7 ZZ
    OPCodes_RMB(7, ZeroPageAddressing());
}

void HuC6280::OPCode0x78()
{
    // SEI
    OPCodes_SetFlag(FLAG_IRQ);
}

void HuC6280::OPCode0x79()
{
    // OK
    // ADC hhll,Y
    OPCodes_ADC(m_memory->Read(AbsoluteAddressing(&m_Y)));
}

void HuC6280::OPCode0x7A()
{
    // OK
    // PLY
    ClearFlag(FLAG_MEMORY);
    u8 result = StackPop8();
    m_Y.SetValue(result);
    SetZeroFlagFromResult(result);
    SetNegativeFlagFromResult(result);
}

void HuC6280::OPCode0x7B()
{
    // UNOFFICIAL
    // RRA $nn,Y
    UnofficialOPCode();
}

void HuC6280::OPCode0x7C()
{
    // OK
    // JMP (hhll,X)
    ClearFlag(FLAG_MEMORY);
    m_PC.SetValue(AbsoluteIndexedIndirectAddressing());
}

void HuC6280::OPCode0x7D()
{
    // OK
    // ADC hhll,X
    OPCodes_ADC(m_memory->Read(AbsoluteAddressing(&m_X)));
}

void HuC6280::OPCode0x7E()
{
    // OK
    // ROR hhll,X
    OPCodes_ROR_Memory(AbsoluteAddressing(&m_X));
}

void HuC6280::OPCode0x7F()
{
    // OK
    // BBR7 ZZ,hhll
    OPcodes_Branch(!IsSetBit(7, m_memory->Read(ZeroPageAddressing())));
}

void HuC6280::OPCode0x80()
{
    // OK
    // BRA hhll
    OPcodes_Branch(true);
}

void HuC6280::OPCode0x81()
{
    // STA $(nn,X)
    OPCodes_Store(&m_A, ZeroPageIndexedIndirectAddressing());
}

void HuC6280::OPCode0x82()
{
    // OK
    // CLX
    ClearFlag(FLAG_MEMORY);
    m_X.SetValue(0x00);
}

void HuC6280::OPCode0x83()
{
    // UNOFFICIAL
    // SAX $(nn,X)
    UnofficialOPCode();
}

void HuC6280::OPCode0x84()
{
    // STY $n
    OPCodes_Store(&m_Y, ZeroPageAddressing());
}

void HuC6280::OPCode0x85()
{
    // STA $n
    OPCodes_Store(&m_A, ZeroPageAddressing());
}

void HuC6280::OPCode0x86()
{
    // STX $n
    OPCodes_Store(&m_X, ZeroPageAddressing());
}

void HuC6280::OPCode0x87()
{
    // UNOFFICIAL
    // SAX $n
    UnofficialOPCode();
}

void HuC6280::OPCode0x88()
{
    // OK
    // DEY
    OPCodes_DEC_Reg(&m_Y);
}

void HuC6280::OPCode0x89()
{
    // OK
    // BIT #nn
    OPCodes_BIT(m_PC.GetValue());
    m_PC.Increment();
}

void HuC6280::OPCode0x8A()
{
    // TXA
    OPCodes_Transfer(&m_X, &m_A);
}

void HuC6280::OPCode0x8B()
{
    // UNOFFICIAL
    // XAA #$n
    UnofficialOPCode();
}

void HuC6280::OPCode0x8C()
{
    // STY $nn
    OPCodes_Store(&m_Y, AbsoluteAddressing());
}

void HuC6280::OPCode0x8D()
{
    // STA $nn
    OPCodes_Store(&m_A, AbsoluteAddressing());
}

void HuC6280::OPCode0x8E()
{
    // STX $nn
    OPCodes_Store(&m_X, AbsoluteAddressing());
}

void HuC6280::OPCode0x8F()
{
    // OK
    // BBS0 ZZ,hhll
    OPcodes_Branch(IsSetBit(0, m_memory->Read(ZeroPageAddressing())));
}

void HuC6280::OPCode0x90()
{
    // OK
    // BCC hhll
    OPcodes_Branch(!IsSetFlag(FLAG_CARRY));
}

void HuC6280::OPCode0x91()
{
    // STA ($n),Y
    OPCodes_Store(&m_A, ZeroPageIndirectIndexedAddressing());
}

void HuC6280::OPCode0x92()
{
    // UNOFFICIAL
    // KILL
    UnofficialOPCode();
}

void HuC6280::OPCode0x93()
{
    // UNOFFICIAL
    // AHX ($n),Y
    UnofficialOPCode();
}

void HuC6280::OPCode0x94()
{
    // STY $n,X
    OPCodes_Store(&m_Y, ZeroPageAddressing(&m_X));
}

void HuC6280::OPCode0x95()
{
    // STA $n,X
    OPCodes_Store(&m_A, ZeroPageAddressing(&m_X));
}

void HuC6280::OPCode0x96()
{
    // STX $n,Y
    OPCodes_Store(&m_X, ZeroPageAddressing(&m_Y));
}

void HuC6280::OPCode0x97()
{
    // UNOFFICIAL
    // SAX $n,Y
    UnofficialOPCode();
}

void HuC6280::OPCode0x98()
{
    // TYA
    OPCodes_Transfer(&m_Y, &m_A);
}

void HuC6280::OPCode0x99()
{
    // STA $nn,Y
    OPCodes_Store(&m_A, AbsoluteAddressing(&m_Y));
}

void HuC6280::OPCode0x9A()
{
    // TXS
    OPCodes_Transfer(&m_X, &m_S);
}

void HuC6280::OPCode0x9B()
{
    // UNOFFICIAL
    // TAS $nn,Y
    UnofficialOPCode();
}

void HuC6280::OPCode0x9C()
{
    // UNOFFICIAL
    // SHY $nn,X
    UnofficialOPCode();
}

void HuC6280::OPCode0x9D()
{
    // STA $nn,X
    OPCodes_Store(&m_A, AbsoluteAddressing(&m_X));
}

void HuC6280::OPCode0x9E()
{
    // UNOFFICIAL
    // SHX $nn,Y
    UnofficialOPCode();
}

void HuC6280::OPCode0x9F()
{
    // OK
    // BBS1 ZZ,hhll
    OPcodes_Branch(IsSetBit(1, m_memory->Read(ZeroPageAddressing())));
}

void HuC6280::OPCode0xA0()
{
    // OK
    // LDY #nn
    OPCodes_LD(&m_Y, ImmediateAddressing());
}

void HuC6280::OPCode0xA1()
{
    // OK
    // LDA $(ZZ,X)
    OPCodes_LD(&m_A, m_memory->Read(ZeroPageIndexedIndirectAddressing()));
}

void HuC6280::OPCode0xA2()
{
    // OK
    // LDX #nn
    OPCodes_LD(&m_X, ImmediateAddressing());
}

void HuC6280::OPCode0xA3()
{
    // UNOFFICIAL
    // LAX $(nn,X)
    UnofficialOPCode();
}

void HuC6280::OPCode0xA4()
{
    // OK
    // LDY ZZ
    OPCodes_LD(&m_Y, m_memory->Read(ZeroPageAddressing()));
}

void HuC6280::OPCode0xA5()
{
    // OK
    // LDA ZZ
    OPCodes_LD(&m_A, m_memory->Read(ZeroPageAddressing()));
}

void HuC6280::OPCode0xA6()
{
    // OK
    // LDX ZZ
    OPCodes_LD(&m_X, m_memory->Read(ZeroPageAddressing()));
}

void HuC6280::OPCode0xA7()
{
    // UNOFFICIAL
    // LAX $n
    UnofficialOPCode();
}

void HuC6280::OPCode0xA8()
{
    // TAY
    OPCodes_Transfer(&m_A, &m_Y);
}

void HuC6280::OPCode0xA9()
{
    // OK
    // LDA #nn
    OPCodes_LD(&m_A, ImmediateAddressing());
}

void HuC6280::OPCode0xAA()
{
    // TAX
    OPCodes_Transfer(&m_A, &m_X);
}

void HuC6280::OPCode0xAB()
{
    // UNOFFICIAL
    // LAX #$n
    UnofficialOPCode();
}

void HuC6280::OPCode0xAC()
{
    // OK
    // LDY hhll
    OPCodes_LD(&m_Y, m_memory->Read(AbsoluteAddressing()));
}

void HuC6280::OPCode0xAD()
{
    // OK
    // LDA hhll
    OPCodes_LD(&m_A, m_memory->Read(AbsoluteAddressing()));
}

void HuC6280::OPCode0xAE()
{
    // OK
    // LDX hhll
    OPCodes_LD(&m_X, m_memory->Read(AbsoluteAddressing()));
}

void HuC6280::OPCode0xAF()
{
    // OK
    // BBS2 ZZ,hhll
    OPcodes_Branch(IsSetBit(2, m_memory->Read(ZeroPageAddressing())));
}

void HuC6280::OPCode0xB0()
{
    // OK
    // BCS hhll
    OPcodes_Branch(IsSetFlag(FLAG_CARRY));
}

void HuC6280::OPCode0xB1()
{
    // OK
    // LDA ($n),Y
    OPCodes_LD(&m_A, m_memory->Read(ZeroPageIndirectIndexedAddressing()));
}

void HuC6280::OPCode0xB2()
{
    // OK
    // LDA (ZZ)
    OPCodes_LD(&m_A, m_memory->Read(ZeroPageIndirectAddressing()));
}

void HuC6280::OPCode0xB3()
{
    // UNOFFICIAL
    // LAX ($n),Y
    UnofficialOPCode();
}

void HuC6280::OPCode0xB4()
{
    // OK
    // LDY ZZ,X
    OPCodes_LD(&m_Y, m_memory->Read(ZeroPageAddressing(&m_X)));
}

void HuC6280::OPCode0xB5()
{
    // OK
    // LDA ZZ,X
    OPCodes_LD(&m_A, m_memory->Read(ZeroPageAddressing(&m_X)));
}

void HuC6280::OPCode0xB6()
{
    // OK
    // LDX ZZ,Y
    OPCodes_LD(&m_X, m_memory->Read(ZeroPageAddressing(&m_Y)));
}

void HuC6280::OPCode0xB7()
{
    // UNOFFICIAL
    // LAX $n,Y
    UnofficialOPCode();
}

void HuC6280::OPCode0xB8()
{
    // OK
    // CLV
    OPCodes_ClearFlag(FLAG_OVERFLOW);
}

void HuC6280::OPCode0xB9()
{
    // OK
    // LDA hhll,Y
    OPCodes_LD(&m_A, m_memory->Read(AbsoluteAddressing(&m_Y)));
}

void HuC6280::OPCode0xBA()
{
    // TSX
    OPCodes_Transfer(&m_S, &m_X);
}

void HuC6280::OPCode0xBB()
{
    // UNOFFICIAL
    // LAS $nn,Y
    UnofficialOPCode();
}

void HuC6280::OPCode0xBC()
{
    // OK
    // LDY hhll,X
    OPCodes_LD(&m_Y, m_memory->Read(AbsoluteAddressing(&m_X)));
}

void HuC6280::OPCode0xBD()
{
    // OK
    // LDA hhll,X
    OPCodes_LD(&m_A, m_memory->Read(AbsoluteAddressing(&m_X)));
}

void HuC6280::OPCode0xBE()
{
    // OK
    // LDX hhll,Y
    OPCodes_LD(&m_X, m_memory->Read(AbsoluteAddressing(&m_Y)));
}

void HuC6280::OPCode0xBF()
{
    // OK
    // BBS3 ZZ,hhll
    OPcodes_Branch(IsSetBit(3, m_memory->Read(ZeroPageAddressing())));
}

void HuC6280::OPCode0xC0()
{
    // OK
    // CPY #nn
    OPCodes_CMP(&m_Y, ImmediateAddressing());
}

void HuC6280::OPCode0xC1()
{
    // OK
    // CMP (ZZ,X)
    OPCodes_CMP(&m_A, m_memory->Read(ZeroPageIndexedIndirectAddressing()));
}

void HuC6280::OPCode0xC2()
{
    // OK
    // CLY
    ClearFlag(FLAG_MEMORY);
    m_Y.SetValue(0x00);
}

void HuC6280::OPCode0xC3()
{
    // UNOFFICIAL
    // DCP $(nn,X)
    UnofficialOPCode();
}

void HuC6280::OPCode0xC4()
{
    // OK
    // CPY ZZ
    OPCodes_CMP(&m_Y, m_memory->Read(ZeroPageAddressing()));
}

void HuC6280::OPCode0xC5()
{
    // OK
    // CMP ZZ
    OPCodes_CMP(&m_A, m_memory->Read(ZeroPageAddressing()));
}

void HuC6280::OPCode0xC6()
{
    // OK
    // DEC ZZ
    OPCodes_DEC_Mem(ZeroPageAddressing());
}

void HuC6280::OPCode0xC7()
{
    // UNOFFICIAL
    // DCP $n
    UnofficialOPCode();
}

void HuC6280::OPCode0xC8()
{
    // OK
    // INY
    OPCodes_INC_Reg(&m_Y);
}

void HuC6280::OPCode0xC9()
{
    // OK
    // CMP #nn
    OPCodes_CMP(&m_A, ImmediateAddressing());
}

void HuC6280::OPCode0xCA()
{
    // OK
    // DEX
    OPCodes_DEC_Reg(&m_X);
}

void HuC6280::OPCode0xCB()
{
    // UNOFFICIAL
    // CMP #$n
    UnofficialOPCode();
}

void HuC6280::OPCode0xCC()
{
    // OK
    // CPY hhll
    OPCodes_CMP(&m_Y, m_memory->Read(AbsoluteAddressing()));
}

void HuC6280::OPCode0xCD()
{
    // OK
    // CMP hhll
    OPCodes_CMP(&m_A, m_memory->Read(AbsoluteAddressing()));
}

void HuC6280::OPCode0xCE()
{
    // OK
    // DEC hhll
    OPCodes_DEC_Mem(AbsoluteAddressing());
}

void HuC6280::OPCode0xCF()
{
    // OK
    // BBS4 ZZ,hhll
    OPcodes_Branch(IsSetBit(4, m_memory->Read(ZeroPageAddressing())));
}

void HuC6280::OPCode0xD0()
{
    // OK
    // BNE hhll
    OPcodes_Branch(!IsSetFlag(FLAG_ZERO));
}

void HuC6280::OPCode0xD1()
{
    // OK
    // CMP (ZZ),Y
    OPCodes_CMP(&m_A, m_memory->Read(ZeroPageIndirectIndexedAddressing()));
}

void HuC6280::OPCode0xD2()
{
    // OK
    // CMP (ZZ)
    OPCodes_CMP(&m_A, m_memory->Read(ZeroPageIndirectAddressing()));
}

void HuC6280::OPCode0xD3()
{
    // UNOFFICIAL
    // DCP ($n),Y
    UnofficialOPCode();
}

void HuC6280::OPCode0xD4()
{
    // UNOFFICIAL
    // NOP $n,X
    UnofficialOPCode();
}

void HuC6280::OPCode0xD5()
{
    // OK
    // CMP ZZ,X
    OPCodes_CMP(&m_A, m_memory->Read(ZeroPageAddressing(&m_X)));
}

void HuC6280::OPCode0xD6()
{
    // OK
    // DEC ZZ,X
    OPCodes_DEC_Mem(ZeroPageAddressing(&m_X));
}

void HuC6280::OPCode0xD7()
{
    // UNOFFICIAL
    // DCP $n,X
    UnofficialOPCode();
}

void HuC6280::OPCode0xD8()
{
    // OK
    // CLD
    OPCodes_ClearFlag(FLAG_DECIMAL);
}

void HuC6280::OPCode0xD9()
{
    // OK
    // CMP $nn,Y
    OPCodes_CMP(&m_A, m_memory->Read(AbsoluteAddressing(&m_Y)));
}

void HuC6280::OPCode0xDA()
{
    // OK
    // PHX
    ClearFlag(FLAG_MEMORY);
    StackPush8(m_X.GetValue());
}

void HuC6280::OPCode0xDB()
{
    // UNOFFICIAL
    // DCP $nn,Y
    UnofficialOPCode();
}

void HuC6280::OPCode0xDC()
{
    // UNOFFICIAL
    // NOP $nn,X
    UnofficialOPCode();
}

void HuC6280::OPCode0xDD()
{
    // OK
    // CMP hhll,X
    OPCodes_CMP(&m_A, m_memory->Read(AbsoluteAddressing(&m_X)));
}

void HuC6280::OPCode0xDE()
{
    // OK
    // DEC hhll,X
    OPCodes_DEC_Mem(AbsoluteAddressing(&m_X));
}

void HuC6280::OPCode0xDF()
{
    // OK
    // BBS5 ZZ,hhll
    OPcodes_Branch(IsSetBit(5, m_memory->Read(ZeroPageAddressing())));
}

void HuC6280::OPCode0xE0()
{
    // OK
    // CPX #nn
    OPCodes_CMP(&m_X, ImmediateAddressing());
}

void HuC6280::OPCode0xE1()
{
    // SBC $(nn,X)
    OPCodes_SBC(m_memory->Read(ZeroPageIndexedIndirectAddressing()));
}

void HuC6280::OPCode0xE2()
{
    // UNOFFICIAL
    // NOP #$n
    UnofficialOPCode();
}

void HuC6280::OPCode0xE3()
{
    // UNOFFICIAL
    // ISC $(nn,X)
    UnofficialOPCode();
}

void HuC6280::OPCode0xE4()
{
    // OK
    // CPX ZZ
    OPCodes_CMP(&m_X, m_memory->Read(ZeroPageAddressing()));
}

void HuC6280::OPCode0xE5()
{
    // SBC $n
    OPCodes_SBC(m_memory->Read(ZeroPageAddressing()));
}

void HuC6280::OPCode0xE6()
{
    // OK
    // INC ZZ
    OPCodes_INC_Mem(ZeroPageAddressing());
}

void HuC6280::OPCode0xE7()
{
    // UNOFFICIAL
    // ISC $n
    UnofficialOPCode();
}

void HuC6280::OPCode0xE8()
{
    // OK
    // INX
    OPCodes_INC_Reg(&m_X);
}

void HuC6280::OPCode0xE9()
{
    // SBC #$n
    OPCodes_SBC(ImmediateAddressing());
}

void HuC6280::OPCode0xEA()
{
    // NOP
}

void HuC6280::OPCode0xEB()
{
    // UNOFFICIAL
    // SBC #$n
    UnofficialOPCode();
}

void HuC6280::OPCode0xEC()
{
    // OK
    // CPX hhll
    OPCodes_CMP(&m_X, m_memory->Read(AbsoluteAddressing()));
}

void HuC6280::OPCode0xED()
{
    // SBC $nn
    OPCodes_SBC(m_memory->Read(AbsoluteAddressing()));
}

void HuC6280::OPCode0xEE()
{
    // OK
    // INC hhll
    OPCodes_INC_Mem(AbsoluteAddressing());
}

void HuC6280::OPCode0xEF()
{
    // OK
    // BBS6 ZZ,hhll
    OPcodes_Branch(IsSetBit(6, m_memory->Read(ZeroPageAddressing())));
}

void HuC6280::OPCode0xF0()
{
    // OK
    // BEQ hhll
    OPcodes_Branch(IsSetFlag(FLAG_ZERO));
}

void HuC6280::OPCode0xF1()
{
    // SBC ($n),Y
    OPCodes_SBC(m_memory->Read(ZeroPageIndirectIndexedAddressing()));
}

void HuC6280::OPCode0xF2()
{
    // UNOFFICIAL
    // KILL
    UnofficialOPCode();
}

void HuC6280::OPCode0xF3()
{
    // UNOFFICIAL
    // ISC ($n),Y
    UnofficialOPCode();
}

void HuC6280::OPCode0xF4()
{
    // UNOFFICIAL
    // NOP $n,X
    UnofficialOPCode();
}

void HuC6280::OPCode0xF5()
{
    // SBC $n,X
    OPCodes_SBC(m_memory->Read(ZeroPageAddressing(&m_X)));
}

void HuC6280::OPCode0xF6()
{
    // OK
    // INC ZZ,X
    OPCodes_INC_Mem(ZeroPageAddressing(&m_X));
}

void HuC6280::OPCode0xF7()
{
    // UNOFFICIAL
    // ISC $n,X
    UnofficialOPCode();
}

void HuC6280::OPCode0xF8()
{
    // SED
    OPCodes_SetFlag(FLAG_DECIMAL);
}

void HuC6280::OPCode0xF9()
{
    // SBC $nn,Y
    OPCodes_SBC(m_memory->Read(AbsoluteAddressing(&m_Y)));
}

void HuC6280::OPCode0xFA()
{
    // OK
    // PLX
    ClearFlag(FLAG_MEMORY);
    u8 result = StackPop8();
    m_X.SetValue(result);
    SetZeroFlagFromResult(result);
    SetNegativeFlagFromResult(result);
}

void HuC6280::OPCode0xFB()
{
    // UNOFFICIAL
    // ISC $nn,Y
    UnofficialOPCode();
}

void HuC6280::OPCode0xFC()
{
    // UNOFFICIAL
    // NOP $nn,X
    UnofficialOPCode();
}

void HuC6280::OPCode0xFD()
{
    // SBC $nn,X
    OPCodes_SBC(m_memory->Read(AbsoluteAddressing(&m_X)));
}

void HuC6280::OPCode0xFE()
{
    // OK
    // INC hhll,X
    OPCodes_INC_Mem(AbsoluteAddressing(&m_X));
}

void HuC6280::OPCode0xFF()
{
    // OK
    // BBS7 ZZ,hhll
    OPcodes_Branch(IsSetBit(7, m_memory->Read(ZeroPageAddressing())));
}