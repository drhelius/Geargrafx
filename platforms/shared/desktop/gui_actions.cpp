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

#define GUI_ACTIONS_IMPORT
#include "gui_actions.h"
#include "gui.h"
#include "gui_debug.h"
#include "gui_debug_memory.h"
#include "gui_debug_trace_logger.h"
#include "config.h"
#include "emu.h"
#include "ogl_renderer.h"
#include "rewind.h"
#include "video_recorder.h"
#include "events.h"
#include "geargrafx.h"
#include "application.h"
#include "display.h"
#include "utils.h"

static std::string get_auto_file_path(int dir_option, const std::string& custom_path, const char* extension);

void gui_action_load_defaults(void)
{
    if (gui_is_rom_loading() || emu_is_media_loading())
        return;

    if (!gui_debug_trace_logger_stop())
        return;

    emu_stop_vgm_recording();
    emu_stop_video_recording();
    emu_turbolink_stop();
    emu_save_persistent_data();

    GeargrafxCore* core = emu_get_core();
    bool close_media = core->GetMedia()->IsCDROM();

    if (close_media)
    {
        gui_debug_auto_save_settings();
        core->GetMedia()->Reset();
        application_update_title_with_rom(NULL);
    }

    core->UnloadBios(true);
    core->UnloadBios(false);
    core->UnloadPacBios(GG_LASERACTIVE_REGION_JAPAN);
    core->UnloadPacBios(GG_LASERACTIVE_REGION_US);

    config_load_defaults();
    gui_apply_settings();

    emu_resume();
    emu_reset(false);

    if (close_media)
        gui_debug_reset();
    else
        gui_debug_memory_reset();

    gui_debug_memory_apply_settings();
    gui_debug_trace_logger_init();
    update_savestates_data();
    events_sync_input();
    ogl_renderer_unload_shader_preset();
    application_apply_settings();

    config_write();
}

void gui_action_reset(void)
{
    gui_set_status_message("Resetting...", 3000);

    gui_debug_trace_logger_clear();

    emu_resume();
    emu_reset();

    if (config_emulator.start_paused)
    {
        emu_pause();

        for (int i = 0; i < SYSTEM_TEXTURE_WIDTH * SYSTEM_TEXTURE_HEIGHT * 4; i++)
        {
            emu_frame_buffer[i] = 0;
        }
    }
}

void gui_action_reload_rom(void)
{
    if (!emu_is_empty())
    {
#if defined(GG_ENABLE_PHYSICAL_CDROM)
        if (emu_get_core()->GetMedia()->IsPhysicalCdRom())
        {
            gui_load_physical_cdrom(emu_get_core()->GetMedia()->GetPhysicalCdRomDeviceId());
            return;
        }
#endif

        char rom_path[4096];
        strncpy_fit(rom_path, emu_get_core()->GetMedia()->GetFilePath(), sizeof(rom_path));
        gui_load_rom(rom_path);
    }
}

void gui_action_eject_physical_cdrom(void)
{
    #if defined(GG_ENABLE_PHYSICAL_CDROM)
    if (emu_is_empty() || !emu_get_core()->GetMedia()->IsPhysicalCdRom())
    {
        Debug("Physical CD-ROM eject requested but no physical CD-ROM is loaded");
        gui_set_status_message("No physical CD-ROM loaded", 3000);
        return;
    }

    char device_id[256];
    strncpy_fit(device_id, emu_get_core()->GetMedia()->GetPhysicalCdRomDeviceId(), sizeof(device_id));

    Log("Physical CD-ROM eject requested from GUI: %s", device_id);

    if (emu_eject_physical_cdrom())
    {
        application_update_title_with_rom(NULL);
        gui_set_status_message("Physical CD-ROM ejected", 3000);
    }
    else
        gui_set_error_message("Unable to eject physical CD-ROM");
    #endif
}

void gui_action_pause(void)
{
    if (emu_is_paused())
    {
        gui_set_status_message("Resumed", 3000);
        emu_resume();
    }
    else
    {
        gui_set_status_message("Paused", 3000);
        emu_pause();
    }
}

void gui_action_ffwd(void)
{
    if (emu_turbolink_is_active())
    {
        config_emulator.ffwd = false;
        return;
    }

    config_audio.sync = !config_emulator.ffwd;

    if (config_emulator.ffwd)
    {
        gui_set_status_message("Fast Forward ON", 3000);
        display_disable_vsync();
    }
    else
    {
        gui_set_status_message("Fast Forward OFF", 3000);
        display_use_vsync_if_enabled();
        emu_audio_reset();
    }
}

