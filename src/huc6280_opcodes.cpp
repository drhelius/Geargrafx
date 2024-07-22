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
    // ORA (ZP,X)
    OPCodes_ORA(m_memory->Read(ZeroPageIndexedIndirectAddressing()));
}

void HuC6280::OPCode0x02()
{
    // SXY
    OPCodes_Swap(&m_X, &m_Y);
}

void HuC6280::OPCode0x03()
{
    // ST0 #nn
    OPCodes_STN(0, ImmediateAddressing());
}

void HuC6280::OPCode0x04()
{
    // TSB ZP
    OPCodes_TSB(ZeroPageAddressing());
}

void HuC6280::OPCode0x05()
{
    // ORA ZP
    OPCodes_ORA(m_memory->Read(ZeroPageAddressing()));
}

void HuC6280::OPCode0x06()
{
    // ASL ZP
    OPCodes_ASL_Memory(ZeroPageAddressing());
}

void HuC6280::OPCode0x07()
{
    // RMB0 ZP
    OPCodes_RMB(0, ZeroPageAddressing());
}

void HuC6280::OPCode0x08()
{
    // PHP
    ClearFlag(FLAG_TRANSFER);
    SetFlag(FLAG_BREAK);
    StackPush8(m_P.GetValue());
}

void HuC6280::OPCode0x09()
{
    // ORA #nn
    OPCodes_ORA(ImmediateAddressing());
}

void HuC6280::OPCode0x0A()
{
    // ASL A
    OPCodes_ASL_Accumulator();
}

void HuC6280::OPCode0x0B()
{
    UnofficialOPCode();
}

void HuC6280::OPCode0x0C()
{
    // TSB hhll
    OPCodes_TSB(AbsoluteAddressing());
}

void HuC6280::OPCode0x0D()
{
    // ORA hhll
    OPCodes_ORA(m_memory->Read(AbsoluteAddressing()));
}

void HuC6280::OPCode0x0E()
{
    // ASL hhll
    OPCodes_ASL_Memory(AbsoluteAddressing());
}

void HuC6280::OPCode0x0F()
{
    // BBR0 ZP,rr
    OPcodes_Branch(!IsSetBit(0, m_memory->Read(ZeroPageAddressing())));
}

void HuC6280::OPCode0x10()
{
    // BPL rr
    OPcodes_Branch(!IsSetFlag(FLAG_NEGATIVE));
}

void HuC6280::OPCode0x11()
{
    // ORA (ZP),Y
    OPCodes_ORA(m_memory->Read(ZeroPageIndirectIndexedAddressing()));
}

void HuC6280::OPCode0x12()
{
    // ORA (ZP)
    OPCodes_ORA(m_memory->Read(ZeroPageIndirectAddressing()));
}

void HuC6280::OPCode0x13()
{
    // ST1 #nn
    OPCodes_STN(2, ImmediateAddressing());
}

void HuC6280::OPCode0x14()
{
    // TRB ZP
    OPCodes_TRB(ZeroPageAddressing());
}

void HuC6280::OPCode0x15()
{
    // ORA ZP,X
    OPCodes_ORA(m_memory->Read(ZeroPageAddressing(&m_X)));
}

void HuC6280::OPCode0x16()
{
    // ASL ZP,X
    OPCodes_ASL_Memory(ZeroPageAddressing(&m_X));
}

void HuC6280::OPCode0x17()
{
    // RMB1 ZP
    OPCodes_RMB(1, ZeroPageAddressing());
}

void HuC6280::OPCode0x18()
{
    // CLC
    OPCodes_ClearFlag(FLAG_CARRY);
}

void HuC6280::OPCode0x19()
{
    // ORA hhll,Y
    OPCodes_ORA(m_memory->Read(AbsoluteAddressing(&m_Y)));
}

void HuC6280::OPCode0x1A()
{
    // INC A
    OPCodes_INC_Reg(&m_A);
}

void HuC6280::OPCode0x1B()
{
    UnofficialOPCode();
}

void HuC6280::OPCode0x1C()
{
    // TRB hhll
    OPCodes_TRB(AbsoluteAddressing());
}

void HuC6280::OPCode0x1D()
{
    // ORA hhll,X
    OPCodes_ORA(m_memory->Read(AbsoluteAddressing(&m_X)));
}

void HuC6280::OPCode0x1E()
{
    // ASL hhll,X
    OPCodes_ASL_Memory(AbsoluteAddressing(&m_X));
}

void HuC6280::OPCode0x1F()
{
    // BBR1 ZP,rr
    OPcodes_Branch(!IsSetBit(1, m_memory->Read(ZeroPageAddressing())));
}

