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

#ifndef CDROM_H
#define CDROM_H

#include <iostream>
#include <fstream>
#include "common.h"

class ScsiController;

class CdRom
{
public:
    struct CdRom_State
    {
        u8* tmp;
    };

public:
    CdRom(ScsiController* scsi_controller);
    ~CdRom();
    void Init();
    void Reset();
    u8 ReadRegister(u16 address);
    void WriteRegister(u16 address, u8 value);
    CdRom_State* GetState();
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);

private:
    CdRom_State m_state;
    ScsiController* m_scsi_controller;

};

static const u8 k_super_cdrom_signature[4] = { 0x00, 0xAA, 0x55, 0x03 };

#endif /* CDROM_H */