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

HuC6280::HuC6280(Memory* memory)
{
    m_memory = memory;
    InitOPCodeFunctors();
    m_t_states = 0;
    m_interrupt_asserted = false;
    m_nmi_interrupt_requested = false;
    m_high_speed = false;
    m_processor_state.A = &m_A;
    m_processor_state.X = &m_X;
    m_processor_state.Y = &m_Y;
    m_processor_state.S = &m_S;
    m_processor_state.P = &m_P;
    m_processor_state.PC = &m_PC;
    m_processor_state.SPEED = &m_high_speed;
}

HuC6280::~HuC6280()
{
}

void HuC6280::Init()
{
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
    m_t_states = 0;
    m_interrupt_asserted = false;
    m_nmi_interrupt_requested = false;
    m_high_speed = false;
}

unsigned int HuC6280::RunFor(unsigned int t_states)
{
    unsigned int count = 0;

    while (count < t_states)
    {
        count += Tick();
    }

    return count;
}

unsigned int HuC6280::Tick()
{
    m_t_states = 0;

    if (m_nmi_interrupt_requested)
    {
        m_nmi_interrupt_requested = false;
        StackPush16(m_PC.GetValue());
        ClearFlag(FLAG_BREAK);
        StackPush8(m_P.GetValue());
        SetFlag(FLAG_INTERRUPT);
        m_PC.SetLow(m_memory->Read(0xFFFA));
        m_PC.SetHigh(m_memory->Read(0xFFFB));
        m_t_states += 7;
        DisassembleNextOPCode();
        return m_t_states;
    }
    else if (!IsSetFlag(FLAG_INTERRUPT) && m_interrupt_asserted)
    { 
        StackPush16(m_PC.GetValue());
        ClearFlag(FLAG_BREAK);
        StackPush8(m_P.GetValue());
        SetFlag(FLAG_INTERRUPT);
        m_PC.SetLow(m_memory->Read(0xFFFE));
        m_PC.SetHigh(m_memory->Read(0xFFFF));
        m_t_states += 7;
        DisassembleNextOPCode();
        return m_t_states;
    } 

    u8 opcode = Fetch8();
    (this->*m_opcodes[opcode])();
    DisassembleNextOPCode();

    m_t_states += k_opcode_tstates[opcode];

    return m_t_states;
}

HuC6280::Processor_State* HuC6280::GetState()
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
        strncat(record->bytes, value, 20);
        strncat(record->bytes, " ", 20);
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

    if (record->bank < 0x80)
    {
        strncpy(record->segment, "ROM ", 5);
    }
    else if (record->bank < 0xFC && record->bank >= 0xF8)
    {
        strncpy(record->segment, "WRAM", 5);
    }
    else
    {
        strncpy(record->segment, "????", 5);
    }
#endif
}
