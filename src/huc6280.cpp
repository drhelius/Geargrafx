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

#include <string>
#include <stdexcept>
#include <stdlib.h>
#include <string.h>
#include "huc6280.h"
#include "huc6280_timing.h"
#include "huc6280_names.h"
#include "memory.h"

HuC6280::HuC6280()
{
    InitOPCodeFunctors();
    m_cycles = 0;
    m_irq1_asserted = false;
    m_irq2_asserted = false;
    m_nmi_requested = false;
    m_high_speed = false;
    m_timer_cycles = 0;
    m_clock_cycles = 0;
    m_timer_enabled = false;
    m_timer_counter = 0;
    m_timer_reload = 0;
    m_timer_irq = false;
    m_interrupt_disable_register = 0;
    m_interrupt_request_register = 0;
    m_debug_next_irq = 0;
    m_breakpoint_hit = false;
    m_run_to_breakpoint_requested = false;
    m_processor_state.A = &m_A;
    m_processor_state.X = &m_X;
    m_processor_state.Y = &m_Y;
    m_processor_state.S = &m_S;
    m_processor_state.P = &m_P;
    m_processor_state.PC = &m_PC;
    m_processor_state.SPEED = &m_high_speed;
    m_processor_state.TIMER = &m_timer_enabled;
    m_processor_state.TIMER_IRQ = &m_timer_irq;
    m_processor_state.TIMER_COUNTER = &m_timer_counter;
    m_processor_state.TIMER_RELOAD = &m_timer_reload;
    m_processor_state.IRQ1 = &m_irq1_asserted;
    m_processor_state.IRQ2 = &m_irq2_asserted;
    m_processor_state.NMI = &m_nmi_requested;
    m_processor_state.IDR = &m_interrupt_disable_register;
    m_processor_state.IRR = &m_interrupt_request_register;
}

HuC6280::~HuC6280()
{
}

void HuC6280::Init(Memory* memory, HuC6270* huc6270)
{
    m_memory = memory;
    m_huc6270 = huc6270;
}

void HuC6280::Reset()
{
    m_PC.SetLow(m_memory->Read(0xFFFE));
    m_PC.SetHigh(m_memory->Read(0xFFFF));
    m_debug_next_irq = 1;
    DisassembleNextOPCode();
    m_A.SetValue(rand() & 0xFF);
    m_X.SetValue(rand() & 0xFF);
    m_Y.SetValue(rand() & 0xFF);
    m_S.SetValue(rand() & 0xFF);
    m_P.SetValue(rand() & 0xFF);
    ClearFlag(FLAG_TRANSFER);
    ClearFlag(FLAG_DECIMAL);
    SetFlag(FLAG_INTERRUPT);
    SetFlag(FLAG_BREAK);
    m_cycles = 0;
    m_clock_cycles = 0;
    m_irq1_asserted = false;
    m_irq2_asserted = false;
    m_nmi_requested = false;
    m_high_speed = false;
    m_timer_cycles = 0;
    m_timer_enabled = false;
    m_timer_counter = 0;
    m_timer_reload = 0;
    m_timer_irq = false;
    m_interrupt_disable_register = 0;
    m_interrupt_request_register = 0;
    m_breakpoint_hit = false;
    m_run_to_breakpoint_requested = false;
}

