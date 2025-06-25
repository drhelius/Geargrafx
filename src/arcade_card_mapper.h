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

#ifndef ARCADE_CARD_MAPPER_H
#define ARCADE_CARD_MAPPER_H

#include "common.h"
#include "mapper.h"

class Media;
class Memory;

class ArcadeCardMapper : public Mapper
{
public:

    enum ArcadeCard_OffSetTrigger
    {
        OFFSET_TRIGGER_NONE = 0,
        OFFSET_TRIGGER_LOW__BYTE = 1,
        OFFSET_TRIGGER_HIGH_BYTE = 2,
        OFFSET_TRIGGER_REG_0A = 3
    };

    struct ArcadeCard_Port
    {
        u32 base;
        u16 offset;
        u16 increment;
        u8 control;
        bool add_offset;
        bool auto_increment;
        bool signed_offset;
        bool increment_base;
        ArcadeCard_OffSetTrigger offset_trigger;
    };

    struct ArcadeCard_State
    {
        ArcadeCard_Port* PORTS;
        u32* REGISTER;
        u8* SHIFT_AMOUNT;
        u8* ROTATE_AMOUNT;
    };

public:
    ArcadeCardMapper(Media* media, Memory* memory);
    virtual ~ArcadeCardMapper();
    virtual u8 Read(u8 bank, u16 address);
    virtual void Write(u8 bank, u16 address, u8 value);
    virtual u8 ReadHardware(u16 address);
    virtual void WriteHardware(u16 address, u8 value);
    virtual void Reset();
    virtual void SaveState(std::ostream& stream);
    virtual void LoadState(std::istream& stream);
    u8* GetRAM(void);
    ArcadeCard_State* GetState(void);

private:
    u8 ReadPortData(u8 port);
    void WritePortData(u8 port, u8 value);
    u8 ReadPortRegister(u8 port, u8 reg);
    void WritePortRegister(u8 port, u8 reg, u8 value);
    u8 ReadRegister(u8 reg);
    void WriteRegister(u8 reg, u8 value);
    void WriteControlRegister(u8 port, u8 value);
    void Increment(u8 port);
    void AddOffset(u8 port);
    u32 EffectiveAddress(u8 port);

private:
    ArcadeCard_State m_state;
    u8* m_card_memory;
    ArcadeCard_Port m_ports[4];
    u32 m_register;
    u8 m_shift_amount;
    u8 m_rotate_amount;
};

#include "arcade_card_mapper_inline.h"

#endif /* ARCADE_CARD_MAPPER_H */