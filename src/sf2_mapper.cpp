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

#include "sf2_mapper.h"
#include "cartridge.h"

SF2Mapper::SF2Mapper(Cartridge* cartridge) : Mapper(cartridge)
{
    Reset();
}

SF2Mapper::~SF2Mapper()
{
}

u8 SF2Mapper::Read(u8 bank, u16 address)
{
    if (bank < 0x40)
    {
        u8** rom_map = m_cartridge->GetROMMap();
        return rom_map[bank][address];
    }
    else
    {
        u8* rom = m_cartridge->GetROM();
        int bank_address = (bank * 0x2000) + m_bank_address;
        return rom[bank_address + address];
    }
}

void SF2Mapper::Write(u8 bank, u16 address, u8 value)
{
    if ((bank == 0x00) && ((address & 0x1FF0) == 0x1FF0))
    {
        UNUSED(value);
        m_bank = address & 0x0F;
        m_bank_address = m_bank * 0x80000;
    }
    else
    {
        Debug("SF2Mapper: Attempted write to ROM at bank=%02X, value=%02X, address=%04X", bank, value, address);
    }
}

void SF2Mapper::Reset()
{
    m_bank = 0;
    m_bank_address = 0;
}

void SF2Mapper::SaveState(std::ostream& stream)
{
    using namespace std;
    stream.write(reinterpret_cast<const char*> (&m_bank), sizeof(m_bank));
    stream.write(reinterpret_cast<const char*> (&m_bank_address), sizeof(m_bank_address));

}

void SF2Mapper::LoadState(std::istream& stream)
{
    using namespace std;
    stream.read(reinterpret_cast<char*> (&m_bank), sizeof(m_bank));
    stream.read(reinterpret_cast<char*> (&m_bank_address), sizeof(m_bank_address));
}