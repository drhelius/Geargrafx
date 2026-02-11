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

#include <string>
#include <assert.h>

#define SOUND_QUEUE_IMPORT
#include "sound_queue.h"

static s16* volatile sound_queue_buffers;
static SDL_sem* volatile sound_queue_free_sem;
static s16* volatile sound_queue_currently_playing;
static int volatile sound_queue_read_buffer;
static int volatile sound_queue_write_buffer;
static int sound_queue_write_position;
static bool sound_queue_sound_open;
static bool sound_queue_sync_output;
static int sound_queue_buffer_size;
static int sound_queue_buffer_count;

static void sdl_error(const char* str);
static s16* get_buffer(int index);
static void fill_buffer(u8* buffer, int count);
static void fill_buffer_callback(void* user_data, u8* buffer, int count);
static bool is_running_in_wsl(void);

static void sdl_error(const char* str)
{
    const char* sdl_str = SDL_GetError();
    if (sdl_str && *sdl_str)
        Log("Sound Queue: %s - SDL Error: %s", str, sdl_str);
    else
        Log("Sound Queue: %s", str);
}

void sound_queue_init(void)
{
    sound_queue_buffers = NULL;
    sound_queue_free_sem = NULL;
    sound_queue_sound_open = false;
    sound_queue_sync_output = true;

    int audio_drivers_count = SDL_GetNumAudioDrivers();

    Debug("SoundQueue: %d audio backends", audio_drivers_count);

    for (int i = 0; i < audio_drivers_count; i++)
    {
        Debug("SoundQueue: %s", SDL_GetAudioDriver(i));
    }

    std::string platform = SDL_GetPlatform();
    if (platform == "Linux")
    {
        if (is_running_in_wsl())
        {
            Debug("SoundQueue: Running in WSL");
            SDL_SetHint("SDL_AUDIODRIVER", "pulseaudio");
        }
        else
        {
            Debug("SoundQueue: Running in Linux");
        }
    }

    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
        sdl_error("Couldn't init AUDIO subsystem");

    Log("SoundQueue: %s driver selected", SDL_GetCurrentAudioDriver());

    int audio_devices_count = SDL_GetNumAudioDevices(0);

    Debug("SoundQueue: %d audio devices", audio_devices_count);

    for (int i = 0; i < audio_devices_count; i++)
    {
        Debug("SoundQueue: %s", SDL_GetAudioDeviceName(i, 0));
    }
}

void sound_queue_destroy(void)
{
    sound_queue_stop();
}

bool sound_queue_start(int sample_rate, int channel_count, int buffer_size, int buffer_count)
{
    Log("SoundQueue: Starting with %d Hz, %d channels, %d buffer size, %d buffers ...", sample_rate, channel_count, buffer_size, buffer_count);

    sound_queue_write_buffer = 0;
    sound_queue_write_position = 0;
    sound_queue_read_buffer = 0;
    sound_queue_buffer_size = buffer_size;
    sound_queue_buffer_count = buffer_count;

    sound_queue_buffers = new s16[sound_queue_buffer_size * sound_queue_buffer_count];
    sound_queue_currently_playing = sound_queue_buffers;

    for (int i = 0; i < (sound_queue_buffer_size * sound_queue_buffer_count); i++)
        sound_queue_buffers[i] = 0;

    sound_queue_free_sem = SDL_CreateSemaphore(sound_queue_buffer_count - 1);
    if (!sound_queue_free_sem)
    {
        sdl_error("Couldn't create semaphore");
        return false;
    }

    SDL_AudioSpec spec;
    spec.freq = sample_rate;
    spec.format = AUDIO_S16SYS;
    spec.channels = channel_count;
    spec.silence = 0;
    spec.samples = sound_queue_buffer_size / channel_count;
    spec.size = 0;
    spec.callback = fill_buffer_callback;
    spec.userdata = NULL;

    Log("SoundQueue: Desired -  frequency: %d format: f %d s %d be %d sz %d channels: %d samples: %d", spec.freq, SDL_AUDIO_ISFLOAT(spec.format), SDL_AUDIO_ISSIGNED(spec.format), SDL_AUDIO_ISBIGENDIAN(spec.format), SDL_AUDIO_BITSIZE(spec.format), spec.channels, spec.samples);

    if (SDL_OpenAudio(&spec, NULL) < 0)
    {
        sdl_error("Couldn't open SDL audio");
        return false;
    }

    Log("SoundQueue: Obtained - frequency: %d format: f %d s %d be %d sz %d channels: %d samples: %d", spec.freq, SDL_AUDIO_ISFLOAT(spec.format), SDL_AUDIO_ISSIGNED(spec.format), SDL_AUDIO_ISBIGENDIAN(spec.format), SDL_AUDIO_BITSIZE(spec.format), spec.channels, spec.samples);

    SDL_PauseAudio(false);
    sound_queue_sound_open = true;

    return true;
}

