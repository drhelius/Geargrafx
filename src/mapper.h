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

#ifndef MAPPER_H
#define MAPPER_H

#include <iostream>
#include <fstream>
#include "types.h"

class Media;

class Mapper
{
public:
    Mapper(Media* media);
    virtual ~Mapper();
    virtual u8 Read(u8 bank, u16 address) = 0;
    virtual void Write(u8 bank, u16 address, u8 value) = 0;
    virtual void Reset() = 0;
    virtual void SaveState(std::ostream& stream);
    virtual void LoadState(std::istream& stream);

protected:
    Media* m_media;
};

#endif /* MAPPER_H */