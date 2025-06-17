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

#ifndef SF2_MAPPER_H
#define SF2_MAPPER_H

#include "types.h"
#include "mapper.h"

class Media;

class SF2Mapper : public Mapper
{
public:
    SF2Mapper(Media* media);
    virtual ~SF2Mapper();
    virtual u8 Read(u8 bank, u16 address);
    virtual void Write(u8 bank, u16 address, u8 value);
    virtual void Reset();
    virtual void SaveState(std::ostream& stream);
    virtual void LoadState(std::istream& stream);

private:
    int m_bank;
    int m_bank_address;
};

#endif /* SF2_MAPPER_H */