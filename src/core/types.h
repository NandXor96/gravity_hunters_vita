#pragma once
#include <SDL2/SDL.h>

#define DISPLAY_W 960
#define DISPLAY_H 544
/* Physics tick size: was 1/120 leading to alternating 1 or 2 substeps per 60Hz frame on timing jitter,
 * causing visible projectile speed fluctuation (sometimes only 1 substep executed).
 * Using 1/60 aligns with display refresh for steadier perceived motion on Vita. */
#define FIXED_DT (1.0f/60.0f)

#define MAX_ENEMIES 9
#define MAX_PLANETS 16
#define MAX_SHOOTERS 10
#define MAX_PROJECTILES_PER_SHOOTER 100
#define TRAIL_LEN 10
#define MAX_EXPLOSIONS 64

typedef unsigned int u32;

typedef struct Vec2 { float x, y; } Vec2;
