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

#ifndef HUC6280_H
#define HUC6280_H

#include <vector>
#include <stack>
#include <iostream>
#include <fstream>
#include "common.h"
#include "huc6280_registers.h"

#define FLAG_CARRY      0x01
#define FLAG_ZERO       0x02
#define FLAG_INTERRUPT  0x04
#define FLAG_DECIMAL    0x08
#define FLAG_BREAK      0x10
#define FLAG_TRANSFER   0x20
#define FLAG_OVERFLOW   0x40
#define FLAG_NEGATIVE   0x80

#if defined(GG_TESTING)
#define ZERO_PAGE_ADDR  0x0000
#define STACK_ADDR      0x0100
#else
#define ZERO_PAGE_ADDR  0x2000
#define STACK_ADDR      0x2100
#endif

class Memory;
class HuC6270;

class HuC6280
{
public:
    struct HuC6280_State
    {
        SixteenBitRegister* PC;
        EightBitRegister* A;
        EightBitRegister* X;
        EightBitRegister* Y;
        EightBitRegister* S;
        EightBitRegister* P;
        s32* SPEED;
        bool* TIMER;
        u8* TIMER_COUNTER;
        u8* TIMER_RELOAD;
        u8* IDR;
        u8* IRR;
        u32* CYCLES;
    };

    enum GG_Breakpoint_Type
    {
        HuC6280_BREAKPOINT_TYPE_ROMRAM = 0,
        HuC6280_BREAKPOINT_TYPE_VRAM,
        HuC6280_BREAKPOINT_TYPE_PALETTE_RAM,
        HuC6280_BREAKPOINT_TYPE_HUC6270_REGISTER,
        HuC6280_BREAKPOINT_TYPE_HUC6260_REGISTER,
        HuC6280_BREAKPOINT_TYPE_COUNT
    };

    struct GG_Breakpoint
    {
        bool enabled;
        int type;
        u16 address1;
        u16 address2;
        bool read;
        bool write;
        bool execute;
        bool range;
    };

    struct GG_CallStackEntry
    {
        u16 src;
        u16 dest;
        u16 back;
    };

public:
    HuC6280();
    ~HuC6280();
    void Init(Memory* memory, HuC6270* huc6270);
    void Reset();
    u32 RunInstruction(bool* completed = NULL);
    void ClockTimer(u32 cycles);
    void AssertIRQ1(bool asserted);
    void AssertIRQ2(bool asserted);
    void InjectCycles(unsigned int cycles);
    u8 ReadInterruptRegister(u16 address);
    void WriteInterruptRegister(u16 address, u8 value);
    u8 ReadTimerRegister();
    void WriteTimerRegister(u16 address, u8 value);
    HuC6280_State* GetState();
    void DisassembleNextOPCode();
    void SetResetValue(int value);
    void EnableBreakpoints(bool enable, bool irqs);
    bool BreakpointHit();
    bool RunToBreakpointHit();
    void ResetBreakpoints();
    bool AddBreakpoint(int type, char* text, bool read, bool write, bool execute);
    bool AddBreakpoint(u16 address);
    void AddRunToBreakpoint(u16 address);
    void RemoveBreakpoint(int type, u16 address);
    bool IsBreakpoint(int type, u16 address);
    std::vector<GG_Breakpoint>* GetBreakpoints();
    void ClearDisassemblerCallStack();
    std::stack<GG_CallStackEntry>* GetDisassemblerCallStack();
    void CheckMemoryBreakpoints(int type, u16 address, bool read);
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);

