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
    InitPointer(m_trace_logger);
    InitPointer(m_clock_hardware_fn);
    InitPointer(m_clock_hardware_context);
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
    RefreshBreakpointFlags();
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

void HuC6280::SetTraceLogger(TraceLogger* trace_logger)
{
    m_trace_logger = trace_logger;
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
    m_clocked_master_cycles = 0;
    m_extra_master_cycles = 0;
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
    RefreshBreakpointFlags();
}

static const u32 k_breakpoint_max_address[HuC6280::HuC6280_BREAKPOINT_TYPE_COUNT] =
{
    0xFFFF,   // CPU Address
    0x7FFF,   // VRAM
    0x01FF,   // Palette RAM
    0x0013,   // HuC6270 Reg
    0x0006,   // HuC6260 Reg
    0x7FFF,   // WRAM
    0x00FF,   // ZP
    0x27FFFF, // ROM
    0x2FFFF,  // Card RAM
    0xFFFF,   // CD RAM
    0x07FF,   // BRAM
};

bool HuC6280::GetBreakpointMaxAddress(const GG_Breakpoint& brk, u32& max_address)
{
    if (brk.type < 0 || brk.type >= HuC6280_BREAKPOINT_TYPE_COUNT)
        return false;

    switch (brk.type)
    {
        case HuC6280_BREAKPOINT_TYPE_WRAM:
        {
            int size = m_memory->GetWorkingRAMSize();
            if (size <= 0)
                return false;

            max_address = (u32)(size - 1);
            return true;
        }
        case HuC6280_BREAKPOINT_TYPE_ROM:
        {
            int size = m_memory->GetROMSize();
            if (size <= 0)
                return false;

            max_address = (u32)(size - 1);
            return true;
        }

        case HuC6280_BREAKPOINT_TYPE_CARD_RAM:
        {
            int size = m_memory->GetCardRAMSize();
            if (size <= 0)
                return false;

            max_address = (u32)(size - 1);
            return true;
        }

        case HuC6280_BREAKPOINT_TYPE_CDROM_RAM:
        {
            int size = m_memory->GetCDROMRAMSize();
            if (size <= 0)
                return false;

            max_address = (u32)(size - 1);
            return true;
        }

        default:
            max_address = k_breakpoint_max_address[brk.type];
            return true;
    }
}

bool HuC6280::BreakpointAddressValid(const GG_Breakpoint &brk)
{
    u32 max_address = 0;
    if (!GetBreakpointMaxAddress(brk, max_address))
        return false;

    if (brk.range)
    {
        if (brk.address1 > brk.address2)
            return false;

        if (brk.address1 > max_address || brk.address2 > max_address)
            return false;
    }
    else
    {
        if (brk.address1 > max_address)
            return false;
    }

    return true;
}