void sound_queue_stop(void)
{
    if (sound_queue_sound_open)
    {
        sound_queue_sound_open = false;
        SDL_PauseAudio(true);
        SDL_CloseAudio();
    }

    if (sound_queue_free_sem)
    {
        SDL_DestroySemaphore(sound_queue_free_sem);
        sound_queue_free_sem = NULL;
    }

    delete [] sound_queue_buffers;
    sound_queue_buffers = NULL;
}

int sound_queue_get_sample_count(void)
{
    int buffer_free = SDL_SemValue(sound_queue_free_sem) * sound_queue_buffer_size + (sound_queue_buffer_size - sound_queue_write_position);
    return sound_queue_buffer_size * sound_queue_buffer_count - buffer_free;
}

s16* sound_queue_get_currently_playing(void)
{
    return sound_queue_currently_playing;
}

bool sound_queue_is_open(void)
{
    return sound_queue_sound_open;
}

void sound_queue_write(s16* samples, int count, bool sync)
{
    if (!sound_queue_sound_open)
        return;

    sound_queue_sync_output = sync;

    while (count)
    {
        int n = sound_queue_buffer_size - sound_queue_write_position;
        if (n > count)
            n = count;

        memcpy(get_buffer(sound_queue_write_buffer) + sound_queue_write_position, samples, n * sizeof(s16));
        samples += n;
        sound_queue_write_position += n;
        count -= n;

        if (sound_queue_write_position >= sound_queue_buffer_size)
        {
            sound_queue_write_position = 0;

            if (sound_queue_sync_output)
            {
                sound_queue_write_buffer = (sound_queue_write_buffer + 1) % sound_queue_buffer_count;
                SDL_SemWait(sound_queue_free_sem);
            }
            else
            {
                int next = (sound_queue_write_buffer + 1) % sound_queue_buffer_count;
                if (next != sound_queue_read_buffer)
                    sound_queue_write_buffer = next;
            }
        }
    }
}

static s16* get_buffer(int index)
{
    assert(index < sound_queue_buffer_count);
    return sound_queue_buffers + (index * sound_queue_buffer_size);
}

static void fill_buffer(u8* buffer, int count)
{
    bool has_data;

    if (sound_queue_sync_output)
        has_data = (SDL_SemValue(sound_queue_free_sem) < (unsigned int)sound_queue_buffer_count - 1);
    else
        has_data = (sound_queue_read_buffer != sound_queue_write_buffer);

    if (has_data)
    {
        sound_queue_currently_playing = get_buffer(sound_queue_read_buffer);
        memcpy(buffer, get_buffer(sound_queue_read_buffer), count);
        sound_queue_read_buffer = (sound_queue_read_buffer + 1) % sound_queue_buffer_count;

        if (sound_queue_sync_output)
            SDL_SemPost(sound_queue_free_sem);
    }
    else
    {
        memset(buffer, 0, count);
    }
}

static void fill_buffer_callback(void* user_data, u8* buffer, int count)
{
    (void)user_data;
    fill_buffer(buffer, count);
}

static bool is_running_in_wsl(void)
{
    FILE *file;

    if ((file = fopen("/proc/sys/fs/binfmt_misc/WSLInterop", "r")))
    {
        fclose(file);
        return true;
    }

    return false;
}