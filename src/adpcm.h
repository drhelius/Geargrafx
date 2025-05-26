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

#ifndef ADPCM_H
#define ADPCM_H

#include <iostream>
#include <fstream>
#include "common.h"

class Adpcm
{

public:
    Adpcm();
    ~Adpcm();
    void Init();
    void Reset();
    void Clock(u32 cycles);
    u8 Read(u16 address);
    void Write(u16 address, u8 value);
    int EndFrame(s16* sample_buffer);
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);

private:
    void Sync();
    void ComputeDeltaLUT();

private:
    s16 m_step_delta[49 * 8] = {};
};

static const s16 k_adpcm_index_shift[8] = { -1, -1, -1, -1, 2, 4, 6, 8 };

#include "adpcm_inline.h"

#endif /* ADPCM_H */