private:
    typedef void (HuC6280::*opcodeptr) (void);
    opcodeptr m_opcodes[256];
    SixteenBitRegister m_PC;
    EightBitRegister m_A;
    EightBitRegister m_X;
    EightBitRegister m_Y;
    EightBitRegister m_S;
    EightBitRegister m_P;
    u8 m_zn_flags_lut[256];
    u32 m_cycles;
    u32 m_last_instruction_cycles;
    s32 m_irq_pending;
    s32 m_speed;
    int m_transfer_state;
    int m_transfer_count;
    u16 m_transfer_length;
    u16 m_transfer_source;
    u16 m_transfer_dest;
    Memory* m_memory;
    HuC6270* m_huc6270;
    HuC6280_State m_processor_state;
    bool m_timer_enabled;
    s32 m_timer_cycles;
    u8 m_timer_counter;
    u8 m_timer_reload;
    u8 m_interrupt_disable_register;
    u8 m_interrupt_request_register;
    bool m_skip_flag_transfer_clear;
    s32 m_debug_next_irq;
    bool m_breakpoints_enabled;
    bool m_breakpoints_irq_enabled;
    bool m_cpu_breakpoint_hit;
    bool m_memory_breakpoint_hit;
    bool m_run_to_breakpoint_hit;
    std::vector<GG_Breakpoint> m_breakpoints;
    GG_Breakpoint m_run_to_breakpoint;
    bool m_run_to_breakpoint_requested;
    std::stack<GG_CallStackEntry> m_disassembler_call_stack;
    int m_reset_value;

