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
#define EMU_IMPORT
#include "emu.h"

#include "geargrafx.h"
#include "sound_queue.h"
#include "config.h"
#include "mcp/mcp_manager.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#if defined(_WIN32)
#define STBIW_WINDOWS_UTF8
#endif
#include "stb_image_write.h"

static GeargrafxCore* geargrafx;
static SoundQueue* sound_queue;
static s16* audio_buffer;
static bool audio_enabled;
static McpManager* mcp_manager;

static void save_ram(void);
static void load_ram(void);
static void save_mb128(void);
static void load_mb128(void);
static void reset_buffers(void);
static const char* get_configurated_dir(int option, const char* path); 
static void init_debug(void);
static void destroy_debug(void);
static void update_debug(void);
static void update_debug_background(void);
static void update_debug_sprites(void);

bool emu_init(GG_Input_Pump_Fn input_pump_fn)
{
    emu_frame_buffer = new u8[2048 * 512 * 4];
    audio_buffer = new s16[GG_AUDIO_BUFFER_SIZE];

    init_debug();
    reset_buffers();

    geargrafx = new GeargrafxCore();
    geargrafx->Init(input_pump_fn);
    geargrafx->GetMedia()->SetTempPath(config_temp_path);

    sound_queue = new SoundQueue();
    if (!sound_queue->Start(GG_AUDIO_SAMPLE_RATE, 2, GG_AUDIO_BUFFER_SIZE, GG_AUDIO_BUFFER_COUNT))
        return false;

    for (int i = 0; i < 5; i++)
        InitPointer(emu_savestates_screenshots[i].data);

    audio_enabled = true;
    emu_audio_sync = true;
    emu_debug_disable_breakpoints = false;
    emu_debug_irq_breakpoints = false;
    emu_debug_command = Debug_Command_None;
    emu_debug_pc_changed = false;
    emu_debug_step_frames_pending = 0;

    mcp_manager = new McpManager();
    mcp_manager->Init(geargrafx);

    return true;
}

void emu_destroy(void)
{
    save_ram();
    save_mb128();
    SafeDelete(mcp_manager);
    SafeDeleteArray(audio_buffer);
    SafeDelete(sound_queue);
    SafeDelete(geargrafx);
    SafeDeleteArray(emu_frame_buffer);
    destroy_debug();

    for (int i = 0; i < 5; i++)
        SafeDeleteArray(emu_savestates_screenshots[i].data);
}

bool emu_load_media(const char* file_path)
{
    emu_debug_command = Debug_Command_None;
    reset_buffers();

    save_ram();
    save_mb128();
    if (!geargrafx->LoadMedia(file_path))
        return false;
    load_ram();
    load_mb128();

    if (config_debug.debug && (config_debug.dis_look_ahead_count > 0))
        geargrafx->GetHuC6280()->DisassembleAhead(config_debug.dis_look_ahead_count);

    update_savestates_data();

    return true;
}

