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
#include <assert.h>
#include "huc6280.h"
#include "memory.h"

HuC6280::HuC6280()
{
    InitOPCodeFunctors();
    m_breakpoints_enabled = false;
    m_breakpoints_irq_enabled = false;
    m_reset_value = -1;
    m_processor_state.A = &m_A;
    m_processor_state.X = &m_X;
    m_processor_state.Y = &m_Y;
    m_processor_state.S = &m_S;
    m_processor_state.P = &m_P;
    m_processor_state.PC = &m_PC;
    m_processor_state.SPEED = &m_speed;
    m_processor_state.TIMER = &m_timer_enabled;
    m_processor_state.TIMER_COUNTER = &m_timer_counter;
    m_processor_state.TIMER_RELOAD = &m_timer_reload;
    m_processor_state.IDR = &m_interrupt_disable_register;
    m_processor_state.IRR = &m_interrupt_request_register;
}

HuC6280::~HuC6280()
{
}

void HuC6280::Init(Memory* memory, HuC6202* huc6202)
{
    m_memory = memory;
    m_huc6202 = huc6202;
    CreateZNFlagsTable();
}

void HuC6280::Reset()
{
    m_PC.SetLow(m_memory->Read(0xFFFE));
    m_PC.SetHigh(m_memory->Read(0xFFFF));
    m_debug_next_irq = 1;
    DisassembleNextOPCode();

    if (m_reset_value < 0)
    {
        m_A.SetValue(rand() & 0xFF);
        m_X.SetValue(rand() & 0xFF);
        m_Y.SetValue(rand() & 0xFF);
        m_S.SetValue(rand() & 0xFF);
        m_P.SetValue(rand() & 0xFF);
    }
    else
    {
        m_A.SetValue(m_reset_value & 0xFF);
        m_X.SetValue(m_reset_value & 0xFF);
        m_Y.SetValue(m_reset_value & 0xFF);
        m_S.SetValue(m_reset_value & 0xFF);
        m_P.SetValue(m_reset_value & 0xFF);
    }

#if defined(GG_TESTING)
    SetFlag(FLAG_TRANSFER);
#else
    ClearFlag(FLAG_TRANSFER);
#endif
    ClearFlag(FLAG_DECIMAL);
    SetFlag(FLAG_INTERRUPT);
    ClearFlag(FLAG_BREAK);
    m_cycles = 0;
    m_irq_pending = 0;
    m_speed = 0;
    m_transfer_state = 0;
    m_transfer_count = 0;
    m_transfer_length = 0;
    m_transfer_source = 0;
    m_transfer_dest = 0;
    m_timer_cycles = 0;
    m_timer_enabled = false;
    m_timer_counter = 0;
    m_timer_reload = 0;
    m_interrupt_disable_register = 0;
    m_interrupt_request_register = 0;
    m_transfer_flag = false;
    m_cpu_breakpoint_hit = false;
    m_memory_breakpoint_hit = false;
    m_run_to_breakpoint_hit = false;
    m_run_to_breakpoint_requested = false;
    ClearDisassemblerCallStack();
}

HuC6280::HuC6280_State* HuC6280::GetState()
{
    return &m_processor_state;
}

void HuC6280::SetResetValue(int value)
{
    m_reset_value = value;
}

void HuC6280::EnableBreakpoints(bool enable, bool irqs)
{
    m_breakpoints_enabled = enable;
    m_breakpoints_irq_enabled = irqs;
}

void HuC6280::ResetBreakpoints()
{
    m_breakpoints.clear();
}

