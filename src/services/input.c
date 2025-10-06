#include "input.h"
#include <SDL2/SDL.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "../core/types.h"


static void repeat_update(struct RepeatState *rs, int is_down, unsigned int now, bool *out_step) {

    *out_step = false;
    if (is_down) {
        if (!rs->held) {
            rs->held = 1;
            rs->first_press_time_ms = now;
            rs->next_time_ms = now; // immediate tap step

}
        if (now >= rs->next_time_ms) {
            *out_step = true;
            if (now - rs->first_press_time_ms < REPEAT_INITIAL_DELAY) {
                // still inside initial delay window: schedule the next trigger exactly at end of delay
                rs->next_time_ms = rs->first_press_time_ms + REPEAT_INITIAL_DELAY;
        } else {
                // after initial delay: fire every frame (set next time to now so next frame will trigger again)
                rs->next_time_ms = now;
            }
        }
    } else {
        rs->held = 0;
    }
}

static void repeat_update_interval(struct RepeatState *rs, int is_down, unsigned int now, unsigned int delay_ms, unsigned int interval_ms, bool *out_step) {

    *out_step = false;
    if (is_down) {
        if (!rs->held) {
            rs->held = 1;
            rs->first_press_time_ms = now;
            rs->next_time_ms = now + delay_ms;
            *out_step = true;
        } else if (now >= rs->next_time_ms) {
            *out_step = true;
            rs->next_time_ms = now + interval_ms;
        }
    } else {
        rs->held = 0;
        rs->next_time_ms = now;
        rs->first_press_time_ms = now;
    }
}

Input *input_create(void) {

    Input *in = calloc(1, sizeof(Input));
    in->controller = SDL_GameControllerOpen(0);
    return in;

}
void input_destroy(Input *in) {
    free(in);
}
void input_poll(Input *in, InputState *out) {
    if (!out)
        return;
    if (!in) {
        memset(out, 0, sizeof(*out));
        return;
    }

    // Default all outputs to a known state before applying platform/controller input.
    out->move_up = out->move_down = out->move_left = out->move_right = false;
    out->speed_up = out->speed_down = false;
    out->turn_left = out->turn_right = false;
    out->menu_up = out->menu_down = out->menu_left = out->menu_right = false;
    out->fire = out->pause = out->confirm = out->back = false;
    out->button_triangle = false;
    out->mod_small = out->mod_large = false;
    out->speed_up_step = out->speed_down_step = false;
    out->turn_left_step = out->turn_right_step = false;
    out->boost = false;
    out->stick_x = out->stick_y = 0.f;
    out->stick_angle = 0.f;
    out->stick_strength = 0.f;
    out->stick_active = false;

    SDL_GameController *pad = in->controller;
    Sint16 raw_ax = 0;
    Sint16 raw_ay = 0;

#ifdef PLATFORM_VITA
    if (pad) {
        out->confirm = SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_A);
        out->back = SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_B);
        out->pause = SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_START);
        out->speed_up = SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_UP);
        out->speed_down = SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
        out->turn_left = SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
        out->turn_right = SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
        out->fire = SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_A);
        out->boost = SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_B); // placeholder mapping
        out->button_triangle = SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_Y);
        out->mod_small = SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
        out->mod_large = SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
        raw_ax = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTX);
        raw_ay = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTY);
    }
