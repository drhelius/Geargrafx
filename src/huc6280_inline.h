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

#include "huc6280.h"
#include "memory.h"

inline bool HuC6280::Clock()
{
    ClockTimer();

    bool instruction_completed = false;

    if (m_clock % k_huc6280_speed_divisor[m_speed] == 0)
    {
        if (m_clock_cycles <= 0)
        {
            if (m_irq_pending != 0)
                m_clock_cycles += TickIRQ();
            else
                m_clock_cycles += TickOPCode();
        }

        m_clock_cycles--;
        instruction_completed = (m_clock_cycles == 0);
    }

    m_clock = (m_clock + 1) % 12;

    return instruction_completed;
}

inline void HuC6280::AssertIRQ1(bool asserted)
{
    if (m_after_cli && m_irq1_asserted && !asserted)
        m_force_irq1 = true;

    m_irq1_asserted = asserted;

    if (m_irq1_asserted)
        m_interrupt_request_register = SetBit(m_interrupt_request_register, 1);
    else
        m_interrupt_request_register = UnsetBit(m_interrupt_request_register, 1);
}

inline void HuC6280::AssertIRQ2(bool asserted)
{
    if (m_after_cli && m_irq2_asserted && !asserted)
        m_force_irq2 = true;

    m_irq2_asserted = asserted;

    if (m_irq2_asserted)
        m_interrupt_request_register = SetBit(m_interrupt_request_register, 0);
    else
        m_interrupt_request_register = UnsetBit(m_interrupt_request_register, 0);
}

inline void HuC6280::InjectCycles(unsigned int cycles)
{
    m_cycles += cycles;
}

inline u8 HuC6280:: ReadInterruptRegister(u32 address)
{
    if ((address & 1) == 0)
    {
        // Acknowledge TIQ
        m_interrupt_request_register = UnsetBit(m_interrupt_request_register, 2);
        m_timer_irq = false;
        return m_interrupt_disable_register;
    }
    else
    {
        return m_interrupt_request_register;
    }
}

inline void HuC6280::WriteInterruptRegister(u32 address, u8 value)
{
    if ((address & 1) == 0)
    {
        m_interrupt_disable_register = value & 0x07;
    }
    else
    {
        // Acknowledge TIQ
        m_interrupt_request_register = UnsetBit(m_interrupt_request_register, 2);
        m_timer_irq = false;
    }
}

inline void HuC6280::ClockTimer()
{
    m_timer_cycles++;

    if(m_timer_reload_requested)
    {
        m_timer_counter = m_timer_reload;
        m_timer_reload_requested = false;
        return;
    }

    if (m_timer_cycles >= k_huc6280_timer_divisor)
    {
        m_timer_cycles = 0;

        if (m_timer_enabled)
        {
            m_timer_counter--;

            if (m_timer_counter == 0xFF)
            {
                m_timer_reload_requested = true;
                m_timer_irq = true;
                m_interrupt_request_register = SetBit(m_interrupt_request_register, 2);
            }
        }
    }
}

inline u8 HuC6280::ReadTimerRegister()
{
    // Debug("Timer register read (counter): %02X", m_timer_counter);
    return m_timer_counter;
}

inline void HuC6280::WriteTimerRegister(u32 address, u8 value)
{
    // Debug("Timer register write at %06X, value=%02X", address, value);

    switch (address & 0x01)
    {
        case 0:
        {
            m_timer_reload = value & 0x7F;
            // Debug("Timer reload: %02X", m_timer_reload);
            break;
        }
        case 1:
        {
            bool enabled = (value & 0x01);
            if (!m_timer_enabled && enabled)
            {
                m_timer_counter = m_timer_reload;
                m_timer_cycles = 0;
                m_timer_reload_requested = false;
                // Debug("Timer reload when enabled: %02X", m_timer_reload);
            }
            m_timer_enabled = enabled;
            // Debug("Timer enabled: %s", m_timer_enabled ? "true" : "false");
            break;
        }
        default:
            Debug("Invalid timer register write at %06X, value=%02X", address, value);
            break;
    }
}

inline u8 HuC6280::Fetch8()
{
    u8 value = m_memory->Read(m_PC.GetValue());
    m_PC.Increment();
    return value;
}

inline u16 HuC6280::Fetch16()
{
    u16 pc = m_PC.GetValue();
    u8 l = m_memory->Read(pc);
    u8 h = m_memory->Read(pc + 1);
    m_PC.SetValue(pc + 2);
    return Address16(h , l);
}

inline u16 HuC6280::Address16(u8 high, u8 low)
{
    return static_cast<u16>(high << 8 ) | low;
}

