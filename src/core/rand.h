#pragma once
#include "types.h"

typedef struct Rng
{
    u32 state;
} Rng;
void rng_seed(Rng *r, u32 seed);
u32 rng_next(Rng *r);
float rng_rangef(Rng *r, float min, float max);
int rng_rangei(Rng *r, int min, int max);
