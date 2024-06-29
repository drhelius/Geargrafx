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
    // BRK
    OPCodes_BRK();
}

void HuC6280::OPCode0x01()
{
    // ORA $(nn,X)
    OPCodes_ORA(m_memory->Read(IndexedIndirectAddressing()));
}

void HuC6280::OPCode0x02()
{
    // UNOFFICIAL
    // KILL
    UnofficialOPCode();
}

void HuC6280::OPCode0x03()
{
    // UNOFFICIAL
    // SLO $(nn,X)
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
    // ORA $n
    OPCodes_ORA(m_memory->Read(ZeroPageAddressing()));
}

void HuC6280::OPCode0x06()
{
    // ASL $n
    OPCodes_ASL_Memory(ZeroPageAddressing());
}

void HuC6280::OPCode0x07()
{
    // UNOFFICIAL
    // SLO $n
    UnofficialOPCode();
}

void HuC6280::OPCode0x08()
{
    // PHP
    SetFlag(FLAG_BRK);
    StackPush8(m_P.GetValue());
}

void HuC6280::OPCode0x09()
{
    // ORA #$n
    OPCodes_ORA(ImmediateAddressing());
}

void HuC6280::OPCode0x0A()
{
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
    // ORA $nn
    OPCodes_ORA(m_memory->Read(AbsoluteAddressing()));
}

void HuC6280::OPCode0x0E()
{
    // ASL $nn
    OPCodes_ASL_Memory(AbsoluteAddressing());
}

void HuC6280::OPCode0x0F()
{
    // UNOFFICIAL
    // SLO $nn
    UnofficialOPCode();
}

void HuC6280::OPCode0x10()
{
    // BPL $s
    OPcodes_Branch(!IsSetFlag(FLAG_NEGATIVE));
}

void HuC6280::OPCode0x11()
{
    // ORA ($n),Y
    OPCodes_ORA(m_memory->Read(IndirectIndexedAddressing()));
}

void HuC6280::OPCode0x12()
{
    // UNOFFICIAL
    // KILL
    UnofficialOPCode();
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
    // ORA $n,X
    OPCodes_ORA(m_memory->Read(ZeroPageAddressing(&m_X)));
}

void HuC6280::OPCode0x16()
{
    // ASL $n,X
    OPCodes_ASL_Memory(ZeroPageAddressing(&m_X));
}

void HuC6280::OPCode0x17()
{
    // UNOFFICIAL
    // SLO $n,X
    UnofficialOPCode();
}

void HuC6280::OPCode0x18()
{
    // CLC
    OPCodes_ClearFlag(FLAG_CARRY);
}

void HuC6280::OPCode0x19()
{
    // ORA $nn,Y
    OPCodes_ORA(m_memory->Read(AbsoluteAddressing(&m_Y)));
}

void HuC6280::OPCode0x1A()
{
    // UNOFFICIAL
    // NOP
    UnofficialOPCode();
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
    // ORA $nn,X
    OPCodes_ORA(m_memory->Read(AbsoluteAddressing(&m_X)));
}

void HuC6280::OPCode0x1E()
{
    // ASL $nn,X
    OPCodes_ASL_Memory(AbsoluteAddressing(&m_X));
}

void HuC6280::OPCode0x1F()
{
    // UNOFFICIAL
    // SLO $nn,X
    UnofficialOPCode();
}

void HuC6280::OPCode0x20()
{
    // JSR $nn
    u16 target = AbsoluteAddressing();
    StackPush16(m_PC.GetValue() - 1);
    m_PC.SetValue(target);
}

void HuC6280::OPCode0x21()
{
    // AND $(nn,X)
    OPCodes_AND(m_memory->Read(IndexedIndirectAddressing()));
}

void HuC6280::OPCode0x22()
{
    // UNOFFICIAL
    // KILL
    UnofficialOPCode();
}

void HuC6280::OPCode0x23()
{
    // UNOFFICIAL
    // RLA $(nn,X)
    UnofficialOPCode();
}

void HuC6280::OPCode0x24()
{
    // BIT $n
    OPCodes_BIT(ZeroPageAddressing());
}