inline bool HuC6280::PageCrossed(u16 old_address, u16 new_address)
{
    return (old_address ^ new_address) > 0x00FF;
}

inline u16 HuC6280::ZeroPageX()
{
    return ZERO_PAGE_ADDR | m_X.GetValue();
}

inline void HuC6280::SetOrClearZNFlags(u8 result)
{
    ClearFlag(FLAG_ZERO | FLAG_NEGATIVE);
    m_P.SetValue(m_P.GetValue() | m_zn_flags_lut[result]);
}

inline void HuC6280::SetZNFlags(u8 result)
{
    m_P.SetValue(m_P.GetValue() | m_zn_flags_lut[result]);
}

inline void HuC6280::SetOverflowFlag(u8 result)
{
    m_P.SetValue((m_P.GetValue() & 0xBF) | (result & 0x40));
}

inline void HuC6280::SetFlag(u8 flag)
{
    m_P.SetValue(m_P.GetValue() | flag);
}

inline void HuC6280::ClearFlag(u8 flag)
{
    m_P.SetValue(m_P.GetValue() & (~flag));
}

inline bool HuC6280::IsSetFlag(u8 flag)
{
    return (m_P.GetValue() & flag) != 0;
}

inline void HuC6280::StackPush16(u16 value)
{
    m_memory->Write(STACK_ADDR | m_S.GetValue(), static_cast<u8>(value >> 8));
    m_S.Decrement();
    m_memory->Write(STACK_ADDR | m_S.GetValue(), static_cast<u8>(value & 0x00FF));
    m_S.Decrement();
}

inline void HuC6280::StackPush8(u8 value)
{
    m_memory->Write(STACK_ADDR | m_S.GetValue(), value);
    m_S.Decrement();
}

inline u16 HuC6280::StackPop16()
{
    m_S.Increment();
    u8 l = m_memory->Read(STACK_ADDR | m_S.GetValue());
    m_S.Increment();
    u8 h = m_memory->Read(STACK_ADDR | m_S.GetValue());
    return Address16(h , l);
}

inline u8 HuC6280::StackPop8()
{
    m_S.Increment();
    return m_memory->Read(STACK_ADDR | m_S.GetValue());
}

inline u8 HuC6280::ImmediateAddressing()
{
    return Fetch8();
}

inline u16 HuC6280::ZeroPageAddressing()
{
    return ZERO_PAGE_ADDR | Fetch8();
}

inline u16 HuC6280::ZeroPageAddressing(EightBitRegister* reg)
{
    return ZERO_PAGE_ADDR | ((Fetch8() + reg->GetValue()) & 0xFF);
}

inline u16 HuC6280::ZeroPageRelativeAddressing()
{
    u16 address = ZeroPageAddressing();
    s8 offset = static_cast<s8>(Fetch8());
    return address + offset;
}

inline u16 HuC6280::ZeroPageIndirectAddressing()
{
    u16 address = ZeroPageAddressing();
    u8 l = m_memory->Read(address);
    u8 h = m_memory->Read((address + 1) & 0x20FF);
    return Address16(h, l);
}

inline u16 HuC6280::ZeroPageIndexedIndirectAddressing()
{
    u16 address = (ZeroPageAddressing() + m_X.GetValue()) & 0x20FF;
    u8 l = m_memory->Read(address);
    u8 h = m_memory->Read((address + 1) & 0x20FF);
    return Address16(h, l);
}

inline u16 HuC6280::ZeroPageIndirectIndexedAddressing()
{
    u16 address = ZeroPageAddressing();
    u8 l = m_memory->Read(address);
    u8 h = m_memory->Read((address + 1) & 0x20FF);
    return Address16(h, l) + m_Y.GetValue();
}

inline s8 HuC6280::RelativeAddressing()
{
    return static_cast<s8>(Fetch8());
}

inline u16 HuC6280::AbsoluteAddressing()
{
    return Fetch16();
}

inline u16 HuC6280::AbsoluteAddressing(EightBitRegister* reg)
{
    u16 address = Fetch16();
    u16 result = address + reg->GetValue();
    return result;
}

inline u16 HuC6280::AbsoluteIndirectAddressing()
{
    u16 address = Fetch16();
    u8 l = m_memory->Read(address);
    u8 h = m_memory->Read(address + 1);
    return Address16(h, l);
}

inline u16 HuC6280::AbsoluteIndexedIndirectAddressing()
{
    u16 address = Fetch16() + m_X.GetValue();
    u8 l = m_memory->Read(address);
    u8 h = m_memory->Read(address + 1);
    return Address16(h, l);
}

#endif /* HUC6280_INLINE_H */