void HuC6280::OPCode0x20()
{
    // JSR $nn
    ClearFlag(FLAG_TRANSFER);
    u16 target = AbsoluteAddressing();
    StackPush16(m_PC.GetValue() - 1);
    m_PC.SetValue(target);
}

void HuC6280::OPCode0x21()
{
    // AND (ZP,X)
    OPCodes_AND(m_memory->Read(ZeroPageIndexedIndirectAddressing()));
}

void HuC6280::OPCode0x22()
{
    // SAX
    OPCodes_Swap(&m_A, &m_X);
}

void HuC6280::OPCode0x23()
{
    // ST2 #nn
    OPCodes_STN(3, ImmediateAddressing());
}

void HuC6280::OPCode0x24()
{
    // BIT ZP
    OPCodes_BIT(ZeroPageAddressing());
}

void HuC6280::OPCode0x25()
{
    // AND ZP
    OPCodes_AND(m_memory->Read(ZeroPageAddressing()));
}

void HuC6280::OPCode0x26()
{
    // ROL ZP
    OPCodes_ROL_Memory(ZeroPageAddressing());
}

void HuC6280::OPCode0x27()
{
    // RMB2 ZP
    OPCodes_RMB(2, ZeroPageAddressing());
}

void HuC6280::OPCode0x28()
{
    // PLP
    m_P.SetValue(StackPop8());
}

void HuC6280::OPCode0x29()
{
    // AND #nn
    OPCodes_AND(ImmediateAddressing());
}

void HuC6280::OPCode0x2A()
{
    // ROL A
    OPCodes_ROL_Accumulator();
}

void HuC6280::OPCode0x2B()
{
    UnofficialOPCode();
}

void HuC6280::OPCode0x2C()
{
    // BIT hhll
    OPCodes_BIT(AbsoluteAddressing());
}

void HuC6280::OPCode0x2D()
{
    // AND hhll
    OPCodes_AND(m_memory->Read(AbsoluteAddressing()));
}

void HuC6280::OPCode0x2E()
{
    // ROL hhll
    OPCodes_ROL_Memory(AbsoluteAddressing());
}

void HuC6280::OPCode0x2F()
{
    // BBR2 ZP,rr
    OPcodes_Branch(!IsSetBit(2, m_memory->Read(ZeroPageAddressing())));
}

void HuC6280::OPCode0x30()
{
    // BMI rr
    OPcodes_Branch(IsSetFlag(FLAG_NEGATIVE));
}

void HuC6280::OPCode0x31()
{
    // AND (ZP),Y
    OPCodes_AND(m_memory->Read(ZeroPageIndirectIndexedAddressing()));
}

void HuC6280::OPCode0x32()
{
    // AND (ZP)
    OPCodes_AND(m_memory->Read(ZeroPageIndirectAddressing()));
}

void HuC6280::OPCode0x33()
{
    UnofficialOPCode();
}

void HuC6280::OPCode0x34()
{
    // BIT ZP,X
    OPCodes_BIT(ZeroPageAddressing(&m_X));
}

void HuC6280::OPCode0x35()
{
    // AND ZP,X
    OPCodes_AND(m_memory->Read(ZeroPageAddressing(&m_X)));
}

void HuC6280::OPCode0x36()
{
    // ROL ZP,X
    OPCodes_ROL_Memory(ZeroPageAddressing(&m_X));
}

void HuC6280::OPCode0x37()
{
    // RMB3 ZP
    OPCodes_RMB(3, ZeroPageAddressing());
}

void HuC6280::OPCode0x38()
{
    // SEC
    OPCodes_SetFlag(FLAG_CARRY);
}

void HuC6280::OPCode0x39()
{
    // AND hhll,Y
    OPCodes_AND(m_memory->Read(AbsoluteAddressing(&m_Y)));
}

void HuC6280::OPCode0x3A()
{
    // DEC A
    OPCodes_DEC_Reg(&m_A);
}

void HuC6280::OPCode0x3B()
{
    UnofficialOPCode();
}

void HuC6280::OPCode0x3C()
{
    // BIT hhll,X
    OPCodes_BIT(AbsoluteAddressing(&m_X));
}

void HuC6280::OPCode0x3D()
{
    // AND hhll,X
    OPCodes_AND(m_memory->Read(AbsoluteAddressing(&m_X)));
}

void HuC6280::OPCode0x3E()
{
    // ROL hhll,X
    OPCodes_ROL_Memory(AbsoluteAddressing(&m_X));
}

void HuC6280::OPCode0x3F()
{
    // BBR3 ZP,rr
    OPcodes_Branch(!IsSetBit(3, m_memory->Read(ZeroPageAddressing())));
}

