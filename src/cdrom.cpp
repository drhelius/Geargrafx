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

#include "cdrom.h"

CdRom::CdRom()
{
}

CdRom::~CdRom()
{
}

void CdRom::Init()
{
    Reset();
}

void CdRom::Reset()
{
}

u8 CdRom::ReadRegister(u16 address)
{
    return 0xFF;
}

void CdRom::WriteRegister(u16 address, u8 value)
{
}

CdRom::CdRom_State* CdRom::GetState()
{
    return &m_state;
}

void CdRom::SaveState(std::ostream& stream)
{
    using namespace std;
}

void CdRom::LoadState(std::istream& stream)
{
    using namespace std;
}
