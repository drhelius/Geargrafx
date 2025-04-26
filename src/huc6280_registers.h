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
    EightBitRegister() { m_value = 0; }
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

INLINE u8 EightBitRegister::GetValue() const
{
    return m_value;
}

INLINE void EightBitRegister::SetValue(u8 value)
{
    m_value = value;
}

INLINE void EightBitRegister::Increment()
{
    m_value++;
}

INLINE void EightBitRegister::Increment(u8 value)
{
    m_value += value;
}

INLINE void EightBitRegister::Decrement()
{
    m_value--;
}

INLINE void EightBitRegister::Decrement(u8 value)
{
    m_value -= value;
}

INLINE void EightBitRegister::SaveState(std::ostream& stream)
{
    stream.write(reinterpret_cast<const char*> (&m_value), sizeof(m_value));
}

INLINE void EightBitRegister::LoadState(std::istream& stream)
{
    stream.read(reinterpret_cast<char*> (&m_value), sizeof(m_value));
}

//////////////////////////////////////////////////////////////////////////

class SixteenBitRegister
{
public:
    SixteenBitRegister() { m_value.v = 0; }
    void SetLow(u8 low);
    u8 GetLow() const;
    void SetHigh(u8 high);
    u8 GetHigh() const;
    u8* GetHighRegister();
    u8* GetLowRegister();
    void SetValue(u16 value);
    u16 GetValue() const;
    void Increment();
    void Increment(u16 value);
    void Decrement();
    void Decrement(u16 value);
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);

private:
    union sixteenBit
    {
        u16 v;
        struct
        {
#ifdef GG_LITTLE_ENDIAN
            u8 low;
            u8 high;
#else
            uint8_t high;
            uint8_t low;
#endif
        };
    } m_value;
};


INLINE void SixteenBitRegister::SetLow(u8 low)
{
    m_value.low = low;
}

INLINE u8 SixteenBitRegister::GetLow() const
{
    return m_value.low;
}

INLINE void SixteenBitRegister::SetHigh(u8 high)
{
    m_value.high = high;
}

INLINE u8 SixteenBitRegister::GetHigh() const
{
    return m_value.high;
}

INLINE u8* SixteenBitRegister::GetHighRegister()
{
    return &m_value.high;
}

INLINE u8* SixteenBitRegister::GetLowRegister()
{
    return &m_value.low;
}

INLINE void SixteenBitRegister::SetValue(u16 value)
{
    m_value.v = value;
}

INLINE u16 SixteenBitRegister::GetValue() const
{
    return m_value.v;
}

INLINE void SixteenBitRegister::Increment()
{
    m_value.v++;
}

INLINE void SixteenBitRegister::Increment(u16 value)
{
    m_value.v += value;
}

INLINE void SixteenBitRegister::Decrement()
{
    m_value.v--;
}

INLINE void SixteenBitRegister::Decrement(u16 value)
{
    m_value.v -= value;
}

INLINE void SixteenBitRegister::SaveState(std::ostream& stream)
{
    stream.write(reinterpret_cast<const char*> (&m_value.v), sizeof(m_value.v));
}

INLINE void SixteenBitRegister::LoadState(std::istream& stream)
{
    stream.read(reinterpret_cast<char*> (&m_value.v), sizeof(m_value.v));
}

#endif /* HUC6280_REGISTERS_H */