void HuC6280::OPCode0x40()
{
    // RTI
    m_P.SetValue(StackPop8());
    m_PC.SetValue(StackPop16());
}

void HuC6280::OPCode0x41()
{
    // EOR (ZP,X)
    OPCodes_EOR(m_memory->Read(ZeroPageIndexedIndirectAddressing()));
}

void HuC6280::OPCode0x42()
{
    // SAY
    OPCodes_Swap(&m_A, &m_Y);
}

void HuC6280::OPCode0x43()
{
    // TMA
    OPCodes_TMA();
}

void HuC6280::OPCode0x44()
{
    // BSR rr
    OPCodes_Subroutine();
}

void HuC6280::OPCode0x45()
{
    // EOR ZP
    OPCodes_EOR(m_memory->Read(ZeroPageAddressing()));
}

void HuC6280::OPCode0x46()
{
    // LSR ZP
    OPCodes_LSR_Memory(ZeroPageAddressing());
}

void HuC6280::OPCode0x47()
{
    // RMB4 ZP
    OPCodes_RMB(4, ZeroPageAddressing());
}

void HuC6280::OPCode0x48()
{
    // PHA
    StackPush8(m_A.GetValue());
}

void HuC6280::OPCode0x49()
{
    // EOR #nn
    OPCodes_EOR(ImmediateAddressing());
}

void HuC6280::OPCode0x4A()
{
    // LSR A
    OPCodes_LSR_Accumulator();
}

void HuC6280::OPCode0x4B()
{
    UnofficialOPCode();
}

void HuC6280::OPCode0x4C()
{
    // JMP hhll
    ClearFlag(FLAG_TRANSFER);
    m_PC.SetValue(AbsoluteAddressing());
}

void HuC6280::OPCode0x4D()
{
    // EOR hhll
    OPCodes_EOR(m_memory->Read(AbsoluteAddressing()));
}

void HuC6280::OPCode0x4E()
{
    // LSR hhll
    OPCodes_LSR_Memory(AbsoluteAddressing());
}

void HuC6280::OPCode0x4F()
{
    // BBR4 ZP,rr
    OPcodes_Branch(!IsSetBit(4, m_memory->Read(ZeroPageAddressing())));
}

void HuC6280::OPCode0x50()
{
    // BVC rr
    OPcodes_Branch(!IsSetFlag(FLAG_OVERFLOW));
}

void HuC6280::OPCode0x51()
{
    // EOR (ZP),Y
    OPCodes_EOR(m_memory->Read(ZeroPageIndirectIndexedAddressing()));
}

void HuC6280::OPCode0x52()
{
    // EOR (ZP)
    OPCodes_EOR(m_memory->Read(ZeroPageIndirectAddressing()));
}

void HuC6280::OPCode0x53()
{
    // TAM
    OPCodes_TAM();
}

void HuC6280::OPCode0x54()
{
    // CSL
    m_high_speed = false;
}

void HuC6280::OPCode0x55()
{
    // EOR ZP,X
    OPCodes_EOR(m_memory->Read(ZeroPageAddressing(&m_X)));
}

void HuC6280::OPCode0x56()
{
    // LSR ZP,X
    OPCodes_LSR_Memory(ZeroPageAddressing(&m_X));
}

void HuC6280::OPCode0x57()
{
    // RMB5 ZP
    OPCodes_RMB(5, ZeroPageAddressing());
}

void HuC6280::OPCode0x58()
{
    // CLI
    OPCodes_ClearFlag(FLAG_INTERRUPT);
}

void HuC6280::OPCode0x59()
{
    // EOR hhll,Y
    OPCodes_EOR(m_memory->Read(AbsoluteAddressing(&m_Y)));
}

void HuC6280::OPCode0x5A()
{
    // PHY
    ClearFlag(FLAG_TRANSFER);
    StackPush8(m_Y.GetValue());
}

void HuC6280::OPCode0x5B()
{
    UnofficialOPCode();
}

void HuC6280::OPCode0x5C()
{
    UnofficialOPCode();
}

void HuC6280::OPCode0x5D()
{
    // EOR hhll,X
    OPCodes_EOR(m_memory->Read(AbsoluteAddressing(&m_X)));
}

void HuC6280::OPCode0x5E()
{
    // LSR hhll,X
    OPCodes_LSR_Memory(AbsoluteAddressing(&m_X));
}

void HuC6280::OPCode0x5F()
{
    // BBR5 ZP,r
    OPcodes_Branch(!IsSetBit(5, m_memory->Read(ZeroPageAddressing())));
}

