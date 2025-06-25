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

#ifndef ARCADE_CARD_MAPPER_INLINE_H
#define ARCADE_CARD_MAPPER_INLINE_H

#include "arcade_card_mapper.h"

INLINE u8 ArcadeCardMapper::ReadPortData(u8 port)
{
    u32 address = EffectiveAddress(port);
    Increment(port);
    return m_card_memory[address];
}

INLINE void ArcadeCardMapper::WritePortData(u8 port, u8 value)
{
    u32 address = EffectiveAddress(port);
    Increment(port);
    m_card_memory[address] = value;
}

INLINE void ArcadeCardMapper::Increment(u8 port)
{
    if(m_ports[port].auto_increment)
    {
        if(m_ports[port].increment_base)
            m_ports[port].base = (m_ports[port].base + m_ports[port].increment) & 0xFFFFFF;
        else
            m_ports[port].offset = (m_ports[port].offset + m_ports[port].increment) & 0xFFFF;
    }
}

INLINE void ArcadeCardMapper::AddOffset(u8 port)
{
    s32 real_offset = m_ports[port].signed_offset
        ? (s32)((s16)(m_ports[port].offset)) : (s32)(m_ports[port].offset);

    m_ports[port].base = (m_ports[port].base + real_offset) & 0xFFFFFF;
}

INLINE u32 ArcadeCardMapper::EffectiveAddress(u8 port)
{
    u32 address = m_ports[port].base;

    if (m_ports[port].add_offset)
    {
        s32 real_offset = m_ports[port].signed_offset
        ? (s32)((s16)(m_ports[port].offset)) : (s32)(m_ports[port].offset);

        address += real_offset;
    }

    return address & 0x1FFFFF;
}

INLINE void ArcadeCardMapper::WriteControlRegister(u8 port, u8 value)
{
    m_ports[port].control = value & 0x7F;
    m_ports[port].auto_increment = IS_SET_BIT(value, 0);
    m_ports[port].add_offset = IS_SET_BIT(value, 1);
    m_ports[port].signed_offset = IS_SET_BIT(value, 3);
    m_ports[port].increment_base = IS_SET_BIT(value, 4);
    m_ports[port].offset_trigger = (ArcadeCard_OffSetTrigger)((value >> 5) & 0x03);
}

INLINE u8* ArcadeCardMapper::GetRAM(void)
{
    return m_card_memory;
}

INLINE ArcadeCardMapper::ArcadeCard_State* ArcadeCardMapper::GetState(void)
{
    return &m_state;
}

#endif /* ARCADE_CARD_MAPPER_INLINE_H */