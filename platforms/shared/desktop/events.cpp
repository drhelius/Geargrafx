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

#include <SDL3/SDL.h>
#include "geargrafx.h"
#include "config.h"
#include "gui.h"
#include "emu.h"
#include "application.h"
#include "gamepad.h"

#define EVENTS_IMPORT
#include "events.h"

static bool input_updated = false;
static Uint16 input_last_state[GG_MAX_GAMEPADS] = { };
static bool input_turbo_toggle_prev[GG_MAX_GAMEPADS][2] = { };

static bool events_check_hotkey(const SDL_Event* event, const config_Hotkey& hotkey, bool allow_repeat);
static Uint16 input_build_state(int controller);
static void input_apply_state(int controller, Uint16 before, Uint16 now);

void events_shortcuts(const SDL_Event* event)
{
    if (event->type != SDL_EVENT_KEY_DOWN)
        return;

    // Check special case hotkeys first
    if (events_check_hotkey(event, config_hotkeys[config_HotkeyIndex_Quit], false))
    {
        application_trigger_quit();
        return;
    }

    if (events_check_hotkey(event, config_hotkeys[config_HotkeyIndex_Fullscreen], false))
    {
        config_emulator.fullscreen = !config_emulator.fullscreen;
        application_trigger_fullscreen(config_emulator.fullscreen);
        return;
    }

    // Check slot selection hotkeys
    for (int i = 0; i < 5; i++)
    {
        if (events_check_hotkey(event, config_hotkeys[config_HotkeyIndex_SelectSlot1 + i], false))
        {
            config_emulator.save_slot = i;
            return;
        }
    }

    // Check all hotkeys mapped to gui shortcuts
    for (int i = 0; i < GUI_HOTKEY_MAP_COUNT; i++)
    {
        if (gui_hotkey_map[i].shortcut >= 0 && events_check_hotkey(event, config_hotkeys[gui_hotkey_map[i].config_index], gui_hotkey_map[i].allow_repeat))
        {
            gui_shortcut((gui_ShortCutEvent)gui_hotkey_map[i].shortcut);
            return;
        }
    }

    // Fixed hotkeys for debug copy/paste/select operations
    int key = event->key.scancode;
    SDL_Keymod mods = event->key.mod;

    if (event->key.repeat == 0 && key == SDL_SCANCODE_A && (mods & SDL_KMOD_CTRL))
    {
        gui_shortcut(gui_ShortcutDebugSelectAll);
        return;
    }

    if (event->key.repeat == 0 && key == SDL_SCANCODE_C && (mods & SDL_KMOD_CTRL))
    {
        gui_shortcut(gui_ShortcutDebugCopy);
        return;
    }

    if (event->key.repeat == 0 && key == SDL_SCANCODE_V && (mods & SDL_KMOD_CTRL))
    {
        gui_shortcut(gui_ShortcutDebugPaste);
        return;
    }

    // ESC to exit fullscreen
    if (event->key.repeat == 0 && key == SDL_SCANCODE_ESCAPE)
    {
        if (config_emulator.fullscreen && !config_emulator.always_show_menu)
        {
            config_emulator.fullscreen = false;
            application_trigger_fullscreen(false);
        }
    }
}

void events_emu(void)
{
    if (input_updated || gui_in_use)
        return;
    input_updated = true;

    SDL_PumpEvents();

    int max_controller = config_input.turbo_tap ? GG_MAX_GAMEPADS : 1;

    for (int controller = 0; controller < max_controller; controller++)
    {
        Uint16 now = input_build_state(controller);
        Uint16 before = input_last_state[controller];

        if (now != before)
            input_apply_state(controller, before, now);

        input_last_state[controller] = now;

        gamepad_check_shortcuts(controller);
    }
}

void events_reset_input(void)
{
    input_updated = false;
}

bool events_input_updated(void)
{
    return input_updated;
}

