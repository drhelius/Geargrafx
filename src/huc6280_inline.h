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

#include <string.h>
#include "huc6280.h"
#include "huc6280_timing.h"
#include "huc6280_names.h"
#include "memory.h"

INLINE bool HuC6280::Clock()
{
    if (m_clock % 3 == 0)
        ClockTimer();

    bool instruction_completed = false;

    if (m_clock % k_huc6280_speed_divisor[m_speed] == 0)
    {
        if (m_clock_cycles <= 0)
        {
            if ((m_irq_pending != 0) && (m_transfer_state == 0))
            {
                m_clock_cycles += TickIRQ();
                if (m_clock_cycles == 0)
                    m_clock_cycles += TickOPCode();
            }
            else
                m_clock_cycles += TickOPCode();
        }

        m_clock_cycles--;
        instruction_completed = ((m_clock_cycles == 0) && (m_transfer_state == 0) && (m_irq_pending == 0));
    }

    m_clock = (m_clock + 1) % 12;

    return instruction_completed;
}

INLINE u32 HuC6280::TickOPCode()
{
    m_memory_breakpoint_hit = false;
    m_skip_flag_transfer_clear = false;
    m_cycles = 0;

    u8 opcode = Fetch8();
    (this->*m_opcodes[opcode])();

#if defined(GG_TESTING)
    SetFlag(FLAG_TRANSFER);
#else
    if (!m_skip_flag_transfer_clear)
        ClearFlag(FLAG_TRANSFER);
#endif

    DisassembleNextOPCode();

    m_cycles += k_huc6280_opcode_cycles[opcode];

    m_last_instruction_cycles = m_cycles;

    return m_cycles;
}

INLINE u32 HuC6280::TickIRQ()
{
    assert(m_irq_pending != 0);

    m_cycles = 0;
    u16 vector = 0;

    // TIQ
    if (IS_SET_BIT(m_irq_pending, 2) && IS_SET_BIT(m_interrupt_request_register, 2))
    {
        vector = 0xFFFA;
        m_debug_next_irq = 3;
    }
    // IRQ1
    else if (IS_SET_BIT(m_irq_pending, 1))
    {
        vector = 0xFFF8;
        m_debug_next_irq = 4;
    }
    // IRQ2
    else if (IS_SET_BIT(m_irq_pending, 0))
    {
        vector = 0xFFF6;
        m_debug_next_irq = 5;
    }
    else
        return 0;

    u16 pc = m_PC.GetValue();
    StackPush16(pc);
    StackPush8(m_P.GetValue() & ~FLAG_BREAK);
    SetFlag(FLAG_INTERRUPT);
    ClearFlag(FLAG_DECIMAL | FLAG_TRANSFER);
    m_PC.SetLow(MemoryRead(vector));
    m_PC.SetHigh(MemoryRead(vector + 1));
    m_cycles += 8;

#if !defined(GG_DISABLE_DISASSEMBLER)
    DisassembleNextOPCode();
    if (m_breakpoints_irq_enabled)
        m_cpu_breakpoint_hit = true;
    u16 dest = m_PC.GetValue();
    PushCallStack(pc, dest, pc);
#endif

    return m_cycles;
}

INLINE void HuC6280::CheckIRQs()
{
    m_irq_pending = IsSetFlag(FLAG_INTERRUPT) ? 0 : m_interrupt_request_register & ~m_interrupt_disable_register;
}

INLINE void HuC6280::AssertIRQ1(bool asserted)
{
    if (asserted)
        m_interrupt_request_register = SET_BIT(m_interrupt_request_register, 1);
    else
        m_interrupt_request_register = UNSET_BIT(m_interrupt_request_register, 1);
}

INLINE void HuC6280::AssertIRQ2(bool asserted)
{
    if (asserted)
        m_interrupt_request_register = SET_BIT(m_interrupt_request_register, 0);
    else
        m_interrupt_request_register = UNSET_BIT(m_interrupt_request_register, 0);
}

