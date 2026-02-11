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

#include <SDL.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "geargrafx.h"
#include "config.h"
#include "gui.h"
#include "ogl_renderer.h"
#include "emu.h"
#include "application.h"

#define DISPLAY_IMPORT
#include "display.h"

static Uint64 frame_time_start = 0;
static Uint64 frame_time_end = 0;
static int monitor_refresh_rate = 60;
static int vsync_frames_per_emu_frame = 1;
static int vsync_frame_counter = 0;

void display_begin_frame(void)
{
    frame_time_start = SDL_GetPerformanceCounter();
}

void display_render(void)
{
    ogl_renderer_begin_render();
    ImGui_ImplSDL2_NewFrame();
    gui_render();
    ogl_renderer_render();
    ogl_renderer_end_render();

    SDL_GL_SwapWindow(application_sdl_window);
}

void display_frame_throttle(void)
{
    frame_time_end = SDL_GetPerformanceCounter();

    if (emu_is_empty() || emu_is_paused() || emu_is_debug_idle() || !emu_is_audio_open() || config_emulator.ffwd)
    {
        Uint64 count_per_sec = SDL_GetPerformanceFrequency();
        float elapsed = (float)(frame_time_end - frame_time_start) / (float)count_per_sec;
        elapsed *= 1000.0f;

        float min = 16.666f;

        if (config_emulator.ffwd)
        {
            switch (config_emulator.ffwd_speed)
            {
                case 0:
                    min = 16.666f / 1.5f;
                    break;
                case 1: 
                    min = 16.666f / 2.0f;
                    break;
                case 2:
                    min = 16.666f / 2.5f;
                    break;
                case 3:
                    min = 16.666f / 3.0f;
                    break;
                default:
                    min = 0.0f;
            }
        }

        if (elapsed < min)
            SDL_Delay((Uint32)(min - elapsed));
    }
}

bool display_should_run_emu_frame(void)
{
    if (config_video.sync && vsync_frames_per_emu_frame > 1 && !config_emulator.ffwd)
    {
        bool should_run = (vsync_frame_counter == 0);
        vsync_frame_counter++;
        if (vsync_frame_counter >= vsync_frames_per_emu_frame)
            vsync_frame_counter = 0;
        return should_run;
    }

    return true;
}

void display_update_frame_pacing(void)
{
    int display = SDL_GetWindowDisplayIndex(application_sdl_window);

    if (display < 0)
    {
        Log("SDL Error: SDL_GetWindowDisplayIndex - %s", SDL_GetError());
        SDL_ClearError();
        display = 0;
    }

    SDL_DisplayMode mode;
    if (SDL_GetCurrentDisplayMode(display, &mode) == 0 && mode.refresh_rate > 0)
        monitor_refresh_rate = mode.refresh_rate;
    else
    {
        Log("SDL Error: SDL_GetCurrentDisplayMode - %s", SDL_GetError());
        SDL_ClearError();
        monitor_refresh_rate = 60;
    }

    const int emu_fps = 60;

    if (monitor_refresh_rate <= emu_fps + 5)
        vsync_frames_per_emu_frame = 1;
    else
        vsync_frames_per_emu_frame = (monitor_refresh_rate + emu_fps / 2) / emu_fps;

    vsync_frames_per_emu_frame = CLAMP(vsync_frames_per_emu_frame, 1, 8);

    vsync_frame_counter = 0;

    Debug("Monitor refresh rate: %d Hz, vsync frames per emu frame: %d", monitor_refresh_rate, vsync_frames_per_emu_frame);
}

void display_recreate_gl_context(void)
{
    ogl_renderer_destroy();
    ImGui_ImplSDL2_Shutdown();

    SDL_GLContext old_context = display_gl_context;
    display_gl_context = SDL_GL_CreateContext(application_sdl_window);

    if (display_gl_context)
    {
        SDL_GL_MakeCurrent(application_sdl_window, display_gl_context);
        SDL_GL_DeleteContext(old_context);
        SDL_GL_SetSwapInterval(1);
        ImGui_ImplSDL2_InitForOpenGL(application_sdl_window, display_gl_context);
        ogl_renderer_init();
        display_update_frame_pacing();
    }
}