private:

    void HandleIRQ();
    void CheckIRQs();

    void CheckBreakpoints();
    void PushCallStack(u16 src, u16 dest, u16 back);
    void PopCallStack();

    u8 Fetch8();
    u16 Fetch16();
    u16 Address16(u8 high, u8 low);
    bool PageCrossed(u16 old_address, u16 new_address);
    u16 ZeroPageX();

    void CreateZNFlagsTable();
    void SetOrClearZNFlags(u8 result);
    void SetZNFlags(u8 result);
    void SetOverflowFlag(u8 result);
    void SetFlag(u8 flag);
    void ClearFlag(u8 flag);
    bool IsSetFlag(u8 flag);

    void StackPush16(u16 value);
    void StackPush8(u8 value);
    u16 StackPop16();
    u8 StackPop8();

    u8 ImmediateAddressing();
    u16 ZeroPageAddressing();
    u16 ZeroPageAddressing(EightBitRegister* reg);
    u16 ZeroPageIndirectAddressing();
    u16 ZeroPageRelativeAddressing();
    u16 ZeroPageIndexedIndirectAddressing();
    u16 ZeroPageIndirectIndexedAddressing();
    s8 RelativeAddressing();
    u16 AbsoluteAddressing();
    u16 AbsoluteAddressing(EightBitRegister* reg);
    u16 AbsoluteIndirectAddressing();
    u16 AbsoluteIndexedIndirectAddressing();

    void PopulateDisassemblerRecord(GG_Disassembler_Record* record, u8 opcode, u16 address);

    void UnofficialOPCode();
    void OPCodes_ADC(u8 value);
    void OPCodes_AND(u8 value);
    void OPCodes_ASL_Accumulator();
    void OPCodes_ASL_Memory(u16 address);
    void OPcodes_Branch(bool condition);
    void OPCodes_BIT(u16 address);
    void OPCodes_BIT_Immediate(u16 address);
    void OPCodes_BRK();
    void OPCodes_Subroutine();
    void OPCodes_CMP(EightBitRegister* reg, u8 value);
    void OPCodes_DEC_Mem(u16 address);
    void OPCodes_DEC_Reg(EightBitRegister* reg);
    void OPCodes_EOR(u8 value);
    void OPCodes_INC_Mem(u16 address);
    void OPCodes_INC_Reg(EightBitRegister* reg);
    void OPCodes_LD(EightBitRegister* reg, u8 value);
    void OPCodes_LSR_Accumulator();
    void OPCodes_LSR_Memory(u16 address);
    void OPCodes_ORA(u8 value);
    void OPCodes_RMB(u8 bit, u16 address);
    void OPCodes_ROL_Accumulator();
    void OPCodes_ROL_Memory(u16 address);
    void OPCodes_ROR_Accumulator();
    void OPCodes_ROR_Memory(u16 address);
    void OPCodes_SBC(u8 value);
    void OPCodes_SMB(u8 bit, u16 address);
    void OPCodes_Store(EightBitRegister* reg, u16 address);
    void OPCodes_STZ(u16 address);
    void OPCodes_Swap(EightBitRegister* reg1, EightBitRegister* reg2);
    void OPCodes_TAM();
    void OPCodes_TMA();
    void OPCodes_Transfer(EightBitRegister* source, EightBitRegister* dest);
    void OPCodes_TRB(u16 address);
    void OPCodes_TSB(u16 address);
    void OPCodes_TST(u8 value, u16 address);
    void OPCodes_TAI();
    void OPCodes_TDD();
    void OPCodes_TIA();
    void OPCodes_TII();
    void OPCodes_TIN();
    void OPCodes_TransferStart();
    void OPCodes_TransferEnd();

    void InitOPCodeFunctors();

    void OPCode0x00(); void OPCode0x01(); void OPCode0x02(); void OPCode0x03();
    void OPCode0x04(); void OPCode0x05(); void OPCode0x06(); void OPCode0x07();
    void OPCode0x08(); void OPCode0x09(); void OPCode0x0A(); void OPCode0x0B();
    void OPCode0x0C(); void OPCode0x0D(); void OPCode0x0E(); void OPCode0x0F();
    void OPCode0x10(); void OPCode0x11(); void OPCode0x12(); void OPCode0x13();
    void OPCode0x14(); void OPCode0x15(); void OPCode0x16(); void OPCode0x17();
    void OPCode0x18(); void OPCode0x19(); void OPCode0x1A(); void OPCode0x1B();
    void OPCode0x1C(); void OPCode0x1D(); void OPCode0x1E(); void OPCode0x1F();
    void OPCode0x20(); void OPCode0x21(); void OPCode0x22(); void OPCode0x23();
    void OPCode0x24(); void OPCode0x25(); void OPCode0x26(); void OPCode0x27();
    void OPCode0x28(); void OPCode0x29(); void OPCode0x2A(); void OPCode0x2B();
    void OPCode0x2C(); void OPCode0x2D(); void OPCode0x2E(); void OPCode0x2F();
    void OPCode0x30(); void OPCode0x31(); void OPCode0x32(); void OPCode0x33();
    void OPCode0x34(); void OPCode0x35(); void OPCode0x36(); void OPCode0x37();
    void OPCode0x38(); void OPCode0x39(); void OPCode0x3A(); void OPCode0x3B();
    void OPCode0x3C(); void OPCode0x3D(); void OPCode0x3E(); void OPCode0x3F();
    void OPCode0x40(); void OPCode0x41(); void OPCode0x42(); void OPCode0x43();
    void OPCode0x44(); void OPCode0x45(); void OPCode0x46(); void OPCode0x47();
    void OPCode0x48(); void OPCode0x49(); void OPCode0x4A(); void OPCode0x4B();
    void OPCode0x4C(); void OPCode0x4D(); void OPCode0x4E(); void OPCode0x4F();
    void OPCode0x50(); void OPCode0x51(); void OPCode0x52(); void OPCode0x53();
    void OPCode0x54(); void OPCode0x55(); void OPCode0x56(); void OPCode0x57();
    void OPCode0x58(); void OPCode0x59(); void OPCode0x5A(); void OPCode0x5B();
    void OPCode0x5C(); void OPCode0x5D(); void OPCode0x5E(); void OPCode0x5F();
    void OPCode0x60(); void OPCode0x61(); void OPCode0x62(); void OPCode0x63();
    void OPCode0x64(); void OPCode0x65(); void OPCode0x66(); void OPCode0x67();
    void OPCode0x68(); void OPCode0x69(); void OPCode0x6A(); void OPCode0x6B();
    void OPCode0x6C(); void OPCode0x6D(); void OPCode0x6E(); void OPCode0x6F();
    void OPCode0x70(); void OPCode0x71(); void OPCode0x72(); void OPCode0x73();
    void OPCode0x74(); void OPCode0x75(); void OPCode0x76(); void OPCode0x77();
    void OPCode0x78(); void OPCode0x79(); void OPCode0x7A(); void OPCode0x7B();
    void OPCode0x7C(); void OPCode0x7D(); void OPCode0x7E(); void OPCode0x7F();
    void OPCode0x80(); void OPCode0x81(); void OPCode0x82(); void OPCode0x83();
    void OPCode0x84(); void OPCode0x85(); void OPCode0x86(); void OPCode0x87();
    void OPCode0x88(); void OPCode0x89(); void OPCode0x8A(); void OPCode0x8B();
    void OPCode0x8C(); void OPCode0x8D(); void OPCode0x8E(); void OPCode0x8F();
    void OPCode0x90(); void OPCode0x91(); void OPCode0x92(); void OPCode0x93();
    void OPCode0x94(); void OPCode0x95(); void OPCode0x96(); void OPCode0x97();
    void OPCode0x98(); void OPCode0x99(); void OPCode0x9A(); void OPCode0x9B();
    void OPCode0x9C(); void OPCode0x9D(); void OPCode0x9E(); void OPCode0x9F();
    void OPCode0xA0(); void OPCode0xA1(); void OPCode0xA2(); void OPCode0xA3();
    void OPCode0xA4(); void OPCode0xA5(); void OPCode0xA6(); void OPCode0xA7();
    void OPCode0xA8(); void OPCode0xA9(); void OPCode0xAA(); void OPCode0xAB();
    void OPCode0xAC(); void OPCode0xAD(); void OPCode0xAE(); void OPCode0xAF();
    void OPCode0xB0(); void OPCode0xB1(); void OPCode0xB2(); void OPCode0xB3();
    void OPCode0xB4(); void OPCode0xB5(); void OPCode0xB6(); void OPCode0xB7();
    void OPCode0xB8(); void OPCode0xB9(); void OPCode0xBA(); void OPCode0xBB();
    void OPCode0xBC(); void OPCode0xBD(); void OPCode0xBE(); void OPCode0xBF();
    void OPCode0xC0(); void OPCode0xC1(); void OPCode0xC2(); void OPCode0xC3();
    void OPCode0xC4(); void OPCode0xC5(); void OPCode0xC6(); void OPCode0xC7();
    void OPCode0xC8(); void OPCode0xC9(); void OPCode0xCA(); void OPCode0xCB();
    void OPCode0xCC(); void OPCode0xCD(); void OPCode0xCE(); void OPCode0xCF();
    void OPCode0xD0(); void OPCode0xD1(); void OPCode0xD2(); void OPCode0xD3();
    void OPCode0xD4(); void OPCode0xD5(); void OPCode0xD6(); void OPCode0xD7();
    void OPCode0xD8(); void OPCode0xD9(); void OPCode0xDA(); void OPCode0xDB();
    void OPCode0xDC(); void OPCode0xDD(); void OPCode0xDE(); void OPCode0xDF();
    void OPCode0xE0(); void OPCode0xE1(); void OPCode0xE2(); void OPCode0xE3();
    void OPCode0xE4(); void OPCode0xE5(); void OPCode0xE6(); void OPCode0xE7();
    void OPCode0xE8(); void OPCode0xE9(); void OPCode0xEA(); void OPCode0xEB();
    void OPCode0xEC(); void OPCode0xED(); void OPCode0xEE(); void OPCode0xEF();
    void OPCode0xF0(); void OPCode0xF1(); void OPCode0xF2(); void OPCode0xF3();
    void OPCode0xF4(); void OPCode0xF5(); void OPCode0xF6(); void OPCode0xF7();
    void OPCode0xF8(); void OPCode0xF9(); void OPCode0xFA(); void OPCode0xFB();
    void OPCode0xFC(); void OPCode0xFD(); void OPCode0xFE(); void OPCode0xFF();
};

static const int k_huc6280_speed_divisor[2] = { 12, 3 };
static const int k_huc6280_timer_divisor = (1024 * 3);

#include "huc6280_inline.h"
#include "huc6280_opcodes_inline.h"

#endif /* HUC6280_H */