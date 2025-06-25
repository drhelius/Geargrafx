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

#include "mapper.h"
#include "log.h"
#include "media.h"
#include "memory.h"

Mapper::Mapper(Media* media, Memory* memory)
{
    m_media = media;
    m_memory = memory;
}

Mapper::~Mapper()
{
}

u8 Mapper::ReadHardware(u16)
{
    Debug("Mapper::ReadHardware not implemented");
    return 0xFF;
}

void Mapper::WriteHardware(u16, u8)
{
    Debug("Mapper::WriteHardware not implemented");
}

void Mapper::SaveState(std::ostream&)
{
    Debug("Mapper::SaveState not implemented");
}

void Mapper::LoadState(std::istream&)
{
    Debug("Mapper::LoadState not implemented");
}