void HuC6280::RefreshBreakpointFlags()
{
    memset(m_breakpoint_cache, 0, sizeof(m_breakpoint_cache));
    memset(m_physical_breakpoint_cache, 0, sizeof(m_physical_breakpoint_cache));

    for (long unsigned int i = 0; i < m_breakpoints.size(); i++)
    {
        GG_Breakpoint* brk = &m_breakpoints[i];

        if (!brk->enabled)
            continue;

        if (brk->type < 0 || brk->type >= HuC6280_BREAKPOINT_TYPE_COUNT)
            continue;

        if (brk->read && BreakpointAccessSupported(brk->type, HuC6280_BREAKPOINT_ACCESS_READ))
            m_breakpoint_cache[brk->type][HuC6280_BREAKPOINT_ACCESS_READ] = true;

        if (brk->write && BreakpointAccessSupported(brk->type, HuC6280_BREAKPOINT_ACCESS_WRITE))
            m_breakpoint_cache[brk->type][HuC6280_BREAKPOINT_ACCESS_WRITE] = true;

        if (brk->execute && BreakpointAccessSupported(brk->type, HuC6280_BREAKPOINT_ACCESS_EXECUTE))
            m_breakpoint_cache[brk->type][HuC6280_BREAKPOINT_ACCESS_EXECUTE] = true;
    }

    m_physical_breakpoint_cache[HuC6280_BREAKPOINT_ACCESS_READ] =
        m_breakpoint_cache[HuC6280_BREAKPOINT_TYPE_WRAM][HuC6280_BREAKPOINT_ACCESS_READ] ||
        m_breakpoint_cache[HuC6280_BREAKPOINT_TYPE_ZERO_PAGE][HuC6280_BREAKPOINT_ACCESS_READ] ||
        m_breakpoint_cache[HuC6280_BREAKPOINT_TYPE_ROM][HuC6280_BREAKPOINT_ACCESS_READ] ||
        m_breakpoint_cache[HuC6280_BREAKPOINT_TYPE_CARD_RAM][HuC6280_BREAKPOINT_ACCESS_READ] ||
        m_breakpoint_cache[HuC6280_BREAKPOINT_TYPE_CDROM_RAM][HuC6280_BREAKPOINT_ACCESS_READ] ||
        m_breakpoint_cache[HuC6280_BREAKPOINT_TYPE_BACKUP_RAM][HuC6280_BREAKPOINT_ACCESS_READ];

    m_physical_breakpoint_cache[HuC6280_BREAKPOINT_ACCESS_WRITE] =
        m_breakpoint_cache[HuC6280_BREAKPOINT_TYPE_WRAM][HuC6280_BREAKPOINT_ACCESS_WRITE] ||
        m_breakpoint_cache[HuC6280_BREAKPOINT_TYPE_ZERO_PAGE][HuC6280_BREAKPOINT_ACCESS_WRITE] ||
        m_breakpoint_cache[HuC6280_BREAKPOINT_TYPE_CARD_RAM][HuC6280_BREAKPOINT_ACCESS_WRITE] ||
        m_breakpoint_cache[HuC6280_BREAKPOINT_TYPE_CDROM_RAM][HuC6280_BREAKPOINT_ACCESS_WRITE] ||
        m_breakpoint_cache[HuC6280_BREAKPOINT_TYPE_BACKUP_RAM][HuC6280_BREAKPOINT_ACCESS_WRITE];

    m_physical_breakpoint_cache[HuC6280_BREAKPOINT_ACCESS_EXECUTE] =
        m_breakpoint_cache[HuC6280_BREAKPOINT_TYPE_ROM][HuC6280_BREAKPOINT_ACCESS_EXECUTE] ||
        m_breakpoint_cache[HuC6280_BREAKPOINT_TYPE_CARD_RAM][HuC6280_BREAKPOINT_ACCESS_EXECUTE] ||
        m_breakpoint_cache[HuC6280_BREAKPOINT_TYPE_CDROM_RAM][HuC6280_BREAKPOINT_ACCESS_EXECUTE];
}

bool HuC6280::BreakpointAccessSupported(int type, GG_Breakpoint_Access access) const
{
    if (type < 0 || type >= HuC6280_BREAKPOINT_TYPE_COUNT)
        return false;

    switch (access)
    {
        case HuC6280_BREAKPOINT_ACCESS_READ:
            return true;

        case HuC6280_BREAKPOINT_ACCESS_WRITE:
            return type != HuC6280_BREAKPOINT_TYPE_ROM;

        case HuC6280_BREAKPOINT_ACCESS_EXECUTE:
            return type == HuC6280_BREAKPOINT_TYPE_CPU_ADDRESS ||
                type == HuC6280_BREAKPOINT_TYPE_ROM ||
                type == HuC6280_BREAKPOINT_TYPE_CARD_RAM ||
                type == HuC6280_BREAKPOINT_TYPE_CDROM_RAM;

        default:
            return false;
    }
}

bool HuC6280::BreakpointHasAccess(const GG_Breakpoint& brk, GG_Breakpoint_Access access) const
{
    switch (access)
    {
        case HuC6280_BREAKPOINT_ACCESS_READ:
            return brk.read;
        case HuC6280_BREAKPOINT_ACCESS_WRITE:
            return brk.write;
        case HuC6280_BREAKPOINT_ACCESS_EXECUTE:
            return brk.execute;
        default:
            return false;
    }
}

void HuC6280::SetBreakpointAccess(GG_Breakpoint& brk, GG_Breakpoint_Access access, bool enabled)
{
    switch (access)
    {
        case HuC6280_BREAKPOINT_ACCESS_READ:
            brk.read = enabled;
            break;
        case HuC6280_BREAKPOINT_ACCESS_WRITE:
            brk.write = enabled;
            break;
        case HuC6280_BREAKPOINT_ACCESS_EXECUTE:
            brk.execute = enabled;
            break;
        default:
            break;
    }
}