INLINE void HuC6280::InjectCycles(unsigned int cycles)
{
    m_cycles += cycles;
    CheckIRQs();
}

INLINE u8 HuC6280::MemoryRead(u16 address, bool block_transfer)
{
    CheckIRQs();
    return m_memory->Read(address, block_transfer);
}

INLINE void HuC6280::MemoryWrite(u16 address, u8 value)
{
    CheckIRQs();
    m_memory->Write(address, value);
}

INLINE u8 HuC6280:: ReadInterruptRegister(u16 address)
{
    if ((address & 1) == 0)
        return m_interrupt_disable_register;
    else
        return m_interrupt_request_register;
}

INLINE void HuC6280::WriteInterruptRegister(u16 address, u8 value)
{
    if ((address & 1) == 0)
        m_interrupt_disable_register = value & 0x07;
    else
    {
        // Acknowledge TIQ
        m_interrupt_request_register = UNSET_BIT(m_interrupt_request_register, 2);
    }
}

INLINE void HuC6280::ClockTimer()
{
    if (!m_timer_enabled)
        return;

    m_timer_cycles -= 3;

    if (m_timer_cycles == 0)
    {
        m_timer_cycles = k_huc6280_timer_divisor;

        if (m_timer_counter == 0)
        {
            m_timer_counter = m_timer_reload;
            m_interrupt_request_register = SET_BIT(m_interrupt_request_register, 2);
        }
        else
            m_timer_counter--;
    }
}

INLINE u8 HuC6280::ReadTimerRegister()
{
    if(m_timer_counter == 0 && m_timer_cycles <= 5 * 3)
        return 0x7F;
    else
        return m_timer_counter;
}

INLINE void HuC6280::WriteTimerRegister(u16 address, u8 value)
{
    if (address & 0x01)
    {
        bool enabled = (value & 0x01);
        if (m_timer_enabled != enabled)
        {
            m_timer_enabled = enabled;
            m_timer_counter = m_timer_reload;
            m_timer_cycles = k_huc6280_timer_divisor;
        }
    }
    else
        m_timer_reload = value & 0x7F;
}

INLINE u8 HuC6280::Fetch8()
{
    u8 value = MemoryRead(m_PC.GetValue());
    m_PC.Increment();
    return value;
}

INLINE u16 HuC6280::Fetch16()
{
    u16 pc = m_PC.GetValue();
    u8 l = MemoryRead(pc);
    u8 h = MemoryRead(pc + 1);
    m_PC.SetValue(pc + 2);
    return Address16(h , l);
}

INLINE u16 HuC6280::Address16(u8 high, u8 low)
{
    return static_cast<u16>(high << 8 ) | low;
}

INLINE bool HuC6280::PageCrossed(u16 old_address, u16 new_address)
{
    return (old_address ^ new_address) > 0x00FF;
}

INLINE u16 HuC6280::ZeroPageX()
{
    return ZERO_PAGE_ADDR | m_X.GetValue();
}

INLINE void HuC6280::SetOrClearZNFlags(u8 result)
{
    ClearFlag(FLAG_ZERO | FLAG_NEGATIVE);
    m_P.SetValue(m_P.GetValue() | m_zn_flags_lut[result]);
}

INLINE void HuC6280::SetZNFlags(u8 result)
{
    m_P.SetValue(m_P.GetValue() | m_zn_flags_lut[result]);
}

INLINE void HuC6280::SetOverflowFlag(u8 result)
{
    m_P.SetValue((m_P.GetValue() & 0xBF) | (result & 0x40));
}

INLINE void HuC6280::SetFlag(u8 flag)
{
    m_P.SetValue(m_P.GetValue() | flag);
}

INLINE void HuC6280::ClearFlag(u8 flag)
{
    m_P.SetValue(m_P.GetValue() & (~flag));
}

INLINE bool HuC6280::IsSetFlag(u8 flag)
{
    return (m_P.GetValue() & flag) != 0;
}

