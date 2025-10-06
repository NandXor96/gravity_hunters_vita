#include "rand.h"

void rng_seed(Rng *r, u32 seed) {

    r->state = seed ? seed : 1;

}
u32 rng_next(Rng *r) {
    u32 x = r->state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    r->state = x;
    return x;
}
float rng_rangef(Rng *r, float min, float max) {
    return min + (rng_next(r) / (float)0xFFFFFFFFu) * (max - min);
}
int rng_rangei(Rng *r, int min, int max) {
    return min + (int)(rng_next(r) % (u32)(max - min + 1));
}