void HuC6280::OPCode0x60()
{
    // RTS
    ClearFlag(FLAG_TRANSFER);
    m_PC.SetValue(StackPop16() + 1);
}

void HuC6280::OPCode0x61()
{
    // ADC (ZP,X)
    OPCodes_ADC(m_memory->Read(ZeroPageIndexedIndirectAddressing()));
}

void HuC6280::OPCode0x62()
{
    // CLA
    ClearFlag(FLAG_TRANSFER);
    m_A.SetValue(0x00);
}

void HuC6280::OPCode0x63()
{
    UnofficialOPCode();
}

void HuC6280::OPCode0x64()
{
    // STZ ZP
    OPCodes_STZ(ZeroPageAddressing());
}

void HuC6280::OPCode0x65()
{
    // ADC ZP
    OPCodes_ADC(m_memory->Read(ZeroPageAddressing()));
}

void HuC6280::OPCode0x66()
{
    // ROR ZP
    OPCodes_ROR_Memory(ZeroPageAddressing());
}

void HuC6280::OPCode0x67()
{
    // RMB6 ZP
    OPCodes_RMB(6, ZeroPageAddressing());
}

void HuC6280::OPCode0x68()
{
    // PLA
    ClearFlag(FLAG_TRANSFER);
    u8 result = StackPop8();
    m_A.SetValue(result);
    SetZeroFlagFromResult(result);
    SetNegativeFlagFromResult(result);
}

void HuC6280::OPCode0x69()
{
    // ADC #nn
    OPCodes_ADC(ImmediateAddressing());
}

void HuC6280::OPCode0x6A()
{
    // ROR A
    OPCodes_ROR_Accumulator();
}

void HuC6280::OPCode0x6B()
{
    UnofficialOPCode();
}

void HuC6280::OPCode0x6C()
{
    // JMP (hhll)
    ClearFlag(FLAG_TRANSFER);
    m_PC.SetValue(AbsoluteIndirectAddressing());
}

void HuC6280::OPCode0x6D()
{
    // ADC (hhll)
    OPCodes_ADC(m_memory->Read(AbsoluteAddressing()));
}

void HuC6280::OPCode0x6E()
{
    // ROR hhll
    OPCodes_ROR_Memory(AbsoluteAddressing());
}

void HuC6280::OPCode0x6F()
{
    // BBR6 ZP,rr
    OPcodes_Branch(!IsSetBit(6, m_memory->Read(ZeroPageAddressing())));
}

void HuC6280::OPCode0x70()
{
    // BVS rr
    OPcodes_Branch(IsSetFlag(FLAG_OVERFLOW));
}

void HuC6280::OPCode0x71()
{
    // ADC (ZP),Y
    OPCodes_ADC(m_memory->Read(ZeroPageIndirectIndexedAddressing()));
}

void HuC6280::OPCode0x72()
{
    // ADC (ZP)
    OPCodes_ADC(m_memory->Read(ZeroPageIndirectAddressing()));
}

void HuC6280::OPCode0x73()
{
    // TII
    OPCodes_TII();
}

void HuC6280::OPCode0x74()
{
    // STZ ZP,X
    OPCodes_STZ(ZeroPageAddressing(&m_X));
}

void HuC6280::OPCode0x75()
{
    // ADC ZP,X
    OPCodes_ADC(m_memory->Read(ZeroPageAddressing(&m_X)));
}

void HuC6280::OPCode0x76()
{
    // ROR ZP,X
    OPCodes_ROR_Memory(ZeroPageAddressing(&m_X));
}

void HuC6280::OPCode0x77()
{
    // RMB7 ZP
    OPCodes_RMB(7, ZeroPageAddressing());
}

void HuC6280::OPCode0x78()
{
    // SEI
    OPCodes_SetFlag(FLAG_INTERRUPT);
}

void HuC6280::OPCode0x79()
{
    // ADC hhll,Y
    OPCodes_ADC(m_memory->Read(AbsoluteAddressing(&m_Y)));
}

void HuC6280::OPCode0x7A()
{
    // PLY
    ClearFlag(FLAG_TRANSFER);
    u8 result = StackPop8();
    m_Y.SetValue(result);
    SetZeroFlagFromResult(result);
    SetNegativeFlagFromResult(result);
}

void HuC6280::OPCode0x7B()
{
    UnofficialOPCode();
}

void HuC6280::OPCode0x7C()
{
    // JMP (hhll,X)
    ClearFlag(FLAG_TRANSFER);
    m_PC.SetValue(AbsoluteIndexedIndirectAddressing());
}

void HuC6280::OPCode0x7D()
{
    // ADC hhll,X
    OPCodes_ADC(m_memory->Read(AbsoluteAddressing(&m_X)));
}