INLINE void HuC6280::StackPush16(u16 value)
{
    MemoryWrite(STACK_ADDR | m_S.GetValue(), static_cast<u8>(value >> 8));
    m_S.Decrement();
    MemoryWrite(STACK_ADDR | m_S.GetValue(), static_cast<u8>(value & 0x00FF));
    m_S.Decrement();
}

INLINE void HuC6280::StackPush8(u8 value)
{
    MemoryWrite(STACK_ADDR | m_S.GetValue(), value);
    m_S.Decrement();
}

INLINE u16 HuC6280::StackPop16()
{
    m_S.Increment();
    u8 l = MemoryRead(STACK_ADDR | m_S.GetValue());
    m_S.Increment();
    u8 h = MemoryRead(STACK_ADDR | m_S.GetValue());
    return Address16(h , l);
}

INLINE u8 HuC6280::StackPop8()
{
    m_S.Increment();
    return MemoryRead(STACK_ADDR | m_S.GetValue());
}

INLINE u8 HuC6280::ImmediateAddressing()
{
    return Fetch8();
}

INLINE u16 HuC6280::ZeroPageAddressing()
{
    return ZERO_PAGE_ADDR | Fetch8();
}

INLINE u16 HuC6280::ZeroPageAddressing(EightBitRegister* reg)
{
    return ZERO_PAGE_ADDR | ((Fetch8() + reg->GetValue()) & 0xFF);
}

INLINE u16 HuC6280::ZeroPageRelativeAddressing()
{
    u16 address = ZeroPageAddressing();
    s8 offset = static_cast<s8>(Fetch8());
    return address + offset;
}

INLINE u16 HuC6280::ZeroPageIndirectAddressing()
{
    u16 address = ZeroPageAddressing();
    u8 l = MemoryRead(address);
    u8 h = MemoryRead((address + 1) & 0x20FF);
    return Address16(h, l);
}

INLINE u16 HuC6280::ZeroPageIndexedIndirectAddressing()
{
    u16 address = (ZeroPageAddressing() + m_X.GetValue()) & 0x20FF;
    u8 l = MemoryRead(address);
    u8 h = MemoryRead((address + 1) & 0x20FF);
    return Address16(h, l);
}

INLINE u16 HuC6280::ZeroPageIndirectIndexedAddressing()
{
    u16 address = ZeroPageAddressing();
    u8 l = MemoryRead(address);
    u8 h = MemoryRead((address + 1) & 0x20FF);
    return Address16(h, l) + m_Y.GetValue();
}

INLINE s8 HuC6280::RelativeAddressing()
{
    return static_cast<s8>(Fetch8());
}

INLINE u16 HuC6280::AbsoluteAddressing()
{
    return Fetch16();
}

INLINE u16 HuC6280::AbsoluteAddressing(EightBitRegister* reg)
{
    u16 address = Fetch16();
    u16 result = address + reg->GetValue();
    return result;
}

INLINE u16 HuC6280::AbsoluteIndirectAddressing()
{
    u16 address = Fetch16();
    u8 l = MemoryRead(address);
    u8 h = MemoryRead(address + 1);
    return Address16(h, l);
}

INLINE u16 HuC6280::AbsoluteIndexedIndirectAddressing()
{
    u16 address = Fetch16() + m_X.GetValue();
    u8 l = MemoryRead(address);
    u8 h = MemoryRead(address + 1);
    return Address16(h, l);
}

INLINE bool HuC6280::RunToBreakpointHit()
{
    return m_run_to_breakpoint_hit && (m_clock_cycles == 0);
}

INLINE std::vector<HuC6280::GG_Breakpoint>* HuC6280::GetBreakpoints()
{
    return &m_breakpoints;
}

INLINE std::stack<HuC6280::GG_CallStackEntry>* HuC6280::GetDisassemblerCallStack()
{
    return &m_disassembler_call_stack;
}

