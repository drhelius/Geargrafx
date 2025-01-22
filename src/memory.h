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

#include <iostream>
#include <fstream>
#include "common.h"

class Cartridge;
class HuC6260;
class HuC6270;
class HuC6280;
class Input;
class Audio;
class Mapper;
class SF2Mapper;

class Memory
{
public:
    struct GG_Disassembler_Record
    {
        u32 address;
        u8 bank;
        char name[64];
        char bytes[25];
        char segment[5];
        u8 opcodes[7];
        int size;
        bool jump;
        u16 jump_address;
        u8 jump_bank;
        bool subroutine;
        int irq;
    };

public:
    Memory(HuC6260* huc6260, HuC6270* huc6270, HuC6280* huc6280, Cartridge* cartridge, Input* input, Audio* audio);
    ~Memory();
    void Init();
    void Reset();
    u8 Read(u16 address, bool block_transfer = false);
    void Write(u16 address, u8 value);
    void SetMpr(u8 index, u8 value);
    u8 GetMpr(u8 index);
    void SetMprTAM(u8 bits, u8 value);
    u8 GetMprTMA(u8 bits);
    u32 GetPhysicalAddress(u16 address);
    u8 GetBank(u16 address);
    GG_Disassembler_Record* GetDisassemblerRecord(u16 address);
    GG_Disassembler_Record* GetOrCreateDisassemblerRecord(u16 address);
    void ResetDisassemblerRecords();
    u8* GetWram();
    GG_Disassembler_Record** GetAllDisassemblerRecords();
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);

private:
    HuC6260* m_huc6260;
    HuC6270* m_huc6270;
    HuC6280* m_huc6280;
    Cartridge* m_cartridge;
    SF2Mapper* m_sf2_mapper;
    Input* m_input;
    Audio* m_audio;
    u8 m_mpr[8];
    u8* m_wram;
    GG_Disassembler_Record** m_disassembler;
    u8 m_io_buffer;
    u8 m_mpr_buffer;
    u8* m_test_memory;
    Mapper* m_current_mapper;
};

#include "memory_inline.h"

#endif /* MEMORY_H */