void HuC6280::OPCode0x7E()
{
    // ROR hhll,X
    OPCodes_ROR_Memory(AbsoluteAddressing(&m_X));
}

void HuC6280::OPCode0x7F()
{
    // BBR7 ZP,rr
    OPcodes_Branch(!IsSetBit(7, m_memory->Read(ZeroPageAddressing())));
}

void HuC6280::OPCode0x80()
{
    // BRA rr
    OPcodes_Branch(true);
}

void HuC6280::OPCode0x81()
{
    // STA (ZP,X)
    OPCodes_Store(&m_A, ZeroPageIndexedIndirectAddressing());
}

void HuC6280::OPCode0x82()
{
    // CLX
    ClearFlag(FLAG_TRANSFER);
    m_X.SetValue(0x00);
}

void HuC6280::OPCode0x83()
{
    // TST #nn,ZP
    u8 nn = Fetch8();
    OPCodes_TST(nn, m_memory->Read(ZeroPageAddressing()));
}

void HuC6280::OPCode0x84()
{
    // STY ZP
    OPCodes_Store(&m_Y, ZeroPageAddressing());
}

void HuC6280::OPCode0x85()
{
    // STA ZP
    OPCodes_Store(&m_A, ZeroPageAddressing());
}

void HuC6280::OPCode0x86()
{
    // STX ZP
    OPCodes_Store(&m_X, ZeroPageAddressing());
}

void HuC6280::OPCode0x87()
{
    // SMB0 ZP
    OPCodes_SMB(0, ZeroPageAddressing());
}

void HuC6280::OPCode0x88()
{
    // DEY
    OPCodes_DEC_Reg(&m_Y);
}

void HuC6280::OPCode0x89()
{
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
    UnofficialOPCode();
}

void HuC6280::OPCode0x8C()
{
    // STY hhll
    OPCodes_Store(&m_Y, AbsoluteAddressing());
}

void HuC6280::OPCode0x8D()
{
    // STA hhll
    OPCodes_Store(&m_A, AbsoluteAddressing());
}

void HuC6280::OPCode0x8E()
{
    // STX hhll
    OPCodes_Store(&m_X, AbsoluteAddressing());
}

void HuC6280::OPCode0x8F()
{
    // BBS0 ZP,rr
    OPcodes_Branch(IsSetBit(0, m_memory->Read(ZeroPageAddressing())));
}

void HuC6280::OPCode0x90()
{
    // BCC rr
    OPcodes_Branch(!IsSetFlag(FLAG_CARRY));
}

void HuC6280::OPCode0x91()
{
    // STA (ZP),Y
    OPCodes_Store(&m_A, ZeroPageIndirectIndexedAddressing());
}

void HuC6280::OPCode0x92()
{
    // STA (ZP)
    OPCodes_Store(&m_A, ZeroPageIndirectAddressing());
}

void HuC6280::OPCode0x93()
{
    // TST #nn,hhll
    u8 nn = Fetch8();
    OPCodes_TST(nn, m_memory->Read(AbsoluteAddressing()));
}

void HuC6280::OPCode0x94()
{
    // STY ZP,X
    OPCodes_Store(&m_Y, ZeroPageAddressing(&m_X));
}

void HuC6280::OPCode0x95()
{
    // STA ZP,X
    OPCodes_Store(&m_A, ZeroPageAddressing(&m_X));
}

void HuC6280::OPCode0x96()
{
    // STX ZP,Y
    OPCodes_Store(&m_X, ZeroPageAddressing(&m_Y));
}

void HuC6280::OPCode0x97()
{
    // SMB1 ZP
    OPCodes_SMB(1, ZeroPageAddressing());
}

void HuC6280::OPCode0x98()
{
    // TYA
    OPCodes_Transfer(&m_Y, &m_A);
}

void HuC6280::OPCode0x99()
{
    // STA hhll,Y
    OPCodes_Store(&m_A, AbsoluteAddressing(&m_Y));
}

void HuC6280::OPCode0x9A()
{
    // TXS
    OPCodes_Transfer(&m_X, &m_S);
}

void HuC6280::OPCode0x9B()
{
    UnofficialOPCode();
}

void HuC6280::OPCode0x9C()
{
    // STZ hhll
    OPCodes_STZ(AbsoluteAddressing());
}

void HuC6280::OPCode0x9D()
{
    // STA hhll,X
    OPCodes_Store(&m_A, AbsoluteAddressing(&m_X));
}

void HuC6280::OPCode0x9E()
{
    // STZ hhll,X
    OPCodes_STZ(AbsoluteAddressing(&m_X));
}

