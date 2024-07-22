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
#include "huc6280_timing.h"
#include "huc6280_names.h"
#include "memory.h"
#include <stdlib.h>
#include <string.h>

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

void HuC6280::Init(Memory* memory)
{
    m_memory = memory;
}

void HuC6280::Reset()
{
    m_PC.SetLow(m_memory->Read(0xFFFE));
    m_PC.SetHigh(m_memory->Read(0xFFFF));
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
    }
    else if (!IsSetFlag(FLAG_INTERRUPT))
    {
        // TIQ
        if (m_timer_irq && !IsSetBit(m_interrupt_disable_register, 2))
        {
            irq = true;
            irq_low = 0xFFFA;
            irq_high = 0xFFFB;
        }
        // IRQ1
        else if (m_irq1_asserted && !IsSetBit(m_interrupt_disable_register, 1))
        {
            irq = true;
            irq_low = 0xFFF8;
            irq_high = 0xFFF9;
        }
        // IRQ2
        else if (m_irq2_asserted && !IsSetBit(m_interrupt_disable_register, 0))
        {
            irq = true;
            irq_low = 0xFFF6;
            irq_high = 0xFFF7;
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
                Debug("Timer counter underflow, IRQ, reload: %02X", m_timer_reload);
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
        return;

    record->size = opcode_size;
    record->address = m_memory->GetPhysicalAddress(address);
    record->bank = m_memory->GetBank(address);
    record->name[0] = 0;
    record->bytes[0] = 0;
    record->jump = false;
    record->jump_address = 0;

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
            snprintf(record->name, 64, k_opcode_names[opcode].name, jump_address, rel);
            break;
        }
        case GG_OPCode_Type_1b_1b_Relative:
        {
            u8 zero_page = m_memory->Read(address + 1);
            s8 rel = m_memory->Read(address + 2);
            u16 jump_address = address + 3 + rel;
            snprintf(record->name, 64, k_opcode_names[opcode].name, zero_page, jump_address, rel);
            break;
        }
        default:
        {
            break;
        }   
    }

    if (record->bank < 0xF7)
    {
        strncpy(record->segment, "ROM ", 5);
    }
    else if (record->bank >= 0xF8 && record->bank < 0xFC)
    {
        strncpy(record->segment, "RAM ", 5);
    }
    else
    {
        strncpy(record->segment, "????", 5);
    }
#endif
}
