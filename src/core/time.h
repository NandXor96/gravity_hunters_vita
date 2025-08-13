#pragma once

typedef struct FrameTimer { double current_time; double accumulator; } FrameTimer;
void timer_init(FrameTimer* t);
double timer_now(void);
double timer_frame_delta(FrameTimer* t);
