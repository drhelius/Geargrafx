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

#include <math.h>
#include <assert.h>
#include <algorithm>
#include "adpcm.h"

Adpcm::Adpcm()
{

}

Adpcm::~Adpcm()
{

}

void Adpcm::Init()
{
    

    ComputeDeltaLUT();
    Reset();
}

void Adpcm::Reset()
{
    
}

void Adpcm::Sync()
{
   
}

int Adpcm::EndFrame(s16* sample_buffer)
{
    Sync();

    int samples = 0;

    return samples;
}

void Adpcm::ComputeDeltaLUT()
{
    for (int step = 0; step < 49; step++)
    {
        int step_value = (int)floor(16.0f * pow(11.0f / 10.0f, (float)step));
        
        for (int nibble = 0; nibble < 8; nibble++)
        {
            m_step_delta[(step << 3) + nibble] = (step_value / 8) +
                (IS_SET_BIT(nibble, 0) ? (step_value / 4) : 0) +
                (IS_SET_BIT(nibble, 1) ? (step_value / 2) : 0) +
                (IS_SET_BIT(nibble, 2) ? (step_value / 1) : 0);
        }
    }
}

void Adpcm::SaveState(std::ostream& stream)
{
    
}

void Adpcm::LoadState(std::istream& stream)
{
    
}