void HuC6280::OPCode0x25()
{
    // AND $n
    OPCodes_AND(m_memory->Read(ZeroPageAddressing()));
}

void HuC6280::OPCode0x26()
{
    // ROL $n
    OPCodes_ROL_Memory(ZeroPageAddressing());
}

void HuC6280::OPCode0x27()
{
    // UNOFFICIAL
    // RLA $n
    UnofficialOPCode();
}

void HuC6280::OPCode0x28()
{
    // PLP
    m_P.SetValue((StackPop8() & 0xCF) | (m_P.GetValue() & 0x30));
}

void HuC6280::OPCode0x29()
{
    // AND #$n
    OPCodes_AND(ImmediateAddressing());
}

void HuC6280::OPCode0x2A()
{
    // ROL
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
    // BIT $nn
    OPCodes_BIT(AbsoluteAddressing());
}

void HuC6280::OPCode0x2D()
{
    // AND $nn
    OPCodes_AND(m_memory->Read(AbsoluteAddressing()));
}

void HuC6280::OPCode0x2E()
{
    // ROL $nn
    OPCodes_ROL_Memory(AbsoluteAddressing());
}

void HuC6280::OPCode0x2F()
{
    // UNOFFICIAL
    // RLA $nn
    UnofficialOPCode();
}

void HuC6280::OPCode0x30()
{
    // BMI $s
    OPcodes_Branch(IsSetFlag(FLAG_NEGATIVE));
}

void HuC6280::OPCode0x31()
{
    // AND ($n),Y
    OPCodes_AND(m_memory->Read(IndirectIndexedAddressing()));
}

void HuC6280::OPCode0x32()
{
    // UNOFFICIAL
    // KILL
    UnofficialOPCode();
}

void HuC6280::OPCode0x33()
{
    // UNOFFICIAL
    // RLA ($n),Y
    UnofficialOPCode();
}

void HuC6280::OPCode0x34()
{
    // UNOFFICIAL
    // NOP $n,X
    UnofficialOPCode();
}

void HuC6280::OPCode0x35()
{
    // AND $n,X
    OPCodes_AND(m_memory->Read(ZeroPageAddressing(&m_X)));
}

void HuC6280::OPCode0x36()
{
    // ROL $n,X
    OPCodes_ROL_Memory(ZeroPageAddressing(&m_X));
}

void HuC6280::OPCode0x37()
{
    // UNOFFICIAL
    // RLA $n,X
    UnofficialOPCode();
}

void HuC6280::OPCode0x38()
{
    // SEC
    OPCodes_SetFlag(FLAG_CARRY);
}

void HuC6280::OPCode0x39()
{
    // AND $nn,Y
    OPCodes_AND(m_memory->Read(AbsoluteAddressing(&m_Y)));
}

void HuC6280::OPCode0x3A()
{
    // UNOFFICIAL
    // NOP
    UnofficialOPCode();
}

void HuC6280::OPCode0x3B()
{
    // UNOFFICIAL
    // RLA $nn,Y
    UnofficialOPCode();
}

void HuC6280::OPCode0x3C()
{
    // UNOFFICIAL
    // NOP $nn,X
    UnofficialOPCode();
}

void HuC6280::OPCode0x3D()
{
    // AND $nn,X
    OPCodes_AND(m_memory->Read(AbsoluteAddressing(&m_X)));
}

void HuC6280::OPCode0x3E()
{
    // ROL $nn,X
    OPCodes_ROL_Memory(AbsoluteAddressing(&m_X));
}

void HuC6280::OPCode0x3F()
{
    // UNOFFICIAL
    // RLA $nn,X
    UnofficialOPCode();
}

void HuC6280::OPCode0x40()
{
    // RTI
    m_P.SetValue((StackPop8() & 0xCF) | (m_P.GetValue() & 0x30));
    m_PC.SetValue(StackPop16());
}

void HuC6280::OPCode0x41()
{
    // EOR $(nn,X)
    OPCodes_EOR(m_memory->Read(IndexedIndirectAddressing()));
}

void HuC6280::OPCode0x42()
{
    // UNOFFICIAL
    // KILL
    UnofficialOPCode();
}