bool HuC6280::AddBreakpoint(int type, char* text, bool read, bool write, bool execute)
{
    int input_len = (int)strlen(text);
    GG_Breakpoint brk;
    brk.enabled = true;
    brk.type = type;
    brk.address1 = 0;
    brk.address2 = 0;
    brk.range = false;
    brk.read = read;
    brk.write = write;
    brk.execute = execute;

    if (!read && !write && !execute)
        return false;

    if ((input_len == 9) && (text[4] == '-'))
    {
        // format: AAAA-BBBB
        if (parse_hex_string(text, 4, &brk.address1) &&
            parse_hex_string(text + 5, 4, &brk.address2))
        {
            brk.range = true;
        }
        else
        {
            return false;
        }
    }
    else if ((input_len > 0) && (input_len <= 4))
    {
        // format: AAAA
        if (!parse_hex_string(text, input_len, &brk.address1))
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    bool found = false;

    for (long unsigned int b = 0; b < m_breakpoints.size(); b++)
    {
        GG_Breakpoint* item = &m_breakpoints[b];

        if (item->type != brk.type)
            continue;

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
    char text[6];
    snprintf(text, 6, "%04X", address);
    return AddBreakpoint(HuC6280_BREAKPOINT_TYPE_ROMRAM, text, false, false, true);
}

void HuC6280::AddRunToBreakpoint(u16 address)
{
    m_run_to_breakpoint.enabled = true;
    m_run_to_breakpoint.type = HuC6280_BREAKPOINT_TYPE_ROMRAM;
    m_run_to_breakpoint.address1 = address;
    m_run_to_breakpoint.address2 = 0;
    m_run_to_breakpoint.range = false;
    m_run_to_breakpoint.read = false;
    m_run_to_breakpoint.write = false;
    m_run_to_breakpoint.execute = true;
    m_run_to_breakpoint_requested = true;
}

void HuC6280::RemoveBreakpoint(int type, u16 address)
{
    for (long unsigned int b = 0; b < m_breakpoints.size(); b++)
    {
        GG_Breakpoint* item = &m_breakpoints[b];

        if (!item->range && (item->address1 == address) && (item->type == type))
        {
            m_breakpoints.erase(m_breakpoints.begin() + b);
            break;
        }
    }
}

bool HuC6280::IsBreakpoint(int type, u16 address)
{
    for (long unsigned int b = 0; b < m_breakpoints.size(); b++)
    {
        GG_Breakpoint* item = &m_breakpoints[b];

        if (!item->range && (item->address1 == address) && (item->type == type))
        {
            return true;
        }
    }

    return false;
}

void HuC6280::ClearDisassemblerCallStack()
{
    while(!m_disassembler_call_stack.empty())
        m_disassembler_call_stack.pop();
}

void HuC6280::CheckMemoryBreakpoints(int type, u16 address, bool read)
{
#if !defined(GG_DISABLE_DISASSEMBLER)

    if (!m_breakpoints_enabled)
        return;

    for (int i = 0; i < (int)m_breakpoints.size(); i++)
    {
        GG_Breakpoint* brk = &m_breakpoints[i];

        if (!brk->enabled)
            continue;
        if (brk->type != type)
            continue;
        if (read && !brk->read)
            continue;
        if (!read && !brk->write)
            continue;

        if (brk->range)
        {
            if (address >= brk->address1 && address <= brk->address2)
            {
                m_memory_breakpoint_hit = true;
                m_run_to_breakpoint_requested = false;
                return;
            }
        }
        else
        {
            if (address == brk->address1)
            {
                m_memory_breakpoint_hit = true;
                m_run_to_breakpoint_requested = false;
                return;
            }
        }
    }
#else
    UNUSED(type);
    UNUSED(address);
    UNUSED(read);
#endif
}

void HuC6280::CreateZNFlagsTable()
{
    for (int i = 0; i < 256; i++)
    {
        m_zn_flags_lut[i] = 0;

        if (i == 0)
            m_zn_flags_lut[i] |= FLAG_ZERO;
        if (i & 0x80)
            m_zn_flags_lut[i] |= FLAG_NEGATIVE;
    }
}

void HuC6280::SaveState(std::ostream& stream)
{
    m_PC.SaveState(stream);
    m_A.SaveState(stream);
    m_X.SaveState(stream);
    m_Y.SaveState(stream);
    m_S.SaveState(stream);
    m_P.SaveState(stream);

    stream.write(reinterpret_cast<const char*> (&m_cycles), sizeof(m_cycles));
    stream.write(reinterpret_cast<const char*> (&m_irq_pending), sizeof(m_irq_pending));
    stream.write(reinterpret_cast<const char*> (&m_speed), sizeof(m_speed));
    stream.write(reinterpret_cast<const char*> (&m_transfer_state), sizeof(m_transfer_state));
    stream.write(reinterpret_cast<const char*> (&m_transfer_count), sizeof(m_transfer_count));
    stream.write(reinterpret_cast<const char*> (&m_transfer_length), sizeof(m_transfer_length));
    stream.write(reinterpret_cast<const char*> (&m_transfer_source), sizeof(m_transfer_source));
    stream.write(reinterpret_cast<const char*> (&m_transfer_dest), sizeof(m_transfer_dest));
    stream.write(reinterpret_cast<const char*> (&m_timer_enabled), sizeof(m_timer_enabled));
    stream.write(reinterpret_cast<const char*> (&m_timer_cycles), sizeof(m_timer_cycles));
    stream.write(reinterpret_cast<const char*> (&m_timer_counter), sizeof(m_timer_counter));
    stream.write(reinterpret_cast<const char*> (&m_timer_reload), sizeof(m_timer_reload));
    stream.write(reinterpret_cast<const char*> (&m_interrupt_disable_register), sizeof(m_interrupt_disable_register));
    stream.write(reinterpret_cast<const char*> (&m_interrupt_request_register), sizeof(m_interrupt_request_register));
    stream.write(reinterpret_cast<const char*> (&m_transfer_flag), sizeof(m_transfer_flag));
    stream.write(reinterpret_cast<const char*> (&m_debug_next_irq), sizeof(m_debug_next_irq));
}

void HuC6280::LoadState(std::istream& stream)
{
    m_PC.LoadState(stream);
    m_A.LoadState(stream);
    m_X.LoadState(stream);
    m_Y.LoadState(stream);
    m_S.LoadState(stream);
    m_P.LoadState(stream);

    stream.read(reinterpret_cast<char*> (&m_cycles), sizeof(m_cycles));
    stream.read(reinterpret_cast<char*> (&m_irq_pending), sizeof(m_irq_pending));
    stream.read(reinterpret_cast<char*> (&m_speed), sizeof(m_speed));
    stream.read(reinterpret_cast<char*> (&m_transfer_state), sizeof(m_transfer_state));
    stream.read(reinterpret_cast<char*> (&m_transfer_count), sizeof(m_transfer_count));
    stream.read(reinterpret_cast<char*> (&m_transfer_length), sizeof(m_transfer_length));
    stream.read(reinterpret_cast<char*> (&m_transfer_source), sizeof(m_transfer_source));
    stream.read(reinterpret_cast<char*> (&m_transfer_dest), sizeof(m_transfer_dest));
    stream.read(reinterpret_cast<char*> (&m_timer_enabled), sizeof(m_timer_enabled));
    stream.read(reinterpret_cast<char*> (&m_timer_cycles), sizeof(m_timer_cycles));
    stream.read(reinterpret_cast<char*> (&m_timer_counter), sizeof(m_timer_counter));
    stream.read(reinterpret_cast<char*> (&m_timer_reload), sizeof(m_timer_reload));
    stream.read(reinterpret_cast<char*> (&m_interrupt_disable_register), sizeof(m_interrupt_disable_register));
    stream.read(reinterpret_cast<char*> (&m_interrupt_request_register), sizeof(m_interrupt_request_register));
    stream.read(reinterpret_cast<char*> (&m_transfer_flag), sizeof(m_transfer_flag));
    stream.read(reinterpret_cast<char*> (&m_debug_next_irq), sizeof(m_debug_next_irq));
}