static Uint16 input_build_state(int controller)
{
    SDL_Keymod mods = SDL_GetModState();
    if (mods & (SDL_KMOD_CTRL | SDL_KMOD_SHIFT | SDL_KMOD_ALT | SDL_KMOD_GUI))
        return 0;

    const bool* keyboard_state = SDL_GetKeyboardState(NULL);
    Uint16 ret = 0;

    if (keyboard_state[config_input_keyboard[controller].key_left])
        ret |= GG_KEY_LEFT;
    if (keyboard_state[config_input_keyboard[controller].key_right])
        ret |= GG_KEY_RIGHT;
    if (keyboard_state[config_input_keyboard[controller].key_up])
        ret |= GG_KEY_UP;
    if (keyboard_state[config_input_keyboard[controller].key_down])
        ret |= GG_KEY_DOWN;
    if (keyboard_state[config_input_keyboard[controller].key_I])
        ret |= GG_KEY_I;
    if (keyboard_state[config_input_keyboard[controller].key_II])
        ret |= GG_KEY_II;
    if (keyboard_state[config_input_keyboard[controller].key_III])
        ret |= GG_KEY_III;
    if (keyboard_state[config_input_keyboard[controller].key_IV])
        ret |= GG_KEY_IV;
    if (keyboard_state[config_input_keyboard[controller].key_V])
        ret |= GG_KEY_V;
    if (keyboard_state[config_input_keyboard[controller].key_VI])
        ret |= GG_KEY_VI;
    if (keyboard_state[config_input_keyboard[controller].key_run])
        ret |= GG_KEY_RUN;
    if (keyboard_state[config_input_keyboard[controller].key_select])
        ret |= GG_KEY_SELECT;

    bool kb_turbo_I  = keyboard_state[config_input_keyboard[controller].key_toggle_turbo_I] != 0;
    bool kb_turbo_II = keyboard_state[config_input_keyboard[controller].key_toggle_turbo_II] != 0;
    bool gp_turbo_I = false;
    bool gp_turbo_II = false;

    SDL_Gamepad* sdl_controller = gamepad_controller[controller];

    if (IsValidPointer(sdl_controller))
    {
        if (gamepad_get_button(sdl_controller, config_input_gamepad[controller].gamepad_I))
            ret |= GG_KEY_I;
        if (gamepad_get_button(sdl_controller, config_input_gamepad[controller].gamepad_II))
            ret |= GG_KEY_II;
        if (gamepad_get_button(sdl_controller, config_input_gamepad[controller].gamepad_III))
            ret |= GG_KEY_III;
        if (gamepad_get_button(sdl_controller, config_input_gamepad[controller].gamepad_IV))
            ret |= GG_KEY_IV;
        if (gamepad_get_button(sdl_controller, config_input_gamepad[controller].gamepad_V))
            ret |= GG_KEY_V;
        if (gamepad_get_button(sdl_controller, config_input_gamepad[controller].gamepad_VI))
            ret |= GG_KEY_VI;
        if (gamepad_get_button(sdl_controller, config_input_gamepad[controller].gamepad_run))
            ret |= GG_KEY_RUN;
        if (gamepad_get_button(sdl_controller, config_input_gamepad[controller].gamepad_select))
            ret |= GG_KEY_SELECT;

        gp_turbo_I  = gamepad_get_button(sdl_controller, config_input_gamepad[controller].gamepad_toggle_turbo_I);
        gp_turbo_II = gamepad_get_button(sdl_controller, config_input_gamepad[controller].gamepad_toggle_turbo_II);

        // Use D-Pad
        if (config_input_gamepad[controller].gamepad_directional == 0)
        {
            if (SDL_GetGamepadButton(sdl_controller, SDL_GAMEPAD_BUTTON_DPAD_LEFT))
                ret |= GG_KEY_LEFT;
            if (SDL_GetGamepadButton(sdl_controller, SDL_GAMEPAD_BUTTON_DPAD_RIGHT))
                ret |= GG_KEY_RIGHT;
            if (SDL_GetGamepadButton(sdl_controller, SDL_GAMEPAD_BUTTON_DPAD_UP))
                ret |= GG_KEY_UP;
            if (SDL_GetGamepadButton(sdl_controller, SDL_GAMEPAD_BUTTON_DPAD_DOWN))
                ret |= GG_KEY_DOWN;
        }
        // Use analog sticks
        else
        {
            const Sint16 STICK_DEAD_ZONE = 8000;
            const Sint16 rawx = SDL_GetGamepadAxis(sdl_controller, (SDL_GamepadAxis)config_input_gamepad[controller].gamepad_x_axis);
            const Sint16 rawy = SDL_GetGamepadAxis(sdl_controller, (SDL_GamepadAxis)config_input_gamepad[controller].gamepad_y_axis);

            const Sint16 x = config_input_gamepad[controller].gamepad_invert_x_axis ? -rawx : rawx;
            const Sint16 y = config_input_gamepad[controller].gamepad_invert_y_axis ? -rawy : rawy;

            if (x < -STICK_DEAD_ZONE)
                ret |= GG_KEY_LEFT;
            else if (x > STICK_DEAD_ZONE)
                ret |= GG_KEY_RIGHT;

            if (y < -STICK_DEAD_ZONE)
                ret |= GG_KEY_UP;
            else if (y > STICK_DEAD_ZONE)
                ret |= GG_KEY_DOWN;
        }
    }

    bool pressed_turbo_I  = kb_turbo_I || gp_turbo_I;
    bool pressed_turbo_II = kb_turbo_II || gp_turbo_II;

    if (pressed_turbo_I && !input_turbo_toggle_prev[controller][0])
    {
        config_input.turbo_enabled[controller][0] = !config_input.turbo_enabled[controller][0];
        emu_set_turbo((GG_Controllers)controller, GG_KEY_I, config_input.turbo_enabled[controller][0]);
    }
    if (pressed_turbo_II && !input_turbo_toggle_prev[controller][1])
    {
        config_input.turbo_enabled[controller][1] = !config_input.turbo_enabled[controller][1];
        emu_set_turbo((GG_Controllers)controller, GG_KEY_II, config_input.turbo_enabled[controller][1]);
    }

    input_turbo_toggle_prev[controller][0] = pressed_turbo_I;
    input_turbo_toggle_prev[controller][1] = pressed_turbo_II;

    return ret;
}

