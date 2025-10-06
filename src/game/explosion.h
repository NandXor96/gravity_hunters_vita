#pragma once
#include "entity.h"
#include <SDL2/SDL.h>

typedef struct Explosion {
    Entity e;
    int type;         /* explosion variant */
    int frame;        /* current frame */
    int frame_count;  /* total frames */
    float frame_time; /* seconds per frame */
    float timer;      /* accum */
    float scale;      /* render scale */
    int active;       /* active flag */
} Explosion;

Explosion *explosion_create(int type, float x, float y, float scale);
void       explosion_destroy(Explosion *ex);
void       explosion_spawn_at_planet_center(struct Planet *planet); /* convenience */
