#pragma once
#include <stdbool.h>
#include <SDL2/SDL.h>

struct Services;
struct Renderer;
struct World;

typedef float (*HudBarValueFn)(struct World *w, void *user); // returns normalized 0..1

typedef struct HudBar {
    SDL_FRect rect; // screen-space rect; rect.x,rect.y are explicit top-left
    SDL_Color fill_color;
    SDL_Color bg_color;
    HudBarValueFn value_fn;
    void *user_data; // opaque pointer passed to value_fn
    bool visible;
    SDL_Texture *icon_tex; // optional icon texture (borrowed)
    SDL_Rect icon_src;      // source rect
    bool has_icon;
} HudBar;

#define HUD_MAX_BARS 8
#define HUD_MAX_STATS 8

typedef struct HudStat {
    SDL_FRect rect; // area including space for icon
    char text[64];
    bool visible;
    SDL_Texture *icon_tex;
    SDL_Rect icon_src;
    bool has_icon;
    // Optional secondary icon (e.g. infinity symbol instead of text)
    SDL_Texture *extra_icon_tex;
    SDL_Rect extra_icon_src;
    bool has_extra_icon;
    // dynamic provider (optional). If set, called each frame to refresh text buffer.
    void (*update_fn)(struct HudStat *stat, struct World *w, void* user);
    void *user_data;
} HudStat;

typedef struct Hud {
    struct Services *svc;
    struct World *world;   // back-ref for value functions
    struct Player *player; // single player reference
    bool player_bars_initialized;
    HudBar bars[HUD_MAX_BARS];
    int bar_count;
    bool visible;
    // Icon textures
    SDL_Texture *tex_hud;     // Icons Tex
    SDL_Rect icon_src[7];     // Array of icon source rectangles
    int shot_speed_bar_index; // index of bar showing shot speed
    int weapon_cd_bar_index;  // index of cooldown progress bar
    int health_bar_index;     // index of health bar (center bottom)
    // Stats (text rows with icons)
    HudStat stats[HUD_MAX_STATS];
    int stat_count;
} Hud;

Hud *hud_create(struct Services *svc, struct Player *player);
void hud_destroy(Hud *h);
void hud_update(Hud *h, struct World *w, float dt);
void hud_render(Hud *h, struct Renderer *r);
// Add a progress bar; x,y = top-left, length = width, height fixed (rect.h provided). Returns index or -1.
int hud_add_bar(Hud *h, float x, float y, float length, float height, SDL_Color fill, SDL_Color bg, HudBarValueFn fn, void *user_data);
int hud_bar_set_icon(Hud *h, int bar_index, SDL_Texture *tex, SDL_Rect src);
// Add a text stat row (icon + dynamic text). Returns index or -1.
int hud_add_stat(Hud *h, float x, float y, float w, float hgt, SDL_Rect icon_src, void (*update_fn)(HudStat*, struct World*, void*), void* user_data);