static void input_apply_state(int controller, Uint16 before, Uint16 now)
{
    Uint16 pressed  = now & (Uint16)(~before);
    Uint16 released = before & (Uint16)(~now);

    if ((pressed | released) == 0)
        return;

    static const Uint16 keys[12] = {
        GG_KEY_LEFT, GG_KEY_RIGHT, GG_KEY_UP, GG_KEY_DOWN,
        GG_KEY_I, GG_KEY_II, GG_KEY_III, GG_KEY_IV,
        GG_KEY_V, GG_KEY_VI, GG_KEY_RUN, GG_KEY_SELECT
    };

    for (unsigned i = 0; i < 12; i++)
    {
        Uint16 key = keys[i];
        if (pressed & key)  emu_key_pressed((GG_Controllers)controller, (GG_Keys)key);
        if (released & key) emu_key_released((GG_Controllers)controller, (GG_Keys)key);
    }
}

static bool events_check_hotkey(const SDL_Event* event, const config_Hotkey& hotkey, bool allow_repeat)
{
    if (event->type != SDL_EVENT_KEY_DOWN)
        return false;

    if (!allow_repeat && event->key.repeat != 0)
        return false;

    if (event->key.scancode != hotkey.key)
        return false;

    SDL_Keymod mods = event->key.mod;
    SDL_Keymod expected = hotkey.mod;

    SDL_Keymod mods_normalized = (SDL_Keymod)0;
    if (mods & (SDL_KMOD_LCTRL | SDL_KMOD_RCTRL)) mods_normalized = (SDL_Keymod)(mods_normalized | SDL_KMOD_CTRL);
    if (mods & (SDL_KMOD_LSHIFT | SDL_KMOD_RSHIFT)) mods_normalized = (SDL_Keymod)(mods_normalized | SDL_KMOD_SHIFT);
    if (mods & (SDL_KMOD_LALT | SDL_KMOD_RALT)) mods_normalized = (SDL_Keymod)(mods_normalized | SDL_KMOD_ALT);
    if (mods & (SDL_KMOD_LGUI | SDL_KMOD_RGUI)) mods_normalized = (SDL_Keymod)(mods_normalized | SDL_KMOD_GUI);

    SDL_Keymod expected_normalized = (SDL_Keymod)0;
    if (expected & (SDL_KMOD_LCTRL | SDL_KMOD_RCTRL | SDL_KMOD_CTRL)) expected_normalized = (SDL_Keymod)(expected_normalized | SDL_KMOD_CTRL);
    if (expected & (SDL_KMOD_LSHIFT | SDL_KMOD_RSHIFT | SDL_KMOD_SHIFT)) expected_normalized = (SDL_Keymod)(expected_normalized | SDL_KMOD_SHIFT);
    if (expected & (SDL_KMOD_LALT | SDL_KMOD_RALT | SDL_KMOD_ALT)) expected_normalized = (SDL_Keymod)(expected_normalized | SDL_KMOD_ALT);
    if (expected & (SDL_KMOD_LGUI | SDL_KMOD_RGUI | SDL_KMOD_GUI)) expected_normalized = (SDL_Keymod)(expected_normalized | SDL_KMOD_GUI);

    return mods_normalized == expected_normalized;
}
