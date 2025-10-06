#pragma once

typedef struct FrameTimer {
    double current_time;
    double accumulator;
} FrameTimer;

/**
 * @brief Initialize a frame timer
 * @param t Timer to initialize
 */
void timer_init(FrameTimer *t);

/**
 * @brief Get current time in seconds
 * @return Current time in seconds
 */
double timer_now(void);

/**
 * @brief Calculate delta time since last frame
 * @param t Frame timer
 * @return Time elapsed since last call in seconds
 */
double timer_frame_delta(FrameTimer *t);
