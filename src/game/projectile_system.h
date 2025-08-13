#pragma once
#include "projectile.h"
#include "../core/types.h"
#include "../services/texture_manager.h"
#include "../services/renderer.h"

typedef struct ShooterPool
{
    struct Projectile *items[MAX_PROJECTILES_PER_SHOOTER];
    int head;
    float last_fire_time;
} ShooterPool;

typedef struct ProjectileSystem
{
    ShooterPool shooters[MAX_SHOOTERS];
    int shooter_count;
    TextureManager *texman; // for tinted projectile textures
} ProjectileSystem;

void projectile_system_init(ProjectileSystem *ps, TextureManager *tm);
void projectile_system_shutdown(ProjectileSystem *ps);
int projectile_system_register_shooter(ProjectileSystem *ps);
bool projectile_system_fire(ProjectileSystem *ps, float world_time, int shooter_index, Entity *owner, float angle, float strength);
struct Player;
struct Enemy; // forward
void projectile_system_update(ProjectileSystem *ps, struct Planet **planets, int planet_count, struct Player *player, struct Enemy **enemies, int enemy_count, float oob_margin_factor, int display_w, int display_h, float dt, float world_time);
void projectile_system_render(ProjectileSystem *ps, Renderer *r);
