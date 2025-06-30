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

#include <iostream>
#include <fstream>
#include "common.h"

class Adpcm;
class HuC6280PSG;
class CdRomAudio;

class Audio
{
public:
    Audio(Adpcm* adpcm, CdRomAudio* cdrom_audio);
    ~Audio();
    void Init();
    void Reset(bool cdrom);
    void Mute(bool mute);
    void SetPSGVolume(float volume);
    void SetADPCMVolume(float volume);
    void SetCDROMVolume(float volume);
    void Clock(u32 cycles);
    void WritePSG(u32 address, u8 value);
    void EndFrame(s16* sample_buffer, int* sample_count);
    HuC6280PSG* GetPSG();
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);

private:
    bool m_mute;
    bool m_is_cdrom;
    HuC6280PSG* m_psg;
    Adpcm* m_adpcm;
    CdRomAudio* m_cdrom_audio;
    s16 m_psg_buffer[GG_AUDIO_BUFFER_SIZE] = {};
    s16 m_adpcm_buffer[GG_AUDIO_BUFFER_SIZE] = {};
    s16 m_cdrom_buffer[GG_AUDIO_BUFFER_SIZE] = {};
    u32 m_cycle_counter;
    float m_psg_volume;
    float m_adpcm_volume;
    float m_cdrom_volume;
};

#include "audio_inline.h"

#endif /* AUDIO_H */