void HuC6280::OPCode0x43()
{
    // UNOFFICIAL
    // SRE $(nn,X);
    UnofficialOPCode();
}

void HuC6280::OPCode0x44()
{
    // UNOFFICIAL
    // NOP $n
    UnofficialOPCode();
}

void HuC6280::OPCode0x45()
{
    // EOR $n
    OPCodes_EOR(m_memory->Read(ZeroPageAddressing()));
}

void HuC6280::OPCode0x46()
{
    // LSR $n
    OPCodes_LSR_Memory(ZeroPageAddressing());
}

void HuC6280::OPCode0x47()
{
    // UNOFFICIAL
    // SRE $n
    UnofficialOPCode();
}

void HuC6280::OPCode0x48()
{
    // PHA
    StackPush8(m_A.GetValue());
}

void HuC6280::OPCode0x49()
{
    // EOR #$n
    OPCodes_EOR(ImmediateAddressing());
}

void HuC6280::OPCode0x4A()
{
    // LSR
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
    // JMP $nn
    m_PC.SetValue(AbsoluteAddressing());
}

void HuC6280::OPCode0x4D()
{
    // EOR $nn
    OPCodes_EOR(m_memory->Read(AbsoluteAddressing()));
}

void HuC6280::OPCode0x4E()
{
    // LSR $nn
    OPCodes_LSR_Memory(AbsoluteAddressing());
}

void HuC6280::OPCode0x4F()
{
    // UNOFFICIAL
    // SRE $nn
    UnofficialOPCode();
}

void HuC6280::OPCode0x50()
{
    // BVC $s
    OPcodes_Branch(!IsSetFlag(FLAG_OVERFLOW));
}

void HuC6280::OPCode0x51()
{
    // EOR ($n),Y
    OPCodes_EOR(m_memory->Read(IndirectIndexedAddressing()));
}

void HuC6280::OPCode0x52()
{
    // UNOFFICIAL
    // KILL
    UnofficialOPCode();
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
    // EOR $n,X
    OPCodes_EOR(m_memory->Read(ZeroPageAddressing(&m_X)));
}

void HuC6280::OPCode0x56()
{
    // LSR $n,X
    OPCodes_LSR_Memory(ZeroPageAddressing(&m_X));
}

void HuC6280::OPCode0x57()
{
    // UNOFFICIAL
    // SRE $n,X
    UnofficialOPCode();
}

void HuC6280::OPCode0x58()
{
    // CLI
    OPCodes_ClearFlag(FLAG_IRQ);
}

void HuC6280::OPCode0x59()
{
    // EOR $nn,Y
    OPCodes_EOR(m_memory->Read(AbsoluteAddressing(&m_Y)));
}

void HuC6280::OPCode0x5A()
{
    // UNOFFICIAL
    // NOP
    UnofficialOPCode();
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
    // EOR $nn,X
    OPCodes_EOR(m_memory->Read(AbsoluteAddressing(&m_X)));
}

void HuC6280::OPCode0x5E()
{
    // LSR $nn,X
    OPCodes_LSR_Memory(AbsoluteAddressing(&m_X));
}

void HuC6280::OPCode0x5F()
{
    // UNOFFICIAL
    // SRE $nn,X
    UnofficialOPCode();
}

void HuC6280::OPCode0x60()
{
    // RTS
    m_PC.SetValue(StackPop16() + 1);
}

void HuC6280::OPCode0x61()
{
    // ADC $(nn,X)
    OPCodes_ADC(m_memory->Read(IndexedIndirectAddressing()));
}

void HuC6280::OPCode0x62()
{
    // UNOFFICIAL
    // KILL
    UnofficialOPCode();
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
    // ADC $n
    OPCodes_ADC(m_memory->Read(ZeroPageAddressing()));
}

void HuC6280::OPCode0x66()
{
    // ROR $n
    OPCodes_ROR_Memory(ZeroPageAddressing());
}

void HuC6280::OPCode0x67()
{
    // UNOFFICIAL
    // RRA $n
    UnofficialOPCode();
}

void HuC6280::OPCode0x68()
{
    // PLA
    u8 result = StackPop8();
    m_A.SetValue(result);
    SetZeroFlagFromResult(result);
    SetNegativeFlagFromResult(result);
}

