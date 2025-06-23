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

#include "arcade_card_mapper.h"
#include "media.h"
#include "memory.h"

ArcadeCardMapper::ArcadeCardMapper(Media* media, Memory* memory) : Mapper(media, memory)
{
    Reset();
    InitPointer(m_card_memory);
    m_card_memory = new u8[0x200000];
}

ArcadeCardMapper::~ArcadeCardMapper()
{
    SafeDeleteArray(m_card_memory);
}

u8 ArcadeCardMapper::Read(u8 bank, u16 address)
{
    if (bank >= 0x40 && bank <= 0x43)
        return ReadPortData(bank - 0x40);
    else
        return m_memory->GetMemoryMap()[bank][address];
}

void ArcadeCardMapper::Write(u8 bank, u16 address, u8 value)
{
    if (bank >= 0x40 && bank <= 0x43)
        WritePortData(bank - 0x40, value);
    else if (m_memory->GetMemoryMapWrite()[bank])
        m_memory->GetMemoryMap()[bank][address] = value;
}

u8 ArcadeCardMapper::ReadHardware(u16 address)
{
    if (address < 0x1A40)
    {
        u8 port = (address >> 4) & 0x03;
        u8 reg = address & 0x0F;
        return ReadPortRegister(port, reg);
    }
    else if (address < 0x1B00)
        return ReadRegister(address & 0xFF);
    else
    {
        Debug("Invalid Arcade Card hardware read at %04X", address);
        return 0xFF;
    }
}

void ArcadeCardMapper::WriteHardware(u16 address, u8 value)
{
    if (address < 0x1A40)
    {
        u8 port = (address >> 4) & 0x03;
        u8 reg = address & 0x0F;
        WritePortRegister(port, reg, value);
    }
    else if (address < 0x1B00)
        WriteRegister(address & 0xFF, value);
    else
    {
        Debug("Invalid Arcade Card hardware write at %04X, value=%02X", address, value);
    }
}

void ArcadeCardMapper::Reset()
{
    m_register = 0;
    m_shift_amount = 0;
    m_rotate_amount = 0;

    for (int i = 0; i < 4; ++i)
    {
        m_ports[i].base = 0;
        m_ports[i].offset = 0;
        m_ports[i].increment = 0;
        m_ports[i].control = 0;
        m_ports[i].add_offset = false;
        m_ports[i].auto_increment = false;
        m_ports[i].signed_offset = false;
        m_ports[i].increment_base = false;
        m_ports[i].offset_trigger = OFFSET_TRIGGER_NONE;
    }
}

u8 ArcadeCardMapper::ReadPortRegister(u8 port, u8 reg)
{
    switch (reg)
    {
        case 0x00:
        case 0x01:
            return ReadPortData(port);
        case 0x02:
            return m_ports[port].base & 0xFF;
        case 0x03:
            return (m_ports[port].base >> 8) & 0xFF;
        case 0x04:
            return (m_ports[port].base >> 16) & 0xFF;
        case 0x05:
            return m_ports[port].offset & 0xFF;
        case 0x06:
            return (m_ports[port].offset >> 8) & 0xFF;
        case 0x07:
            return m_ports[port].increment & 0xFF;
        case 0x08:
            return (m_ports[port].increment >> 8) & 0xFF;
        case 0x09:
            return m_ports[port].control;
        case 0x0A:
            return 0;
        default:
            Debug("Invalid Arcade Card port register read at %02X:%02X", port, reg);
            return 0xFF;
    }
}

void ArcadeCardMapper::WritePortRegister(u8 port, u8 reg, u8 value)
{
    switch (reg)
    {
        case 0x00:
        case 0x01:
            WritePortData(port, value);
            break;
        case 0x02:
            m_ports[port].base = (m_ports[port].base & 0xFFFF00) | value;
            break;
        case 0x03:
            m_ports[port].base = (m_ports[port].base & 0xFF00FF) | (value << 8);
            break;
        case 0x04:
            m_ports[port].base = (m_ports[port].base & 0x00FFFF) | (value << 16);
            break;
        case 0x05:
            m_ports[port].offset = (m_ports[port].offset & 0xFF00) | value;
            if (m_ports[port].offset_trigger == OFFSET_TRIGGER_LOW__BYTE)
                AddOffset(port);
            break;
        case 0x06:
            m_ports[port].offset = (m_ports[port].offset & 0x00FF) | (value << 8);
            if (m_ports[port].offset_trigger == OFFSET_TRIGGER_HIGH_BYTE)
                AddOffset(port);
            break;
        case 0x07:
            m_ports[port].increment = (m_ports[port].increment & 0xFF00) | value;
            break;
        case 0x08:
            m_ports[port].increment = (m_ports[port].increment & 0x00FF) | (value << 8);
            break;
        case 0x09:
            WriteControlRegister(port, value);
            break;
        case 0x0A:
            if (m_ports[port].offset_trigger == OFFSET_TRIGGER_REG_0A)
                AddOffset(port);
            break;
        default:
            Debug("Invalid Arcade Card port register write at %02X:%02X, value=%02X", port, reg, value);
            break;
    }
}