unsigned int HuC6280::Tick()
{
    m_cycles = 0;
    bool irq = false;
    u16 irq_low = 0;
    u16 irq_high = 0;

    // NMI
    if (m_nmi_requested)
    {
        irq = true;
        irq_low = 0xFFFC;
        irq_high = 0xFFFD;
        m_nmi_requested = false;
        m_debug_next_irq = 2;
    }
    else if (!IsSetFlag(FLAG_INTERRUPT))
    {
        // TIQ
        if (m_timer_irq && !IsSetBit(m_interrupt_disable_register, 2))
        {
            irq = true;
            irq_low = 0xFFFA;
            irq_high = 0xFFFB;
            m_debug_next_irq = 3;
        }
        // IRQ1
        else if (m_irq1_asserted && !IsSetBit(m_interrupt_disable_register, 1))
        {
            irq = true;
            irq_low = 0xFFF8;
            irq_high = 0xFFF9;
            m_debug_next_irq = 4;
        }
        // IRQ2
        else if (m_irq2_asserted && !IsSetBit(m_interrupt_disable_register, 0))
        {
            irq = true;
            irq_low = 0xFFF6;
            irq_high = 0xFFF7;
            m_debug_next_irq = 5;
        }
    }

    if (irq)
    {
        StackPush16(m_PC.GetValue());
        ClearFlag(FLAG_BREAK);
        StackPush8(m_P.GetValue());
        SetFlag(FLAG_BREAK);
        SetFlag(FLAG_INTERRUPT);
        m_PC.SetLow(m_memory->Read(irq_low));
        m_PC.SetHigh(m_memory->Read(irq_high));
        m_cycles += 7;
        DisassembleNextOPCode();
        return m_cycles;
    }

    u8 opcode = Fetch8();
    (this->*m_opcodes[opcode])();
    DisassembleNextOPCode();

    m_cycles += k_opcode_cycles[opcode];

    return m_cycles;
}

void HuC6280::ClockTimer()
{
    m_timer_cycles++;

    if (m_timer_cycles >= 1024)
    {
        m_timer_cycles -= 1024;

        if (m_timer_enabled)
        {
            m_timer_counter = (m_timer_counter - 1) & 0x7F;

            if (m_timer_counter == 0x7F)
            {
                m_timer_counter = m_timer_reload;
                m_timer_irq = true;
                SetBit(m_interrupt_request_register, 2);
                // Debug("Timer counter underflow, IRQ, reload: %02X", m_timer_reload);
            }
        }
    }
}

HuC6280::HuC6280_State* HuC6280::GetState()
{
    return &m_processor_state;
}