void HuC6280::OPCode0x69()
{
    // ADC #$n
    OPCodes_ADC(ImmediateAddressing());
}

void HuC6280::OPCode0x6A()
{
    // ROR
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
    // JMP ($nn)
    m_PC.SetValue(IndirectAddressing());
}

void HuC6280::OPCode0x6D()
{
    // ADC $nn
    OPCodes_ADC(m_memory->Read(AbsoluteAddressing()));
}

void HuC6280::OPCode0x6E()
{
    // ROR $nn
    OPCodes_ROR_Memory(AbsoluteAddressing());
}

void HuC6280::OPCode0x6F()
{
    // UNOFFICIAL
    // RRA $nn
    UnofficialOPCode();
}

void HuC6280::OPCode0x70()
{
    // BVS $s
    OPcodes_Branch(IsSetFlag(FLAG_OVERFLOW));
}

void HuC6280::OPCode0x71()
{
    // ADC ($n),Y
    OPCodes_ADC(m_memory->Read(IndirectIndexedAddressing()));
}

void HuC6280::OPCode0x72()
{
    // UNOFFICIAL
    // KILL
    UnofficialOPCode();
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
    // ADC $n,X
    OPCodes_ADC(m_memory->Read(ZeroPageAddressing(&m_X)));
}

void HuC6280::OPCode0x76()
{
    // ROR $n,X
    OPCodes_ROR_Memory(ZeroPageAddressing(&m_X));
}

void HuC6280::OPCode0x77()
{
    // UNOFFICIAL
    // RRA $n,X
    UnofficialOPCode();
}

void HuC6280::OPCode0x78()
{
    // SEI
    OPCodes_SetFlag(FLAG_IRQ);
}

void HuC6280::OPCode0x79()
{
    // ADC $nn,Y
    OPCodes_ADC(m_memory->Read(AbsoluteAddressing(&m_Y)));
}

void HuC6280::OPCode0x7A()
{
    // UNOFFICIAL
    // NOP
    UnofficialOPCode();
}

void HuC6280::OPCode0x7B()
{
    // UNOFFICIAL
    // RRA $nn,Y
    UnofficialOPCode();
}

void HuC6280::OPCode0x7C()
{
    // UNOFFICIAL
    // NOP $nn,X
    UnofficialOPCode();
}

void HuC6280::OPCode0x7D()
{
    // ADC $nn,X
    OPCodes_ADC(m_memory->Read(AbsoluteAddressing(&m_X)));
}

void HuC6280::OPCode0x7E()
{
    // ROR $nn,X
    OPCodes_ROR_Memory(AbsoluteAddressing(&m_X));
}

void HuC6280::OPCode0x7F()
{
    // UNOFFICIAL
    // RRA $nn,X
    UnofficialOPCode();
}

void HuC6280::OPCode0x80()
{
    // UNOFFICIAL
    // NOP #$n
    UnofficialOPCode();
}

void HuC6280::OPCode0x81()
{
    // STA $(nn,X)
    OPCodes_Store(&m_A, IndexedIndirectAddressing());
}

void HuC6280::OPCode0x82()
{
    // UNOFFICIAL
    // NOP #$n
    UnofficialOPCode();
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
    // DEY
    OPCodes_DEC_Reg(&m_Y);
}

void HuC6280::OPCode0x89()
{
    // UNOFFICIAL
    // NOP #$n
    UnofficialOPCode();
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
    // UNOFFICIAL
    // SAX $nn
    UnofficialOPCode();
}

void HuC6280::OPCode0x90()
{
    // BCC $s
    OPcodes_Branch(!IsSetFlag(FLAG_CARRY));
}

void HuC6280::OPCode0x91()
{
    // STA ($n),Y
    OPCodes_Store(&m_A, IndirectIndexedAddressing());
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
    // UNOFFICIAL
    // AHX $nn,Y
    UnofficialOPCode();
}

void HuC6280::OPCode0xA0()
{
    // LDY #$n
    OPCodes_LD(&m_Y, ImmediateAddressing());
}