INLINE void HuC6280::PushCallStack(u16 src, u16 dest, u16 back)
{
#if !defined(GG_DISABLE_DISASSEMBLER)
    GG_CallStackEntry entry;
    entry.src = src;
    entry.dest = dest;
    entry.back = back;
    if (m_disassembler_call_stack.size() < 256)
        m_disassembler_call_stack.push(entry);
#else
    UNUSED(src);
    UNUSED(dest);
    UNUSED(back);
#endif
}

INLINE void HuC6280::PopCallStack()
{
#if !defined(GG_DISABLE_DISASSEMBLER)
    if (!m_disassembler_call_stack.empty())
        m_disassembler_call_stack.pop();
#endif
}

INLINE void HuC6280::CheckBreakpoints()
{
#if !defined(GG_DISABLE_DISASSEMBLER)

    m_cpu_breakpoint_hit = false;
    m_run_to_breakpoint_hit = false;

    if (m_run_to_breakpoint_requested)
    {
        if (m_PC.GetValue() == m_run_to_breakpoint.address1)
        {
            m_run_to_breakpoint_hit = true;
            m_run_to_breakpoint_requested = false;
            return;
        }
    }

    if (!m_breakpoints_enabled)
        return;

    for (int i = 0; i < (int)m_breakpoints.size(); i++)
    {
        GG_Breakpoint* brk = &m_breakpoints[i];

        if (!brk->enabled)
            continue;
        if (!brk->execute)
            continue;
        if (brk->type != HuC6280_BREAKPOINT_TYPE_ROMRAM)
            continue;

        if (brk->range)
        {
            if (m_PC.GetValue() >= brk->address1 && m_PC.GetValue() <= brk->address2)
            {
                m_cpu_breakpoint_hit = true;
                m_run_to_breakpoint_requested = false;
                return;
            }
        }
        else
        {
            if (m_PC.GetValue() == brk->address1)
            {
                m_cpu_breakpoint_hit = true;
                m_run_to_breakpoint_requested = false;
                return;
            }
        }
    }

#endif
}

INLINE void HuC6280::DisassembleNextOPCode()
{
#if !defined(GG_DISABLE_DISASSEMBLER)

    CheckBreakpoints();

    u16 address = m_PC.GetValue();
    GG_Disassembler_Record* record = m_memory->GetOrCreateDisassemblerRecord(address);

    assert(IsValidPointer(record));

    u8 opcode = m_memory->Read(address);
    u8 opcode_size = k_huc6280_opcode_sizes[opcode];

    bool changed = (record->opcodes[0] != opcode);
    record->opcodes[0] = opcode;

    for (int i = 1; i < opcode_size; i++)
    {
        u8 mem_byte = m_memory->Read(address + i);

        if (record->opcodes[i] != mem_byte)
        {
            changed = true;
            record->opcodes[i] = mem_byte;
        }
    }

    if (!changed && record->size != 0)
    {
        if (m_debug_next_irq > 0)
        {
            record->irq = m_debug_next_irq;
            m_debug_next_irq = 0;
        }
        return;
    }

    PopulateDisassemblerRecord(record, opcode, address);
#endif
}