#else
    const Uint8 *ks = SDL_GetKeyboardState(NULL);
    out->move_up = ks[SDL_SCANCODE_W];
    out->move_down = ks[SDL_SCANCODE_S];
    out->move_left = ks[SDL_SCANCODE_A];
    out->move_right = ks[SDL_SCANCODE_D];
    out->speed_up = ks[SDL_SCANCODE_UP];
    out->speed_down = ks[SDL_SCANCODE_DOWN];
    out->turn_left = ks[SDL_SCANCODE_LEFT];
    out->turn_right = ks[SDL_SCANCODE_RIGHT];
    out->fire = ks[SDL_SCANCODE_SPACE];
    out->pause = ks[SDL_SCANCODE_ESCAPE];
    out->confirm = ks[SDL_SCANCODE_RETURN];
    out->back = ks[SDL_SCANCODE_BACKSPACE];
    out->mod_small = ks[SDL_SCANCODE_LCTRL];
    out->mod_large = ks[SDL_SCANCODE_RCTRL];
    out->boost = (ks[SDL_SCANCODE_LALT] || ks[SDL_SCANCODE_RALT]);
    out->button_triangle = ks[SDL_SCANCODE_Y];
    if (pad) {
        if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_A)) {
            out->confirm = true;
            out->fire = true;
        }
        if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_B)) {
            out->back = true;
            out->boost = true;
        }
        out->pause |= SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_START);
        out->speed_up |= SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_UP);
        out->speed_down |= SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
        out->turn_left |= SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
        out->turn_right |= SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
        out->mod_small |= SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
        out->mod_large |= SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
        out->button_triangle |= SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_Y);
        raw_ax = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTX);
        raw_ay = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTY);
    }
#endif

    out->stick_x = raw_ax / 32767.0f;
    out->stick_y = raw_ay / 32767.0f;

    // Auto-repeat (works for both platforms) turned into step events.
    unsigned int now = SDL_GetTicks();
    repeat_update(&in->rpt_turn_left,  out->turn_left,  now, &out->turn_left_step);
    repeat_update(&in->rpt_turn_right, out->turn_right, now, &out->turn_right_step);
    repeat_update(&in->rpt_speed_up,   out->speed_up,   now, &out->speed_up_step);
    repeat_update(&in->rpt_speed_down, out->speed_down, now, &out->speed_down_step);

    // Analog derived values
    float sx = out->stick_x;
    float sy = out->stick_y;
    float mag = sqrtf(sx * sx + sy * sy);
    if (mag < STICK_DEADZONE) {
        out->stick_active = false;
        out->stick_strength = 0.f;
        out->stick_angle = 0.f;
} else {
        float nm = (mag - STICK_DEADZONE) / (1.f - STICK_DEADZONE);
        if (nm < 0.f) nm = 0.f; if (nm > 1.f) nm = 1.f;
        out->stick_strength = nm;
        out->stick_active = true;
        out->stick_angle = atan2f(sy, sx); // radians (-pi..pi)
    }

    const float MENU_STICK_THRESHOLD = 0.5f;
    bool stick_up = out->stick_active && (out->stick_y <= -MENU_STICK_THRESHOLD);
    bool stick_down = out->stick_active && (out->stick_y >= MENU_STICK_THRESHOLD);
    bool stick_left = out->stick_active && (out->stick_x <= -MENU_STICK_THRESHOLD);
    bool stick_right = out->stick_active && (out->stick_x >= MENU_STICK_THRESHOLD);

    bool nav_up_now = out->speed_up || out->move_up || stick_up;
    bool nav_down_now = out->speed_down || out->move_down || stick_down;
    bool nav_left_now = out->turn_left || out->move_left || stick_left;
    bool nav_right_now = out->turn_right || out->move_right || stick_right;

    repeat_update_interval(&in->rpt_menu_up, nav_up_now, now, MENU_REPEAT_DELAY_MS, MENU_REPEAT_INTERVAL_MS, &out->menu_up);
    repeat_update_interval(&in->rpt_menu_down, nav_down_now, now, MENU_REPEAT_DELAY_MS, MENU_REPEAT_INTERVAL_MS, &out->menu_down);
    repeat_update_interval(&in->rpt_menu_left, nav_left_now, now, MENU_REPEAT_DELAY_MS, MENU_REPEAT_INTERVAL_MS, &out->menu_left);
    repeat_update_interval(&in->rpt_menu_right, nav_right_now, now, MENU_REPEAT_DELAY_MS, MENU_REPEAT_INTERVAL_MS, &out->menu_right);
}
