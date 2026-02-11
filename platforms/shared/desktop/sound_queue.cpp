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
#include <string.h>

#define SOUND_QUEUE_IMPORT
#include "sound_queue.h"
#include "utils.h"

static SDL_AudioStream* sound_queue_stream;
static bool sound_queue_sound_open;
static int sound_queue_max_queued_bytes;
static int sound_queue_buffer_size;
static s16* sound_queue_last_written;

static bool is_running_in_wsl(void);

void sound_queue_init(void)
{
    InitPointer(sound_queue_stream);
    InitPointer(sound_queue_last_written);
    sound_queue_sound_open = false;

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

    if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
        SDL_ERROR("SDL_InitSubSystem(SDL_INIT_AUDIO)");

    Log("SoundQueue: %s driver selected", SDL_GetCurrentAudioDriver());

    int audio_devices_count = 0;
    SDL_AudioDeviceID* devices = SDL_GetAudioPlaybackDevices(&audio_devices_count);

    Debug("SoundQueue: %d audio devices", audio_devices_count);

    if (devices)
    {
        for (int i = 0; i < audio_devices_count; i++)
        {
            Debug("SoundQueue: %s", SDL_GetAudioDeviceName(devices[i]));
        }
        SDL_free(devices);
    }
}

void sound_queue_destroy(void)
{
    sound_queue_stop();
}

bool sound_queue_start(int sample_rate, int channel_count, int buffer_size, int buffer_count)
{
    Log("SoundQueue: Starting with %d Hz, %d channels, %d buffer size, %d buffers ...", sample_rate, channel_count, buffer_size, buffer_count);

    sound_queue_buffer_size = buffer_size;
    sound_queue_max_queued_bytes = buffer_size * buffer_count * (int)sizeof(s16);

    sound_queue_last_written = new s16[buffer_size];
    memset(sound_queue_last_written, 0, buffer_size * sizeof(s16));

    SDL_AudioSpec spec;
    spec.freq = sample_rate;
    spec.format = SDL_AUDIO_S16;
    spec.channels = channel_count;

    Log("SoundQueue: Desired - frequency: %d format: 0x%04X channels: %d", spec.freq, spec.format, spec.channels);

    sound_queue_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);

    if (!sound_queue_stream)
    {
        SDL_ERROR("SDL_OpenAudioDeviceStream");
        return false;
    }

    Log("SoundQueue: Audio stream opened successfully");

    SDL_ResumeAudioStreamDevice(sound_queue_stream);
    sound_queue_sound_open = true;

    return true;
}

void sound_queue_stop(void)
{
    if (sound_queue_sound_open)
    {
        sound_queue_sound_open = false;
        if (sound_queue_stream)
        {
            SDL_PauseAudioStreamDevice(sound_queue_stream);
            SDL_DestroyAudioStream(sound_queue_stream);
            InitPointer(sound_queue_stream);
        }

        Log("SoundQueue: Stopped");
    }

    SafeDeleteArray(sound_queue_last_written);
}

int sound_queue_get_sample_count(void)
{
    if (!sound_queue_stream)
        return 0;
    return SDL_GetAudioStreamQueued(sound_queue_stream) / (int)sizeof(s16);
}

s16* sound_queue_get_currently_playing(void)
{
    return sound_queue_last_written;
}

bool sound_queue_is_open(void)
{
    return sound_queue_sound_open;
}

void sound_queue_write(s16* samples, int count, bool sync)
{
    if (!sound_queue_sound_open || !sound_queue_stream)
        return;

    int bytes = count * (int)sizeof(s16);

    if (sync)
    {
        int queued = SDL_GetAudioStreamQueued(sound_queue_stream);
        if (queued >= sound_queue_max_queued_bytes)
        {
            Debug("SoundQueue: Sync overrun, queued %d >= max %d, waiting...", queued, sound_queue_max_queued_bytes);
            while (SDL_GetAudioStreamQueued(sound_queue_stream) >= sound_queue_max_queued_bytes)
                SDL_Delay(1);
        }
    }
    else
    {
        if (SDL_GetAudioStreamQueued(sound_queue_stream) >= sound_queue_max_queued_bytes)
        {
            Debug("SoundQueue: Async overrun, clearing stream (queued %d >= max %d)", SDL_GetAudioStreamQueued(sound_queue_stream), sound_queue_max_queued_bytes);
            SDL_ClearAudioStream(sound_queue_stream);
        }
    }

    SDL_PutAudioStreamData(sound_queue_stream, samples, bytes);

    int copy_count = count < sound_queue_buffer_size ? count : sound_queue_buffer_size;
    memcpy(sound_queue_last_written, samples + (count - copy_count), copy_count * sizeof(s16));
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