u8 ArcadeCardMapper::ReadRegister(u8 reg)
{
    switch (reg)
    {
        case 0xE0:
        case 0xE1:
        case 0xE2:
        case 0xE3:
        {
            int dis = (reg & 0x03) << 3;
            return (m_register >> dis) & 0xFF;
        }
        case 0xE4:
            return m_shift_amount;
        case 0xE5:
            return m_rotate_amount;
        case 0xEC:
        case 0xED:
            return 0x00;
        case 0xFE:
            return 0x10;
        case 0xFF:
            return 0x51;
        default:
            Debug("Invalid Arcade Card register read at %02X", reg);
            return 0xFF;
    }
}

void ArcadeCardMapper::WriteRegister(u8 reg, u8 value)
{
    switch (reg)
    {
        case 0xE0:
        case 0xE1:
        case 0xE2:
        case 0xE3:
        {
            int dis = (reg & 0x03) << 3;
            m_register = (m_register & ~(0xFF << dis)) | (value << dis);
            break;
        }
        case 0xE4:
            m_shift_amount = value;
            if(value != 0)
            {
                s8 signed_amount = (s8)(value << 4) >> 4;

                if (signed_amount > 0)
                {
                    u8 n = (u8)(signed_amount);
                    m_register <<= n;
                }
                else if (signed_amount < 0)
                {
                    u8 n = (u8)(-signed_amount);
                    m_register >>= n;
                }
            }
            break;
        case 0xE5:
            m_rotate_amount = value;
            if (value != 0)
            {
                s8 signed_amount = (s8)(value << 4) >> 4;

                if (signed_amount > 0)
                {
                    u8 n = (u8)(signed_amount);
                    m_register = (m_register << n) | (m_register >> (32 - n));
                }
                else if (signed_amount < 0)
                {
                    u8 n = (u8)(-signed_amount);
                    m_register = (m_register >> n) | (m_register << (32 - n));
                }
            }
            break;
        default:
            Debug("Invalid Arcade Card register write at %02X, value=%02X", reg, value);
            break;
    }
}

void ArcadeCardMapper::SaveState(std::ostream& stream)
{
    using namespace std;
    stream.write(reinterpret_cast<const char*> (&m_register), sizeof(m_register));
    stream.write(reinterpret_cast<const char*> (&m_shift_amount), sizeof(m_shift_amount));
    stream.write(reinterpret_cast<const char*> (&m_rotate_amount), sizeof(m_rotate_amount));

    for (int i = 0; i < 4; ++i)
    {
        stream.write(reinterpret_cast<const char*> (&m_ports[i].base), sizeof(m_ports[i].base));
        stream.write(reinterpret_cast<const char*> (&m_ports[i].offset), sizeof(m_ports[i].offset));
        stream.write(reinterpret_cast<const char*> (&m_ports[i].increment), sizeof(m_ports[i].increment));
        stream.write(reinterpret_cast<const char*> (&m_ports[i].control), sizeof(m_ports[i].control));
        stream.write(reinterpret_cast<const char*> (&m_ports[i].add_offset), sizeof(m_ports[i].add_offset));
        stream.write(reinterpret_cast<const char*> (&m_ports[i].auto_increment), sizeof(m_ports[i].auto_increment));
        stream.write(reinterpret_cast<const char*> (&m_ports[i].signed_offset), sizeof(m_ports[i].signed_offset));
        stream.write(reinterpret_cast<const char*> (&m_ports[i].increment_base), sizeof(m_ports[i].increment_base));
        stream.write(reinterpret_cast<const char*> (&m_ports[i].offset_trigger), sizeof(m_ports[i].offset_trigger));
    }

    stream.write(reinterpret_cast<const char*>(m_card_memory), 0x200000);
}

void ArcadeCardMapper::LoadState(std::istream& stream)
{
    using namespace std;
    stream.read(reinterpret_cast<char*> (&m_register), sizeof(m_register));
    stream.read(reinterpret_cast<char*> (&m_shift_amount), sizeof(m_shift_amount));
    stream.read(reinterpret_cast<char*> (&m_rotate_amount), sizeof(m_rotate_amount));

    for (int i = 0; i < 4; ++i)
    {
        stream.read(reinterpret_cast<char*> (&m_ports[i].base), sizeof(m_ports[i].base));
        stream.read(reinterpret_cast<char*> (&m_ports[i].offset), sizeof(m_ports[i].offset));
        stream.read(reinterpret_cast<char*> (&m_ports[i].increment), sizeof(m_ports[i].increment));
        stream.read(reinterpret_cast<char*> (&m_ports[i].control), sizeof(m_ports[i].control));
        stream.read(reinterpret_cast<char*> (&m_ports[i].add_offset), sizeof(m_ports[i].add_offset));
        stream.read(reinterpret_cast<char*> (&m_ports[i].auto_increment), sizeof(m_ports[i].auto_increment));
        stream.read(reinterpret_cast<char*> (&m_ports[i].signed_offset), sizeof(m_ports[i].signed_offset));
        stream.read(reinterpret_cast<char*> (&m_ports[i].increment_base), sizeof(m_ports[i].increment_base));
        stream.read(reinterpret_cast<char*> (&m_ports[i].offset_trigger), sizeof(m_ports[i].offset_trigger));
    }

    stream.read(reinterpret_cast<char*>(m_card_memory), 0x200000);
}