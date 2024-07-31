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

#include <stdlib.h>
#include <time.h>
#include "geargrafx_core.h"
#include "common.h"
#include "cartridge.h"
#include "memory.h"
#include "huc6260.h"
#include "huc6270.h"
#include "huc6280.h"
#include "audio.h"
#include "input.h"

GeargrafxCore::GeargrafxCore()
{
    InitPointer(m_memory);
    InitPointer(m_huc6260);
    InitPointer(m_huc6270);
    InitPointer(m_huc6280);
    InitPointer(m_audio);
    InitPointer(m_input);
    InitPointer(m_cartridge);
    m_paused = true;
    m_clock = 0;
}

GeargrafxCore::~GeargrafxCore()
{
    SafeDelete(m_cartridge);
    SafeDelete(m_input);
    SafeDelete(m_audio);
    SafeDelete(m_huc6280);
    SafeDelete(m_huc6270);
    SafeDelete(m_huc6260);
    SafeDelete(m_memory);
}

void GeargrafxCore::Init(GG_Pixel_Format pixel_format)
{
    Debug("--== %s %s by Ignacio Sanchez ==--", GEARGRAFX_TITLE, GEARGRAFX_VERSION);

    srand((unsigned int)time(NULL));

    m_cartridge = new Cartridge();
    m_huc6280 = new HuC6280();
    m_huc6270 = new HuC6270(m_huc6280);
    m_huc6260 = new HuC6260(m_huc6270);
    m_input = new Input();
    m_audio = new Audio();
    m_memory = new Memory(m_huc6260, m_huc6270, m_huc6280, m_cartridge, m_input, m_audio);

    m_cartridge->Init();
    m_memory->Init();
    m_huc6260->Init();
    m_huc6270->Init(m_huc6260, pixel_format);
    m_huc6280->Init(m_memory, m_huc6270);
    m_audio->Init();
    m_input->Init();
}

bool GeargrafxCore::RunToVBlank(u8* frame_buffer, s16* sample_buffer, int* sample_count, GG_Debug_Run* debug)
{
    if (m_paused || !m_cartridge->IsReady())
        return false;

#ifndef GG_DISABLE_DISASSEMBLER
    bool debug_enable = false;
    if (IsValidPointer(debug))
    {
        debug_enable = true;
        m_huc6280->EnableBreakpoints(debug->stop_on_breakpoint);
    }
#endif

    bool instruction_completed = false;
    bool stop = false;
    const int timer_divider = 3;
    const int audio_divider = 6;
    int huc6280_divider = m_huc6280->IsHighSpeed() ? 3 : 12;
    int huc6260_divider = m_huc6260->GetClockDivider();

    do
    {
        m_clock++;
        instruction_completed = false;

        if (m_clock % huc6280_divider == 0)
            instruction_completed = m_huc6280->Clock();

        if (m_clock % timer_divider == 0)
            m_huc6280->ClockTimer();

        if (m_clock % huc6260_divider == 0)
            stop = m_huc6270->Clock(frame_buffer);

        if (m_clock % audio_divider == 0)
            m_audio->Clock();

#ifndef GG_DISABLE_DISASSEMBLER
        if (debug_enable && debug->step_debugger && instruction_completed)
            stop = true;

        if (debug_enable && instruction_completed && m_huc6280->BreakpointHit())
            stop = true;

        if (debug_enable && debug->stop_on_run_to_breakpoint && instruction_completed && m_huc6280->RunToBreakpointHit())
            stop = true;
#endif
        // Failsafe: if the emulator is running too long, stop it
        // if (m_clock >= 89683)
        // {
        //     m_clock -= 89683;
        //     stop = true;
        // }
    }
    while (!stop);

    m_audio->EndFrame(sample_buffer, sample_count);
    RenderFrameBuffer(frame_buffer);

#ifndef GG_DISABLE_DISASSEMBLER
    return m_huc6280->BreakpointHit() || m_huc6280->RunToBreakpointHit();
#else
    return false;
#endif
}

bool GeargrafxCore::LoadROM(const char* file_path)
{
    if (m_cartridge->LoadFromFile(file_path))
    {
        m_memory->ResetDisassemblerRecords();
        Reset();
        return true;
    }
    else
        return false;
}

bool GeargrafxCore::LoadROMFromBuffer(const u8* buffer, int size)
{
    if (m_cartridge->LoadFromBuffer(buffer, size))
    {
        m_memory->ResetDisassemblerRecords();
        Reset();
        return true;
    }
    else
        return false;
}

