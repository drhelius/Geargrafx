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

#include "../../../src/geargrafx.h"
#include "../audio/sound_queue.h"

#define EMU_IMPORT
#include "emu.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#ifdef _WIN32
#define STBIW_WINDOWS_UTF8
#endif
#include "stb/stb_image_write.h"

static GeargrafxCore* geargrafx;
static SoundQueue* sound_queue;
static s16* audio_buffer;
static bool audio_enabled;
static bool debugging = false;
static bool debug_step = false;
static bool debug_next_frame = false;

static void save_ram(void);
static void load_ram(void);
static void init_debug(void);
static void destroy_debug(void);
static void update_debug(void);
static void update_debug_background_buffer(void);
static void update_debug_tile_buffer(void);
static void update_debug_sprite_buffers(void);

void emu_init(void)
{
    int screen_size = GG_MAX_RESOLUTION_WIDTH * GG_MAX_RESOLUTION_HEIGHT;

    emu_frame_buffer = new u8[screen_size * 3];

    for (int i=0, j=0; i < screen_size; i++, j+=3)
    {
        emu_frame_buffer[j] = 0;
        emu_frame_buffer[j+1] = 0;
        emu_frame_buffer[j+2] = 0;
    }

    init_debug();

    geargrafx = new GeargrafxCore();
    geargrafx->Init();

    sound_queue = new SoundQueue();
    sound_queue->Start(GG_AUDIO_SAMPLE_RATE, 2, GG_AUDIO_BUFFER_SIZE, GG_AUDIO_BUFFER_COUNT);

    audio_buffer = new s16[GG_AUDIO_BUFFER_SIZE];
    for (int i = 0; i < GG_AUDIO_BUFFER_SIZE; i++)
        audio_buffer[i] = 0;

    audio_enabled = true;
    emu_audio_sync = true;
    // emu_debug_disable_breakpoints_cpu = false;
    // emu_debug_disable_breakpoints_mem = false;
    // emu_debug_tile_palette = 0;
    emu_savefiles_dir_option = 0;
    emu_savestates_dir_option = 0;
    emu_savefiles_path[0] = 0;
    emu_savestates_path[0] = 0;
}

void emu_destroy(void)
{
    save_ram();
    SafeDeleteArray(audio_buffer);
    SafeDelete(sound_queue);
    SafeDelete(geargrafx);
    SafeDeleteArray(emu_frame_buffer);
    destroy_debug();
}

void emu_load_rom(const char* file_path)
{
    save_ram();
    geargrafx->LoadROM(file_path);
    load_ram();
    // emu_debug_continue();
}

void emu_update(void)
{
    if (!emu_is_empty())
    {
        int sampleCount = 0;
        geargrafx->RunToVBlank(emu_frame_buffer, audio_buffer, &sampleCount);

        // if (!debugging || debug_step || debug_next_frame)
        // {
        //     bool breakpoints = (!emu_debug_disable_breakpoints_cpu && !emu_debug_disable_breakpoints_mem) || IsValidPointer(geargrafx->GetMemory()->GetRunToBreakpoint());

        //     if (geargrafx->RunToVBlank(emu_frame_buffer, audio_buffer, &sampleCount, debug_step, breakpoints))
        //     {
        //         debugging = true;
        //     }

        //     debug_next_frame = false;
        //     debug_step = false;
        // }

        // update_debug();

        if ((sampleCount > 0) && !geargrafx->IsPaused())
        {
            sound_queue->Write(audio_buffer, sampleCount, emu_audio_sync);
        }
    }
}

void emu_key_pressed(GG_Controllers controller, GG_Keys key)
{
    geargrafx->KeyPressed(controller, key);
}

void emu_key_released(GG_Controllers controller, GG_Keys key)
{
    geargrafx->KeyReleased(controller, key);
}

void emu_pause(void)
{
    geargrafx->Pause(true);
}

void emu_resume(void)
{
    geargrafx->Pause(false);
}

bool emu_is_paused(void)
{
    return geargrafx->IsPaused();
}

bool emu_is_empty(void)
{
    return !geargrafx->GetCartridge()->IsReady();
}


void emu_reset(void)
{
    save_ram();
    geargrafx->ResetROM(false);
    load_ram();
}

void emu_audio_mute(bool mute)
{
    audio_enabled = !mute;
    geargrafx->GetAudio()->Mute(mute);
}

void emu_audio_reset(void)
{
    sound_queue->Stop();
    sound_queue->Start(GG_AUDIO_SAMPLE_RATE, 2, GG_AUDIO_BUFFER_SIZE, GG_AUDIO_BUFFER_COUNT);
}

bool emu_is_audio_enabled(void)
{
    return audio_enabled;
}

void emu_save_ram(const char* file_path)
{
    // if (!emu_is_empty())
    //     geargrafx->SaveRam(file_path, true);
}

void emu_load_ram(const char* file_path)
{
    // if (!emu_is_empty())
    // {
    //     save_ram();
    //     geargrafx->ResetROM(&config);
    //     geargrafx->LoadRam(file_path, true);
    // }
}