void gui_action_rewind_pressed(void)
{
    if (emu_is_empty() || !config_rewind.enabled || emu_turbolink_is_active())
        return;
    if (rewind_get_snapshot_count() < 1)
        return;

    if (rewind_is_active())
        return;

    emu_reset_rewind_timing();
    rewind_set_active(true);
    display_use_vsync_if_enabled();
    gui_set_status_message("Rewinding...", 500);
}

void gui_action_rewind_released(void)
{
    if (!rewind_is_active())
        return;

    rewind_set_active(false);
    events_sync_input();
    emu_reset_rewind_timing();
    if (config_emulator.ffwd)
        display_disable_vsync();
    else
        display_use_vsync_if_enabled();
    emu_audio_reset();
}

void gui_action_save_screenshot(const char* path)
{
    using namespace std;

    if (!emu_get_core()->GetMedia()->IsReady())
        return;

    string file_path;

    if (path != NULL)
    {
        file_path = path;
        if (file_path.find_last_of(".") == string::npos)
            file_path += ".png";
    }
    else
        file_path = get_auto_file_path(config_emulator.screenshots_dir_option, config_emulator.screenshots_path, ".png");

    emu_save_screenshot(file_path.c_str());

    string message = "Screenshot saved to " + file_path;
    gui_set_status_message(message.c_str(), 3000);
}

bool gui_action_start_video_recording(const char* path)
{
    using namespace std;

    if (!emu_get_core()->GetMedia()->IsReady())
        return false;

    string file_path;

    if (path != NULL)
        file_path = path;
    else
        file_path = get_auto_file_path(config_emulator.video_recordings_dir_option, config_emulator.video_recordings_path, ".avi");

    if (!emu_start_video_recording(file_path.c_str()))
    {
        gui_set_error_message("Unable to start video recording");
        return false;
    }

    string message = "Recording video to " + file_path;
    gui_set_status_message(message.c_str(), 3000);
    return true;
}

void gui_action_stop_video_recording(void)
{
    using namespace std;

    if (!emu_is_video_recording())
        return;

    string message = "Video saved to " + string(video_recorder_get_file_path());
    emu_stop_video_recording();
    gui_set_status_message(message.c_str(), 3000);
}

void gui_action_toggle_video_recording(void)
{
    if (emu_is_video_recording())
        gui_action_stop_video_recording();
    else
        gui_action_start_video_recording(NULL);
}

void gui_action_save_sprite(const char* path, int vdc, int index)
{
    using namespace std;

    if (!emu_get_core()->GetMedia()->IsReady())
        return;

    emu_save_sprite(path, vdc, index);

    string message = "Sprite saved to " + string(path);
    gui_set_status_message(message.c_str(), 3000);
}

void gui_action_save_all_sprites(const char* folder_path, int vdc)
{
    using namespace std;

    if (!emu_get_core()->GetMedia()->IsReady())
        return;

    for (int i = 0; i < 64; i++)
    {
        char file_path[512];
        snprintf(file_path, sizeof(file_path), "%s/sprite_vdc%d_id%02d.png", folder_path, vdc, i);
        emu_save_sprite(file_path, vdc, i);
    }

    string message = "All sprites saved to " + string(folder_path);
    gui_set_status_message(message.c_str(), 3000);
}

void gui_action_save_background(const char* path, int vdc)
{
    using namespace std;

    if (!emu_get_core()->GetMedia()->IsReady())
        return;

    emu_save_background(path, vdc);

    string message = "Background saved to " + string(path);
    gui_set_status_message(message.c_str(), 3000);
}

static std::string get_auto_file_path(int dir_option, const std::string& custom_path, const char* extension)
{
    using namespace std;

    time_t now = time(0);
    tm ltm;

    char date_time_buffer[32] = {};
    if (get_local_time(now, &ltm))
        strftime(date_time_buffer, sizeof(date_time_buffer), "%Y-%m-%d %H%M%S", &ltm);
    string date_time = date_time_buffer;

    string file_path;

    switch ((Directory_Location)dir_option)
    {
        default:
        case Directory_Location_Default:
        {
            file_path = file_path.assign(config_root_path)+ "/" + string(emu_get_core()->GetMedia()->GetFileName()) + " - " + date_time + extension;
            break;
        }
        case Directory_Location_ROM:
        {
#if defined(GG_ENABLE_PHYSICAL_CDROM)
            if (emu_get_core()->GetMedia()->IsPhysicalCdRom())
                file_path = file_path.assign(config_root_path) + "/" + string(emu_get_core()->GetMedia()->GetFileName()) + " - " + date_time + extension;
            else
#endif
            file_path = file_path.assign(emu_get_core()->GetMedia()->GetFilePath()) + " - " + date_time + extension;
            break;
        }
        case Directory_Location_Custom:
        {
            file_path = file_path.assign(custom_path)+ "/" + string(emu_get_core()->GetMedia()->GetFileName()) + " - " + date_time + extension;
            break;
        }
    }

    return file_path;
}