bool HuC6280::BreakpointAccessesValid(const GG_Breakpoint& brk) const
{
    bool active = false;

    if (brk.read)
    {
        if (!BreakpointAccessSupported(brk.type, HuC6280_BREAKPOINT_ACCESS_READ))
            return false;
        active = true;
    }

    if (brk.write)
    {
        if (!BreakpointAccessSupported(brk.type, HuC6280_BREAKPOINT_ACCESS_WRITE))
            return false;
        active = true;
    }

    if (brk.execute)
    {
        if (!BreakpointAccessSupported(brk.type, HuC6280_BREAKPOINT_ACCESS_EXECUTE))
            return false;
        active = true;
    }

    return active;
}

static bool parse_breakpoint_address_text(const char* text, u32& address1, u32& address2, bool& range)
{
    if (!IsValidPointer(text))
        return false;

    const char* dash = strchr(text, '-');

    if (dash == NULL)
    {
        // Single address
        if (!parse_hex_address(text, address1))
            return false;

        address2 = 0;
        range = false;
        return true;
    }

    // Range

    // Reject multiple dashes.
    if (strchr(dash + 1, '-') != NULL)
        return false;

    size_t left_len = dash - text;
    size_t right_len = strlen(dash + 1);

    if (left_len == 0 || right_len == 0)
        return false;

    char left[9];
    char right[9];

    if (left_len >= sizeof(left) || right_len >= sizeof(right))
        return false;

    memcpy(left, text, left_len);
    left[left_len] = 0;

    memcpy(right, dash + 1, right_len);
    right[right_len] = 0;

    if ((!parse_hex_address(left, address1)) ||
        (!parse_hex_address(right, address2)) ||
        (address1 > address2))
        return false;

    range = true;
    return true;
}

bool HuC6280::AddBreakpoint(int type, const char* text, bool read, bool write, bool execute)
{
    u32 address1 = 0;
    u32 address2 = 0;
    bool range = false;

    if (!parse_breakpoint_address_text(text, address1, address2, range))
        return false;

    return AddBreakpoint(type, address1, address2, range, read, write, execute);
}

bool HuC6280::AddBreakpoint(u16 address)
{
    return AddBreakpoint(
        HuC6280_BREAKPOINT_TYPE_CPU_ADDRESS,
        address, 0, false, false, false, true);
}

bool HuC6280::AddBreakpoint(int type, u32 address1, u32 address2,
    bool range, bool read, bool write, bool execute)
{
    GG_Breakpoint brk{};
    brk.enabled = true;
    brk.type = type;
    brk.address1 = address1;
    brk.address2 = range ? address2 : 0;
    brk.range = range;
    brk.read = read;
    brk.write = write;
    brk.execute = execute;

    if (!BreakpointAccessesValid(brk) || !BreakpointAddressValid(brk))
        return false;

    bool changed = false;

    for (long unsigned int b = 0; b < m_breakpoints.size(); b++)
    {
        GG_Breakpoint* item = &m_breakpoints[b];

        if (item->type != brk.type)
            continue;

        bool same_breakpoint = false;

        if (brk.range)
            same_breakpoint = item->range && item->address1 == brk.address1 && item->address2 == brk.address2;
        else
            same_breakpoint = !item->range && item->address1 == brk.address1;

        if (!same_breakpoint)
            continue;

        bool old_read = item->read;
        bool old_write = item->write;
        bool old_execute = item->execute;
        bool old_enabled = item->enabled;

        item->read |= brk.read;
        item->write |= brk.write;
        item->execute |= brk.execute;
        item->enabled = true;

        changed =
            item->read != old_read ||
            item->write != old_write ||
            item->execute != old_execute ||
            item->enabled != old_enabled;

        if (changed)
            RefreshBreakpointFlags();

        return true;
    }

    m_breakpoints.push_back(brk);
    RefreshBreakpointFlags();

    return true;
}

