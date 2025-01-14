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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include "libretro.h"
#include "../../src/geargrafx.h"

// #ifdef _WIN32
// static const char slash = '\\';
// #else
// static const char slash = '/';
// #endif

#define RETRO_DEVICE_PCE_PAD    RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 0)

#define MAX_PADS 1
#define JOYPAD_BUTTONS 8

static retro_environment_t environ_cb;
static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

static struct retro_log_callback logging;
static retro_log_printf_t log_cb;

static char retro_system_directory[4096];
static char retro_game_path[4096];

static s16 audio_buf[GG_AUDIO_BUFFER_SIZE];
static int audio_sample_count = 0;

static int current_screen_width = 0;
static int current_screen_height = 0;
static float current_aspect_ratio = 0.0f;
static float aspect_ratio = 0.0f;

static bool allow_up_down = false;

static bool libretro_supports_bitmasks;
static int joypad_current[MAX_PADS][JOYPAD_BUTTONS];
static int joypad_old[MAX_PADS][JOYPAD_BUTTONS];
static unsigned input_device[MAX_PADS];

static GG_Keys keymap[] = {
    GG_KEY_UP,
    GG_KEY_DOWN,
    GG_KEY_LEFT,
    GG_KEY_RIGHT,
    GG_KEY_1,
    GG_KEY_2,
    GG_KEY_SELECT,
    GG_KEY_RUN
};

static GeargrafxCore* core;
static u8* frame_buffer;

static void set_controller_info(void);
static void update_input(void);
static void set_variabless(void);
static void check_variables(void);

static void fallback_log(enum retro_log_level level, const char *fmt, ...)
{
    (void)level;
    va_list va;
    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);
}

static int IsButtonPressed(int joypad_bits, int button)
{
    return (joypad_bits & (1 << button)) ? 1 : 0;
}


unsigned retro_api_version(void)
{
    return RETRO_API_VERSION;
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
    audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
    audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
    input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
    input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
    video_cb = cb;
}

void retro_set_environment(retro_environment_t cb)
{
    environ_cb = cb;
    set_controller_info();
    set_variabless();
}

void retro_init(void)
{
    if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
        log_cb = logging.log;
    else
        log_cb = fallback_log;

    const char *dir = NULL;
    if (environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &dir) && dir)
        snprintf(retro_system_directory, sizeof(retro_system_directory), "%s", dir);
    else
        snprintf(retro_system_directory, sizeof(retro_system_directory), "%s", ".");

    log_cb(RETRO_LOG_INFO, "%s (%s) libretro\n", GG_TITLE, GG_VERSION);

    core = new GeargrafxCore();
    core->Init(GG_PIXEL_RGB565);

    frame_buffer = new u8[HUC6270_MAX_RESOLUTION_WIDTH * HUC6270_MAX_RESOLUTION_HEIGHT * 2];

    for (int i = 0; i < MAX_PADS; i++)
    {
        for (int j = 0; j < JOYPAD_BUTTONS; j++)
        {
            joypad_current[i][j] = 0;
            joypad_old[i][j] = 0;
        }
    }

    for (int i = 0; i < MAX_PADS; i++)
        input_device[i] = RETRO_DEVICE_PCE_PAD;

    libretro_supports_bitmasks = environ_cb(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, NULL);
}

void retro_deinit(void)
{
    SafeDeleteArray(frame_buffer);
    SafeDelete(core);
}

void retro_reset(void)
{
    log_cb(RETRO_LOG_DEBUG, "Resetting...\n");

    check_variables();
    core->ResetROM(true);
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
    if (port >= MAX_PADS)
    {
        log_cb(RETRO_LOG_DEBUG, "retro_set_controller_port_device invalid port number: %u\n", port);
        return;
    }

    input_device[port] = device;

    switch ( device )
    {
        case RETRO_DEVICE_NONE:
            log_cb(RETRO_LOG_INFO, "Controller %u: Unplugged\n", port);
            break;
        case RETRO_DEVICE_PCE_PAD:
        case RETRO_DEVICE_JOYPAD:
            log_cb(RETRO_LOG_INFO, "Controller %u: PCE Pad\n", port);
            break;
        default:
            log_cb(RETRO_LOG_DEBUG, "Setting descriptors for unsupported device.\n");
            break;
    }
}

void retro_get_system_info(struct retro_system_info *info)
{
    memset(info, 0, sizeof(*info));
    info->library_name     = "Geargrafx";
    info->library_version  = GG_VERSION;
    info->need_fullpath    = false;
    info->valid_extensions = "pce|sgx|bin|rom";
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
    GG_Runtime_Info runtime_info;
    core->GetRuntimeInfo(runtime_info);

    current_screen_width = runtime_info.screen_width;
    current_screen_height = runtime_info.screen_height;

    info->geometry.base_width   = runtime_info.screen_width;
    info->geometry.base_height  = runtime_info.screen_height;
    info->geometry.max_width    = 1024;//HUC6270_MAX_RESOLUTION_WIDTH;
    info->geometry.max_height   = 512;//HUC6270_MAX_RESOLUTION_HEIGHT;
    info->geometry.aspect_ratio = aspect_ratio;
    info->timing.fps            = 59.82;
    info->timing.sample_rate    = 44100.0;
}