void HuC6280::OPCode0xA1()
{
    // LDA $(nn,X)
    OPCodes_LD(&m_A, m_memory->Read(IndexedIndirectAddressing()));
}

void HuC6280::OPCode0xA2()
{
    // LDX #$n
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
    // LDY $n
    OPCodes_LD(&m_Y, m_memory->Read(ZeroPageAddressing()));
}

void HuC6280::OPCode0xA5()
{
    // LDA $n
    OPCodes_LD(&m_A, m_memory->Read(ZeroPageAddressing()));
}

void HuC6280::OPCode0xA6()
{
    // LDX $n
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
    // LDA #$n
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
    // LDY $nn
    OPCodes_LD(&m_Y, m_memory->Read(AbsoluteAddressing()));
}

void HuC6280::OPCode0xAD()
{
    // LDA $nn
    OPCodes_LD(&m_A, m_memory->Read(AbsoluteAddressing()));
}

void HuC6280::OPCode0xAE()
{
    // LDX $nn
    OPCodes_LD(&m_X, m_memory->Read(AbsoluteAddressing()));
}

void HuC6280::OPCode0xAF()
{
    // UNOFFICIAL
    // LAX $nn
    UnofficialOPCode();
}

void HuC6280::OPCode0xB0()
{
    // BCS $s
    OPcodes_Branch(IsSetFlag(FLAG_CARRY));
}

void HuC6280::OPCode0xB1()
{
    // LDA ($n),Y
    OPCodes_LD(&m_A, m_memory->Read(IndirectIndexedAddressing()));
}

void HuC6280::OPCode0xB2()
{
    // UNOFFICIAL
    // KILL
    UnofficialOPCode();
}

void HuC6280::OPCode0xB3()
{
    // UNOFFICIAL
    // LAX ($n),Y
    UnofficialOPCode();
}

void HuC6280::OPCode0xB4()
{
    // LDY $n,X
    OPCodes_LD(&m_Y, m_memory->Read(ZeroPageAddressing(&m_X)));
}

void HuC6280::OPCode0xB5()
{
    // LDA $n,X
    OPCodes_LD(&m_A, m_memory->Read(ZeroPageAddressing(&m_X)));
}

void HuC6280::OPCode0xB6()
{
    // LDX $n,Y
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
    // CLV
    OPCodes_ClearFlag(FLAG_OVERFLOW);
}

void HuC6280::OPCode0xB9()
{
    // LDA $nn,Y
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
    // LDY $nn,X
    OPCodes_LD(&m_Y, m_memory->Read(AbsoluteAddressing(&m_X)));
}

void HuC6280::OPCode0xBD()
{
    // LDA $nn,X
    OPCodes_LD(&m_A, m_memory->Read(AbsoluteAddressing(&m_X)));
}

void HuC6280::OPCode0xBE()
{
    // LDX $nn,Y
    OPCodes_LD(&m_X, m_memory->Read(AbsoluteAddressing(&m_Y)));
}

void HuC6280::OPCode0xBF()
{
    // UNOFFICIAL
    // LAX $nn,Y
    UnofficialOPCode();
}

void HuC6280::OPCode0xC0()
{
    // CPY #$n
    OPCodes_CMP(&m_Y, ImmediateAddressing());
}

void HuC6280::OPCode0xC1()
{
    // CMP $(nn,X)
    OPCodes_CMP(&m_A, m_memory->Read(IndexedIndirectAddressing()));
}

void HuC6280::OPCode0xC2()
{
    // UNOFFICIAL
    // NOP #$n
    UnofficialOPCode();
}

void HuC6280::OPCode0xC3()
{
    // UNOFFICIAL
    // DCP $(nn,X)
    UnofficialOPCode();
}

void HuC6280::OPCode0xC4()
{
    // CPY $n
    OPCodes_CMP(&m_Y, m_memory->Read(ZeroPageAddressing()));
}

void HuC6280::OPCode0xC5()
{
    // CMP $n
    OPCodes_CMP(&m_A, m_memory->Read(ZeroPageAddressing()));
}

void HuC6280::OPCode0xC6()
{
    // DEC $n
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
    // INY
    OPCodes_INC_Reg(&m_Y);
}