void HuC6280::OPCode0x9F()
{
    // BBS1 ZP,rr
    OPcodes_Branch(IsSetBit(1, m_memory->Read(ZeroPageAddressing())));
}

void HuC6280::OPCode0xA0()
{
    // LDY #nn
    OPCodes_LD(&m_Y, ImmediateAddressing());
}

void HuC6280::OPCode0xA1()
{
    // LDA $(ZP,X)
    OPCodes_LD(&m_A, m_memory->Read(ZeroPageIndexedIndirectAddressing()));
}

void HuC6280::OPCode0xA2()
{
    // LDX #nn
    OPCodes_LD(&m_X, ImmediateAddressing());
}

void HuC6280::OPCode0xA3()
{
    // TST #nn,ZP,X
    u8 nn = Fetch8();
    OPCodes_TST(nn, m_memory->Read(ZeroPageAddressing(&m_X)));
}

void HuC6280::OPCode0xA4()
{
    // LDY ZP
    OPCodes_LD(&m_Y, m_memory->Read(ZeroPageAddressing()));
}

void HuC6280::OPCode0xA5()
{
    // LDA ZP
    OPCodes_LD(&m_A, m_memory->Read(ZeroPageAddressing()));
}

void HuC6280::OPCode0xA6()
{
    // LDX ZP
    OPCodes_LD(&m_X, m_memory->Read(ZeroPageAddressing()));
}

void HuC6280::OPCode0xA7()
{
    // SMB2 ZP
    OPCodes_SMB(2, ZeroPageAddressing());
}

void HuC6280::OPCode0xA8()
{
    // TAY
    OPCodes_Transfer(&m_A, &m_Y);
}

void HuC6280::OPCode0xA9()
{
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
    UnofficialOPCode();
}

void HuC6280::OPCode0xAC()
{
    // LDY hhll
    OPCodes_LD(&m_Y, m_memory->Read(AbsoluteAddressing()));
}

void HuC6280::OPCode0xAD()
{
    // LDA hhll
    OPCodes_LD(&m_A, m_memory->Read(AbsoluteAddressing()));
}

void HuC6280::OPCode0xAE()
{
    // LDX hhll
    OPCodes_LD(&m_X, m_memory->Read(AbsoluteAddressing()));
}

void HuC6280::OPCode0xAF()
{
    // BBS2 ZP,rr
    OPcodes_Branch(IsSetBit(2, m_memory->Read(ZeroPageAddressing())));
}

void HuC6280::OPCode0xB0()
{
    // BCS rr
    OPcodes_Branch(IsSetFlag(FLAG_CARRY));
}

void HuC6280::OPCode0xB1()
{
    // LDA ($n),Y
    OPCodes_LD(&m_A, m_memory->Read(ZeroPageIndirectIndexedAddressing()));
}

void HuC6280::OPCode0xB2()
{
    // LDA (ZP)
    OPCodes_LD(&m_A, m_memory->Read(ZeroPageIndirectAddressing()));
}

void HuC6280::OPCode0xB3()
{
    // TST #nn,hhll,X
    u8 nn = Fetch8();
    OPCodes_TST(nn, m_memory->Read(AbsoluteAddressing(&m_X)));
}

void HuC6280::OPCode0xB4()
{
    // LDY ZP,X
    OPCodes_LD(&m_Y, m_memory->Read(ZeroPageAddressing(&m_X)));
}

void HuC6280::OPCode0xB5()
{
    // LDA ZP,X
    OPCodes_LD(&m_A, m_memory->Read(ZeroPageAddressing(&m_X)));
}

void HuC6280::OPCode0xB6()
{
    // LDX ZP,Y
    OPCodes_LD(&m_X, m_memory->Read(ZeroPageAddressing(&m_Y)));
}

void HuC6280::OPCode0xB7()
{
    // SMB3 ZP
    OPCodes_SMB(3, ZeroPageAddressing());
}

void HuC6280::OPCode0xB8()
{
    // CLV
    OPCodes_ClearFlag(FLAG_OVERFLOW);
}

void HuC6280::OPCode0xB9()
{
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
    UnofficialOPCode();
}

void HuC6280::OPCode0xBC()
{
    // LDY hhll,X
    OPCodes_LD(&m_Y, m_memory->Read(AbsoluteAddressing(&m_X)));
}

void HuC6280::OPCode0xBD()
{
    // LDA hhll,X
    OPCodes_LD(&m_A, m_memory->Read(AbsoluteAddressing(&m_X)));
}

void HuC6280::OPCode0xBE()
{
    // LDX hhll,Y
    OPCodes_LD(&m_X, m_memory->Read(AbsoluteAddressing(&m_Y)));
}

