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

#include <iostream>
#include <fstream>
#include "common.h"

class Audio;
class Input;
class HuC6260;
class HuC6270;
class HuC6280;
class HuC6202;
class Memory;
class Cartridge;
class CdRom;
class CdRomMedia;
class ScsiController;

class GeargrafxCore
{
public:

    struct GG_Debug_Run
    {
        bool step_debugger;
        bool stop_on_breakpoint;
        bool stop_on_run_to_breakpoint;
        bool stop_on_irq;
    };

    typedef void (*GG_Debug_Callback)(void);

public:
    GeargrafxCore();
    ~GeargrafxCore();
    void Init(GG_Pixel_Format pixel_format = GG_PIXEL_RGBA8888);
    bool RunToVBlank(u8* frame_buffer, s16* sample_buffer, int* sample_count, GG_Debug_Run* debug = NULL);
    bool LoadROM(const char* file_path);
    bool LoadROMFromBuffer(const u8* buffer, int size, const char* path = NULL);
    void ResetROM(bool preserve_ram);
    void KeyPressed(GG_Controllers controller, GG_Keys key);
    void KeyReleased(GG_Controllers controller, GG_Keys key);
    void Pause(bool paused);
    bool IsPaused();
    void SaveRam();
    void SaveRam(const char* path, bool full_path = false);
    void LoadRam();
    void LoadRam(const char* path, bool full_path = false);
    bool SaveState(const char* path = NULL, int index = -1, bool screenshot = false);
    bool SaveState(u8* buffer, size_t& size, bool screenshot = false);
    bool LoadState(const char* path = NULL, int index = -1);
    bool LoadState(const u8* buffer, size_t size);
    bool GetSaveStateHeader(int index, const char* path, GG_SaveState_Header* header);
    bool GetSaveStateScreenshot(int index, const char* path, GG_SaveState_Screenshot* screenshot);
    void ResetSound();
    bool GetRuntimeInfo(GG_Runtime_Info& runtime_info);
    Memory* GetMemory();
    Cartridge* GetCartridge();
    HuC6202* GetHuC6202();
    HuC6260* GetHuC6260();
    HuC6270* GetHuC6270_1();
    HuC6270* GetHuC6270_2();
    HuC6280* GetHuC6280();
    CdRom* GetCDROM();
    CdRomMedia* GetCDROMMedia();
    ScsiController* GetScsiController();
    Audio* GetAudio();
    Input* GetInput();
    u64 GetMasterClockCycles();
    void SetDebugCallback(GG_Debug_Callback callback);

private:
    void Reset();
    template<bool debugger, bool is_cdrom, bool is_sgx>
    bool RunToVBlankTemplate(u8* frame_buffer, s16* sample_buffer, int* sample_count, GG_Debug_Run* debug);
    bool SaveState(std::ostream& stream, size_t& size, bool screenshot);
    bool LoadState(std::istream& stream);
    std::string GetSaveStatePath(const char* path, int index);

private:
    Memory* m_memory;
    HuC6202* m_huc6202;
    HuC6260* m_huc6260;
    HuC6270* m_huc6270_1;
    HuC6270* m_huc6270_2;
    HuC6280* m_huc6280;
    Audio* m_audio;
    Input* m_input;
    Cartridge* m_cartridge;
    CdRom* m_cdrom;
    CdRomMedia* m_cdrom_media;
    ScsiController* m_scsi_controller;
    bool m_paused;
    GG_Debug_Callback m_debug_callback;
    u64 m_master_clock_cycles;
};

#include "geargrafx_core_inline.h"

#endif /* GEARGRAFX_CORE_H */