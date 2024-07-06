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

#ifndef MEMORY_H
#define MEMORY_H

#include "common.h"

class Cartridge;

class Memory
{
public:
    struct GG_Disassembler_Record
    {
        u32 address;
        char name[64];
        char bytes[20];
        int size;
        u8 opcodes[7];
        bool jump;
        u16 jump_address;
    };

public:
    Memory(Cartridge* cartridge);
    ~Memory();
    void Init();
    void Reset();
    u8 Read(u16 address);
    void Write(u16 address, u8 value);
    void SetMpr(u8 index, u8 value);
    u8 GetMpr(u8 index);
    GG_Disassembler_Record* GetOrCreatDisassemblerRecord(u16 address);

private:
    Cartridge* m_cartridge;
    u8 m_mpr[8];
    u8* m_wram;
    GG_Disassembler_Record** m_disassemblerMemoryMap;

private:
    u32 GetPhysicalAddress(u16 address);
};

#include "memory_inline.h"

#endif /* MEMORY_H */