void HuC6280::OPCode0xBF()
{
    // BBS3 ZP,rr
    OPcodes_Branch(IsSetBit(3, m_memory->Read(ZeroPageAddressing())));
}

void HuC6280::OPCode0xC0()
{
    // CPY #nn
    OPCodes_CMP(&m_Y, ImmediateAddressing());
}

void HuC6280::OPCode0xC1()
{
    // CMP (ZP,X)
    OPCodes_CMP(&m_A, m_memory->Read(ZeroPageIndexedIndirectAddressing()));
}

void HuC6280::OPCode0xC2()
{
    // CLY
    ClearFlag(FLAG_TRANSFER);
    m_Y.SetValue(0x00);
}

void HuC6280::OPCode0xC3()
{
    // TDD
    OPCodes_TDD();
}

void HuC6280::OPCode0xC4()
{
    // CPY ZP
    OPCodes_CMP(&m_Y, m_memory->Read(ZeroPageAddressing()));
}

void HuC6280::OPCode0xC5()
{
    // CMP ZP
    OPCodes_CMP(&m_A, m_memory->Read(ZeroPageAddressing()));
}

void HuC6280::OPCode0xC6()
{
    // DEC ZP
    OPCodes_DEC_Mem(ZeroPageAddressing());
}

void HuC6280::OPCode0xC7()
{
    // SMB4 ZP
    OPCodes_SMB(4, ZeroPageAddressing());
}

void HuC6280::OPCode0xC8()
{
    // INY
    OPCodes_INC_Reg(&m_Y);
}

void HuC6280::OPCode0xC9()
{
    // CMP #nn
    OPCodes_CMP(&m_A, ImmediateAddressing());
}

void HuC6280::OPCode0xCA()
{
    // DEX
    OPCodes_DEC_Reg(&m_X);
}

void HuC6280::OPCode0xCB()
{
    UnofficialOPCode();
}

void HuC6280::OPCode0xCC()
{
    // CPY hhll
    OPCodes_CMP(&m_Y, m_memory->Read(AbsoluteAddressing()));
}

void HuC6280::OPCode0xCD()
{
    // CMP hhll
    OPCodes_CMP(&m_A, m_memory->Read(AbsoluteAddressing()));
}

void HuC6280::OPCode0xCE()
{
    // DEC hhll
    OPCodes_DEC_Mem(AbsoluteAddressing());
}

void HuC6280::OPCode0xCF()
{
    // BBS4 ZP,rr
    OPcodes_Branch(IsSetBit(4, m_memory->Read(ZeroPageAddressing())));
}

void HuC6280::OPCode0xD0()
{
    // BNE rr
    OPcodes_Branch(!IsSetFlag(FLAG_ZERO));
}

void HuC6280::OPCode0xD1()
{
    // CMP (ZP),Y
    OPCodes_CMP(&m_A, m_memory->Read(ZeroPageIndirectIndexedAddressing()));
}

void HuC6280::OPCode0xD2()
{
    // CMP (ZP)
    OPCodes_CMP(&m_A, m_memory->Read(ZeroPageIndirectAddressing()));
}

void HuC6280::OPCode0xD3()
{
    // TIN
    OPCodes_TIN();
}

void HuC6280::OPCode0xD4()
{
    // CSH
    m_high_speed = true;
}

void HuC6280::OPCode0xD5()
{
    // CMP ZP,X
    OPCodes_CMP(&m_A, m_memory->Read(ZeroPageAddressing(&m_X)));
}

void HuC6280::OPCode0xD6()
{
    // DEC ZP,X
    OPCodes_DEC_Mem(ZeroPageAddressing(&m_X));
}

void HuC6280::OPCode0xD7()
{
    // SMB5 ZP
    OPCodes_SMB(5, ZeroPageAddressing());
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
    // PHX
    ClearFlag(FLAG_TRANSFER);
    StackPush8(m_X.GetValue());
}

void HuC6280::OPCode0xDB()
{
    UnofficialOPCode();
}

void HuC6280::OPCode0xDC()
{
    UnofficialOPCode();
}

void HuC6280::OPCode0xDD()
{
    // CMP hhll,X
    OPCodes_CMP(&m_A, m_memory->Read(AbsoluteAddressing(&m_X)));
}

void HuC6280::OPCode0xDE()
{
    // DEC hhll,X
    OPCodes_DEC_Mem(AbsoluteAddressing(&m_X));
}

void HuC6280::OPCode0xDF()
{
    // BBS5 ZP,rr
    OPcodes_Branch(IsSetBit(5, m_memory->Read(ZeroPageAddressing())));
}

