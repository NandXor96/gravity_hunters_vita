#pragma once
#include <stdbool.h>
#include <SDL2/SDL.h>

typedef struct InputState {
    bool move_up, move_down, move_left, move_right; // WASD
    bool speed_up, speed_down; // Arrow Up/Down adjust projectile speed
    bool turn_left, turn_right; // Arrow Left/Right rotation
    // Menu navigation (single-frame edge events are generated in input_poll)
    bool menu_up, menu_down, menu_left, menu_right;
    bool fire, pause, confirm, back;
    bool button_triangle;
    // Modifiers (small / large step)
    bool mod_small, mod_large; // L / R shoulder (Vita) or LCTRL / RCTRL (PC)
    // Auto-repeat step events (fire true ONLY on the frame a repeat triggers)
    bool speed_up_step, speed_down_step;
    bool turn_left_step, turn_right_step;
    // Boost button (edge detection handled in gameplay code if needed)
    bool boost;
    // Raw analog stick values (normalized -1..1)
    float stick_x, stick_y;
    // Derived analog info
    float stick_angle;    // radians
    float stick_strength; // 0..1 after deadzone removal
    bool  stick_active;   // true if outside deadzone
} InputState;

typedef struct Input {
    SDL_GameController *controller;
    // internal repeat bookkeeping
    struct RepeatState {
        unsigned int next_time_ms; // SDL_GetTicks when next repeat fires
        unsigned int first_press_time_ms;
        int held; // 0/1 previous frame held state
    } rpt_turn_left, rpt_turn_right, rpt_speed_up, rpt_speed_down,
      rpt_menu_up, rpt_menu_down, rpt_menu_left, rpt_menu_right;
} Input;
Input *input_create(void);
void input_destroy(Input *in);
void input_poll(Input *in, InputState *out);
