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

#ifndef HUC6280_REGISTERS_H
#define HUC6280_REGISTERS_H

#include "common.h"

class EightBitRegister
{
public:
    EightBitRegister() : m_value(0) { }
    u8 GetValue() const;
    void SetValue(u8 value);
    void Increment();
    void Increment(u8 value);
    void Decrement();
    void Decrement(u8 value);
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);

private:
    u8 m_value;
};

inline u8 EightBitRegister::GetValue() const
{
    return m_value;
}

inline void EightBitRegister::SetValue(u8 value)
{
    m_value = value;
}

inline void EightBitRegister::Increment()
{
    m_value++;
}

inline void EightBitRegister::Increment(u8 value)
{
    m_value += value;
}

inline void EightBitRegister::Decrement()
{
    m_value--;
}

inline void EightBitRegister::Decrement(u8 value)
{
    m_value -= value;
}

inline void EightBitRegister::SaveState(std::ostream& stream)
{
    stream.write(reinterpret_cast<const char*> (&m_value), sizeof(m_value));
}

inline void EightBitRegister::LoadState(std::istream& stream)
{
    stream.read(reinterpret_cast<char*> (&m_value), sizeof(m_value));
}

//////////////////////////////////////////////////////////////////////////

class SixteenBitRegister
{
public:
    SixteenBitRegister() : m_value(0) { }
    u8 GetLow() const;
    u8 GetHigh() const;
    u16 GetValue() const;
    void SetLow(u8 low);
    void SetHigh(u8 high);
    void SetValue(u16 value);
    void Increment();
    void Increment(u16 value);
    void Decrement();
    void Decrement(u16 value);
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);

private:
    u16 m_value;
};

inline u8 SixteenBitRegister::GetLow() const
{
    return static_cast<u8>(m_value & 0x00FF);
}

inline u8 SixteenBitRegister::GetHigh() const
{
    return static_cast<u8>((m_value >> 8) & 0x00FF);
}

inline u16 SixteenBitRegister::GetValue() const
{
    return m_value;
}

inline void SixteenBitRegister::SetLow(u8 low)
{
    m_value = (m_value & 0xFF00) | low;
}

inline void SixteenBitRegister::SetHigh(u8 high)
{
    m_value = static_cast<u16>(high << 8) | (m_value & 0x00FF);
}

inline void SixteenBitRegister::SetValue(u16 value)
{
    m_value = value;
}

inline void SixteenBitRegister::Increment()
{
    m_value++;
}

inline void SixteenBitRegister::Increment(u16 value)
{
    m_value += value;
}

inline void SixteenBitRegister::Decrement()
{
    m_value--;
}

inline void SixteenBitRegister::Decrement(u16 value)
{
    m_value -= value;
}

inline void SixteenBitRegister::SaveState(std::ostream& stream)
{
    stream.write(reinterpret_cast<const char*> (&m_value), sizeof(m_value));
}

inline void SixteenBitRegister::LoadState(std::istream& stream)
{
    stream.read(reinterpret_cast<char*> (&m_value), sizeof(m_value));
}

#endif /* HUC6280_REGISTERS_H */