void HuC6280::AddRunToBreakpoint(u16 address)
{
    m_run_to_breakpoint.enabled = true;
    m_run_to_breakpoint.type = HuC6280_BREAKPOINT_TYPE_CPU_ADDRESS;
    m_run_to_breakpoint.address1 = address;
    m_run_to_breakpoint.address2 = 0;
    m_run_to_breakpoint.range = false;
    m_run_to_breakpoint.read = false;
    m_run_to_breakpoint.write = false;
    m_run_to_breakpoint.execute = true;
    m_run_to_breakpoint_requested = true;
}

bool HuC6280::RemoveBreakpoint(int type, u32 address)
{
    for (long unsigned int b = 0; b < m_breakpoints.size(); b++)
    {
        GG_Breakpoint* item = &m_breakpoints[b];

        if (!item->range && (item->address1 == address) && (item->type == type))
        {
            m_breakpoints.erase(m_breakpoints.begin() + b);
            RefreshBreakpointFlags();
            return true;
        }
    }

    return false;
}

bool HuC6280::RemoveBreakpointRange(int type, u32 address1, u32 address2)
{
    for (long unsigned int b = 0; b < m_breakpoints.size(); b++)
    {
        GG_Breakpoint* item = &m_breakpoints[b];

        if (item->range && item->address1 == address1 && item->address2 == address2 && item->type == type)
        {
            m_breakpoints.erase(m_breakpoints.begin() + b);
            RefreshBreakpointFlags();
            return true;
        }
    }

    return false;
}

bool HuC6280::RemoveBreakpointAt(int index)
{
    if (index < 0 || index >= (int)m_breakpoints.size())
        return false;

    m_breakpoints.erase(m_breakpoints.begin() + index);
    RefreshBreakpointFlags();
    return true;
}

bool HuC6280::SetBreakpointEnabled(int index, bool enabled)
{
    if (index < 0 || index >= (int)m_breakpoints.size())
        return false;

    if (m_breakpoints[index].enabled == enabled)
        return true;

    m_breakpoints[index].enabled = enabled;
    RefreshBreakpointFlags();
    return true;
}

bool HuC6280::ToggleBreakpointAccess(int index, GG_Breakpoint_Access access)
{
    if (index < 0 || index >= (int)m_breakpoints.size())
        return false;

    GG_Breakpoint& brk = m_breakpoints[index];

    if (!BreakpointAccessSupported(brk.type, access))
        return false;

    bool current = BreakpointHasAccess(brk, access);

    if (!brk.enabled)
    {
        brk.enabled = true;
        SetBreakpointAccess(brk, access, true);
        RefreshBreakpointFlags();
        return true;
    }

    int active_flags = 0;

    for (int i = 0; i < HuC6280_BREAKPOINT_ACCESS_COUNT; i++)
    {
        GG_Breakpoint_Access item = (GG_Breakpoint_Access)i;
        if (BreakpointAccessSupported(brk.type, item) && BreakpointHasAccess(brk, item))
            active_flags++;
    }

    if (current && active_flags <= 1)
    {
        brk.enabled = false;
        RefreshBreakpointFlags();
        return true;
    }

    SetBreakpointAccess(brk, access, !current);

    RefreshBreakpointFlags();
    return true;
}

bool HuC6280::IsBreakpoint(int type, u32 address)
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

bool HuC6280::IsBreakpointRange(int type, u32 address1, u32 address2)
{
    for (long unsigned int b = 0; b < m_breakpoints.size(); b++)
    {
        GG_Breakpoint* item = &m_breakpoints[b];

        if (item->range && item->address1 == address1 && item->address2 == address2 && item->type == type)
        {
            return true;
        }
    }

    return false;
}

void HuC6280::SetBreakpoints(const std::vector<GG_Breakpoint>& breakpoints)
{
    m_breakpoints.clear();

    for (long unsigned int i = 0; i < breakpoints.size(); i++)
    {
        const GG_Breakpoint& bp = breakpoints[i];

        if (BreakpointAccessesValid(bp) && BreakpointAddressValid(bp))
            m_breakpoints.push_back(bp);
    }

    RefreshBreakpointFlags();
}

void HuC6280::ClearDisassemblerCallStack()
{
    while(!m_disassembler_call_stack.empty())
        m_disassembler_call_stack.pop();
}

void HuC6280::CheckMemoryBreakpoints(int type, u32 address, bool read)
{
#if !defined(GG_DISABLE_DISASSEMBLER)

    if (!HasMemoryBreakpoints(type, read))
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