INLINE void HuC6280::PopulateDisassemblerRecord(GG_Disassembler_Record* record, u8 opcode, u16 address)
{
#if !defined(GG_DISABLE_DISASSEMBLER)
    u8 opcode_size = k_huc6280_opcode_sizes[opcode];

    record->size = opcode_size;
    record->address = m_memory->GetPhysicalAddress(address);
    record->bank = m_memory->GetBank(address);
    record->name[0] = 0;
    record->bytes[0] = 0;
    record->jump = false;
    record->jump_address = 0;
    record->jump_bank = 0;
    record->subroutine = false;
    record->irq = 0;

    if (m_debug_next_irq > 0)
    {
        record->irq = m_debug_next_irq;
        m_debug_next_irq = 0;
    }

    for (int i = 0; i < opcode_size; i++)
    {
        char value[4];
        snprintf(value, 4, "%02X", record->opcodes[i]);
        strncat(record->bytes, value, 24);
        strncat(record->bytes, " ", 24);
    }

    switch (k_huc6280_opcode_names[opcode].type)
    {
        case GG_OPCode_Type_Implied:
        {
            snprintf(record->name, 64, "%s", k_huc6280_opcode_names[opcode].name);
            break;
        }
        case GG_OPCode_Type_1b:
        {
            snprintf(record->name, 64, k_huc6280_opcode_names[opcode].name, m_memory->Read(address + 1));
            break;
        }
        case GG_OPCode_Type_1b_1b:
        {
            snprintf(record->name, 64, k_huc6280_opcode_names[opcode].name, m_memory->Read(address + 1), m_memory->Read(address + 2));
            break;
        }
        case GG_OPCode_Type_1b_2b:
        {
            snprintf(record->name, 64, k_huc6280_opcode_names[opcode].name, m_memory->Read(address + 1), m_memory->Read(address + 2) | (m_memory->Read(address + 3) << 8));
            break;
        }
        case GG_OPCode_Type_2b:
        {
            snprintf(record->name, 64, k_huc6280_opcode_names[opcode].name, m_memory->Read(address + 1) | (m_memory->Read(address + 2) << 8));
            break;
        }
        case GG_OPCode_Type_2b_2b_2b:
        {
            snprintf(record->name, 64, k_huc6280_opcode_names[opcode].name, m_memory->Read(address + 1) | (m_memory->Read(address + 2) << 8), m_memory->Read(address + 3) | (m_memory->Read(address + 4) << 8), m_memory->Read(address + 5) | (m_memory->Read(address + 6) << 8));
            break;
        }
        case GG_OPCode_Type_1b_Relative:
        {
            s8 rel = m_memory->Read(address + 1);
            u16 jump_address = address + 2 + rel;
            record->jump = true;
            record->jump_address = jump_address;
            record->jump_bank = m_memory->GetBank(jump_address);
            snprintf(record->name, 64, k_huc6280_opcode_names[opcode].name, jump_address, rel);
            break;
        }
        case GG_OPCode_Type_1b_1b_Relative:
        {
            u8 zero_page = m_memory->Read(address + 1);
            s8 rel = m_memory->Read(address + 2);
            u16 jump_address = address + 3 + rel;
            record->jump = true;
            record->jump_address = jump_address;
            record->jump_bank = m_memory->GetBank(jump_address);
            snprintf(record->name, 64, k_huc6280_opcode_names[opcode].name, zero_page, jump_address, rel);
            break;
        }
        case GG_OPCode_Type_ST0:
        {
            u8 reg = m_memory->Read(address + 1) & 0x1F;
            snprintf(record->name, 64, k_huc6280_opcode_names[opcode].name, reg, k_register_names[reg]);
            break;
        }
        default:
        {
            break;
        }
    }

    // JMP hhll, JSR hhll
    if (opcode == 0x4C || opcode == 0x20)
    {
        u16 jump_address = Address16(m_memory->Read(address + 2), m_memory->Read(address + 1));
        record->jump = true;
        record->jump_address = jump_address;
        record->jump_bank = m_memory->GetBank(jump_address);
    }

    // BSR rr, JSR hhll
    if (opcode == 0x44 || opcode == 0x20)
    {
        record->subroutine = true;
    }

    if (record->bank < 0xF7)
    {
        strncpy(record->segment, "ROM", 5);
    }
    else if (record->bank == 0xF7)
    {
        strncpy(record->segment, "BAT", 5);
    }
    else if (record->bank >= 0xF8 && record->bank < 0xFC)
    {
        strncpy(record->segment, "RAM", 5);
    }
    else
    {
        strncpy(record->segment, "???", 5);
    }
#else
    UNUSED(record);
    UNUSED(opcode);
    UNUSED(address);
#endif
}

#endif /* HUC6280_INLINE_H */