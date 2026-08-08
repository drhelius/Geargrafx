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

#ifndef RANDOM_H
#define RANDOM_H

#include <iostream>
#include "common.h"

class Random
{
public:
    Random()
    {
        m_state = 0x12345678;
    }

    void Seed(u32 seed)
    {
        SetState(seed);
    }

    void SaveState(std::ostream& stream)
    {
        stream.write(reinterpret_cast<const char*>(&m_state), sizeof(m_state));
    }

    void LoadState(std::istream& stream)
    {
        u32 state;
        stream.read(reinterpret_cast<char*>(&state), sizeof(state));
        SetState(state);
    }

    INLINE u32 Next()
    {
        m_state ^= m_state << 13;
        m_state ^= m_state >> 17;
        m_state ^= m_state << 5;

        return m_state;
    }

    INLINE u8 Next8Bit()
    {
        return (u8)(Next() >> 24);
    }

    INLINE u16 Next16Bit()
    {
        return (u16)(Next() >> 16);
    }

    INLINE u32 Next(u32 limit)
    {
        if (limit == 0)
            return 0;

        return (u32)(((u64)Next() * limit) >> 32);
    }

    INLINE u32 NextMask(u32 mask)
    {
        return Next() & mask;
    }

private:
    void SetState(u32 state)
    {
        m_state = (state == 0) ? 0x12345678 : state;
    }

private:
    u32 m_state;
};

#endif /* RANDOM_H */