bool GeargrafxCore::GetRuntimeInfo(GG_Runtime_Info& runtime_info)
{
    runtime_info.screen_width = GG_MAX_RESOLUTION_WIDTH;
    runtime_info.screen_height = GG_MAX_RESOLUTION_HEIGHT;

    if (m_cartridge->IsReady())
    {
    //     // if (m_video->GetOverscan() == Video::OverscanFull284)
    //     //     runtime_info.screen_width = GC_RESOLUTION_WIDTH + GC_RESOLUTION_SMS_OVERSCAN_H_284_L + GC_RESOLUTION_SMS_OVERSCAN_H_284_R;
    //     // if (m_video->GetOverscan() == Video::OverscanFull320)
    //     //     runtime_info.screen_width = GC_RESOLUTION_WIDTH + GC_RESOLUTION_SMS_OVERSCAN_H_320_L + GC_RESOLUTION_SMS_OVERSCAN_H_320_R;
    //     // if (m_video->GetOverscan() != Video::OverscanDisabled)
    //     //     runtime_info.screen_height = GC_RESOLUTION_HEIGHT + (2 * (m_cartridge->IsPAL() ? GC_RESOLUTION_OVERSCAN_V_PAL : GC_RESOLUTION_OVERSCAN_V));
        return true;
    }

    return false;
}

Memory* GeargrafxCore::GetMemory()
{
    return m_memory;
}

Cartridge* GeargrafxCore::GetCartridge()
{
    return m_cartridge;
}

HuC6260* GeargrafxCore::GetHuC6260()
{
    return m_huc6260;
}

HuC6270* GeargrafxCore::GetHuC6270()
{
    return m_huc6270;
}

HuC6280* GeargrafxCore::GetHuC6280()
{
    return m_huc6280;
}

Audio* GeargrafxCore::GetAudio()
{
    return m_audio;
}

Input* GeargrafxCore::GetInput()
{
    return m_input;
}

void GeargrafxCore::KeyPressed(GG_Controllers controller, GG_Keys key)
{
    m_input->KeyPressed(controller, key);
}

void GeargrafxCore::KeyReleased(GG_Controllers controller, GG_Keys key)
{
    m_input->KeyReleased(controller, key);
}

void GeargrafxCore::Pause(bool paused)
{
    if (!m_paused && paused)
        Log("Geargrafx PAUSED");
    else if (m_paused && !paused)
        Log("Geargrafx RESUMED");
    m_paused = paused;
}

bool GeargrafxCore::IsPaused()
{
    return m_paused;
}

void GeargrafxCore::ResetROM(bool preserve_ram)
{
    if (m_cartridge->IsReady())
    {
        Log("Geargrafx RESET");
        Reset();
        m_huc6280->DisassembleNextOPCode();
    }
}

void GeargrafxCore::ResetSound()
{
    m_audio->Reset();
}

// void GeargrafxCore::SaveRam()
// {
//     SaveRam(NULL);
// }

// void GeargrafxCore::SaveRam(const char*, bool)
// {
//     // TODO
// }

// void GeargrafxCore::LoadRam()
// {
//     LoadRam(NULL);
// }

// void GeargrafxCore::LoadRam(const char*, bool)
// {
//     // TODO
// }

// void GeargrafxCore::SaveState(int index)
// {
//     Log("Creating save state %d...", index);

//     SaveState(NULL, index);

//     Log("Save state %d created", index);
// }

// void GeargrafxCore::SaveState(const char* szPath, int index)
// {
//     Log("Creating save state...");

//     using namespace std;

//     size_t size;
//     SaveState(NULL, size);

//     u8* buffer = new u8[size];
//     string path = "";

//     if (IsValidPointer(szPath))
//     {
//         path += szPath;
//         path += "/";
//         path += m_cartridge->GetFileName();
//     }
//     else
//     {
//         path = m_cartridge->GetFilePath();
//     }

//     string::size_type i = path.rfind('.', path.length());

//     if (i != string::npos) {
//         path.replace(i + 1, 3, "state");
//     }

//     stringstream sstm;

//     if (index < 0)
//         sstm << szPath;
//     else
//         sstm << path << index;

//     Log("Save state file: %s", sstm.str().c_str());

//     ofstream file(sstm.str().c_str(), ios::out | ios::binary);

//     SaveState(file, size);

//     SafeDeleteArray(buffer);

//     file.close();

//     Log("Save state created");
// }

// bool GeargrafxCore::SaveState(u8* buffer, size_t& size)
// {
//     bool ret = false;

//     if (m_cartridge->IsReady())
//     {
//         using namespace std;

//         stringstream stream;

//         if (SaveState(stream, size))
//             ret = true;

//         if (IsValidPointer(buffer))
//         {
//             Log("Saving state to buffer [%d bytes]...", size);
//             memcpy(buffer, stream.str().c_str(), size);
//             ret = true;
//         }
//     }
//     else
//     {
//         Log("Invalid rom.");
//     }

//     return ret;
// }

// bool GeargrafxCore::SaveState(std::ostream& stream, size_t& size)
// {
//     if (m_cartridge->IsReady())
//     {
//         Log("Gathering save state data...");

//         m_memory->SaveState(stream);
//         m_processor->SaveState(stream);
//         m_audio->SaveState(stream);
//         m_video->SaveState(stream);
//         m_input->SaveState(stream);

//         size = static_cast<size_t>(stream.tellp());
//         size += (sizeof(u32) * 2);

