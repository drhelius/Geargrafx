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

#include "types.h"

#define GG_AUDIO_SAMPLE_RATE 44100
#define GG_AUDIO_BUFFER_SIZE 2048
#define GG_AUDIO_BUFFER_COUNT 3

class Audio
{
public:
    Audio();
    ~Audio();
    void Init();
    void Reset();
    void Mute(bool mute);
    void Tick(unsigned int cycles);
    void EndFrame(s16* sample_buffer, int* sample_count);
    // void SaveState(std::ostream& stream);
    // void LoadState(std::istream& stream);

private:
    u64 m_elapsed_cycles;
    int m_sample_rate;
    bool m_mute;
};

inline void Audio::Tick(unsigned int cycles)
{
    m_elapsed_cycles += cycles;
}
