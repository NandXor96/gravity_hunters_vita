#pragma once
#include <SDL2/SDL.h>

#define DISPLAY_W 960
#define DISPLAY_H 544
/* Physics tick size: was 1/120 leading to alternating 1 or 2 substeps per 60Hz frame on timing jitter,
 * causing visible projectile speed fluctuation (sometimes only 1 substep executed).
 * Using 1/60 aligns with display refresh for steadier perceived motion on Vita. */
#define FIXED_DT (1.0f/60.0f)

// World
#define MAX_ENEMIES 9
#define MAX_PLANETS 16
#define MAX_SHOOTERS 10
#define MAX_PROJECTILES_PER_SHOOTER 100
#define TRAIL_LEN 10
#define MAX_EXPLOSIONS 64

typedef unsigned int u32;

typedef struct Vec2 { float x, y; } Vec2;

/* Common project-wide macros centralized here for easier tuning. */
/* Physics / simulation */
#ifndef PROJ_GRAVITY_CONST
#define PROJ_GRAVITY_CONST 500.0f
#endif
#ifndef PROJ_EPSILON
#define PROJ_EPSILON 0.0001f
#endif

/* Projectile simulation max time (seconds) used by AI simulation) */
#ifndef SIM_MAX_PROJECTILE_TIME
#define SIM_MAX_PROJECTILE_TIME 3.0f
#endif

/* Numeric constants */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Input timings */
#ifndef REPEAT_INITIAL_DELAY
#define REPEAT_INITIAL_DELAY 320u
#endif
#ifndef STICK_DEADZONE
#define STICK_DEADZONE 0.8f
#endif

/* Texture flags */
#ifndef TEXF_SHEET
#define TEXF_SHEET (1u << 0)
#endif

/* Enemy constants */
#ifndef ENEMY_SHOT_HIT_RADIUS
#define ENEMY_SHOT_HIT_RADIUS 10.0f
#endif

/* HUD constants (visual/layout tuning) */
#ifndef HUD_STAT_TEXT_OFFSET_Y
#define HUD_STAT_TEXT_OFFSET_Y   -1.0f
#endif
#ifndef HUD_STAT_EXTRA_ICON_OFFSET_Y
#define HUD_STAT_EXTRA_ICON_OFFSET_Y 2.0f
#endif
#ifndef HUD_BG_HEIGHT
#define HUD_BG_HEIGHT 58.0f
#endif
#ifndef HUD_BG_ALPHA
#define HUD_BG_ALPHA 150
#endif
#ifndef HUD_BG_BORDER_ALPHA
#define HUD_BG_BORDER_ALPHA 180
#endif
