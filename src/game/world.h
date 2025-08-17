#pragma once
#include "entity.h"
#include "../core/rand.h"

#include "projectile_system.h"

typedef struct World
{
    struct Services *svc;
    struct Rng rng;
    u32 seed;
    struct Player *player;
    struct Enemy *enemies[MAX_ENEMIES];
    int enemy_count;
    struct Planet **planets;
    int planet_count;
    ProjectileSystem projsys;
    struct Explosion* explosions[MAX_EXPLOSIONS];
    int explosion_count;
    int score, kills, deaths;
    bool active;
    bool paused;
    float proj_oob_margin_factor; // fraction of display size as extra margin (e.g. 0.2f)
    float time; // accumulated world time
    struct Hud* hud; // UI overlay owned by world
    // Time limit handling
    float time_limit; // seconds; -1 = infinite
    int   time_over_triggered; // guard so callback fires once
    void (*on_time_over)(struct World*, void* user);
    void *on_time_over_user;
} World;

/* SIM_MAX_PROJECTILE_TIME is centralized in core/types.h */

/* Projectile OOB helper: fills output bounds (min_x,min_y,max_x,max_y) for projectile simulation
 * based on world's display size and proj_oob_margin_factor. All callers should use this instead
 * of recomputing bounds locally.
 */
void world_get_proj_oob_bounds(World *w, float *out_min_x, float *out_min_y, float *out_max_x, float *out_max_y);

// Forward HUD API
struct Hud* hud_create(struct Services* svc, struct Player* player);
void hud_destroy(struct Hud* h);
void hud_update(struct Hud* h, struct World* w, float dt);
void hud_render(struct Hud* h, struct Renderer* r);

World *world_create(struct Services *svc, u32 seed, unsigned int max_planet_area);
void world_destroy(World *w);
void world_update(World *w, float dt);
void world_render(World *w, struct Renderer *r);

bool world_add_planet(World *w, float x, float y, float radius, float mass, SDL_Texture *tex);
bool world_add_player(World *w, float x, float y);
bool world_spawn_enemy(World *w, int kind, float x, float y);
int world_register_shooter(World *w);
bool world_fire_projectile(World *w, int shooter_index, Entity *owner, float angle, float strength);
bool world_add_explosion(World* w, int type, float x, float y, float scale);
void world_set_time_limit(World* w, float seconds); // -1 for infinite
