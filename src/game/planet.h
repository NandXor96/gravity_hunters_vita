#pragma once
#include "entity.h"
#include "../core/math.h"

typedef struct Planet {
    Entity e;
    float radius;
    float radius_sq; // cached radius^2 for fast collision tests
    float mass;
    SDL_Rect src; // source rect within planet spritesheet
    struct World *world; // back-reference for spawning effects
} Planet;
Planet *planet_create(float x, float y, float radius, float mass, SDL_Texture *tex, SDL_Rect src);
void planet_destroy(Planet *p);
Circle planet_collider(const Planet *p);
