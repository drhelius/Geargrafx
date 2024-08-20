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

#ifndef AUDIO_H
#define AUDIO_H

#include "types.h"

class HuC6280PSG;

class Audio
{
public:
    Audio();
    ~Audio();
    void Init();
    void Reset();
    void Mute(bool mute);
    void Clock();
    void WritePSG(u32 address, u8 value);
    void EndFrame(s16* sample_buffer, int* sample_count);
    // void SaveState(std::ostream& stream);
    // void LoadState(std::istream& stream);

private:
    bool m_mute;
    HuC6280PSG* m_psg;
    s16* m_psg_buffer;
};

#include "audio_inline.h"

#endif /* AUDIO_H */
