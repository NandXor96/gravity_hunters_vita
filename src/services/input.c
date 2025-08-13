#include "input.h"
#include <SDL2/SDL.h>
#include <stdlib.h>
#include <math.h>

// Repeat timing (ms)
#define REPEAT_INITIAL_DELAY 320u

// Analog stick deadzone
#define STICK_DEADZONE 0.25f

static void repeat_update(struct RepeatState *rs, int is_down, unsigned int now, bool *out_step)
{
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

Input *input_create(void) {
    Input *in = calloc(1, sizeof(Input));
    in->controller = SDL_GameControllerOpen(0);
    return in;
}
void input_destroy(Input *in) { free(in); }
void input_poll(Input *in, InputState *out)
{
    if(!out) return;
    // clear step outputs each poll (they are edge style)
    out->speed_up_step = out->speed_down_step = false;
    out->turn_left_step = out->turn_right_step = false;

    #ifdef PLATFORM_VITA
    out->confirm = SDL_GameControllerGetButton(in->controller, SDL_CONTROLLER_BUTTON_A);
    out->speed_up = SDL_GameControllerGetButton(in->controller, SDL_CONTROLLER_BUTTON_DPAD_UP);
    out->speed_down = SDL_GameControllerGetButton(in->controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
    out->turn_left = SDL_GameControllerGetButton(in->controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
    out->turn_right = SDL_GameControllerGetButton(in->controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
    out->fire = SDL_GameControllerGetButton(in->controller, SDL_CONTROLLER_BUTTON_A);
    out->mod_small = SDL_GameControllerGetButton(in->controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
    out->mod_large = SDL_GameControllerGetButton(in->controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
    // Analog (left stick)
    Sint16 ax = SDL_GameControllerGetAxis(in->controller, SDL_CONTROLLER_AXIS_LEFTX);
    Sint16 ay = SDL_GameControllerGetAxis(in->controller, SDL_CONTROLLER_AXIS_LEFTY);
    out->stick_x = ax / 32767.0f;
    out->stick_y = ay / 32767.0f;
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
    // No controller => zero analog.
    out->stick_x = 0.f; out->stick_y = 0.f;
    #endif

    // Auto-repeat (works for both platforms) turned into step events.
    unsigned int now = SDL_GetTicks();
    repeat_update(&in->rpt_turn_left,  out->turn_left,  now, &out->turn_left_step);
    repeat_update(&in->rpt_turn_right, out->turn_right, now, &out->turn_right_step);
    repeat_update(&in->rpt_speed_up,   out->speed_up,   now, &out->speed_up_step);
    repeat_update(&in->rpt_speed_down, out->speed_down, now, &out->speed_down_step);

    // Analog derived values
    float sx = out->stick_x;
    float sy = out->stick_y;
    float mag = sqrtf(sx*sx + sy*sy);
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
}
