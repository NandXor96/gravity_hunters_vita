#pragma once
#include <SDL2/SDL.h>

#define DISPLAY_W 960
#define DISPLAY_H 544
/* Physics tick size: was 1/120 leading to alternating 1 or 2 substeps per 60Hz frame on timing jitter,
 * causing visible projectile speed fluctuation (sometimes only 1 substep executed).
 * Using 1/60 aligns with display refresh for steadier perceived motion on Vita. */
#define FIXED_DT (1.0f/60.0f)

// Menu
#define MENU_REPEAT_DELAY_MS    500u
#define MENU_REPEAT_INTERVAL_MS 100u
#define MENU_COLOR_BUTTON_BASE            (SDL_Color){34, 34, 34, 255}
#define MENU_COLOR_BUTTON_DISABLED        (SDL_Color){22, 22, 22, 190}
#define MENU_COLOR_BUTTON_INACTIVE        (SDL_Color){60, 60, 60, 255}
#define MENU_COLOR_BUTTON_HIGHLIGHT       (SDL_Color){47, 203, 225, 255}
#define MENU_COLOR_SELECTBOX_BACKGROUND   (SDL_Color){24, 24, 24, 255}
#define MENU_COLOR_SELECTBOX_OUTLINE      (SDL_Color){80, 80, 80, 255}
#define MENU_COLOR_TEXT_BACKGROUND        (SDL_Color){20, 20, 20, 180}
#define MENU_COLOR_TEXT_BACKGROUND_DARKER (SDL_Color){20, 20, 20, 240}
#define OVERLAY_BACKDROP_COLOR            (SDL_Color){0, 0, 0, 160}

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

#define PROJ_DAMAGE_INCREASE_PER_SECOND 5

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