//         u32 header_magic = GC_SAVESTATE_MAGIC;
//         u32 header_size = static_cast<u32>(size);

//         stream.write(reinterpret_cast<const char*> (&header_magic), sizeof(header_magic));
//         stream.write(reinterpret_cast<const char*> (&header_size), sizeof(header_size));

//         Log("Save state size: %d", static_cast<size_t>(stream.tellp()));

//         return true;
//     }

//     Log("Invalid rom.");

//     return false;
// }

// void GeargrafxCore::LoadState(int index)
// {
//     Log("Loading save state %d...", index);

//     LoadState(NULL, index);

//     Log("State %d file loaded", index);
// }

// void GeargrafxCore::LoadState(const char* szPath, int index)
// {
//     Log("Loading save state...");

//     using namespace std;

//     string sav_path = "";

//     if (IsValidPointer(szPath))
//     {
//         sav_path += szPath;
//         sav_path += "/";
//         sav_path += m_cartridge->GetFileName();
//     }
//     else
//     {
//         sav_path = m_cartridge->GetFilePath();
//     }

//     string rom_path = sav_path;

//     string::size_type i = sav_path.rfind('.', sav_path.length());

//     if (i != string::npos) {
//         sav_path.replace(i + 1, 3, "state");
//     }

//     std::stringstream sstm;

//     if (index < 0)
//         sstm << szPath;
//     else
//         sstm << sav_path << index;

//     Log("Opening save file: %s", sstm.str().c_str());

//     ifstream file;

//     file.open(sstm.str().c_str(), ios::in | ios::binary);

//     if (!file.fail())
//     {
//         if (LoadState(file))
//         {
//             Log("Save state loaded");
//         }
//     }
//     else
//     {
//         Log("Save state file doesn't exist");
//     }

//     file.close();
// }

// bool GeargrafxCore::LoadState(const u8* buffer, size_t size)
// {
//     if (m_cartridge->IsReady() && (size > 0) && IsValidPointer(buffer))
//     {
//         Log("Gathering load state data [%d bytes]...", size);

//         using namespace std;

//         stringstream stream;

//         stream.write(reinterpret_cast<const char*> (buffer), size);

//         return LoadState(stream);
//     }

//     Log("Invalid rom or memory.");

//     return false;
// }

// bool GeargrafxCore::LoadState(std::istream& stream)
// {
//     if (m_cartridge->IsReady())
//     {
//         using namespace std;

//         u32 header_magic = 0;
//         u32 header_size = 0;

//         stream.seekg(0, ios::end);
//         size_t size = static_cast<size_t>(stream.tellg());
//         stream.seekg(0, ios::beg);

//         Log("Load state stream size: %d", size);

//         stream.seekg(size - (2 * sizeof(u32)), ios::beg);
//         stream.read(reinterpret_cast<char*> (&header_magic), sizeof(header_magic));
//         stream.read(reinterpret_cast<char*> (&header_size), sizeof(header_size));
//         stream.seekg(0, ios::beg);

//         Log("Load state magic: 0x%08x", header_magic);
//         Log("Load state size: %d", header_size);

//         if ((header_size == size) && (header_magic == GC_SAVESTATE_MAGIC))
//         {
//             Log("Loading state...");

//             m_memory->LoadState(stream);
//             m_processor->LoadState(stream);
//             m_audio->LoadState(stream);
//             m_video->LoadState(stream);
//             m_input->LoadState(stream);

//             return true;
//         }
//         else
//         {
//             Log("Invalid save state size or header");
//         }
//     }
//     else
//     {
//         Log("Invalid rom");
//     }

//     return false;
// }

void GeargrafxCore::Reset()
{
    m_clock = 0;
    m_paused = false;
    m_memory->Reset();
    m_huc6260->Reset();
    m_huc6270->Reset();
    m_huc6280->Reset();
    m_audio->Reset();
    m_input->Reset();
}

void GeargrafxCore::RenderFrameBuffer(u8* final_framebuffer)
{
    // int size = GC_RESOLUTION_WIDTH_WITH_OVERSCAN * GC_RESOLUTION_HEIGHT_WITH_OVERSCAN;
    // u16* srcBuffer = (m_memory->IsBiosLoaded() ? m_video->GetFrameBuffer() : kNoBiosImage);

    // switch (m_pixelFormat)
    // {
    //     case GG_PIXEL_RGB555:
    //     case GG_PIXEL_BGR555:
    //     case GG_PIXEL_RGB565:
    //     case GG_PIXEL_BGR565:
    //     {
    //         m_video->Render16bit(srcBuffer, finalFrameBuffer, m_pixelFormat, size, true);
    //         break;
    //     }
    //     case GG_PIXEL_RGB888:
    //     case GG_PIXEL_BGR888:
    //     {
    //         m_video->Render24bit(srcBuffer, finalFrameBuffer, m_pixelFormat, size, true);
    //         break;
    //     }
    // }
}