#include "time.h"
#include <SDL2/SDL.h>

void timer_init(FrameTimer *t) {

    t->current_time = SDL_GetTicks() / 1000.0;
    t->accumulator = 0;

}
double timer_now(void) {
    return SDL_GetTicks() / 1000.0;
}
double timer_frame_delta(FrameTimer *t) {
    double now = timer_now();
    double dt = now - t->current_time;
    t->current_time = now;
    return dt;
}
