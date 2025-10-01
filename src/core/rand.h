#pragma once
#include "types.h"

typedef struct Rng {
    u32 state;
} Rng;

/**
 * @brief Initialize random number generator with seed
 * @param r RNG instance
 * @param seed Seed value
 */
void rng_seed(Rng *r, u32 seed);

/**
 * @brief Generate next random u32 value
 * @param r RNG instance
 * @return Random u32 value
 */
u32 rng_next(Rng *r);

/**
 * @brief Generate random float in range
 * @param r RNG instance
 * @param min Minimum value (inclusive)
 * @param max Maximum value (inclusive)
 * @return Random float in [min, max]
 */
float rng_rangef(Rng *r, float min, float max);

/**
 * @brief Generate random integer in range
 * @param r RNG instance
 * @param min Minimum value (inclusive)
 * @param max Maximum value (inclusive)
 * @return Random integer in [min, max]
 */
int rng_rangei(Rng *r, int min, int max);