void emu_save_state_slot(int index)
{
    // if (!emu_is_empty())
    // {
    //     if ((emu_savestates_dir_option == 0) && (strcmp(emu_savestates_path, "")))
    //         geargrafx->SaveState(emu_savestates_path, index);
    //     else
    //         geargrafx->SaveState(index);
    // }
}

void emu_load_state_slot(int index)
{
    // if (!emu_is_empty())
    // {
    //     if ((emu_savestates_dir_option == 0) && (strcmp(emu_savestates_path, "")))
    //         geargrafx->LoadState(emu_savestates_path, index);
    //     else
    //         geargrafx->LoadState(index);
    // }
}

void emu_save_state_file(const char* file_path)
{
    // if (!emu_is_empty())
    //     geargrafx->SaveState(file_path, -1);
}

void emu_load_state_file(const char* file_path)
{
    // if (!emu_is_empty())
    //     geargrafx->LoadState(file_path, -1);
}

void emu_get_runtime(GG_Runtime_Info& runtime)
{
    geargrafx->GetRuntimeInfo(runtime);
}

void emu_get_info(char* info)
{
    if (!emu_is_empty())
    {
        Cartridge* cart = geargrafx->GetCartridge();
        GG_Runtime_Info runtime;
        geargrafx->GetRuntimeInfo(runtime);

        const char* filename = cart->GetFileName();
        u32 crc = cart->GetCRC();
        int rom_size = cart->GetROMSize();
        int rom_banks = cart->GetROMBankCount();

        sprintf(info, "File Name: %s\nCRC: %08X\nROM Size: %d bytes, %d KB\nROM Banks: %d\nScreen Resolution: %dx%d", filename, crc, rom_size, rom_size / 1024, rom_banks, runtime.screen_width, runtime.screen_height);
    }
    else
    {
        sprintf(info, "There is no ROM loaded!");
    }
}

GeargrafxCore* emu_get_core(void)
{
    return geargrafx;
}

// void emu_debug_step(void)
// {
//     debugging = debug_step = true;
//     debug_next_frame = false;
//     geargrafx->Pause(false);
// }

// void emu_debug_continue(void)
// {
//     debugging = debug_step = debug_next_frame = false;
//     geargrafx->Pause(false);
// }

// void emu_debug_next_frame(void)
// {
//     debugging = debug_next_frame = true;
//     debug_step = false;
//     geargrafx->Pause(false);
// }

void emu_video_no_sprite_limit(bool enabled)
{
    // geargrafx->GetVideo()->SetNoSpriteLimit(enabled);
}

void emu_set_overscan(int overscan)
{
    // switch (overscan)
    // {
    //     case 0:
    //         geargrafx->GetVideo()->SetOverscan(Video::OverscanDisabled);
    //         break;
    //     case 1:
    //         geargrafx->GetVideo()->SetOverscan(Video::OverscanTopBottom);
    //         break;
    //     case 2:
    //         geargrafx->GetVideo()->SetOverscan(Video::OverscanFull284);
    //         break;
    //     case 3:
    //         geargrafx->GetVideo()->SetOverscan(Video::OverscanFull320);
    //         break;
    //     default:
    //         geargrafx->GetVideo()->SetOverscan(Video::OverscanDisabled);
    // }
}

void emu_save_screenshot(const char* file_path)
{
    if (!geargrafx->GetCartridge()->IsReady())
        return;

    GG_Runtime_Info runtime;
    emu_get_runtime(runtime);

    Log("Saving screenshot to %s", file_path);

    stbi_write_png(file_path, runtime.screen_width, runtime.screen_height, 3, emu_frame_buffer, runtime.screen_width * 3);

    Log("Screenshot saved!");
}

static void save_ram(void)
{
    // if ((emu_savefiles_dir_option == 0) && (strcmp(emu_savefiles_path, "")))
    //     geargrafx->SaveRam(emu_savefiles_path);
    // else
    //     geargrafx->SaveRam();
}

static void load_ram(void)
{
    // if ((emu_savefiles_dir_option == 0) && (strcmp(emu_savefiles_path, "")))
    //     geargrafx->LoadRam(emu_savefiles_path);
    // else
    //     geargrafx->LoadRam();
}

static void init_debug(void)
{

}

static void destroy_debug(void) 
{

}

static void update_debug(void)
{
    update_debug_background_buffer();
    update_debug_tile_buffer();
    update_debug_sprite_buffers();

    // Video* video = geargrafx->GetVideo();

    // video->Render24bit(debug_background_buffer, emu_debug_background_buffer, GC_PIXEL_RGB888, 256 * 256);
    // video->Render24bit(debug_tile_buffer, emu_debug_tile_buffer, GC_PIXEL_RGB888, 32 * 32 * 64);

    // for (int s = 0; s < 64; s++)
    // {
    //     video->Render24bit(debug_sprite_buffers[s], emu_debug_sprite_buffers[s], GC_PIXEL_RGB888, 16 * 16);
    // }
}

static void update_debug_background_buffer(void)
{

}

static void update_debug_tile_buffer(void)
{

}

static void update_debug_sprite_buffers(void)
{

}