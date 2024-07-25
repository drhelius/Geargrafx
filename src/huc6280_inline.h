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
    if (m_clock_cycles == 0)
        m_clock_cycles = Tick();

    m_clock_cycles--;

    return m_clock_cycles == 0;
}

inline void HuC6280::AssertIRQ1(bool asserted)
{
    // Debug("IRQ1 asserted: %s", asserted ? "true" : "false");
    m_irq1_asserted = asserted;
    if (m_irq1_asserted)
        SetBit(m_interrupt_request_register, 1);
    else
        UnsetBit(m_interrupt_request_register, 1);
}

inline void HuC6280::AssertIRQ2(bool asserted)
{
    Debug("IRQ2 asserted: %s", asserted ? "true" : "false");
    m_irq2_asserted = asserted;
    if (m_irq2_asserted)
        SetBit(m_interrupt_request_register, 0);
    else
        UnsetBit(m_interrupt_request_register, 0);
}

inline void HuC6280::RequestNMI()
{
    Debug("NMI requested");
    m_nmi_requested = true;
}

inline bool HuC6280::IsHighSpeed()
{
    return m_high_speed;
}

inline void HuC6280::InjectCycles(unsigned int cycles)
{
    m_cycles += cycles;
}

inline u8 HuC6280:: ReadInterruptRegister(u32 address)
{
    // Debug("Interrupt register read at %06X", address);
    if (address == 0x1FF402)
    {
        // Debug("Interrupt disable register: %02X", m_interrupt_disable_register);
        return m_interrupt_disable_register;
    }
    else if (address == 0x1FF403)
    {
        // Debug("Interrupt request register: %02X", m_interrupt_request_register);
        return m_interrupt_request_register;
    }
    else
    {
        Debug("Invalid interrupt register read at %06X", address);
        return 0;
    }
}

inline void HuC6280::WriteInterruptRegister(u32 address, u8 value)
{
    // Debug("Interrupt register write at %06X, value=%02X", address, value);
    if (address == 0x1FF402)
    {
        m_interrupt_disable_register = value & 0x07;
        // Debug("Interrupt disable register: %02X", m_interrupt_disable_register);
    }
    else if (address == 0x1FF403)
    {
        // Acknowledge TIQ
        UnsetBit(m_interrupt_request_register, 2);
        m_timer_irq = false;
        // Debug("Interrupt request register: %02X", m_interrupt_request_register);
    }
    else
    {
        Debug("Invalid interrupt register write at %06X, value=%02X", address, value);
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
    return 0x2000 | m_X.GetValue();
}

inline void HuC6280::SetZeroFlagFromResult(u8 result)
{
    if (result == 0)
        SetFlag(FLAG_ZERO);
    else
        ClearFlag(FLAG_ZERO);
}

inline void HuC6280::SetOverflowFlagFromResult(u8 result)
{
    m_P.SetValue((m_P.GetValue() & 0xBF) | (result & 0x40));
}

inline void HuC6280::SetNegativeFlagFromResult(u8 result)
{
    m_P.SetValue((m_P.GetValue() & 0x7F) | (result & 0x80));
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
    m_memory->Write(0x2100 | m_S.GetValue(), static_cast<u8>((value >> 8) & 0x00FF));
    m_S.Decrement();
    m_memory->Write(0x2100 | m_S.GetValue(), static_cast<u8>(value & 0x00FF));
    m_S.Decrement();
}

inline void HuC6280::StackPush8(u8 value)
{
    m_memory->Write(0x2100 | m_S.GetValue(), value);
    m_S.Decrement();
}

inline u16 HuC6280::StackPop16()
{
    m_S.Increment();
    u8 l = m_memory->Read(0x2100 | m_S.GetValue());
    m_S.Increment();
    u8 h = m_memory->Read(0x2100 | m_S.GetValue());
    return Address16(h , l);
}

inline u8 HuC6280::StackPop8()
{
    m_S.Increment();
    u8 result = m_memory->Read(0x2100 | m_S.GetValue());
    return result;
}

inline u8 HuC6280::ImmediateAddressing()
{
    return Fetch8();
}

inline u16 HuC6280::ZeroPageAddressing()
{
    return 0x2000 | Fetch8();
}

inline u16 HuC6280::ZeroPageAddressing(EightBitRegister* reg)
{
    return 0x2000 | ((Fetch8() + reg->GetValue()) & 0xFF);
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
    address = Address16(h, l);
    u16 result = address + m_Y.GetValue();
    return result;
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