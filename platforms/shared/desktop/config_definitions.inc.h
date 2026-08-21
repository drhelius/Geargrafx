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

#include "config_macros.h"
#include "shader_preset.h"

static inline void process(config_Operation operation)
{
    //**************************************
    // Debug
    //**************************************

    // Debugger windows
    CONFIG_BOOL("Debug", "Debug", config_debug.debug, false);
    CONFIG_BOOL("Debug", "Disassembler", config_debug.show_disassembler, true);
    CONFIG_BOOL("Debug", "Screen", config_debug.show_screen, true);
    CONFIG_BOOL("Debug", "Memory", config_debug.show_memory, false);
    CONFIG_BOOL("Debug", "Processor", config_debug.show_processor, true);
    CONFIG_BOOL("Debug", "CallStack", config_debug.show_call_stack, false);
    CONFIG_BOOL("Debug", "Breakpoints", config_debug.show_breakpoints, false);
    CONFIG_BOOL("Debug", "Symbols", config_debug.show_symbols, false);
    CONFIG_BOOL("Debug", "HuC6202Info", config_debug.show_huc6202_info, false);
    CONFIG_BOOL("Debug", "HuC6260Info", config_debug.show_huc6260_info, false);
    CONFIG_BOOL("Debug", "HuC6260Palettes", config_debug.show_huc6260_palettes, false);
    CONFIG_BOOL("Debug", "HuC6270Registers1", config_debug.show_huc6270_1_registers, false);
    CONFIG_BOOL("Debug", "HuC6270Background1", config_debug.show_huc6270_1_background, false);
    CONFIG_BOOL("Debug", "HuC6270Sprites1", config_debug.show_huc6270_1_sprites, false);
    CONFIG_BOOL("Debug", "HuC6270Tiles1", config_debug.show_huc6270_1_tiles, false);
    CONFIG_BOOL("Debug", "HuC6270Info1", config_debug.show_huc6270_1_info, false);
    CONFIG_BOOL("Debug", "HuC6270Registers2", config_debug.show_huc6270_2_registers, false);
    CONFIG_BOOL("Debug", "HuC6270Background2", config_debug.show_huc6270_2_background, false);
    CONFIG_BOOL("Debug", "HuC6270Sprites2", config_debug.show_huc6270_2_sprites, false);
    CONFIG_BOOL("Debug", "HuC6270Tiles2", config_debug.show_huc6270_2_tiles, false);
    CONFIG_BOOL("Debug", "HuC6270Info2", config_debug.show_huc6270_2_info, false);
    CONFIG_BOOL("Debug", "PSG", config_debug.show_psg, false);
    CONFIG_BOOL("Debug", "CDROM", config_debug.show_cdrom, false);
    CONFIG_BOOL("Debug", "CDROMTOC", config_debug.show_cdrom_toc, false);
    CONFIG_BOOL("Debug", "CDROMAudio", config_debug.show_cdrom_audio, false);
    CONFIG_BOOL("Debug", "ADPCM", config_debug.show_adpcm, false);
    CONFIG_BOOL("Debug", "ArcadeCard", config_debug.show_arcade_card, false);
    CONFIG_BOOL("Debug", "TraceLogger", config_debug.show_trace_logger, false);
    CONFIG_BOOL("Debug", "Rewind", config_debug.show_rewind, false);

    // Trace logger
    CONFIG_BOOL("Debug", "TraceCounter", config_debug.trace_counter, true);
    CONFIG_BOOL("Debug", "TraceCycles", config_debug.trace_cycles, false);
    CONFIG_BOOL("Debug", "TraceBank", config_debug.trace_bank, true);
    CONFIG_BOOL("Debug", "TraceRegisters", config_debug.trace_registers, true);
    CONFIG_BOOL("Debug", "TraceFlags", config_debug.trace_flags, true);
    CONFIG_BOOL("Debug", "TraceBytes", config_debug.trace_bytes, true);
    CONFIG_BOOL("Debug", "TraceCpuEnabled", config_debug.trace_cpu_enabled, true);
    CONFIG_BOOL("Debug", "TraceCpu", config_debug.trace_cpu, true);
    CONFIG_BOOL("Debug", "TraceCpuIrq", config_debug.trace_cpu_irq, true);
    CONFIG_BOOL("Debug", "TraceVdc", config_debug.trace_vdc, false);
    CONFIG_BOOL("Debug", "TraceInput", config_debug.trace_input, false);
    CONFIG_BOOL("Debug", "TraceTimer", config_debug.trace_timer, false);
    CONFIG_BOOL("Debug", "TraceCdrom", config_debug.trace_cdrom, false);
    CONFIG_BOOL("Debug", "TracePsg", config_debug.trace_psg, false);
    CONFIG_BOOL("Debug", "TraceAdpcm", config_debug.trace_adpcm, false);
    CONFIG_BOOL("Debug", "TraceVce", config_debug.trace_vce, false);
    CONFIG_BOOL("Debug", "TraceScsi", config_debug.trace_scsi, false);
    CONFIG_BOOL("Debug", "TraceSystem", config_debug.trace_system, false);
    CONFIG_INT_RANGE("Debug", "TraceVdcEvents", config_debug.trace_vdc_events, TRACE_VDC_FILTER_ALL, 0, TRACE_VDC_FILTER_ALL);
    CONFIG_INT_RANGE("Debug", "TraceInputEvents", config_debug.trace_input_events, TRACE_INPUT_FILTER_ALL, 0, TRACE_INPUT_FILTER_ALL);
    CONFIG_INT_RANGE("Debug", "TraceTimerEvents", config_debug.trace_timer_events, TRACE_TIMER_FILTER_ALL, 0, TRACE_TIMER_FILTER_ALL);
    CONFIG_INT_RANGE("Debug", "TraceCdromEvents", config_debug.trace_cdrom_events, TRACE_CDROM_FILTER_ALL, 0, TRACE_CDROM_FILTER_ALL);
    CONFIG_INT_RANGE("Debug", "TracePsgEvents", config_debug.trace_psg_events, TRACE_PSG_FILTER_ALL, 0, TRACE_PSG_FILTER_ALL);
    CONFIG_INT_RANGE("Debug", "TraceAdpcmEvents", config_debug.trace_adpcm_events, TRACE_ADPCM_FILTER_ALL, 0, TRACE_ADPCM_FILTER_ALL);
    CONFIG_INT_RANGE("Debug", "TraceVceEvents", config_debug.trace_vce_events, TRACE_VCE_FILTER_ALL, 0, TRACE_VCE_FILTER_ALL);
    CONFIG_INT_RANGE("Debug", "TraceScsiEvents", config_debug.trace_scsi_events, TRACE_SCSI_FILTER_ALL, 0, TRACE_SCSI_FILTER_ALL);
    CONFIG_INT_RANGE("Debug", "TraceSystemEvents", config_debug.trace_system_events, TRACE_SYSTEM_FILTER_ALL, 0, TRACE_SYSTEM_FILTER_ALL);
    CONFIG_INT_RANGE("Debug", "TraceOutput", config_debug.trace_output, 0, 0, 1);
    CONFIG_INT_RANGE("Debug", "TraceCapacity", config_debug.trace_capacity, 0, 0, 4);
    CONFIG_INT_RANGE("Debug", "TraceDiskDirOption", config_debug.trace_disk_dir_option, 0, 0, 2);
    CONFIG_INT_RANGE("Debug", "TraceDiskSize", config_debug.trace_disk_size, 2, 0, 6);
    CONFIG_STRING_NOT_EMPTY("Debug", "TraceDiskPath", config_debug.trace_disk_path, config_root_path);

    // Disassembler
    CONFIG_BOOL("Debug", "DisMem", config_debug.dis_show_mem, true);
    CONFIG_BOOL("Debug", "DisSymbols", config_debug.dis_show_symbols, true);
    CONFIG_BOOL("Debug", "DisSegment", config_debug.dis_show_segment, true);
    CONFIG_BOOL("Debug", "DisBank", config_debug.dis_show_bank, true);
    CONFIG_BOOL("Debug", "DisAutoSymbols", config_debug.dis_show_auto_symbols, true);
    CONFIG_BOOL("Debug", "DisDimAutoSymbols", config_debug.dis_dim_auto_symbols, false);
    CONFIG_BOOL("Debug", "DisReplaceSymbols", config_debug.dis_replace_symbols, true);
    CONFIG_BOOL("Debug", "DisReplaceLabels", config_debug.dis_replace_labels, true);
    CONFIG_INT_RANGE("Debug", "DisSyntax", config_debug.dis_syntax, GG_Disassembler_Syntax_Geargrafx, GG_Disassembler_Syntax_Geargrafx, GG_Disassembler_Syntax_Count - 1);
    CONFIG_INT("Debug", "DisLookAheadCount", config_debug.dis_look_ahead_count, 20);
    CONFIG_BOOL("Debug", "PauseOnBRK", config_debug.pause_on_brk, false);
    CONFIG_INT_RANGE("Debug", "PauseOnBRKValue", config_debug.pause_on_brk_value, 0xFF, 0, 0xFF);
    CONFIG_BOOL("Debug", "PauseOnBRKTriggerIRQ", config_debug.pause_on_brk_trigger_irq, false);

    // Interface
    CONFIG_INT_RANGE("Debug", "FontSize", config_debug.font_size, 0, 0, 3);
    CONFIG_INT("Debug", "Scale", config_debug.scale, 2);
    CONFIG_BOOL("Debug", "MultiViewport", config_debug.multi_viewport, false);
    CONFIG_BOOL("Debug", "SingleInstance", config_debug.single_instance, false);
    CONFIG_BOOL("Debug", "AutoDebugSettings", config_debug.auto_debug_settings, false);

    // Reset values
    CONFIG_INT("Debug", "InitRam", config_debug.reset_ram, 1);
    CONFIG_INT("Debug", "InitCardRam", config_debug.reset_card_ram, 1);
    CONFIG_INT("Debug", "InitRegisters", config_debug.reset_registers, 0);
    CONFIG_INT("Debug", "InitColorTable", config_debug.reset_color_table, 0);
    CONFIG_INT("Debug", "InitMPR", config_debug.reset_mpr, 0);
    CONFIG_INT("Debug", "InitArcadeCard", config_debug.reset_arcade_card, 1);

    // Memory editors
    for (int i = 0; i < config_memory_editor_count; i++)
    {
        char section[32];
        snprintf(section, sizeof(section), "MemEditor_%d", i);
        CONFIG_INT(section, "BytesPerRow", config_debug.mem_editor_bytes_per_row[i], 16);
        CONFIG_INT(section, "PreviewDataType", config_debug.mem_editor_preview_data_type[i], 0);
        CONFIG_INT(section, "PreviewEndianess", config_debug.mem_editor_preview_endianess[i], 0);
        CONFIG_BOOL(section, "UppercaseHex", config_debug.mem_editor_uppercase_hex[i], true);
        CONFIG_BOOL(section, "GrayOutZeros", config_debug.mem_editor_gray_out_zeros[i], true);
    }

    //**************************************
    // Emulator
    //**************************************

    // Window and interface
    CONFIG_BOOL("Emulator", "Maximized", config_emulator.maximized, false);
    CONFIG_BOOL("Emulator", "FullScreen", config_emulator.fullscreen, false);
    CONFIG_INT("Emulator", "FullScreenMode", config_emulator.fullscreen_mode, 0);
    CONFIG_BOOL("Emulator", "AlwaysShowMenu", config_emulator.always_show_menu, false);
    CONFIG_INT_RANGE("Emulator", "Theme", config_emulator.theme, config_Theme_Dark, config_Theme_Light, config_Theme_Dark);
    CONFIG_INT("Emulator", "WindowWidth", config_emulator.window_width, 770);
    CONFIG_INT("Emulator", "WindowHeight", config_emulator.window_height, 600);
    CONFIG_BOOL("Emulator", "StatusMessages", config_emulator.status_messages, false);
    CONFIG_BOOL("Emulator", "AllowScreenSaver", config_emulator.allow_screensaver, false);

    // Emulation
    CONFIG_INT("Emulator", "FFWD", config_emulator.ffwd_speed, 1);
    CONFIG_INT_RANGE("Emulator", "RunAhead", config_emulator.runahead, 0, 0, 3);
    CONFIG_INT_RANGE("Emulator", "SaveSlot", config_emulator.save_slot, 0, 0, 4);
    CONFIG_BOOL("Emulator", "StartPaused", config_emulator.start_paused, false);
    CONFIG_BOOL("Emulator", "PauseWhenInactive", config_emulator.pause_when_inactive, true);
    CONFIG_BOOL("Emulator", "BackupRAM", config_emulator.backup_ram, true);
    CONFIG_INT("Emulator", "ConsoleType", config_emulator.console_type, 0);
    CONFIG_INT("Emulator", "CDROMType", config_emulator.cdrom_type, 0);
    CONFIG_BOOL("Emulator", "PreloadCDROM", config_emulator.preload_cdrom, false);

    // Files and paths
    CONFIG_INT("Emulator", "SaveFilesDirOption", config_emulator.savefiles_dir_option, 0);
    CONFIG_STRING_NOT_EMPTY("Emulator", "SaveFilesPath", config_emulator.savefiles_path, config_root_path);
    CONFIG_INT("Emulator", "SaveStatesDirOption", config_emulator.savestates_dir_option, 0);
    CONFIG_STRING_NOT_EMPTY("Emulator", "SaveStatesPath", config_emulator.savestates_path, config_root_path);
    CONFIG_INT("Emulator", "ScreenshotDirOption", config_emulator.screenshots_dir_option, 0);
    CONFIG_STRING_NOT_EMPTY("Emulator", "ScreenshotPath", config_emulator.screenshots_path, config_root_path);
    CONFIG_INT("Emulator", "BackupRAMDirOption", config_emulator.backup_ram_dir_option, 0);
    CONFIG_STRING_NOT_EMPTY("Emulator", "BackupRAMPath", config_emulator.backup_ram_path, config_root_path);
    CONFIG_INT("Emulator", "MB128DirOption", config_emulator.mb128_dir_option, 0);
    CONFIG_STRING_NOT_EMPTY("Emulator", "MB128Path", config_emulator.mb128_path, config_root_path);
    CONFIG_INT("Emulator", "MB128Mode", config_emulator.mb128_mode, 0);
    CONFIG_STRING("Emulator", "LastOpenPath", config_emulator.last_open_path, "");
    CONFIG_STRING("Emulator", "SysCardBiosPath", config_emulator.syscard_bios_path, "");
    CONFIG_STRING("Emulator", "GameExpressBiosPath", config_emulator.gameexpress_bios_path, "");
    CONFIG_STRING_ARRAY("Emulator", "RecentROM%d", config_emulator.recent_roms, config_max_recent_roms, "");

    // Services
    CONFIG_INT("Emulator", "MCPTCPPort", config_emulator.mcp_tcp_port, 7777);
    CONFIG_STRING_NOT_EMPTY("Emulator", "MCPHTTPAddress", config_emulator.mcp_http_address, "127.0.0.1");

    //**************************************
    // Video
    //**************************************

    // Display
    CONFIG_INT("Video", "Scale", config_video.scale, 0);
    CONFIG_INT("Video", "ScaleManual", config_video.scale_manual, 1);
    CONFIG_INT("Video", "AspectRatio", config_video.ratio, 1);
    CONFIG_INT("Video", "Overscan", config_video.overscan, 0);
    CONFIG_INT("Video", "ScanlineMode", config_video.scanline_mode, 0);
    CONFIG_INT("Video", "ScanlineStart", config_video.scanline_start, 11);
    CONFIG_INT("Video", "ScanlineEnd", config_video.scanline_end, 234);
    CONFIG_INT_RANGE("Video", "Palette", config_video.palette, 0, 0, 3);
    CONFIG_BOOL("Video", "FPS", config_video.fps, false);
    CONFIG_BOOL("Video", "SpriteLimit", config_video.sprite_limit, false);
    CONFIG_BOOL("Video", "SafeVdcDefaults", config_video.safe_vdc_defaults, false);
    CONFIG_INT_RANGE("Video", "ShaderMode", config_video.shader_mode, config_ShaderMode_PixelPerfect, config_ShaderMode_PixelPerfect, config_ShaderMode_External);

    if (operation == config_Operation_Write)
    {
        std::string preset_file = get_filename(config_video.shader_preset_path.c_str());
        CONFIG_STRING("Video", "ShaderPresetFile", preset_file, "");
    }
    else
    {
        CONFIG_STRING("Video", "ShaderPresetFile", config_video.shader_preset_path, "");
    }

    CONFIG_INT_RANGE("Video", "SyncMode", config_video.sync_mode, config_VideoSync_Disabled, config_VideoSync_Disabled, config_VideoSync_VRR);

    // Background colors
    CONFIG_FLOAT("Video", "BackgroundColorR", config_video.background_color[config_Theme_Dark][0], 0.1f);
    CONFIG_FLOAT("Video", "BackgroundColorG", config_video.background_color[config_Theme_Dark][1], 0.1f);
    CONFIG_FLOAT("Video", "BackgroundColorB", config_video.background_color[config_Theme_Dark][2], 0.1f);
    CONFIG_FLOAT("Video", "BackgroundColorDebuggerR", config_video.background_color_debugger[config_Theme_Dark][0], 0.2f);
    CONFIG_FLOAT("Video", "BackgroundColorDebuggerG", config_video.background_color_debugger[config_Theme_Dark][1], 0.2f);
    CONFIG_FLOAT("Video", "BackgroundColorDebuggerB", config_video.background_color_debugger[config_Theme_Dark][2], 0.2f);
    CONFIG_FLOAT("Video", "BackgroundColorLightR", config_video.background_color[config_Theme_Light][0], 128.0f / 255.0f);
    CONFIG_FLOAT("Video", "BackgroundColorLightG", config_video.background_color[config_Theme_Light][1], 128.0f / 255.0f);
    CONFIG_FLOAT("Video", "BackgroundColorLightB", config_video.background_color[config_Theme_Light][2], 128.0f / 255.0f);
    CONFIG_FLOAT("Video", "BackgroundColorDebuggerLightR", config_video.background_color_debugger[config_Theme_Light][0], 160.0f / 255.0f);
    CONFIG_FLOAT("Video", "BackgroundColorDebuggerLightG", config_video.background_color_debugger[config_Theme_Light][1], 160.0f / 255.0f);
    CONFIG_FLOAT("Video", "BackgroundColorDebuggerLightB", config_video.background_color_debugger[config_Theme_Light][2], 160.0f / 255.0f);

    // Low-pass filter
    CONFIG_BOOL("Video", "LowpassFilter", config_video.lowpass_filter, true);
    CONFIG_FLOAT("Video", "LowpassIntensity", config_video.lowpass_intensity, 1.0f);
    CONFIG_FLOAT("Video", "LowpassCutoffMhz", config_video.lowpass_cutoff_mhz, 5.0f);
    CONFIG_BOOL("Video", "LowpassSpeed536", config_video.lowpass_speed[0], false);
    CONFIG_BOOL("Video", "LowpassSpeed716", config_video.lowpass_speed[1], true);
    CONFIG_BOOL("Video", "LowpassSpeed108", config_video.lowpass_speed[2], true);

    //**************************************
    // Audio
    //**************************************

    CONFIG_BOOL("Audio", "Enable", config_audio.enable, true);
    CONFIG_BOOL("Audio", "Sync", config_audio.sync, true);
    CONFIG_BOOL("Audio", "HuC6280A", config_audio.huc6280a, true);
    CONFIG_FLOAT_RANGE("Audio", "MasterVolume", config_audio.master_volume, 1.0f, 0.0f, 2.0f);
    CONFIG_FLOAT("Audio", "PSGVolume", config_audio.psg_volume, 1.0f);
    CONFIG_FLOAT("Audio", "CDROMVolume", config_audio.cdrom_volume, 1.0f);
    CONFIG_FLOAT("Audio", "ADPCMVolume", config_audio.adpcm_volume, 1.0f);
    CONFIG_INT("Audio", "BufferCount", config_audio.buffer_count, 3);

    //**************************************
    // Rewind
    //**************************************

    CONFIG_BOOL("Rewind", "Enabled", config_rewind.enabled, true);
    CONFIG_INT_RANGE("Rewind", "BufferSeconds", config_rewind.buffer_seconds, 10, 1, 10);
    CONFIG_INT_MIN("Rewind", "FramesPerSnapshot", config_rewind.frames_per_snapshot, 1, 1);
    CONFIG_FLOAT_RANGE("Rewind", "Speed", config_rewind.speed, 2.0f, 1.0f, 8.0f);

    //**************************************
    // Input
    //**************************************

    CONFIG_BOOL("Input", "TurboTap", config_input.turbo_tap, false);
    CONFIG_BOOL("Input", "AllowUpDown", config_input.allow_up_down, false);
    CONFIG_BOOL("Input", "CaptureMouse", config_emulator.capture_mouse, false);
    CONFIG_INT_RANGE("Input", "MouseSensitivity", config_emulator.mouse_sensitivity, 5, 1, 15);

    // Players
    for (int i = 0; i < GG_MAX_GAMEPADS; i++)
    {
        char section[32];
        snprintf(section, sizeof(section), "Input%d", i + 1);
        CONFIG_INT(section, "ControllerType", config_input.controller_type[i], 0);
        CONFIG_INT(section, "AvenuePad3Button", config_input.avenue_pad_3_button[i], 0);

        for (int j = 0; j < 2; j++)
        {
            char key[32];
            snprintf(key, sizeof(key), "TurboEnabled%d", j + 1);
            CONFIG_BOOL(section, key, config_input.turbo_enabled[i][j], false);
            snprintf(key, sizeof(key), "TurboSpeed%d", j + 1);
            CONFIG_INT(section, key, config_input.turbo_speed[i][j], 4);
        }
    }

    // Keyboard
    const SDL_Scancode keyboard_defaults[3][14] = {
        { SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_UP, SDL_SCANCODE_DOWN,
          SDL_SCANCODE_A, SDL_SCANCODE_S, SDL_SCANCODE_X, SDL_SCANCODE_Z,
          SDL_SCANCODE_C, SDL_SCANCODE_V, SDL_SCANCODE_B, SDL_SCANCODE_N,
          SDL_SCANCODE_W, SDL_SCANCODE_Q },
        { SDL_SCANCODE_J, SDL_SCANCODE_L, SDL_SCANCODE_I, SDL_SCANCODE_K,
          SDL_SCANCODE_G, SDL_SCANCODE_H, SDL_SCANCODE_Y, SDL_SCANCODE_T,
          SDL_SCANCODE_5, SDL_SCANCODE_6, SDL_SCANCODE_7, SDL_SCANCODE_8,
          SDL_SCANCODE_P, SDL_SCANCODE_O },
        { SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN,
          SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN,
          SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN,
          SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN }
    };

    for (int i = 0; i < GG_MAX_GAMEPADS; i++)
    {
        char section[32];
        snprintf(section, sizeof(section), "InputKeyboard%d", i + 1);
        const SDL_Scancode* defaults = keyboard_defaults[i < 2 ? i : 2];
        CONFIG_SCANCODE(section, "KeyLeft", config_input_keyboard[i].key_left, defaults[0]);
        CONFIG_SCANCODE(section, "KeyRight", config_input_keyboard[i].key_right, defaults[1]);
        CONFIG_SCANCODE(section, "KeyUp", config_input_keyboard[i].key_up, defaults[2]);
        CONFIG_SCANCODE(section, "KeyDown", config_input_keyboard[i].key_down, defaults[3]);
        CONFIG_SCANCODE(section, "KeySelect", config_input_keyboard[i].key_select, defaults[4]);
        CONFIG_SCANCODE(section, "KeyRun", config_input_keyboard[i].key_run, defaults[5]);
        CONFIG_SCANCODE(section, "KeyI", config_input_keyboard[i].key_I, defaults[6]);
        CONFIG_SCANCODE(section, "KeyII", config_input_keyboard[i].key_II, defaults[7]);
        CONFIG_SCANCODE(section, "KeyIII", config_input_keyboard[i].key_III, defaults[8]);
        CONFIG_SCANCODE(section, "KeyIV", config_input_keyboard[i].key_IV, defaults[9]);
        CONFIG_SCANCODE(section, "KeyV", config_input_keyboard[i].key_V, defaults[10]);
        CONFIG_SCANCODE(section, "KeyVI", config_input_keyboard[i].key_VI, defaults[11]);
        CONFIG_SCANCODE(section, "KeyToogleTurboI", config_input_keyboard[i].key_toggle_turbo_I, defaults[12]);
        CONFIG_SCANCODE(section, "KeyToogleTurboII", config_input_keyboard[i].key_toggle_turbo_II, defaults[13]);
    }

    // Gamepads
    for (int i = 0; i < GG_MAX_GAMEPADS; i++)
    {
        char section[32];
        snprintf(section, sizeof(section), "InputGamepad%d", i + 1);
        CONFIG_INT(section, "GamepadDirectional", config_input_gamepad[i].gamepad_directional, 0);
        CONFIG_BOOL(section, "GamepadInvertX", config_input_gamepad[i].gamepad_invert_x_axis, false);
        CONFIG_BOOL(section, "GamepadInvertY", config_input_gamepad[i].gamepad_invert_y_axis, false);
        CONFIG_INT(section, "GamepadSelect", config_input_gamepad[i].gamepad_select, SDL_GAMEPAD_BUTTON_BACK);
        CONFIG_INT(section, "GamepadRun", config_input_gamepad[i].gamepad_run, SDL_GAMEPAD_BUTTON_START);
        CONFIG_INT(section, "GamepadX", config_input_gamepad[i].gamepad_x_axis, SDL_GAMEPAD_AXIS_LEFTX);
        CONFIG_INT(section, "GamepadY", config_input_gamepad[i].gamepad_y_axis, SDL_GAMEPAD_AXIS_LEFTY);
        CONFIG_INT(section, "GamepadI", config_input_gamepad[i].gamepad_I, SDL_GAMEPAD_BUTTON_SOUTH);
        CONFIG_INT(section, "GamepadII", config_input_gamepad[i].gamepad_II, SDL_GAMEPAD_BUTTON_EAST);
        CONFIG_INT(section, "GamepadIII", config_input_gamepad[i].gamepad_III, SDL_GAMEPAD_BUTTON_WEST);
        CONFIG_INT(section, "GamepadIV", config_input_gamepad[i].gamepad_IV, SDL_GAMEPAD_BUTTON_NORTH);
        CONFIG_INT(section, "GamepadV", config_input_gamepad[i].gamepad_V, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
        CONFIG_INT(section, "GamepadVI", config_input_gamepad[i].gamepad_VI, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);
        CONFIG_INT(section, "GamepadToogleTurboI", config_input_gamepad[i].gamepad_toggle_turbo_I, SDL_GAMEPAD_BUTTON_RIGHT_STICK);
        CONFIG_INT(section, "GamepadToogleTurboII", config_input_gamepad[i].gamepad_toggle_turbo_II, SDL_GAMEPAD_BUTTON_LEFT_STICK);
    }

    // Gamepad shortcuts
    for (int i = 0; i < GG_MAX_GAMEPADS; i++)
    {
        char section[32];
        snprintf(section, sizeof(section), "InputGamepadShortcuts%d", i + 1);
        CONFIG_INT_ARRAY(section, "Shortcut%d", config_input_gamepad_shortcuts[i].gamepad_shortcuts, config_HotkeyIndex_COUNT, SDL_GAMEPAD_BUTTON_INVALID);
    }

    // Hotkeys
    CONFIG_HOTKEY("OpenROM", config_hotkeys[config_HotkeyIndex_OpenROM], SDL_SCANCODE_O, SDL_KMOD_CTRL);
    CONFIG_HOTKEY("ReloadROM", config_hotkeys[config_HotkeyIndex_ReloadROM], SDL_SCANCODE_D, SDL_KMOD_CTRL);
    CONFIG_HOTKEY("Quit", config_hotkeys[config_HotkeyIndex_Quit], SDL_SCANCODE_Q, SDL_KMOD_CTRL);
    CONFIG_HOTKEY("Reset", config_hotkeys[config_HotkeyIndex_Reset], SDL_SCANCODE_R, SDL_KMOD_CTRL);
    CONFIG_HOTKEY("Pause", config_hotkeys[config_HotkeyIndex_Pause], SDL_SCANCODE_P, SDL_KMOD_CTRL);
    CONFIG_HOTKEY("FFWD", config_hotkeys[config_HotkeyIndex_FFWD], SDL_SCANCODE_F, SDL_KMOD_CTRL);
    CONFIG_HOTKEY("Rewind", config_hotkeys[config_HotkeyIndex_Rewind], SDL_SCANCODE_BACKSPACE, SDL_KMOD_NONE);
    CONFIG_HOTKEY("SaveState", config_hotkeys[config_HotkeyIndex_SaveState], SDL_SCANCODE_S, SDL_KMOD_CTRL);
    CONFIG_HOTKEY("LoadState", config_hotkeys[config_HotkeyIndex_LoadState], SDL_SCANCODE_L, SDL_KMOD_CTRL);
    CONFIG_HOTKEY("Screenshot", config_hotkeys[config_HotkeyIndex_Screenshot], SDL_SCANCODE_X, SDL_KMOD_CTRL);
    CONFIG_HOTKEY("Fullscreen", config_hotkeys[config_HotkeyIndex_Fullscreen], SDL_SCANCODE_F12, SDL_KMOD_NONE);
    CONFIG_HOTKEY("ShowMainMenu", config_hotkeys[config_HotkeyIndex_ShowMainMenu], SDL_SCANCODE_M, SDL_KMOD_CTRL);
    CONFIG_HOTKEY("DebugStepInto", config_hotkeys[config_HotkeyIndex_DebugStepInto], SDL_SCANCODE_F11, SDL_KMOD_NONE);
    CONFIG_HOTKEY("DebugStepOver", config_hotkeys[config_HotkeyIndex_DebugStepOver], SDL_SCANCODE_F10, SDL_KMOD_NONE);
    CONFIG_HOTKEY("DebugStepOut", config_hotkeys[config_HotkeyIndex_DebugStepOut], SDL_SCANCODE_F11, SDL_KMOD_SHIFT);
    CONFIG_HOTKEY("DebugStepFrame", config_hotkeys[config_HotkeyIndex_DebugStepFrame], SDL_SCANCODE_F6, SDL_KMOD_NONE);
    CONFIG_HOTKEY("DebugContinue", config_hotkeys[config_HotkeyIndex_DebugContinue], SDL_SCANCODE_F5, SDL_KMOD_NONE);
    CONFIG_HOTKEY("DebugBreak", config_hotkeys[config_HotkeyIndex_DebugBreak], SDL_SCANCODE_F7, SDL_KMOD_NONE);
    CONFIG_HOTKEY("DebugRunToCursor", config_hotkeys[config_HotkeyIndex_DebugRunToCursor], SDL_SCANCODE_F8, SDL_KMOD_NONE);
    CONFIG_HOTKEY("DebugBreakpoint", config_hotkeys[config_HotkeyIndex_DebugBreakpoint], SDL_SCANCODE_F9, SDL_KMOD_NONE);
    CONFIG_HOTKEY("DebugGoBack", config_hotkeys[config_HotkeyIndex_DebugGoBack], SDL_SCANCODE_BACKSPACE, SDL_KMOD_CTRL);
    CONFIG_HOTKEY("SelectSlot1", config_hotkeys[config_HotkeyIndex_SelectSlot1], SDL_SCANCODE_1, SDL_KMOD_CTRL);
    CONFIG_HOTKEY("SelectSlot2", config_hotkeys[config_HotkeyIndex_SelectSlot2], SDL_SCANCODE_2, SDL_KMOD_CTRL);
    CONFIG_HOTKEY("SelectSlot3", config_hotkeys[config_HotkeyIndex_SelectSlot3], SDL_SCANCODE_3, SDL_KMOD_CTRL);
    CONFIG_HOTKEY("SelectSlot4", config_hotkeys[config_HotkeyIndex_SelectSlot4], SDL_SCANCODE_4, SDL_KMOD_CTRL);
    CONFIG_HOTKEY("SelectSlot5", config_hotkeys[config_HotkeyIndex_SelectSlot5], SDL_SCANCODE_5, SDL_KMOD_CTRL);
    CONFIG_HOTKEY("Mute", config_hotkeys[config_HotkeyIndex_Mute], SDL_SCANCODE_U, SDL_KMOD_CTRL);
}

//**************************************
// Emulator-specific behavior
//**************************************

static void before_read(int file_version);
static void after_read(int file_version);
static void before_write(void);
static void after_write(void);
static void before_defaults(void);
static void after_defaults(void);
static void normalize(void);
static void migrate(int file_version);
static void sync_shader_preset_parameter_defaults(void);

static void before_read(int file_version)
{
    migrate(file_version);
}

static void after_read(int file_version)
{
    UNUSED(file_version);
    sync_shader_preset_parameter_defaults();
}

static void before_write(void)
{
    config_ini_data["Hotkeys"].remove("CaptureMouseScancode");
    config_ini_data["Hotkeys"].remove("CaptureMouseMod");

    if (config_emulator.ffwd)
        config_audio.sync = true;
}

static void after_write(void)
{
    sync_shader_preset_parameter_defaults();
}

static void before_defaults(void)
{
}

static void after_defaults(void)
{
    config_emulator.paused = false;
    config_emulator.ffwd = false;
    config_emulator.show_info = false;
    config_hotkeys[config_HotkeyIndex_CaptureMouse].key = SDL_SCANCODE_F1;
    config_hotkeys[config_HotkeyIndex_CaptureMouse].mod = SDL_KMOD_NONE;
    config_update_hotkey_string(&config_hotkeys[config_HotkeyIndex_CaptureMouse]);
}

static void normalize(void)
{
#if defined(GG_DISABLE_DISASSEMBLER)
    config_debug.debug = false;
#endif
#if !defined(_WIN32)
    if (config_video.sync_mode == config_VideoSync_VRR)
        config_video.sync_mode = config_VideoSync_Fixed;
#endif
}

static void migrate(int file_version)
{
    std::string stored;

    if (file_version < 6)
    {
        write_bool("Debug", "TraceCycles", false);

        bool trace_cpu = read_bool("Debug", "TraceCpu", true);
        bool trace_cpu_irq = read_bool("Debug", "TraceCpuIrq", true);
        write_bool("Debug", "TraceCpuEnabled", trace_cpu || trace_cpu_irq);

        bool default_trace_filters = trace_cpu && trace_cpu_irq &&
            read_bool("Debug", "TraceVdc", true) &&
            read_bool("Debug", "TraceInput", true) &&
            read_bool("Debug", "TraceTimer", true) &&
            read_bool("Debug", "TraceCdrom", true) &&
            read_bool("Debug", "TracePsg", true) &&
            read_bool("Debug", "TraceAdpcm", true) &&
            read_bool("Debug", "TraceVce", true) &&
            read_bool("Debug", "TraceScsi", true);

        if (default_trace_filters)
        {
            write_bool("Debug", "TraceVdc", false);
            write_bool("Debug", "TraceInput", false);
            write_bool("Debug", "TraceTimer", false);
            write_bool("Debug", "TraceCdrom", false);
            write_bool("Debug", "TracePsg", false);
            write_bool("Debug", "TraceAdpcm", false);
            write_bool("Debug", "TraceVce", false);
            write_bool("Debug", "TraceScsi", false);
        }

        write_int("Debug", "TraceVdcEvents", TRACE_VDC_FILTER_ALL);
        write_int("Debug", "TraceInputEvents", TRACE_INPUT_FILTER_ALL);
        write_int("Debug", "TraceTimerEvents", TRACE_TIMER_FILTER_ALL);
        write_int("Debug", "TraceCdromEvents", TRACE_CDROM_FILTER_ALL);
        write_int("Debug", "TracePsgEvents", TRACE_PSG_FILTER_ALL);
        write_int("Debug", "TraceAdpcmEvents", TRACE_ADPCM_FILTER_ALL);
        write_int("Debug", "TraceVceEvents", TRACE_VCE_FILTER_ALL);
        write_int("Debug", "TraceScsiEvents", TRACE_SCSI_FILTER_ALL);
        write_int("Debug", "TraceSystemEvents", TRACE_SYSTEM_FILTER_ALL);
    }

    int sync_mode = -1;
    bool valid_sync_mode = get_setting("Video", "SyncMode", &stored) &&
        parse_int_string(stored, &sync_mode) && sync_mode >= config_VideoSync_Disabled &&
        sync_mode <= config_VideoSync_VRR;

    if (file_version < 5 || !valid_sync_mode)
    {
        bool sync = read_bool("Video", "Sync", true);
        bool vrr = read_bool("Video", "VRR", false);
        sync_mode = sync ? (vrr ? config_VideoSync_VRR : config_VideoSync_Fixed) : config_VideoSync_Disabled;
        write_int("Video", "SyncMode", sync_mode);
    }

    int scale = 0;
    if (get_setting("Video", "Scale", &stored) && parse_int_string(stored, &scale) && scale > 3)
        write_int("Video", "Scale", scale - 2);

    int palette = 0;
    if (file_version < 3 && get_setting("Video", "Palette", &stored) && parse_int_string(stored, &palette))
    {
        if (palette == 1)
            write_int("Video", "Palette", 2);
        else if (palette == 2)
            write_int("Video", "Palette", 3);
    }
}

static void sync_shader_preset_parameter_defaults(void)
{
    ShaderPresetInfo presets[SHADER_PRESET_MAX_DISCOVERED];
    int preset_count = shader_preset_scan_bundled(presets, SHADER_PRESET_MAX_DISCOVERED);

    for (int i = 0; i < preset_count; i++)
    {
        ShaderPreset preset;
        char error[512];
        if (!shader_preset_load(presets[i].path, &preset, error, sizeof(error)))
            continue;

        char preset_file[SHADER_PRESET_MAX_PATH];
        if (!shader_preset_get_config_path(preset.preset_path, preset_file, sizeof(preset_file)))
            continue;

        std::string section = shader_preset_section_name(preset_file);
        for (int j = 0; j < preset.parameter_count; j++)
        {
            ShaderPresetParameter* parameter = &preset.parameters[j];
            if (config_ini_data[section].has(parameter->name))
                continue;

            write_float(section.c_str(), parameter->name, parameter->default_value);
        }
    }
}
