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

#define CDROM_IRQ_ADPCM             0x04
#define CDROM_IRQ_STOP              0x08
#define CDROM_IRQ_STATUS_AND_MSG_IN 0x20
#define CDROM_IRQ_DATA_IN           0x40

class ScsiController;
class HuC6280;
class Memory;
class Audio;
class Adpcm;

class CdRom
{
public:
    struct CdRom_State
    {
        u8* RESET;
        bool* BRAM_ENABLED;
        u8* ACTIVE_IRQS;
        u8* ENABLED_IRQS;
    };

public:
    CdRom(ScsiController* scsi_controller, Audio* audio);
    ~CdRom();
    void Init(HuC6280* huc6280, Memory* memory, Adpcm* adpcm);
    void Reset();
    void Clock(u32 cycles);
    u8 ReadRegister(u16 address);
    void WriteRegister(u16 address, u8 value);
    void SetIRQ(u8 value);
    void ClearIRQ(u8 value);
    CdRom_State* GetState();
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);

private:
    void AssertIRQ2();

private:
    CdRom_State m_state;
    ScsiController* m_scsi_controller;
    Audio* m_audio;
    Adpcm* m_adpcm;
    HuC6280* m_huc6280;
    Memory* m_memory;
    u8 m_reset;
    bool m_bram_enabled;
    u8 m_active_irqs;
    u8 m_enabled_irqs;
};

static const u8 k_super_cdrom_signature[4] = { 0x00, 0xAA, 0x55, 0x03 };

#include "cdrom_inline.h"

#endif /* CDROM_H */