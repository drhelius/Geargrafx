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
#include "gui_actions.h"
#include "emu.h"
#include "application.h"
#include "gamepad.h"

#define EVENTS_IMPORT
#include "events.h"

static bool input_updated = false;
static Uint16 input_last_state[GG_MAX_GAMEPADS] = { };
static bool input_turbo_toggle_prev[GG_MAX_GAMEPADS][2] = { };

static bool events_check_hotkey(const SDL_Event* event, const config_Hotkey& hotkey, bool allow_repeat);
static bool events_match_hotkey_scancode(const SDL_Event* event, const config_Hotkey& hotkey);
static bool events_is_mouse_controller(int controller);
static int events_get_mouse_controller(void);
static config_InputProfile input_get_profile(int controller);
static GG_Keys input_get_avenue_pad_3_button(int controller);
static bool input_keyboard_pressed(const bool* keyboard_state, SDL_Scancode primary, SDL_Scancode secondary);
static bool input_gamepad_pressed(SDL_Gamepad* gamepad, int primary, int secondary);
static Uint16 input_build_state(int controller, bool update_turbo = true);
static Uint16 input_filter_opposing_directions(int controller, Uint16 state);
static void input_apply_state(int controller, Uint16 before, Uint16 now);

void events_shortcuts(const SDL_Event* event)
{
    if (event->type == SDL_EVENT_KEY_UP)
    {
        if (events_match_hotkey_scancode(event, config_hotkeys[config_HotkeyIndex_Rewind]))
            gui_action_rewind_released();
        return;
    }

    if (event->type != SDL_EVENT_KEY_DOWN)
        return;

    if (events_check_hotkey(event, config_hotkeys[config_HotkeyIndex_Rewind], false))
    {
        gui_action_rewind_pressed();
        return;
    }

    // Check special case hotkeys first
    if (events_check_hotkey(event, config_hotkeys[config_HotkeyIndex_Quit], false))
    {
        application_trigger_quit();
        return;
    }

    // Check all hotkeys mapped to gui shortcuts
    for (int i = 0; i < GUI_HOTKEY_MAP_COUNT; i++)
    {
        if (events_check_hotkey(event, config_hotkeys[gui_hotkey_map[i].config_index], gui_hotkey_map[i].allow_repeat))
        {
            gui_shortcut(gui_hotkey_map[i].shortcut);
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

void events_handle_emu_event(const SDL_Event* event)
{
    if (gui_in_use)
        return;

    int mouse_controller = events_get_mouse_controller();

    if (mouse_controller < 0)
        return;

    switch (event->type)
    {
        case SDL_EVENT_MOUSE_MOTION:
        {
            if (event->motion.xrel != 0.0f || event->motion.yrel != 0.0f)
            {
                int sen = MAX(config_emulator.mouse_sensitivity, 1);

                int relx = (int)(event->motion.xrel * ((float)sen / 6.0f));
                int rely = (int)(event->motion.yrel * ((float)sen / 6.0f));
                emu_set_mouse_delta(relx, rely);
            }
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        {
            if (gui_main_window_hovered)
            {
                if (event->button.button == SDL_BUTTON_RIGHT)
                    emu_key_pressed((GG_Controllers)mouse_controller, GG_KEY_I);
                if (event->button.button == SDL_BUTTON_LEFT)
                    emu_key_pressed((GG_Controllers)mouse_controller, GG_KEY_II);
            }
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_UP:
        {
            if (event->button.button == SDL_BUTTON_RIGHT)
                emu_key_released((GG_Controllers)mouse_controller, GG_KEY_I);
            if (event->button.button == SDL_BUTTON_LEFT)
                emu_key_released((GG_Controllers)mouse_controller, GG_KEY_II);
            break;
        }
    }
}

static int events_get_mouse_controller(void)
{
    int max_controller = config_input.turbo_tap ? GG_MAX_GAMEPADS : 1;

    for (int i = 0; i < max_controller; i++)
    {
        if (events_is_mouse_controller(i))
            return i;
    }

    return -1;
}

static bool events_is_mouse_controller(int controller)
{
    if (controller < 0 || controller >= GG_MAX_GAMEPADS)
        return false;

    return config_input.controller_type[controller] == GG_CONTROLLER_MOUSE;
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
        Uint16 now = input_filter_opposing_directions(controller, input_build_state(controller));
        Uint16 before = input_last_state[controller];

        if (now != before)
            input_apply_state(controller, before, now);

        input_last_state[controller] = now;

        gamepad_check_shortcuts(controller);
    }
}

void events_sync_input(void)
{
    SDL_PumpEvents();

    int max_controller = config_input.turbo_tap ? GG_MAX_GAMEPADS : 1;
    static const Uint16 all_keys = GG_KEY_LEFT | GG_KEY_RIGHT | GG_KEY_UP | GG_KEY_DOWN |
        GG_KEY_I | GG_KEY_II | GG_KEY_III | GG_KEY_IV | GG_KEY_V | GG_KEY_VI | GG_KEY_RUN | GG_KEY_SELECT;

    for (int controller = 0; controller < GG_MAX_GAMEPADS; controller++)
    {
        Uint16 now = (controller < max_controller) ? input_filter_opposing_directions(controller, input_build_state(controller, false)) : 0;
        input_apply_state(controller, all_keys, 0);
        input_apply_state(controller, 0, now);
        input_last_state[controller] = now;
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

static config_InputProfile input_get_profile(int controller)
{
    if (config_input.controller_type[controller] == GG_CONTROLLER_AVENUE_PAD_3)
        return config_InputProfile_3Button;
    if (config_input.controller_type[controller] == GG_CONTROLLER_AVENUE_PAD_6)
        return config_InputProfile_6Button;
    return config_InputProfile_2Button;
}

static GG_Keys input_get_avenue_pad_3_button(int controller)
{
    if (config_input.avenue_pad_3_button[controller] == 1)
        return GG_KEY_SELECT;
    if (config_input.avenue_pad_3_button[controller] == 2)
        return GG_KEY_RUN;
    if (!emu_is_empty())
        return emu_get_core()->GetMedia()->GetAvenuePad3Button();
    return GG_KEY_RUN;
}

static bool input_keyboard_pressed(const bool* keyboard_state,
    SDL_Scancode primary, SDL_Scancode secondary)
{
    bool primary_pressed = primary > SDL_SCANCODE_UNKNOWN && primary < SDL_SCANCODE_COUNT &&
        keyboard_state[primary];
    bool secondary_pressed = secondary > SDL_SCANCODE_UNKNOWN &&
        secondary < SDL_SCANCODE_COUNT && keyboard_state[secondary];
    return primary_pressed || secondary_pressed;
}

static bool input_gamepad_pressed(SDL_Gamepad* gamepad, int primary, int secondary)
{
    return gamepad_get_button(gamepad, primary) || gamepad_get_button(gamepad, secondary);
}

static Uint16 input_build_state(int controller, bool update_turbo)
{
    const bool is_mouse_controller = events_is_mouse_controller(controller);
    const Uint16 mouse_button_mask = GG_KEY_I | GG_KEY_II | GG_KEY_RUN | GG_KEY_SELECT;

    SDL_Keymod mods = SDL_GetModState();
    if (mods & (SDL_KMOD_CTRL | SDL_KMOD_SHIFT | SDL_KMOD_ALT | SDL_KMOD_GUI))
        return 0;

    const bool* keyboard_state = SDL_GetKeyboardState(NULL);
    Uint16 ret = 0;
    config_InputProfile profile = input_get_profile(controller);

    const config_Input_Keyboard* keyboard_primary = &config_input_keyboard[controller][profile][0];
    const config_Input_Keyboard* keyboard_secondary = &config_input_keyboard[controller][profile][1];
    const config_Input_Gamepad* gamepad_primary = &config_input_gamepad[controller][profile][0];
    const config_Input_Gamepad* gamepad_secondary = &config_input_gamepad[controller][profile][1];

    SDL_Gamepad* sdl_controller = gamepad_controller[controller];

    if (input_keyboard_pressed(keyboard_state, keyboard_primary->key_left, keyboard_secondary->key_left))
        ret |= GG_KEY_LEFT;
    if (input_keyboard_pressed(keyboard_state, keyboard_primary->key_right, keyboard_secondary->key_right))
        ret |= GG_KEY_RIGHT;
    if (input_keyboard_pressed(keyboard_state, keyboard_primary->key_up, keyboard_secondary->key_up))
        ret |= GG_KEY_UP;
    if (input_keyboard_pressed(keyboard_state, keyboard_primary->key_down, keyboard_secondary->key_down))
        ret |= GG_KEY_DOWN;

    if (input_keyboard_pressed(keyboard_state, keyboard_primary->key_select, keyboard_secondary->key_select) ||
        input_gamepad_pressed(sdl_controller, gamepad_primary->gamepad_select, gamepad_secondary->gamepad_select))
    {
        ret |= GG_KEY_SELECT;
    }
    if (input_keyboard_pressed(keyboard_state, keyboard_primary->key_run, keyboard_secondary->key_run) ||
        input_gamepad_pressed(sdl_controller, gamepad_primary->gamepad_run, gamepad_secondary->gamepad_run))
    {
        ret |= GG_KEY_RUN;
    }
    if (input_keyboard_pressed(keyboard_state, keyboard_primary->key_I, keyboard_secondary->key_I) ||
        input_gamepad_pressed(sdl_controller, gamepad_primary->gamepad_I, gamepad_secondary->gamepad_I))
    {
        ret |= GG_KEY_I;
    }
    if (input_keyboard_pressed(keyboard_state, keyboard_primary->key_II, keyboard_secondary->key_II) ||
        input_gamepad_pressed(sdl_controller, gamepad_primary->gamepad_II, gamepad_secondary->gamepad_II))
    {
        ret |= GG_KEY_II;
    }

    if (profile == config_InputProfile_3Button)
    {
        GG_Keys preferred = input_get_avenue_pad_3_button(controller);
        GG_Keys alternate = preferred == GG_KEY_SELECT ? GG_KEY_RUN : GG_KEY_SELECT;

        if (input_keyboard_pressed(keyboard_state, keyboard_primary->key_III, keyboard_secondary->key_III) ||
            input_gamepad_pressed(sdl_controller, gamepad_primary->gamepad_III, gamepad_secondary->gamepad_III))
        {
            ret |= preferred;
        }
        if (input_keyboard_pressed(keyboard_state, keyboard_primary->key_IV, keyboard_secondary->key_IV) ||
            input_gamepad_pressed(sdl_controller, gamepad_primary->gamepad_IV, gamepad_secondary->gamepad_IV))
        {
            ret |= alternate;
        }
    }
    else if (profile == config_InputProfile_6Button)
    {
        if (input_keyboard_pressed(keyboard_state, keyboard_primary->key_III, keyboard_secondary->key_III) ||
            input_gamepad_pressed(sdl_controller, gamepad_primary->gamepad_III, gamepad_secondary->gamepad_III))
        {
            ret |= GG_KEY_III;
        }
        if (input_keyboard_pressed(keyboard_state, keyboard_primary->key_IV, keyboard_secondary->key_IV) ||
            input_gamepad_pressed(sdl_controller, gamepad_primary->gamepad_IV, gamepad_secondary->gamepad_IV))
        {
            ret |= GG_KEY_IV;
        }
        if (input_keyboard_pressed(keyboard_state, keyboard_primary->key_V, keyboard_secondary->key_V) ||
            input_gamepad_pressed(sdl_controller, gamepad_primary->gamepad_V, gamepad_secondary->gamepad_V))
        {
            ret |= GG_KEY_V;
        }
        if (input_keyboard_pressed(keyboard_state, keyboard_primary->key_VI, keyboard_secondary->key_VI) ||
            input_gamepad_pressed(sdl_controller, gamepad_primary->gamepad_VI, gamepad_secondary->gamepad_VI))
        {
            ret |= GG_KEY_VI;
        }
    }

    bool kb_turbo_I = input_keyboard_pressed(keyboard_state,
        keyboard_primary->key_toggle_turbo_I, keyboard_secondary->key_toggle_turbo_I);
    bool kb_turbo_II = input_keyboard_pressed(keyboard_state,
        keyboard_primary->key_toggle_turbo_II, keyboard_secondary->key_toggle_turbo_II);
    bool gp_turbo_I = input_gamepad_pressed(sdl_controller,
        gamepad_primary->gamepad_toggle_turbo_I, gamepad_secondary->gamepad_toggle_turbo_I);
    bool gp_turbo_II = input_gamepad_pressed(sdl_controller,
        gamepad_primary->gamepad_toggle_turbo_II, gamepad_secondary->gamepad_toggle_turbo_II);

    if (IsValidPointer(sdl_controller))
    {
        // Use D-Pad
        if (gamepad_primary->gamepad_directional == 0)
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
            const int STICK_DEAD_ZONE = 8000;
            const int rawx = SDL_GetGamepadAxis(sdl_controller, (SDL_GamepadAxis)gamepad_primary->gamepad_x_axis);
            const int rawy = SDL_GetGamepadAxis(sdl_controller, (SDL_GamepadAxis)gamepad_primary->gamepad_y_axis);

            const int x = gamepad_primary->gamepad_invert_x_axis ? -rawx : rawx;
            const int y = gamepad_primary->gamepad_invert_y_axis ? -rawy : rawy;

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

    if (is_mouse_controller)
    {
        if (update_turbo)
        {
            input_turbo_toggle_prev[controller][0] = false;
            input_turbo_toggle_prev[controller][1] = false;
        }
        return ret & mouse_button_mask;
    }

    bool pressed_turbo_I  = kb_turbo_I || gp_turbo_I;
    bool pressed_turbo_II = kb_turbo_II || gp_turbo_II;

    if (update_turbo && pressed_turbo_I && !input_turbo_toggle_prev[controller][0])
    {
        config_input.turbo_enabled[controller][0] = !config_input.turbo_enabled[controller][0];
        emu_set_turbo((GG_Controllers)controller, GG_KEY_I, config_input.turbo_enabled[controller][0]);
    }
    if (update_turbo && pressed_turbo_II && !input_turbo_toggle_prev[controller][1])
    {
        config_input.turbo_enabled[controller][1] = !config_input.turbo_enabled[controller][1];
        emu_set_turbo((GG_Controllers)controller, GG_KEY_II, config_input.turbo_enabled[controller][1]);
    }

    if (update_turbo)
    {
        input_turbo_toggle_prev[controller][0] = pressed_turbo_I;
        input_turbo_toggle_prev[controller][1] = pressed_turbo_II;
    }

    return ret;
}

static Uint16 input_filter_opposing_directions(int controller, Uint16 state)
{
    if (config_input.allow_up_down)
        return state;

    Uint16 previous = input_last_state[controller];

    if ((state & GG_KEY_UP) && (state & GG_KEY_DOWN))
    {
        if (previous & GG_KEY_UP)
            state = (Uint16)(state & ~GG_KEY_DOWN);
        else if (previous & GG_KEY_DOWN)
            state = (Uint16)(state & ~GG_KEY_UP);
        else
            state = (Uint16)(state & ~GG_KEY_DOWN);
    }

    if ((state & GG_KEY_LEFT) && (state & GG_KEY_RIGHT))
    {
        if (previous & GG_KEY_LEFT)
            state = (Uint16)(state & ~GG_KEY_RIGHT);
        else if (previous & GG_KEY_RIGHT)
            state = (Uint16)(state & ~GG_KEY_LEFT);
        else
            state = (Uint16)(state & ~GG_KEY_RIGHT);
    }

    return state;
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

static bool events_match_hotkey_scancode(const SDL_Event* event, const config_Hotkey& hotkey)
{
    if (event->type != SDL_EVENT_KEY_UP && event->type != SDL_EVENT_KEY_DOWN)
        return false;
    if (hotkey.key == SDL_SCANCODE_UNKNOWN)
        return false;
    return event->key.scancode == hotkey.key;
}