void HuC6280::OPCode0xC9()
{
    // CMP #$n
    OPCodes_CMP(&m_A, ImmediateAddressing());
}

void HuC6280::OPCode0xCA()
{
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
    // CPY $nn
    OPCodes_CMP(&m_Y, m_memory->Read(AbsoluteAddressing()));
}

void HuC6280::OPCode0xCD()
{
    // CMP $nn
    OPCodes_CMP(&m_A, m_memory->Read(AbsoluteAddressing()));
}

void HuC6280::OPCode0xCE()
{
    // DEC $nn
    OPCodes_DEC_Mem(AbsoluteAddressing());
}

void HuC6280::OPCode0xCF()
{
    // UNOFFICIAL
    // DCP $nn
    UnofficialOPCode();
}

void HuC6280::OPCode0xD0()
{
    // BNE $s
    OPcodes_Branch(!IsSetFlag(FLAG_ZERO));
}

void HuC6280::OPCode0xD1()
{
    // CMP ($n),Y
    OPCodes_CMP(&m_A, m_memory->Read(IndirectIndexedAddressing()));
}

void HuC6280::OPCode0xD2()
{
    // UNOFFICIAL
    // KILL
    UnofficialOPCode();
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
    // CMP $n,X
    OPCodes_CMP(&m_A, m_memory->Read(ZeroPageAddressing(&m_X)));
}

void HuC6280::OPCode0xD6()
{
    // DEC $n,X
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
    // CLD
    OPCodes_ClearFlag(FLAG_DECIMAL);
}

void HuC6280::OPCode0xD9()
{
    // CMP $nn,Y
    OPCodes_CMP(&m_A, m_memory->Read(AbsoluteAddressing(&m_Y)));
}

void HuC6280::OPCode0xDA()
{
    // UNOFFICIAL
    // NOP
    UnofficialOPCode();
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
    // CMP $nn,X
    OPCodes_CMP(&m_A, m_memory->Read(AbsoluteAddressing(&m_X)));
}

void HuC6280::OPCode0xDE()
{
    // DEC $nn,X
    OPCodes_DEC_Mem(AbsoluteAddressing(&m_X));
}

void HuC6280::OPCode0xDF()
{
    // UNOFFICIAL
    // DCP $nn,X
    UnofficialOPCode();
}

void HuC6280::OPCode0xE0()
{
    // CPX #$n
    OPCodes_CMP(&m_X, ImmediateAddressing());
}

void HuC6280::OPCode0xE1()
{
    // SBC $(nn,X)
    OPCodes_SBC(m_memory->Read(IndexedIndirectAddressing()));
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
    // CPX $n
    OPCodes_CMP(&m_X, m_memory->Read(ZeroPageAddressing()));
}

void HuC6280::OPCode0xE5()
{
    // SBC $n
    OPCodes_SBC(m_memory->Read(ZeroPageAddressing()));
}

void HuC6280::OPCode0xE6()
{
    // INC $n
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
    // CPX $nn
    OPCodes_CMP(&m_X, m_memory->Read(AbsoluteAddressing()));
}

void HuC6280::OPCode0xED()
{
    // SBC $nn
    OPCodes_SBC(m_memory->Read(AbsoluteAddressing()));
}

void HuC6280::OPCode0xEE()
{
    // INC $nn
    OPCodes_INC_Mem(AbsoluteAddressing());
}

void HuC6280::OPCode0xEF()
{
    // UNOFFICIAL
    // ISC $nn
    UnofficialOPCode();
}

void HuC6280::OPCode0xF0()
{
    // BEQ $s
    OPcodes_Branch(IsSetFlag(FLAG_ZERO));
}

void HuC6280::OPCode0xF1()
{
    // SBC ($n),Y
    OPCodes_SBC(m_memory->Read(IndirectIndexedAddressing()));
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
    // INC $n,X
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
    // UNOFFICIAL
    // NOP
    UnofficialOPCode();
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
    // INC $nn,X
    OPCodes_INC_Mem(AbsoluteAddressing(&m_X));
}

void HuC6280::OPCode0xFF()
{
    // UNOFFICIAL
    // ISC $nn,X
    UnofficialOPCode();
}