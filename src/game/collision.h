#pragma once
#include "entity.h"

// #define DEBUG_COLLISION // Enable to see collision radius and polygons

struct World; // forward
struct Renderer; // forward

// Run collision detection & dispatch (no global resolution).
void collision_run(struct World *w, float dt);

// Debug Drawing (compile-time only):
#ifdef DEBUG_COLLISION
void collision_debug_draw(struct World *w, struct Renderer *r);
#endif

// Future extensions (polygon SAT) will reuse these helpers internally.