void emu_update(void)
{
    emu_mcp_pump_commands();

    if (emu_is_empty())
        return;

    int sampleCount = 0;

    if (config_debug.debug)
    {
        bool breakpoint_hit = false;
        GeargrafxCore::GG_Debug_Run debug_run;
        debug_run.step_debugger = (emu_debug_command == Debug_Command_Step);
        debug_run.stop_on_breakpoint = !emu_debug_disable_breakpoints;
        debug_run.stop_on_run_to_breakpoint = true;
        debug_run.stop_on_irq = emu_debug_irq_breakpoints;

        if (emu_debug_command != Debug_Command_None)
            breakpoint_hit = geargrafx->RunToVBlank(emu_frame_buffer, audio_buffer, &sampleCount, &debug_run);

        if (breakpoint_hit || emu_debug_command == Debug_Command_StepFrame || emu_debug_command == Debug_Command_Step)
        {
            emu_debug_pc_changed = true;

            if (config_debug.dis_look_ahead_count > 0)
                geargrafx->GetHuC6280()->DisassembleAhead(config_debug.dis_look_ahead_count);
        }

        if (breakpoint_hit)
            emu_debug_command = Debug_Command_None;

        if (emu_debug_command == Debug_Command_StepFrame && emu_debug_step_frames_pending > 0)
        {
            emu_debug_step_frames_pending--;
            if (emu_debug_step_frames_pending > 0)
                emu_debug_command = Debug_Command_StepFrame;
            else
                emu_debug_command = Debug_Command_None;
        }
        else if (emu_debug_command != Debug_Command_Continue)
            emu_debug_command = Debug_Command_None;

        update_debug();
    }
    else
        geargrafx->RunToVBlank(emu_frame_buffer, audio_buffer, &sampleCount);

    if ((sampleCount > 0) && !geargrafx->IsPaused())
    {
        sound_queue->Write(audio_buffer, sampleCount, emu_audio_sync);
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

bool emu_is_debug_idle(void)
{
    return config_debug.debug && (emu_debug_command == Debug_Command_None);
}

bool emu_is_empty(void)
{
    return !geargrafx->GetMedia()->IsReady();
}

void emu_reset(void)
{
    emu_debug_command = Debug_Command_None;
    reset_buffers();

    save_ram();
    save_mb128();
    geargrafx->ResetMedia(false);
    load_ram();
    load_mb128();
}

void emu_audio_huc6280a(bool enabled)
{
    geargrafx->GetAudio()->GetPSG()->EnableHuC6280A(enabled);
}

void emu_audio_mute(bool mute)
{
    audio_enabled = !mute;
    geargrafx->GetAudio()->Mute(mute);
}

void emu_audio_psg_volume(float volume)
{
    geargrafx->GetAudio()->SetPSGVolume(volume);
}

void emu_audio_adpcm_volume(float volume)
{
    geargrafx->GetAudio()->SetADPCMVolume(volume);
}

void emu_audio_cdrom_volume(float volume)
{
    geargrafx->GetAudio()->SetCDROMVolume(volume);
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

bool emu_is_audio_open(void)
{
    return sound_queue->IsOpen();
}

void emu_save_ram(const char* file_path)
{
    if (!emu_is_empty())
        geargrafx->SaveRam(file_path, true);
}

void emu_load_ram(const char* file_path)
{
    if (!emu_is_empty())
    {
        save_ram();
        geargrafx->ResetMedia(false);
        geargrafx->LoadRam(file_path, true);
    }
}

void emu_save_state_slot(int index)
{
    if (!emu_is_empty())
    {
        const char* dir = get_configurated_dir(config_emulator.savestates_dir_option, config_emulator.savestates_path.c_str());
        geargrafx->SaveState(dir, index, true);
        update_savestates_data();
    }
}

void emu_load_state_slot(int index)
{
    if (!emu_is_empty())
    {
        const char* dir = get_configurated_dir(config_emulator.savestates_dir_option, config_emulator.savestates_path.c_str());
        geargrafx->LoadState(dir, index);
    }
}

void emu_save_state_file(const char* file_path)
{
    if (!emu_is_empty())
        geargrafx->SaveState(file_path, -1, true);
}

void emu_load_state_file(const char* file_path)
{
    if (!emu_is_empty())
        geargrafx->LoadState(file_path);
}

void update_savestates_data(void)
{
    if (emu_is_empty())
        return;

    for (int i = 0; i < 5; i++)
    {
        emu_savestates[i].rom_name[0] = 0;
        SafeDeleteArray(emu_savestates_screenshots[i].data);

        const char* dir = get_configurated_dir(config_emulator.savestates_dir_option, config_emulator.savestates_path.c_str());

        if (!geargrafx->GetSaveStateHeader(i + 1, dir, &emu_savestates[i]))
            continue;

        if (emu_savestates[i].screenshot_size > 0)
        {
            emu_savestates_screenshots[i].data = new u8[emu_savestates[i].screenshot_size];
            emu_savestates_screenshots[i].size = emu_savestates[i].screenshot_size;
            geargrafx->GetSaveStateScreenshot(i + 1, dir, &emu_savestates_screenshots[i]);
        }
    }
}


void emu_get_runtime(GG_Runtime_Info& runtime)
{
    geargrafx->GetRuntimeInfo(runtime);
}

void emu_get_info(char* info, int buffer_size)
{
    if (!emu_is_empty())
    {
        Media* media = geargrafx->GetMedia();
        GG_Runtime_Info runtime;
        geargrafx->GetRuntimeInfo(runtime);

        const char* filename = media->GetFileName();
        u32 crc = media->GetCRC();
        int rom_size = media->GetROMSize();
        const char* is_sgx = media->IsSGX() ? "YES" : "NO";
        const char* is_cdrom = media->IsCDROM() ? "YES" : "NO";

        snprintf(info, buffer_size, "File Name: %s\nCRC: %08X\nROM Size: %d bytes, %d KB\nSuperGrafx: %s\nCD-ROM: %s\nScreen Resolution: %dx%d", filename, crc, rom_size, rom_size / 1024, is_sgx, is_cdrom, runtime.screen_width, runtime.screen_height);
    }
    else
    {
        snprintf(info, buffer_size, "There is no ROM loaded!");
    }
}

GeargrafxCore* emu_get_core(void)
{
    return geargrafx;
}

void emu_debug_step_over(void)
{
    HuC6280* processor = emu_get_core()->GetHuC6280();
    HuC6280::HuC6280_State* proc_state = processor->GetState();
    Memory* memory = emu_get_core()->GetMemory();
    u16 pc = proc_state->PC->GetValue();
    GG_Disassembler_Record* record = memory->GetDisassemblerRecord(proc_state->PC->GetValue());

    if (IsValidPointer(record) && record->subroutine)
    {
        u16 return_address = pc + record->size;
        processor->AddRunToBreakpoint(return_address);
        emu_debug_command = Debug_Command_Continue;
    }
    else
        emu_debug_command = Debug_Command_Step;

    geargrafx->Pause(false);
}

void emu_debug_step_into(void)
{
    geargrafx->Pause(false);
    emu_debug_command = Debug_Command_Step;
}

void emu_debug_step_out(void)
{
    HuC6280* processor = emu_get_core()->GetHuC6280();
    std::stack<HuC6280::GG_CallStackEntry>* call_stack = processor->GetDisassemblerCallStack();

    if (call_stack->size() > 0)
    {
        HuC6280::GG_CallStackEntry entry = call_stack->top();
        u16 return_address = entry.back;
        processor->AddRunToBreakpoint(return_address);
        emu_debug_command = Debug_Command_Continue;
    }
    else
        emu_debug_command = Debug_Command_Step;

    geargrafx->Pause(false);
}

void emu_debug_step_frame(void)
{
    geargrafx->Pause(false);
    emu_debug_step_frames_pending++;
    emu_debug_command = Debug_Command_StepFrame;
}

void emu_debug_break(void)
{
    geargrafx->Pause(false);
    if (emu_debug_command == Debug_Command_Continue)
        emu_debug_command = Debug_Command_Step;
}

void emu_debug_continue(void)
{
    geargrafx->Pause(false);
    emu_debug_command = Debug_Command_Continue;
}

void emu_debug_set_callback(GeargrafxCore::GG_Debug_Callback callback)
{
    geargrafx->SetDebugCallback(callback);
}

void emu_set_palette(int palette)
{
    geargrafx->GetHuC6260()->SetPalette(palette);
}

void emu_set_custom_palette(const u8* data)
{
    geargrafx->GetHuC6260()->SetCustomPalette(data);
}

void emu_video_no_sprite_limit(bool enabled)
{
    geargrafx->GetHuC6270_1()->SetNoSpriteLimit(enabled);
    geargrafx->GetHuC6270_2()->SetNoSpriteLimit(enabled);
}

void emu_set_overscan(int overscan)
{
    geargrafx->GetHuC6260()->SetOverscan(overscan == 0 ? false : true);
}

void emu_set_scanline_start_end(int start, int end)
{
    geargrafx->GetHuC6260()->SetScanlineStart(start);
    geargrafx->GetHuC6260()->SetScanlineEnd(end);
}

void emu_set_memory_reset_values(int mpr, int wram, int card_ram, int arcade_card)
{
    geargrafx->GetMemory()->SetResetValues(mpr, wram, card_ram, arcade_card);
}

void emu_set_huc6260_color_table_reset_value(int value)
{
    geargrafx->GetHuC6260()->SetResetValue(value);
}

void emu_set_huc6280_registers_reset_value(int value)
{
    geargrafx->GetHuC6280()->SetResetValue(value);
}

void emu_set_console_type(GG_Console_Type console_type)
{
    geargrafx->GetMedia()->SetConsoleType(console_type);
}
void emu_set_cdrom_type(GG_CDROM_Type cdrom_type)
{
    geargrafx->GetMedia()->SetCDROMType(cdrom_type);
}

void emu_set_preload_cdrom(bool enabled)
{
    geargrafx->GetMedia()->PreloadCdRom(enabled);
}

void emu_set_backup_ram(bool enabled)
{
    geargrafx->GetMedia()->ForceBackupRAM(enabled);
}

void emu_set_turbo_tap(bool enabled)
{
    geargrafx->GetInput()->EnableTurboTap(enabled);
}

void emu_set_mb128_mode(GG_MB128_Mode mode)
{
    bool was_connected = geargrafx->GetInput()->GetMB128()->IsConnected();

    geargrafx->EnableMB128(mode);

    bool is_connected = geargrafx->GetInput()->GetMB128()->IsConnected();

    if (is_connected && !was_connected)
        load_mb128();
    else if (!is_connected && was_connected)
        save_mb128();
}

void emu_set_pad_type(GG_Controllers controller, GG_Controller_Type type)
{
    geargrafx->GetInput()->SetControllerType(controller, type);
}

GG_Controller_Type emu_get_pad_type(GG_Controllers controller)
{
    return geargrafx->GetInput()->GetControllerType(controller);
}

void emu_set_avenue_pad_3_button(GG_Controllers controller, GG_Keys button)
{
    geargrafx->GetInput()->SetAvenuePad3Button(controller, button);
}

void emu_set_turbo(GG_Controllers controller, GG_Keys button, bool enabled)
{
    geargrafx->GetInput()->EnableTurbo(controller, button, enabled);
}

void emu_set_turbo_speed(GG_Controllers controller, GG_Keys button, u8 speed)
{
    geargrafx->GetInput()->SetTurboSpeed(controller, button, speed);
}

void emu_save_screenshot(const char* file_path)
{
    if (!geargrafx->GetMedia()->IsReady())
        return;

    GG_Runtime_Info runtime;
    emu_get_runtime(runtime);

    stbi_write_png(file_path, runtime.screen_width, runtime.screen_height, 4, emu_frame_buffer, runtime.screen_width * 4);

    Log("Screenshot saved to %s", file_path);
}

int emu_get_screenshot_png(unsigned char** out_buffer)
{
    if (!geargrafx->GetMedia()->IsReady())
        return 0;

    GG_Runtime_Info runtime;
    emu_get_runtime(runtime);

    int stride = runtime.screen_width * 4;
    int len = 0;

    *out_buffer = stbi_write_png_to_mem(emu_frame_buffer, stride, 
                                         runtime.screen_width, runtime.screen_height, 
                                         4, &len);

    return len;
}

int emu_get_sprite_png(int vdc, int sprite_index, unsigned char** out_buffer)
{
    if (!geargrafx->GetMedia()->IsReady())
        return 0;

    if (vdc < 0 || vdc > 1 || sprite_index < 0 || sprite_index > 63)
        return 0;

    update_debug_sprites();

    int width = emu_debug_sprite_widths[vdc][sprite_index];
    int height = emu_debug_sprite_heights[vdc][sprite_index];
    u8* buffer = emu_debug_sprite_buffers[vdc][sprite_index];

    if (!buffer || width == 0 || height == 0)
        return 0;

    int len = 0;
    *out_buffer = stbi_write_png_to_mem(buffer, width * 4, width, height, 4, &len);

    return len;
}

void emu_save_sprite(const char* file_path, int vdc, int index)
{
    if (!geargrafx->GetMedia()->IsReady())
        return;

    update_debug_sprites();

    int width = emu_debug_sprite_widths[vdc][index];
    int height = emu_debug_sprite_heights[vdc][index];
    u8* buffer = emu_debug_sprite_buffers[vdc][index];

    stbi_write_png(file_path, width, height, 4, buffer, width * 4);

    Log("Sprite saved to %s", file_path);
}

void emu_save_background(const char* file_path, int vdc)
{
    if (!geargrafx->GetMedia()->IsReady())
        return;

    update_debug_background();

    int width = emu_debug_background_buffer_width[vdc];
    int height = emu_debug_background_buffer_height[vdc];
    u8* buffer = emu_debug_background_buffer[vdc];

    stbi_write_png(file_path, width, height, 4, buffer, width * 4);

    Log("Background saved to %s", file_path);
}

bool emu_load_bios(const char* file_path, bool syscard)
{
    return geargrafx->LoadBios(file_path, syscard);
}

static void save_ram(void)
{
    const char* dir = get_configurated_dir(config_emulator.backup_ram_dir_option, config_emulator.backup_ram_path.c_str());
    geargrafx->SaveRam(dir);
}

static void load_ram(void)
{
    const char* dir = get_configurated_dir(config_emulator.backup_ram_dir_option, config_emulator.backup_ram_path.c_str());
    geargrafx->LoadRam(dir);
}

static void save_mb128(void)
{
    if (geargrafx->GetInput()->GetMB128()->IsDirty())
    {
        const char* dir = (config_emulator.mb128_dir_option == 0) ? config_root_path : config_emulator.mb128_path.c_str();
        geargrafx->SaveMB128(dir);
    }
}

static void load_mb128(void)
{
    const char* dir = (config_emulator.mb128_dir_option == 0) ? config_root_path : config_emulator.mb128_path.c_str();
    geargrafx->LoadMB128(dir);
}

static void reset_buffers(void)
{
    emu_debug_background_buffer_width[0] = 32;
    emu_debug_background_buffer_height[0] = 32;
    emu_debug_background_buffer_width[1] = 32;
    emu_debug_background_buffer_height[1] = 32;

    for (int i = 0; i < 2048 * 512 * 4; i++)
        emu_frame_buffer[i] = 0;

    for (int i = 0; i < GG_AUDIO_BUFFER_SIZE; i++)
        audio_buffer[i] = 0;

    for (int i = 0; i < HUC6270_MAX_BACKGROUND_WIDTH * HUC6270_MAX_BACKGROUND_HEIGHT * 4; i++)
        emu_debug_background_buffer[0][i] = 0;

    for (int i = 0; i < HUC6270_MAX_BACKGROUND_WIDTH * HUC6270_MAX_BACKGROUND_HEIGHT * 4; i++)
        emu_debug_background_buffer[1][i] = 0;

    for (int i = 0; i < 2; i++)
        for (int s = 0; s < 64; s++)
        {
            for (int j = 0; j < HUC6270_MAX_SPRITE_WIDTH * HUC6270_MAX_SPRITE_HEIGHT * 4; j++)
                emu_debug_sprite_buffers[i][s][j] = 0;

            emu_debug_sprite_widths[i][s] = 16;
            emu_debug_sprite_heights[i][s] = 16;
        }
}

static const char* get_configurated_dir(int location, const char* path)
{
    switch ((Directory_Location)location)
    {
        default:
        case Directory_Location_Default:
            return config_root_path;
        case Directory_Location_ROM:
            return NULL;
        case Directory_Location_Custom:
            return path;
    }
}

static void init_debug(void)
{
    emu_debug_background_buffer[0] = new u8[HUC6270_MAX_BACKGROUND_WIDTH * HUC6270_MAX_BACKGROUND_HEIGHT * 4];
    for (int i = 0; i < HUC6270_MAX_BACKGROUND_WIDTH * HUC6270_MAX_BACKGROUND_HEIGHT * 4; i++)
        emu_debug_background_buffer[0][i] = 0;

    emu_debug_background_buffer[1] = new u8[HUC6270_MAX_BACKGROUND_WIDTH * HUC6270_MAX_BACKGROUND_HEIGHT * 4];
    for (int i = 0; i < HUC6270_MAX_BACKGROUND_WIDTH * HUC6270_MAX_BACKGROUND_HEIGHT * 4; i++)
        emu_debug_background_buffer[1][i] = 0;

    for (int i = 0; i < 2; i++)
        for (int s = 0; s < 64; s++)
        {
            emu_debug_sprite_buffers[i][s] = new u8[HUC6270_MAX_SPRITE_WIDTH * HUC6270_MAX_SPRITE_HEIGHT * 4];
            for (int j = 0; j < HUC6270_MAX_SPRITE_WIDTH * HUC6270_MAX_SPRITE_HEIGHT * 4; j++)
                emu_debug_sprite_buffers[i][s][j] = 0;
        }
}

static void destroy_debug(void) 
{
    SafeDeleteArray(emu_debug_background_buffer[0]);
    SafeDeleteArray(emu_debug_background_buffer[1]);

    for (int i = 0; i < 2; i++)
        for (int s = 0; s < 64; s++)
            SafeDeleteArray(emu_debug_sprite_buffers[i][s]);
}

static void update_debug(void)
{
    update_debug_background();
    update_debug_sprites();
}

static void update_debug_background(void)
{
    bool is_sgx = geargrafx->GetMedia()->IsSGX();
    int vdc_min = 0;
    int vdc_max = is_sgx ? 1 : 0;

    for (int vdc = vdc_min; vdc <= vdc_max; vdc++)
    {
        HuC6260* huc6260 = geargrafx->GetHuC6260();
        HuC6270* huc6270 = vdc == 0 ? geargrafx->GetHuC6270_1() : geargrafx->GetHuC6270_2();
        HuC6270::HuC6270_State* huc6270_state = huc6270->GetState();
        u16* vram = huc6270->GetVRAM();
        int screen_reg = (huc6270_state->R[HUC6270_REG_MWR] >> 4) & 0x07;
        int screen_size_x = k_huc6270_screen_size_x[screen_reg];
        int screen_size_y = k_huc6270_screen_size_y[screen_reg];
        emu_debug_background_buffer_width[vdc] = screen_size_x * 8;
        emu_debug_background_buffer_height[vdc] = screen_size_y * 8;
        int bat_size = screen_size_x * screen_size_y;
        int pixels = bat_size * 8 * 8;

        for (int pixel = 0; pixel < pixels; pixel++)
        {
            int x = pixel % emu_debug_background_buffer_width[vdc];
            int y = pixel / emu_debug_background_buffer_width[vdc];

            int bat_entry_index = (x / 8) + ((y / 8) * screen_size_x);
            u16 bat_entry = vram[bat_entry_index];
            int tile_index = bat_entry & 0x07FF;
            int color_table = (bat_entry >> 12) & 0x0F;

            int tile_data = tile_index * 16;
            int tile_y =  (y % 8);
            int tile_x = (pixel % 8);

            int line_start_a = (tile_data + tile_y);
            int line_start_b = (tile_data + tile_y + 8);

            u8 byte1 = vram[line_start_a] & 0xFF;
            u8 byte2 = vram[line_start_a] >> 8;
            u8 byte3 = vram[line_start_b] & 0xFF;
            u8 byte4 = vram[line_start_b] >> 8;

            int color = ((byte1 >> (7 - tile_x)) & 0x01) | (((byte2 >> (7 - tile_x)) & 0x01) << 1) | (((byte3 >> (7 - tile_x)) & 0x01) << 2) | (((byte4 >> (7 - tile_x)) & 0x01) << 3);

            if (color == 0)
                color_table = 0;

            u16 color_value = huc6260->GetColorTable()[(color_table * 16) + color];

            // convert to 8 bit color
            int blue = (color_value & 0x07) * 255 / 7;
            int red = ((color_value >> 3) & 0x07) * 255 / 7;
            int green = ((color_value >> 6) & 0x07) * 255 / 7;

            int index = (pixel * 4);
            emu_debug_background_buffer[vdc][index + 0] = red;
            emu_debug_background_buffer[vdc][index + 1] = green;
            emu_debug_background_buffer[vdc][index + 2] = blue;
            emu_debug_background_buffer[vdc][index + 3] = 255;
        }
    }
}

static void update_debug_sprites(void)
{
    bool is_sgx = geargrafx->GetMedia()->IsSGX();
    int vdc_min = 0;
    int vdc_max = is_sgx ? 1 : 0;

    for (int vdc = vdc_min; vdc <= vdc_max; vdc++)
    {
        HuC6260* huc6260 = geargrafx->GetHuC6260();
        HuC6270* huc6270 = vdc == 0 ? geargrafx->GetHuC6270_1() : geargrafx->GetHuC6270_2();
        u16* vram = huc6270->GetVRAM();
        u16* sat = huc6270->GetSAT();
        u16* color_table = huc6260->GetColorTable();

        for (int i = 0; i < 64; i++)
        {
            int sprite_offset = i << 2;
            u16 flags = sat[sprite_offset + 3] & 0xB98F;
            bool x_flip = (flags & 0x0800);
            bool y_flip = (flags & 0x8000);
            int palette = flags & 0x0F;
            int cgx = (flags >> 8) & 0x01;
            int cgy = (flags >> 12) & 0x03;
            int width = k_huc6270_sprite_width[cgx];
            int height = k_huc6270_sprite_height[cgy];
            u16 pattern = (sat[sprite_offset + 2] >> 1) & 0x3FF;
            pattern &= k_huc6270_sprite_mask_width[cgx];
            pattern &= k_huc6270_sprite_mask_height[cgy];
            u16 sprite_address = pattern << 6;
            bool mode1 = ((huc6270->GetState()->R[HUC6270_REG_MWR] >> 2) & 0x03) == 1;
            int mode1_offset = mode1 ? (sat[sprite_offset + 2] & 1) << 5 : 0;

            emu_debug_sprite_widths[vdc][i] = width;
            emu_debug_sprite_heights[vdc][i] = height;

            for (int y = 0; y < height; y++)
            {
                int flipped_y = y_flip ? (height - 1 - y) : y;
                int tile_y = flipped_y >> 4;
                int tile_line_offset = tile_y * 2 * 64;
                int offset_y = flipped_y & 0xF;
                u16 line_start = sprite_address + tile_line_offset + offset_y;

                for (int x = 0; x < width; x++)
                {
                    int flipped_x = x_flip ? (width - 1 - x) : x;
                    int tile_x = flipped_x >> 4;
                    int tile_x_offset = tile_x * 64;
                    int line = line_start + tile_x_offset + mode1_offset;

                    u16 plane1 = vram[line + 0];
                    u16 plane2 = vram[line + 16];
                    u16 plane3 = mode1 ? 0 : vram[line + 32];
                    u16 plane4 = mode1 ? 0 : vram[line + 48];

                    int pixel_x = 15 - (flipped_x & 0xF);
                    u16 pixel = ((plane1 >> pixel_x) & 0x01) | (((plane2 >> pixel_x) & 0x01) << 1) | (((plane3 >> pixel_x) & 0x01) << 2) | (((plane4 >> pixel_x) & 0x01) << 3);
                    pixel |= (palette << 4);
                    pixel |= 0x100;

                    int color = color_table[pixel & 0x1FF];
                    u8 green = ((color >> 6) & 0x07) * 255 / 7;
                    u8 red = ((color >> 3) & 0x07) * 255 / 7;
                    u8 blue = (color & 0x07) * 255 / 7;

                    if (!(pixel & 0x0F))
                    {
                        red = 255;
                        green = 0;
                        blue = 255;
                    }

                    int pixel_index = ((y * width) + x) << 2;
                    emu_debug_sprite_buffers[vdc][i][pixel_index + 0] = red;
                    emu_debug_sprite_buffers[vdc][i][pixel_index + 1] = green;
                    emu_debug_sprite_buffers[vdc][i][pixel_index + 2] = blue;
                    emu_debug_sprite_buffers[vdc][i][pixel_index + 3] = 255;
                }
            }
        }
    }
}

void emu_start_vgm_recording(const char* file_path)
{
    if (!geargrafx->GetMedia()->IsReady())
        return;

    if (geargrafx->GetAudio()->IsVgmRecording())
    {
        emu_stop_vgm_recording();
    }

    // PC Engine audio chip always runs at 3.579545 MHz
    int clock_rate = 3579545;

    if (geargrafx->GetAudio()->StartVgmRecording(file_path, clock_rate))
    {
        Log("VGM recording started: %s", file_path);
    }
}

void emu_stop_vgm_recording(void)
{
    if (geargrafx->GetAudio()->IsVgmRecording())
    {
        geargrafx->GetAudio()->StopVgmRecording();
        Log("VGM recording stopped");
    }
}

bool emu_is_vgm_recording(void)
{
    return geargrafx->GetAudio()->IsVgmRecording();
}

void emu_mcp_set_transport(int mode, int tcp_port)
{
    if (mcp_manager)
        mcp_manager->SetTransportMode((McpTransportMode)mode, tcp_port);
}

void emu_mcp_start(void)
{
    if (mcp_manager)
        mcp_manager->Start();
}

void emu_mcp_stop(void)
{
    if (mcp_manager)
        mcp_manager->Stop();
}

bool emu_mcp_is_running(void)
{
    return mcp_manager && mcp_manager->IsRunning();
}

int emu_mcp_get_transport_mode(void)
{
    return mcp_manager ? mcp_manager->GetTransportMode() : -1;
}

void emu_mcp_pump_commands(void)
{
    if (mcp_manager && mcp_manager->IsRunning())
        mcp_manager->PumpCommands(geargrafx);
}

