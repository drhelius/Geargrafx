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
#include "sf2_mapper.h"

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
    InitPointer(m_test_memory);
    InitPointer(m_current_mapper);
    InitPointer(m_sf2_mapper);
    m_mpr_reset_value = -1;
    m_wram_reset_value = 0;
}

Memory::~Memory()
{
    SafeDeleteArray(m_wram);
    SafeDeleteArray(m_test_memory);
    if (IsValidPointer(m_disassembler))
    {
        for (int i = 0; i < 0x200000; i++)
        {
            SafeDelete(m_disassembler[i]);
        }
        SafeDeleteArray(m_disassembler);
    }
    SafeDelete(m_sf2_mapper);
}

void Memory::Init()
{
    m_wram = new u8[0x2000];

#if !defined(GG_DISABLE_DISASSEMBLER)
    m_disassembler = new GG_Disassembler_Record*[0x200000];
    for (int i = 0; i < 0x200000; i++)
    {
        InitPointer(m_disassembler[i]);
    }
#endif

#if defined(GG_TESTING)
    m_test_memory = new u8[0x10000];
#endif

    m_current_mapper = NULL;
    m_sf2_mapper = new SF2Mapper(m_cartridge);

    Reset();
}

void Memory::Reset()
{
    m_io_buffer = 0xFF;
    m_mpr_buffer = 0x00;
    m_mpr[7] = 0x00;

    for (int i = 0; i < 7; i++)
    {
        if (m_mpr_reset_value < 0)
        {
            do
            {
                m_mpr[i] = rand() & 0xFF;
            }
            while (m_mpr[i] == 0x00);
        }
        else
            m_mpr[i] = m_mpr_reset_value & 0xFF;
    }

    for (int i = 0; i < 0x2000; i++)
    {
        if (m_wram_reset_value < 0)
            m_wram[i] = rand() & 0xFF;
        else
            m_wram[i] = m_wram_reset_value & 0xFF;
    }

#if defined(GG_TESTING)
    for (int i = 0; i < 0x10000; i++)
        m_test_memory[i] = rand() & 0xFF;
#endif

    if (m_cartridge->GetMapper() == Cartridge::SF2_MAPPER)
    {
        m_sf2_mapper->Reset();
        m_current_mapper = m_sf2_mapper;
    }
    else
        m_current_mapper = NULL;
}

void Memory::SetResetValues(int mpr, int wram)
{
    m_mpr_reset_value = mpr;
    m_wram_reset_value = wram;
}

void Memory::ResetDisassemblerRecords()
{
#if !defined(GG_DISABLE_DISASSEMBLER)
    for (int i = 0; i < 0x200000; i++)
    {
        SafeDelete(m_disassembler[i]);
    }
#endif
}

u8* Memory::GetWram()
{
    return m_wram;
}

GG_Disassembler_Record** Memory::GetAllDisassemblerRecords()
{
    return m_disassembler;
}

void Memory::SaveState(std::ostream& stream)
{
    using namespace std;
    stream.write(reinterpret_cast<const char*> (m_mpr), sizeof(m_mpr));
    stream.write(reinterpret_cast<const char*> (m_wram), sizeof(u8) * 0x2000);
    stream.write(reinterpret_cast<const char*> (&m_io_buffer), sizeof(m_io_buffer));
    stream.write(reinterpret_cast<const char*> (&m_mpr_buffer), sizeof(m_mpr_buffer));
    if (IsValidPointer(m_current_mapper))
        m_current_mapper->SaveState(stream);
}

void Memory::LoadState(std::istream& stream)
{
    using namespace std;
    stream.read(reinterpret_cast<char*> (m_mpr), sizeof(m_mpr));
    stream.read(reinterpret_cast<char*> (m_wram), sizeof(u8) * 0x2000);
    stream.read(reinterpret_cast<char*> (&m_io_buffer), sizeof(m_io_buffer));
    stream.read(reinterpret_cast<char*> (&m_mpr_buffer), sizeof(m_mpr_buffer));
    if (IsValidPointer(m_current_mapper))
        m_current_mapper->LoadState(stream);
}