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
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
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
    InitPointer(m_debug_callback);
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
    Log("Loading %s core %s by Ignacio Sanchez", GG_TITLE, GG_VERSION);

    srand((unsigned int)time(NULL));

    m_cartridge = new Cartridge();
    m_huc6280 = new HuC6280();
    m_huc6270 = new HuC6270(m_huc6280);
    m_huc6260 = new HuC6260(m_huc6270, m_huc6280);
    m_input = new Input();
    m_audio = new Audio();
    m_memory = new Memory(m_huc6260, m_huc6270, m_huc6280, m_cartridge, m_input, m_audio);

    m_cartridge->Init();
    m_memory->Init();
    m_huc6260->Init(pixel_format);
    m_huc6270->Init(m_huc6260);
    m_huc6280->Init(m_memory, m_huc6270);
    m_audio->Init();
    m_input->Init();
}

bool GeargrafxCore::RunToVBlank(u8* frame_buffer, s16* sample_buffer, int* sample_count, GG_Debug_Run* debug)
{
    if (m_paused || !m_cartridge->IsReady())
        return false;

#if !defined(GG_DISABLE_DISASSEMBLER)
    GG_Debug_State debug_state;
    bool get_debug_state = true;
    bool debug_enable = false;
    bool instruction_completed = false;
    if (IsValidPointer(debug))
    {
        debug_enable = true;
        m_huc6280->EnableBreakpoints(debug->stop_on_breakpoint, debug->stop_on_irq);
    }
#else
    UNUSED(debug);
#endif

    m_huc6260->SetBuffer(frame_buffer);
    bool stop = false;

    do
    {
#if !defined(GG_DISABLE_DISASSEMBLER)
        if (get_debug_state)
        {
            get_debug_state = false;
            debug_state.PC = m_huc6280->GetState()->PC->GetValue();
            debug_state.P = m_huc6280->GetState()->P->GetValue();
            debug_state.A = m_huc6280->GetState()->A->GetValue();
            debug_state.X = m_huc6280->GetState()->X->GetValue();
            debug_state.Y = m_huc6280->GetState()->Y->GetValue();
            debug_state.S = m_huc6280->GetState()->S->GetValue();
        }

        instruction_completed = m_huc6280->Clock();
#else
        m_huc6280->Clock();
#endif

        stop = m_huc6260->Clock();

        if (m_clock == 0)
        {
            m_clock = 6;
            m_audio->Clock();
        }

#if !defined(GG_DISABLE_DISASSEMBLER)
        if (debug_enable)
        {
            if (debug->step_debugger)
                stop = instruction_completed;

            if (instruction_completed)
            {
                if (m_huc6280->BreakpointHit())
                    stop = true;

                if (debug->stop_on_run_to_breakpoint && m_huc6280->RunToBreakpointHit())
                    stop = true;

                if (IsValidPointer(m_debug_callback))
                {
                    debug_state.cycles = *m_huc6280->GetState()->CYCLES;
                    m_debug_callback(&debug_state);
                    get_debug_state = true;
                }
            }
        }
#endif

        m_clock--;
    }
    while (!stop);

    m_audio->EndFrame(sample_buffer, sample_count);

#if !defined(GG_DISABLE_DISASSEMBLER)
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
    runtime_info.screen_width = m_huc6260->GetCurrentWidth();
    runtime_info.screen_height = m_huc6260->GetCurrentHeight();
    runtime_info.width_scale = m_huc6260->GetWidthScale();

    return m_cartridge->IsReady();
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

void GeargrafxCore::SetDebugCallback(GG_Debug_Callback callback)
{
    m_debug_callback = callback;
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
    {
        Debug("Core paused");
    }
    else if (m_paused && !paused)
    {
        Debug("Core resumed");
    }

    m_paused = paused;
}

bool GeargrafxCore::IsPaused()
{
    return m_paused;
}

void GeargrafxCore::ResetROM(bool preserve_ram)
{
    if (!m_cartridge->IsReady())
        return;

    using namespace std;
    stringstream stream;

    if (preserve_ram)
        m_memory->SaveRam(stream);

    Log("Geargrafx RESET");
    Reset();
    m_huc6280->DisassembleNextOPCode();

    if (preserve_ram)
    {
        stream.seekg(0, stream.end);
        s32 size = (s32)stream.tellg();
        stream.seekg(0, stream.beg);
        m_memory->LoadRam(stream, size);
    }
}

void GeargrafxCore::ResetSound()
{
    m_audio->Reset();
}

void GeargrafxCore::SaveRam()
{
    SaveRam(NULL);
}

void GeargrafxCore::SaveRam(const char* path, bool full_path)
{
    if (m_cartridge->IsReady() && m_memory->IsBackupRamUsed())
    {
        using namespace std;
        string final_path;

        if (IsValidPointer(path))
        {
            final_path = path;
            if (!full_path)
                final_path += "/";
                final_path += m_cartridge->GetFileName();
        }
        else
            final_path = m_cartridge->GetFilePath();

        string::size_type i = final_path.rfind('.', final_path.length());
        if (i != string::npos)
            final_path.replace(i, final_path.length() - i, ".sav");

        Log("Saving RAM file: %s", final_path.c_str());

        ofstream file(final_path.c_str(), ios::out | ios::binary);
        m_memory->SaveRam(file);

        Debug("RAM saved");
    }
}

void GeargrafxCore::LoadRam()
{
    LoadRam(NULL);
}

void GeargrafxCore::LoadRam(const char* path, bool full_path)
{
    if (m_cartridge->IsReady())
    {
        using namespace std;
        string final_path;

        if (IsValidPointer(path))
        {
            final_path = path;
            if (!full_path)
                final_path += "/";
                final_path += m_cartridge->GetFileName();
        }
        else
            final_path = m_cartridge->GetFilePath();

        string::size_type i = final_path.rfind('.', final_path.length());
        if (i != string::npos)
            final_path.replace(i, final_path.length() - i, ".sav");

        Log("Loading RAM file: %s", final_path.c_str());

        ifstream file(final_path.c_str(), ios::in | ios::binary);

        if (!file.fail())
        {
            file.seekg(0, file.end);
            s32 file_size = (s32)file.tellg();
            file.seekg(0, file.beg);

            if (m_memory->LoadRam(file, file_size))
            {
                Debug("RAM loaded");
            }
            else
            {
                Log("ERROR: Failed to load RAM from %s", final_path.c_str());
                Log("ERROR: Invalid RAM size: %d", file_size);
            }
        }
        else
        {
            Log("RAM file doesn't exist: %s", final_path.c_str());
        }
    }
}

std::string GeargrafxCore::GetSaveStatePath(const char* path, int index)
{
    using namespace std;
    string full_path;

    if (IsValidPointer(path))
    {
        full_path = path;
        full_path += "/";
        full_path += m_cartridge->GetFileName();
    }
    else
        full_path = m_cartridge->GetFilePath();

    string::size_type dot_index = full_path.rfind('.');

    if (dot_index != string::npos)
        full_path.replace(dot_index + 1, full_path.length() - dot_index - 1, "state");

    if (index >= 0)
        full_path += to_string(index);

    return full_path;
}

bool GeargrafxCore::SaveState(const char* path, int index, bool screenshot)
{
    using namespace std;

    string full_path = GetSaveStatePath(path, index);
    Debug("Saving state to %s...", full_path.c_str());

    ofstream stream(full_path.c_str(), ios::out | ios::binary);

    size_t size;
    bool ret = SaveState(stream, size, screenshot);
    if (ret)
        Log("Saved state to %s", full_path.c_str());
    else
        Log("ERROR: Failed to save state to %s", full_path.c_str());
    return ret;
}

bool GeargrafxCore::SaveState(u8* buffer, size_t& size, bool screenshot)
{
    using namespace std;

    Debug("Saving state to buffer [%d bytes]...", size);

    if (!m_cartridge->IsReady())
    {
        Log("ERROR: Cartridge is not ready when trying to save state");
        return false;
    }

    stringstream stream;
    size_t expected_size = 0;
    if (!SaveState(stream, expected_size, screenshot))
    {
        Log("ERROR: Failed to save state to buffer");
        return false;
    }

    if (IsValidPointer(buffer) && (size >= expected_size))
    {
        size = expected_size;
        memcpy(buffer, stream.str().c_str(), size);
        return true;
    }
    else if (!IsValidPointer(buffer) && (size == 0))
    {
        size = expected_size;
        return true;
    }
    else
        return false;
}

bool GeargrafxCore::SaveState(std::ostream& stream, size_t& size, bool screenshot)
{
    using namespace std;

    if (!m_cartridge->IsReady())
    {
        Log("ERROR: Cartridge is not ready when trying to save state");
        return false;
    }

    Debug("Serializing save state...");

    stream.write(reinterpret_cast<const char*> (&m_clock), sizeof(m_clock));
    m_memory->SaveState(stream);
    m_huc6260->SaveState(stream);
    m_huc6270->SaveState(stream);
    m_huc6280->SaveState(stream);
    m_audio->SaveState(stream);
    m_input->SaveState(stream);

    GG_SaveState_Header header;
    header.magic = GG_SAVESTATE_MAGIC;
    header.version = GG_SAVESTATE_VERSION;
    header.timestamp = time(NULL);
    strncpy(header.rom_name, m_cartridge->GetFileName(), sizeof(header.rom_name) - 1);
    header.rom_crc = m_cartridge->GetCRC();

    Debug("Save state header magic: 0x%08x", header.magic);
    Debug("Save state header version: %d", header.version);
    Debug("Save state header timestamp: %d", header.timestamp);
    Debug("Save state header rom name: %s", header.rom_name);
    Debug("Save state header rom crc: 0x%08x", header.rom_crc);

    if (screenshot)
    {
        header.screenshot_width = m_huc6260->GetCurrentWidth();
        header.screenshot_height = m_huc6260->GetCurrentHeight();
        header.screshot_width_scale = m_huc6260->GetWidthScale();

        int bytes_per_pixel = 2;
        if (m_huc6260->GetPixelFormat() == GG_PIXEL_RGBA8888)
            bytes_per_pixel = 4;

        u8* frame_buffer = m_huc6260->GetBuffer();

        header.screenshot_size = header.screenshot_width * header.screenshot_height * bytes_per_pixel;
        stream.write(reinterpret_cast<const char*>(frame_buffer), header.screenshot_size);
    }
    else
    {
        header.screenshot_size = 0;
        header.screenshot_width = 0;
        header.screenshot_height = 0;
    }

    Debug("Save state header screenshot size: %d", header.screenshot_size);
    Debug("Save state header screenshot width: %d", header.screenshot_width);
    Debug("Save state header screenshot height: %d", header.screenshot_height);

    size = static_cast<size_t>(stream.tellp());
    size += sizeof(header);
    header.size = static_cast<u32>(size);

    Debug("Save state header size: %d", header.size);

    stream.write(reinterpret_cast<const char*>(&header), sizeof(header));
    return true;
}

bool GeargrafxCore::LoadState(const char* path, int index)
{
    using namespace std;
    bool ret = false;

    string full_path = GetSaveStatePath(path, index);
    Debug("Loading state from %s...", full_path.c_str());

    ifstream stream;
    stream.open(full_path.c_str(), ios::in | ios::binary);

    if (!stream.fail())
    {
        ret = LoadState(stream);

        if (ret)
            Log("Loaded state from %s", full_path.c_str());
        else
            Log("ERROR: Failed to load state from %s", full_path.c_str());
    }
    else
    {
        Log("ERROR: Load state file doesn't exist: %s", full_path.c_str());
    }

    stream.close();
    return ret;
}

bool GeargrafxCore::LoadState(const u8* buffer, size_t size)
{
    using namespace std;

    Debug("Loading state to buffer [%d bytes]...", size);

    if (!m_cartridge->IsReady())
    {
        Log("ERROR: Cartridge is not ready when trying to load state");
        return false;
    }

    if (!IsValidPointer(buffer) || (size == 0))
    {
        Log("ERROR: Invalid load state buffer");
        return false;
    }

    stringstream stream;
    stream.write(reinterpret_cast<const char*> (buffer), size);

    return LoadState(stream);
}

bool GeargrafxCore::LoadState(std::istream& stream)
{
    using namespace std;

    if (!m_cartridge->IsReady())
    {
        Log("ERROR: Cartridge is not ready when trying to load state");
        return false;
    }

    GG_SaveState_Header header;

    stream.seekg(0, ios::end);
    size_t size = static_cast<size_t>(stream.tellg());
    stream.seekg(0, ios::beg);

    stream.seekg(size - sizeof(header), ios::beg);
    stream.read(reinterpret_cast<char*> (&header), sizeof(header));
    stream.seekg(0, ios::beg);

    Debug("Load state header magic: 0x%08x", header.magic);
    Debug("Load state header version: %d", header.version);
    Debug("Load state header size: %d", header.size);
    Debug("Load state header timestamp: %d", header.timestamp);
    Debug("Load state header rom name: %s", header.rom_name);
    Debug("Load state header rom crc: 0x%08x", header.rom_crc);
    Debug("Load state header screenshot size: %d", header.screenshot_size);
    Debug("Load state header screenshot width: %d", header.screenshot_width);
    Debug("Load state header screenshot height: %d", header.screenshot_height);
    Debug("Load state header screenshot width scale: %d", header.screshot_width_scale);

    if ((header.magic != GG_SAVESTATE_MAGIC))
    {
        Log("Invalid save state: 0x%08x", header.magic);
        return false;
    }

    if (header.version != GG_SAVESTATE_VERSION)
    {
        Log("Invalid save state version: %d", header.version);
        return false;
    }

    if (header.size != size)
    {
        Log("Invalid save state size: %d", header.size);
        return false;
    }

    if (header.rom_crc != m_cartridge->GetCRC())
    {
        Log("Invalid save state rom crc: 0x%08x", header.rom_crc);
        return false;
    }

    Debug("Unserializing save state...");

    stream.read(reinterpret_cast<char*> (&m_clock), sizeof(m_clock));
    m_memory->LoadState(stream);
    m_huc6260->LoadState(stream);
    m_huc6270->LoadState(stream);
    m_huc6280->LoadState(stream);
    m_audio->LoadState(stream);
    m_input->LoadState(stream);

    return true;
}

bool GeargrafxCore::GetSaveStateHeader(int index, const char* path, GG_SaveState_Header* header)
{
    using namespace std;

    string full_path = GetSaveStatePath(path, index);
    Debug("Loading state header from %s...", full_path.c_str());

    ifstream stream;
    stream.open(full_path.c_str(), ios::in | ios::binary);

    if (stream.fail())
    {
        Debug("ERROR: Savestate file doesn't exist %s", full_path.c_str());
        stream.close();
        return false;
    }

    stream.seekg(0, ios::end);
    size_t savestate_size = static_cast<size_t>(stream.tellg());
    stream.seekg(0, ios::beg);

    stream.seekg(savestate_size - sizeof(GG_SaveState_Header), ios::beg);
    stream.read(reinterpret_cast<char*> (header), sizeof(GG_SaveState_Header));
    stream.seekg(0, ios::beg);

    return true;
}

bool GeargrafxCore::GetSaveStateScreenshot(int index, const char* path, GG_SaveState_Screenshot* screenshot)
{
    using namespace std;

    if (!IsValidPointer(screenshot->data) || (screenshot->size == 0))
    {
        Log("ERROR: Invalid save state screenshot buffer");
        return false;
    }

    string full_path = GetSaveStatePath(path, index);
    Debug("Loading state screenshot from %s...", full_path.c_str());

    ifstream stream;
    stream.open(full_path.c_str(), ios::in | ios::binary);

    if (stream.fail())
    {
        Log("ERROR: Savestate file doesn't exist %s", full_path.c_str());
        stream.close();
        return false;
    }

    GG_SaveState_Header header;

    stream.seekg(0, ios::end);
    size_t savestate_size = static_cast<size_t>(stream.tellg());
    stream.seekg(0, ios::beg);

    stream.seekg(savestate_size - sizeof(header), ios::beg);
    stream.read(reinterpret_cast<char*> (&header), sizeof(header));
    stream.seekg(0, ios::beg);

    if (header.screenshot_size == 0)
    {
        Debug("No screenshot data");
        stream.close();
        return false;
    }

    if (screenshot->size < header.screenshot_size)
    {
        Log("ERROR: Invalid screenshot buffer size %d < %d", screenshot->size, header.screenshot_size);
        stream.close();
        return false;
    }

    screenshot->size = header.screenshot_size;
    screenshot->width = header.screenshot_width;
    screenshot->height = header.screenshot_height;
    screenshot->width_scale = header.screshot_width_scale;

    Debug("Screenshot size: %d bytes", screenshot->size);
    Debug("Screenshot width: %d", screenshot->width);
    Debug("Screenshot height: %d", screenshot->height);
    Debug("Screenshot width scale: %d", screenshot->width_scale);

    stream.seekg(savestate_size - sizeof(header) - screenshot->size, ios::beg);
    stream.read(reinterpret_cast<char*> (screenshot->data), screenshot->size);
    stream.close();

    return true;
}

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
