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

HuC6280::HuC6280(Memory* memory)
{
    m_memory = memory;
    InitOPCodeFunctors();
    m_t_states = 0;
    m_interrupt_asserted = false;
    m_nmi_interrupt_requested = false;
    m_page_crossed = false;
    m_branch_taken = false;
}

HuC6280::~HuC6280()
{
}

void HuC6280::Init()
{
}

void HuC6280::Reset()
{
    m_PC.SetLow(m_memory->Read(0xFFFC));
    m_PC.SetHigh(m_memory->Read(0xFFFD));
    m_A.SetValue(0x00);
    m_X.SetValue(0x00);
    m_Y.SetValue(0x00);
    m_S.SetValue(0xFD);
    m_P.SetValue(0x34);
    m_t_states = 0;
    m_interrupt_asserted = false;
    m_nmi_interrupt_requested = false;
    m_page_crossed = false;
    m_branch_taken = false;
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
    m_page_crossed = false;
    m_branch_taken = false;
    
    if (m_nmi_interrupt_requested)
    {
        m_nmi_interrupt_requested = false;
        StackPush16(m_PC.GetValue());
        ClearFlag(FLAG_BRK);
        StackPush8(m_P.GetValue());
        SetFlag(FLAG_IRQ);
        m_PC.SetLow(m_memory->Read(0xFFFA));
        m_PC.SetHigh(m_memory->Read(0xFFFB));
        m_t_states += 7;
        return m_t_states;
    }
    else if (!IsSetFlag(FLAG_IRQ) && m_interrupt_asserted)
    { 
        StackPush16(m_PC.GetValue());
        ClearFlag(FLAG_BRK);
        StackPush8(m_P.GetValue());
        SetFlag(FLAG_IRQ);
        m_PC.SetLow(m_memory->Read(0xFFFE));
        m_PC.SetHigh(m_memory->Read(0xFFFF));
        m_t_states += 7;
        return m_t_states;
    } 

    u8 opcode = Fetch8();

#ifdef HuC6280_DISASM
    {
        u16 opcode_address = m_PC.GetValue() - 1;

        if (!m_memory->IsDisassembled(opcode_address))
        {
            m_memory->Disassemble(opcode_address, kOPCodeNames[opcode]);
        }
    }
#endif

#ifdef HuC6280_DEBUG
    {
        u16 opcode_address = m_PC.GetValue() - 1;
        printf("HuC6280 -->  $%.4X  %s\n", opcode_address, kOPCodeNames[opcode]);
    }
#endif

    (this->*m_opcodes[opcode])();

    m_t_states += k_opcode_tstates[opcode];
    m_t_states += m_page_crossed ? k_opcode_tstates_cross_paged[opcode] : 0;
    m_t_states += m_branch_taken ? 1 : 0;

    return m_t_states;
}

void HuC6280::UnofficialOPCode()
{
    ClearFlag(FLAG_MEMORY);
    u16 opcode_address = m_PC.GetValue() - 1;
    u8 opcode = m_memory->Read(opcode_address);
    Debug("HuC6280 --> ** UNOFFICIAL OP Code (%X) at $%.4X -- %s\n", opcode, opcode_address, k_opcode_names[opcode]);
}
