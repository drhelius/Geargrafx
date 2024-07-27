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

#include <stdlib.h>
#include "memory.h"
#include "huc6260.h"
#include "huc6270.h"
#include "huc6280.h"
#include "cartridge.h"
#include "input.h"
#include "audio.h"

Memory::Memory(HuC6260* huc6260, HuC6270* huc6270, HuC6280* huc6280, Cartridge* cartridge, Input* input, Audio* audio)
{
    m_huc6260 = huc6260;
    m_huc6270 = huc6270;
    m_huc6280 = huc6280;
    m_cartridge = cartridge;
    m_input = input;
    m_audio = audio;
    InitPointer(m_wram);
    InitPointer(m_disassembler);
    m_io_buffer = 0;
}

Memory::~Memory()
{
    SafeDeleteArray(m_wram);
    if (IsValidPointer(m_disassembler))
    {
        for (int i = 0; i < 0x200000; i++)
        {
            SafeDelete(m_disassembler[i]);
        }
        SafeDeleteArray(m_disassembler);
    }
}

void Memory::Init()
{
    m_wram = new u8[0x2000];

#ifndef GG_DISABLE_DISASSEMBLER
    m_disassembler = new GG_Disassembler_Record*[0x200000];
    for (int i = 0; i < 0x200000; i++)
    {
        InitPointer(m_disassembler[i]);
    }
#endif

    Reset();
}

void Memory::Reset()
{
    m_io_buffer = 0;
    m_mpr[7] = 0x00;

    for (int i = 0; i < 7; i++)
    {
        m_mpr[i] = rand() & 0xFF;
    }

    for (int i = 0; i < 0x2000; i++)
    {
        m_wram[i] = rand() & 0xFF;
    }
}

Memory::GG_Disassembler_Record* Memory::GetDisassemblerRecord(u16 address)
{
    return m_disassembler[GetPhysicalAddress(address)];
}

Memory::GG_Disassembler_Record* Memory::GetOrCreateDisassemblerRecord(u16 address)
{
    u32 physical_address = GetPhysicalAddress(address);

    GG_Disassembler_Record* record = m_disassembler[physical_address];

    if (!IsValidPointer(record))
    {
        record = new GG_Disassembler_Record();
        record->address = physical_address;
        record->bank = GetBank(address);
        record->segment[0] = 0;
        record->name[0] = 0;
        record->bytes[0] = 0;
        record->size = 0;
        for (int i = 0; i < 7; i++)
            record->opcodes[i] = 0;
        record->jump = false;
        record->jump_address = 0;
        record->jump_bank = 0;
        record->subroutine = false;
        record->irq = 0;
        m_disassembler[physical_address] = record;
    }

    return record;
}

void Memory::ResetDisassemblerRecords()
{
    for (int i = 0; i < 0x200000; i++)
    {
        SafeDelete(m_disassembler[i]);
    }
}

u8* Memory::GetWram()
{
    return m_wram;
}