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

#include "geargrafx_core.h"
#include "common.h"
#include "cartridge.h"
#include "memory.h"
#include "huc6280.h"
#include "huc6270.h"
#include "audio.h"
#include "input.h"

GeargrafxCore::GeargrafxCore()
{
    // InitPointer(m_memory);
    InitPointer(m_processor);
    InitPointer(m_audio);
    // InitPointer(m_video);
    InitPointer(m_input);
    // InitPointer(m_cartridge);
    m_paused = true;
    m_pixel_format = GG_PIXEL_RGB888;
}

GeargrafxCore::~GeargrafxCore()
{
    // SafeDelete(m_cartridge);
    SafeDelete(m_input);
    // SafeDelete(m_video);
    SafeDelete(m_audio);
    SafeDelete(m_processor);
    // SafeDelete(m_memory);
}

void GeargrafxCore::Init(GG_Pixel_Format pixel_format)
{
    Debug("--== %s %s by Ignacio Sanchez ==--", GEARGRAFX_TITLE, GEARGRAFX_VERSION);

    m_pixel_format = pixel_format;

    // m_cartridge = new Cartridge();
    // m_memory = new Memory(m_cartridge);
    m_processor = new HuC6280();
    m_audio = new Audio();
    // m_video = new Video(m_memory, m_processor);
    m_input = new Input();

    // m_memory->Init();
    m_processor->Init();
    m_audio->Init();
    // m_video->Init();
    m_input->Init();
    // m_cartridge->Init();
}

bool GeargrafxCore::RunToVBlank(u8* frame_buffer, s16* sample_buffer, int* sample_count, bool step, bool stopOnBreakpoints)
{
    bool breakpoint = false;

    // if (!m_paused && m_cartridge->IsReady())
    // {
    //     bool vblank = false;
    //     int totalClocks = 0;
    //     while (!vblank)
    //     {
    //         unsigned int clockCycles = m_processor->RunFor(1);
    //         // vblank = m_video->Tick(clockCycles);
    //         m_audio->Tick(clockCycles);

    //         totalClocks += clockCycles;

    //         // if ((step || (stopOnBreakpoints && m_processor->BreakpointHit())) && !m_processor->DuringInputOpcode())
    //         // {
    //         //     vblank = true;
    //         //     if (m_processor->BreakpointHit())
    //         //         breakpoint = true;
    //         // }

    //         if (totalClocks > 702240)
    //             vblank = true;
    //     }

    //     m_audio->EndFrame(sample_buffer, sample_count);
    //     RenderFrameBuffer(frame_buffer);
    // }

    return breakpoint;
}

bool GeargrafxCore::LoadROM(const char* szFilePath)
{
    // if (m_cartridge->LoadFromFile(szFilePath))
    // {
    //     Reset();

    //     // m_memory->ResetRomDisassembledMemory();
    //     // m_processor->DisassembleNextOpcode();

    //     return true;
    // }
    // else
    //     return false;
}

bool GeargrafxCore::LoadROMFromBuffer(const u8* buffer, int size)
{
    // if (m_cartridge->LoadFromBuffer(buffer, size))
    // {
    //     Reset();

    //     // m_memory->ResetRomDisassembledMemory();
    //     // m_processor->DisassembleNextOpcode();

    //     return true;
    // }
    // else
    //     return false;
}

bool GeargrafxCore::GetRuntimeInfo(GG_Runtime_Info& runtime_info)
{
    runtime_info.screen_width = GG_RESOLUTION_WIDTH;
    runtime_info.screen_height = GG_RESOLUTION_HEIGHT;

    // if (m_cartridge->IsReady())
    // {
    //     // if (m_video->GetOverscan() == Video::OverscanFull284)
    //     //     runtime_info.screen_width = GC_RESOLUTION_WIDTH + GC_RESOLUTION_SMS_OVERSCAN_H_284_L + GC_RESOLUTION_SMS_OVERSCAN_H_284_R;
    //     // if (m_video->GetOverscan() == Video::OverscanFull320)
    //     //     runtime_info.screen_width = GC_RESOLUTION_WIDTH + GC_RESOLUTION_SMS_OVERSCAN_H_320_L + GC_RESOLUTION_SMS_OVERSCAN_H_320_R;
    //     // if (m_video->GetOverscan() != Video::OverscanDisabled)
    //     //     runtime_info.screen_height = GC_RESOLUTION_HEIGHT + (2 * (m_cartridge->IsPAL() ? GC_RESOLUTION_OVERSCAN_V_PAL : GC_RESOLUTION_OVERSCAN_V));
    //     return true;
    // }

    // return false;
}

// Memory* GeargrafxCore::GetMemory()
// {
//     return m_memory;
// }

// Cartridge* GeargrafxCore::GetCartridge()
// {
//     return m_cartridge;
// }

HuC6280* GeargrafxCore::GetProcessor()
{
    return m_processor;
}

// Audio* GeargrafxCore::GetAudio()
// {
//     return m_audio;
// }

// Video* GeargrafxCore::GetVideo()
// {
//     return m_video;
// }

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
    if (paused)
    {
        Log("Geargrafx PAUSED");
    }
    else
    {
        Log("Geargrafx RESUMED");
    }
    m_paused = paused;
}

bool GeargrafxCore::IsPaused()
{
    return m_paused;
}

void GeargrafxCore::ResetROM(bool preserve_ram)
{
    // if (m_cartridge->IsReady())
    // {
    //     Log("Geargrafx RESET");

    //     Reset();

    //     m_processor->DisassembleNextOpcode();
    // }
}

void GeargrafxCore::ResetSound()
{
    // m_audio->Reset(m_cartridge->IsPAL());
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
    // m_memory->Reset();
    m_processor->Reset();
    // m_audio->Reset(m_cartridge->IsPAL());
    // m_video->Reset(m_cartridge->IsPAL());
    m_input->Reset();
    m_paused = false;
}

void GeargrafxCore::RenderFrameBuffer(u8* finalFrameBuffer)
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