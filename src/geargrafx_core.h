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

#ifndef GEARGRAFX_CORE_H
#define GEARGRAFX_CORE_H

#include "types.h"

class Audio;
class Input;
class HuC6280;

class GeargrafxCore
{

public:
    GeargrafxCore();
    ~GeargrafxCore();
    void Init(GG_Pixel_Format pixel_format = GG_PIXEL_RGB888);
    bool RunToVBlank(u8* frame_buffer, s16* sample_buffer, int* sample_count, bool step = false, bool stopOnBreakpoints = false);
    bool LoadROM(const char* szFilePath);
    bool LoadROMFromBuffer(const u8* buffer, int size);
    void ResetROM(bool preserve_ram);
    void KeyPressed(GG_Controllers controller, GG_Keys key);
    void KeyReleased(GG_Controllers controller, GG_Keys key);
    void Pause(bool paused);
    bool IsPaused();
    // void SaveRam();
    // void SaveRam(const char* path, bool full_path = false);
    // void LoadRam();
    // void LoadRam(const char* path, bool full_path = false);
    // void SaveState(int index);
    // void SaveState(const char* path, int index);
    // bool SaveState(u8* buffer, size_t& size);
    // bool SaveState(std::ostream& stream, size_t& size);
    // void LoadState(int index);
    // void LoadState(const char* path, int index);
    // bool LoadState(const u8* buffer, size_t size);
    // bool LoadState(std::istream& stream);
    void ResetSound();
    bool GetRuntimeInfo(GG_Runtime_Info& runtime_info);
    // Memory* GetMemory();
    // Cartridge* GetCartridge();
    HuC6280* GetProcessor();
    Audio* GetAudio();
    // HuC6270* GetVideo();

private:
    void Reset();
    void RenderFrameBuffer(u8* finalFrameBuffer);

private:
    // Memory* m_memory;
    HuC6280* m_processor;
    Audio* m_audio;
    // Video* m_video;
    Input* m_input;
    // Cartridge* m_cartridge;
    bool m_paused;
    GG_Pixel_Format m_pixel_format;
};

#endif /* GEARGRAFX_CORE_H */