void HuC6280::OPCode0xE0()
{
    // CPX #nn
    OPCodes_CMP(&m_X, ImmediateAddressing());
}

void HuC6280::OPCode0xE1()
{
    // SBC $(ZP,X)
    OPCodes_SBC(m_memory->Read(ZeroPageIndexedIndirectAddressing()));
}

void HuC6280::OPCode0xE2()
{
    UnofficialOPCode();
}

void HuC6280::OPCode0xE3()
{
    // TIA
    OPCodes_TIA();
}

void HuC6280::OPCode0xE4()
{
    // CPX ZP
    OPCodes_CMP(&m_X, m_memory->Read(ZeroPageAddressing()));
}

void HuC6280::OPCode0xE5()
{
    // SBC ZP
    OPCodes_SBC(m_memory->Read(ZeroPageAddressing()));
}

void HuC6280::OPCode0xE6()
{
    // INC ZP
    OPCodes_INC_Mem(ZeroPageAddressing());
}

void HuC6280::OPCode0xE7()
{
    // SMB6 ZP
    OPCodes_SMB(6, ZeroPageAddressing());
}

void HuC6280::OPCode0xE8()
{
    // INX
    OPCodes_INC_Reg(&m_X);
}

void HuC6280::OPCode0xE9()
{
    // SBC #nn
    OPCodes_SBC(ImmediateAddressing());
}

void HuC6280::OPCode0xEA()
{
    // NOP
    ClearFlag(FLAG_TRANSFER);
}

void HuC6280::OPCode0xEB()
{
    UnofficialOPCode();
}

void HuC6280::OPCode0xEC()
{
    // CPX hhll
    OPCodes_CMP(&m_X, m_memory->Read(AbsoluteAddressing()));
}

void HuC6280::OPCode0xED()
{
    // SBC hhll
    OPCodes_SBC(m_memory->Read(AbsoluteAddressing()));
}

void HuC6280::OPCode0xEE()
{
    // INC hhll
    OPCodes_INC_Mem(AbsoluteAddressing());
}

void HuC6280::OPCode0xEF()
{
    // BBS6 ZP,rr
    OPcodes_Branch(IsSetBit(6, m_memory->Read(ZeroPageAddressing())));
}

void HuC6280::OPCode0xF0()
{
    // BEQ rr
    OPcodes_Branch(IsSetFlag(FLAG_ZERO));
}

void HuC6280::OPCode0xF1()
{
    // SBC (ZP),Y
    OPCodes_SBC(m_memory->Read(ZeroPageIndirectIndexedAddressing()));
}

void HuC6280::OPCode0xF2()
{
    // SBC (ZP)
    OPCodes_SBC(m_memory->Read(ZeroPageIndirectAddressing()));
}

void HuC6280::OPCode0xF3()
{
    // TAI
    OPCodes_TAI();
}

void HuC6280::OPCode0xF4()
{
    // SET
    OPCodes_SetFlag(FLAG_TRANSFER);
}

void HuC6280::OPCode0xF5()
{
    // SBC ZP,X
    OPCodes_SBC(m_memory->Read(ZeroPageAddressing(&m_X)));
}

void HuC6280::OPCode0xF6()
{
    // INC ZP,X
    OPCodes_INC_Mem(ZeroPageAddressing(&m_X));
}

void HuC6280::OPCode0xF7()
{
    // SMB7 ZP
    OPCodes_SMB(7, ZeroPageAddressing());
}

void HuC6280::OPCode0xF8()
{
    // SED
    OPCodes_SetFlag(FLAG_DECIMAL);
}

void HuC6280::OPCode0xF9()
{
    // SBC hhll,Y
    OPCodes_SBC(m_memory->Read(AbsoluteAddressing(&m_Y)));
}

void HuC6280::OPCode0xFA()
{
    // PLX
    ClearFlag(FLAG_TRANSFER);
    u8 result = StackPop8();
    m_X.SetValue(result);
    SetZeroFlagFromResult(result);
    SetNegativeFlagFromResult(result);
}

void HuC6280::OPCode0xFB()
{
    UnofficialOPCode();
}

void HuC6280::OPCode0xFC()
{
    UnofficialOPCode();
}

void HuC6280::OPCode0xFD()
{
    // SBC hhll,X
    OPCodes_SBC(m_memory->Read(AbsoluteAddressing(&m_X)));
}

void HuC6280::OPCode0xFE()
{
    // INC hhll,X
    OPCodes_INC_Mem(AbsoluteAddressing(&m_X));
}

void HuC6280::OPCode0xFF()
{
    // BBS7 ZP,rr
    OPcodes_Branch(IsSetBit(7, m_memory->Read(ZeroPageAddressing())));
}