void retro_run(void)
{
    bool core_options_updated = false;
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &core_options_updated) && core_options_updated)
        check_variables();

    update_input();

    audio_sample_count = 0;
    core->RunToVBlank(frame_buffer, audio_buf, &audio_sample_count);

    GG_Runtime_Info runtime_info;
    core->GetRuntimeInfo(runtime_info);

    if ((runtime_info.screen_width != current_screen_width) ||
        (runtime_info.screen_height != current_screen_height) ||
        (aspect_ratio != current_aspect_ratio))
    {
        current_screen_width = runtime_info.screen_width;
        current_screen_height = runtime_info.screen_height;
        current_aspect_ratio = aspect_ratio;

        retro_system_av_info info;
        info.geometry.base_width   = runtime_info.screen_width;
        info.geometry.base_height  = runtime_info.screen_height;
        info.geometry.max_width    = runtime_info.screen_width;
        info.geometry.max_height   = runtime_info.screen_height;
        info.geometry.aspect_ratio = aspect_ratio;

        environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &info.geometry);
    }

    video_cb((uint8_t*)frame_buffer, runtime_info.screen_width, runtime_info.screen_height, runtime_info.screen_width * sizeof(u8) * 2);

    if (audio_sample_count > 0)
        audio_batch_cb(audio_buf, audio_sample_count / 2);
}

bool retro_load_game(const struct retro_game_info *info)
{
    check_variables();

    snprintf(retro_game_path, sizeof(retro_game_path), "%s", info->path);
    log_cb(RETRO_LOG_INFO, "Loading game: %s\n", retro_game_path);

    if (!core->LoadROMFromBuffer(reinterpret_cast<const u8*>(info->data), info->size))
    {
        log_cb(RETRO_LOG_ERROR, "Invalid or corrupted ROM.\n");
        return false;
    }

    enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
    if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
    {
        log_cb(RETRO_LOG_ERROR, "RGB565 is not supported.\n");
        return false;
    }

    Cartridge* cart = core->GetCartridge();
    log_cb(RETRO_LOG_INFO, "CRC: %08X\n", cart->GetCRC());
    log_cb(RETRO_LOG_INFO, "System: %s\n", cart->IsSGX() ? "Super Grafx" : "PC Engine");

    return true;
}

void retro_unload_game(void)
{
}

unsigned retro_get_region(void)
{
    // TODO [libretro] Implement region detection
    return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info)
{
    (void)game_type;
    (void)info;
    (void)num_info;
    return false;
}

size_t retro_serialize_size(void)
{
    // TODO [libretro] Implement save states
    return 0;
}

bool retro_serialize(void *data, size_t size)
{
    // TODO [libretro] Implement save states
    return false;
}

bool retro_unserialize(const void *data, size_t size)
{
    // TODO [libretro] Implement save states
    return false;
}

void *retro_get_memory_data(unsigned id)
{
    switch (id)
    {
        case RETRO_MEMORY_SAVE_RAM:
            return NULL;
        case RETRO_MEMORY_SYSTEM_RAM:
            return NULL;
            // TODO [libretro] Implement memory access for cheevos
            //return core->GetMemory()->GetRam();
    }

    return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
    switch (id)
    {
        case RETRO_MEMORY_SAVE_RAM:
            return 0;
        case RETRO_MEMORY_SYSTEM_RAM:
            return 0;
    }

    return 0;
}

void retro_cheat_reset(void)
{
    // TODO [libretro] Implement cheats
}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
    // TODO [libretro] Implement cheats
}

static void set_controller_info(void)
{
    static const struct retro_controller_description port[] = {
        { "Joypad Auto", RETRO_DEVICE_JOYPAD },
        { "Joypad Port Empty", RETRO_DEVICE_NONE },
        { "PC engine Pad", RETRO_DEVICE_PCE_PAD }
    };

    static const struct retro_controller_info ports[] = {
        { port, 3 },
        { NULL, 0 }
    };

    environ_cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)ports);

    struct retro_input_descriptor joypad[] = {
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "Up" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "Down" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "Left" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Right" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "I" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "II" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Run" },

        { 0, 0, 0, 0, NULL }
    };

    environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, joypad);
}