void HuC6280::DisassembleNextOPCode()
{
#ifndef GG_DISABLE_DISASSEMBLER

    CheckBreakpoints();

    u16 address = m_PC.GetValue();
    Memory::GG_Disassembler_Record* record = m_memory->GetOrCreateDisassemblerRecord(address);

    if (!IsValidPointer(record))
    {
        return;
    }

    u8 opcode = m_memory->Read(address);
    u8 opcode_size = k_opcode_sizes[opcode];

    bool changed = false;

    for (int i = 0; i < opcode_size; i++)
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
        m_debug_next_irq = 0;
        return;
    }

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

    switch (k_opcode_names[opcode].type)
    {
        case GG_OPCode_Type_Implied:
        {
            snprintf(record->name, 64, "%s", k_opcode_names[opcode].name);
            break;
        }
        case GG_OPCode_Type_1b:
        {
            snprintf(record->name, 64, k_opcode_names[opcode].name, m_memory->Read(address + 1));
            break;
        }
        case GG_OPCode_Type_1b_1b:
        {
            snprintf(record->name, 64, k_opcode_names[opcode].name, m_memory->Read(address + 1), m_memory->Read(address + 2));
            break;
        }
        case GG_OPCode_Type_1b_2b:
        {
            snprintf(record->name, 64, k_opcode_names[opcode].name, m_memory->Read(address + 1), m_memory->Read(address + 2) | (m_memory->Read(address + 3) << 8));
            break;
        }
        case GG_OPCode_Type_2b:
        {
            snprintf(record->name, 64, k_opcode_names[opcode].name, m_memory->Read(address + 1) | (m_memory->Read(address + 2) << 8));
            break;
        }
        case GG_OPCode_Type_2b_2b_2b:
        {
            snprintf(record->name, 64, k_opcode_names[opcode].name, m_memory->Read(address + 1) | (m_memory->Read(address + 2) << 8), m_memory->Read(address + 3) | (m_memory->Read(address + 4) << 8), m_memory->Read(address + 5) | (m_memory->Read(address + 6) << 8));
            break;
        }
        case GG_OPCode_Type_1b_Relative:
        {
            s8 rel = m_memory->Read(address + 1);
            u16 jump_address = address + 2 + rel;
            record->jump = true;
            record->jump_address = jump_address;
            record->jump_bank = m_memory->GetBank(jump_address);
            snprintf(record->name, 64, k_opcode_names[opcode].name, jump_address, rel);
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
            snprintf(record->name, 64, k_opcode_names[opcode].name, zero_page, jump_address, rel);
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
    else if (record->bank >= 0xF8 && record->bank < 0xFC)
    {
        strncpy(record->segment, "RAM", 5);
    }
    else
    {
        strncpy(record->segment, "???", 5);
    }
#endif
}

bool HuC6280::BreakpointHit()
{
    return m_breakpoint_hit && (m_clock_cycles == 0);
}

void HuC6280::ResetBreakpoints()
{
    m_breakpoints.clear();
}

bool HuC6280::AddBreakpoint(char* text, bool read, bool write, bool execute)
{
    int input_len = (int)strlen(text);
    GG_Breakpoint brk;
    brk.address1 = 0;
    brk.address2 = 0;
    brk.range = false;
    brk.read = read;
    brk.write = write;
    brk.execute = execute;

    if (!read && !write && !execute)
        return false;

    try
    {
        if ((input_len == 9) && (text[4] == '-'))
        {
            std::string str(text);
            std::size_t separator = str.find("-");

            if (separator != std::string::npos)
            {
                brk.address1 = (u16)std::stoul(str.substr(0, separator), 0 , 16);
                brk.address2 = (u16)std::stoul(str.substr(separator + 1 , std::string::npos), 0, 16);
                brk.range = true;
            }
        }
        else if (input_len == 4)
        {
            brk.address1 = (u16)std::stoul(text, 0, 16);
        }
        else
        {
            return false;
        }
    }
    catch(const std::invalid_argument&)
    {
        return false;
    }

    bool found = false;

    for (long unsigned int b = 0; b < m_breakpoints.size(); b++)
    {
        GG_Breakpoint* item = &m_breakpoints[b];

        if (brk.range)
        {
            if (item->range && (item->address1 == brk.address1) && (item->address2 == brk.address2))
            {
                found = true;
                break;
            }
        }
        else
        {
            if (!item->range && (item->address1 == brk.address1))
            {
                found = true;
                break;
            }
        }
    }

    if (!found)
        m_breakpoints.push_back(brk);

    return true;
}

bool HuC6280::AddBreakpoint(u16 address)
{
    char text[5];
    sprintf(text, "%04X", address);
    return AddBreakpoint(text, false, false, true);
}

void HuC6280::AddRunToBreakpoint(u16 address)
{
    m_run_to_breakpoint.address1 = address;
    m_run_to_breakpoint.address2 = 0;
    m_run_to_breakpoint.range = false;
    m_run_to_breakpoint.read = false;
    m_run_to_breakpoint.write = false;
    m_run_to_breakpoint.execute = true;
    m_run_to_breakpoint_requested = true;
}

std::vector<HuC6280::GG_Breakpoint>* HuC6280::GetBreakpoints()
{
    return &m_breakpoints;
}

void HuC6280::CheckBreakpoints()
{
#ifndef GG_DISABLE_DISASSEMBLER

    m_breakpoint_hit = false;

    for (int i = 0; i < (int)m_breakpoints.size(); i++)
    {
        GG_Breakpoint* brk = &m_breakpoints[i];

        if (!brk->execute)
            continue;

        if (brk->range)
        {
            if (m_PC.GetValue() >= brk->address1 && m_PC.GetValue() <= brk->address2)
            {
                m_breakpoint_hit = true;
                m_run_to_breakpoint_requested = false;
                return;
            }
        }
        else
        {
            if (m_PC.GetValue() == brk->address1)
            {
                m_breakpoint_hit = true;
                m_run_to_breakpoint_requested = false;
                return;
            }
        }
    }

    if (m_run_to_breakpoint_requested)
    {
        if (m_PC.GetValue() == m_run_to_breakpoint.address1)
        {
            m_breakpoint_hit = true;
            m_run_to_breakpoint_requested = false;
            return;
        }
    }

#endif
}