static void update_input(void)
{
    int16_t joypad_bits[MAX_PADS];

    input_poll_cb();

    if (libretro_supports_bitmasks)
    {
        for (int j = 0; j < MAX_PADS; j++)
            joypad_bits[j] = input_state_cb(j, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);
    }
    else
    {
        for (int j = 0; j < MAX_PADS; j++)
        {
            joypad_bits[j] = 0;
            for (int i = 0; i < (RETRO_DEVICE_ID_JOYPAD_R3+1); i++)
                joypad_bits[j] |= input_state_cb(j, RETRO_DEVICE_JOYPAD, 0, i) ? (1 << i) : 0;
        }
    }

    // Copy previous state
    for (int j = 0; j < MAX_PADS; j++)
    {
        for (int i = 0; i < JOYPAD_BUTTONS; i++)
            joypad_old[j][i] = joypad_current[j][i];
    }

    // Get current state
    for (int j = 0; j < MAX_PADS; j++)
    {
        int up_pressed = IsButtonPressed(joypad_bits[j], RETRO_DEVICE_ID_JOYPAD_UP);
        int down_pressed = IsButtonPressed(joypad_bits[j], RETRO_DEVICE_ID_JOYPAD_DOWN);
        int left_pressed = IsButtonPressed(joypad_bits[j], RETRO_DEVICE_ID_JOYPAD_LEFT);
        int right_pressed = IsButtonPressed(joypad_bits[j], RETRO_DEVICE_ID_JOYPAD_RIGHT);

        if (allow_up_down)
        {
            joypad_current[j][0] = up_pressed;
            joypad_current[j][1] = down_pressed;
            joypad_current[j][2] = left_pressed;
            joypad_current[j][3] = right_pressed;
        }
        else
        {
            joypad_current[j][0] = (up_pressed && (!down_pressed || joypad_old[j][0])) ? 1 : 0;
            joypad_current[j][1] = (down_pressed && (!up_pressed || joypad_old[j][1])) ? 1 : 0;
            joypad_current[j][2] = (left_pressed && (!right_pressed || joypad_old[j][2])) ? 1 : 0;
            joypad_current[j][3] = (right_pressed && (!left_pressed || joypad_old[j][3])) ? 1 : 0;
        }

        joypad_current[j][4] = IsButtonPressed(joypad_bits[j], RETRO_DEVICE_ID_JOYPAD_A);
        joypad_current[j][5] = IsButtonPressed(joypad_bits[j], RETRO_DEVICE_ID_JOYPAD_B);
        joypad_current[j][6] = IsButtonPressed(joypad_bits[j], RETRO_DEVICE_ID_JOYPAD_SELECT);
        joypad_current[j][7] = IsButtonPressed(joypad_bits[j], RETRO_DEVICE_ID_JOYPAD_START);
    }

    for (int j = 0; j < MAX_PADS; j++)
    {
        for (int i = 0; i < JOYPAD_BUTTONS; i++)
        {
            if (joypad_current[j][i] != joypad_old[j][i])
            {
                if (joypad_current[j][i])
                    core->KeyPressed(static_cast<GG_Controllers>(j), keymap[i]);
                else
                    core->KeyReleased(static_cast<GG_Controllers>(j), keymap[i]);
            }
        }
    }
}

static void set_variabless(void)
{
    struct retro_variable vars[] = {
        { "geargrafx_aspect_ratio", "Aspect Ratio (restart); 1:1 PAR|4:3 DAR|16:9 DAR|16:10 DAR" },
        { "geargrafx_up_down_allowed", "Allow Up+Down / Left+Right; Disabled|Enabled" },
        { "geargrafx_no_sprite_limit", "No Sprite Limit; Disabled|Enabled" },
        { NULL }
    };

    environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, (void *)vars);
}

static void check_variables(void)
{
    struct retro_variable var = {0};

    var.key = "geargrafx_up_down_allowed";
    var.value = NULL;

    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
    {
        if (strcmp(var.value, "Enabled") == 0)
            allow_up_down = true;
        else
            allow_up_down = false;
    }

    var.key = "geargrafx_aspect_ratio";
    var.value = NULL;

    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
    {
        if (strcmp(var.value, "1:1 PAR") == 0)
            aspect_ratio = 0.0f;
        else if (strcmp(var.value, "4:3 DAR") == 0)
            aspect_ratio = 4.0f / 3.0f;
        else if (strcmp(var.value, "16:9 DAR") == 0)
            aspect_ratio = 16.0f / 9.0f;
        else if (strcmp(var.value, "16:10 DAR") == 0)
            aspect_ratio = 16.0f / 10.0f;
        else
            aspect_ratio = 0.0f;
    }

    var.key = "geargrafx_no_sprite_limit";
    var.value = NULL;

    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
    {
        core->GetHuC6270()->SetNoSpriteLimit(strcmp(var.value, "Enabled") == 0);
    }
}
