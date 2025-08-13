#include "enemy.h"
#include <stdlib.h>
#include <math.h>
#include "weapon.h"
#include "projectile.h"
#include "world.h"
#include "player.h"
#include "../services/renderer.h"
#include "../services/texture_manager.h"
#include "../services/services.h"

static void enemy_render(Entity *e, struct Renderer *r)
{
    if (!e || e->type != ENT_ENEMY)
        return;
    Enemy *en = (Enemy *)e;
    if (!en->alive)
        return;
    // Use sprite sheet index = enemy type
    SDL_Rect src = {0, 0, 0, 0};
    if (en->e.texture)
    {
        Services *svc = services_get();
        if (svc && svc->texman)
        {
            src = texman_sheet_src(svc->texman, TEX_ENEMIES_SHEET, (int)en->type);
        }
    }
    float w = en->e.size.x;
    float h = en->e.size.y;
    SDL_FRect dst = {en->e.pos.x - w * 0.5f, en->e.pos.y - h * 0.5f, w, h};
    // Convert angle to degrees; align similar to player (assume artwork faces up) -> apply angle_offset
    const float PI_F = 3.14159265358979323846f;
    float angle_deg = (en->e.angle + en->e.angle_offset) * (180.f / PI_F);
    renderer_draw_texture(r, en->e.texture, (src.w > 0 ? &src : NULL), &dst, angle_deg);
}
static void enemy_update(Entity *e, float dt)
{
    Enemy *enemy = (Enemy *)e;
}

static void enemy_on_hit(Entity *e, Entity *hitter)
{
    if (!e || e->type != ENT_ENEMY)
        return;
    Enemy *en = (Enemy *)e;
    if (!en->alive)
        return;
    if (hitter && hitter->type == ENT_PROJECTILE)
    {
        struct Projectile *pr = (struct Projectile *)hitter;

        // Friendly fire filter: ignore projectiles fired by other enemies
        if (pr && pr->owner_kind == ENT_ENEMY)
            return;

        // Decrease health by projectile damage
        int dmg = 0;
        if (pr)
            dmg = pr->damage;

        en->health -= dmg;

        // Add points for hitting enemy
        if (en->world)
            en->world->score += pr->flight_time * 20; // Todo: Examine if 20 is appropriate

        // When health is zero or below
        if (en->health <= 0)
        {
            en->alive = false; // die

            // Increase world kills
            if (en->world)
                en->world->kills++;

            if (en->world)
                en->world->score += 100; // Add a score of 100 for each kill; Todo: Dynamic scoring
        }

        // Deactivate projectile
        if (pr)
            pr->active = false;

        // Add Explosion
        if (en->world)
        {
            Services *svc = services_get();
            if (svc && svc->texman)
            {
                int types = texman_explosion_type_count(svc->texman);
                if (types > 0)
                {
                    int type = rand() % types;
                    float x;
                    float y;
                    float scale;
                    // Use projectile position on hit where player don't die
                    if (en->alive)
                    {
                        x = hitter->pos.x;
                        y = hitter->pos.y;
                        scale = 0.5f;
                    }
                    // Use enemy position if killed
                    else
                    {
                        x = en->e.pos.x;
                        y = en->e.pos.y;
                        scale = 0.8f;
                    }
                    world_add_explosion(en->world, type, x, y, scale);
                }
            }
        }
    }
}

static Entity *enemy_create_entity(void *params)
{
    (void)params; // new path shouldn't use generic factory directly
    return NULL;  // not used; enemies spawned via world_spawn_enemy
}
static void enemy_destroy_entity(Entity *e) { enemy_destroy((Enemy *)e); }
static const EntityVTable ENEMY_VT = {enemy_create_entity, enemy_destroy_entity, enemy_update, enemy_render, enemy_on_hit};

/* --- Type specific init helpers -------------------------------------------------- */
static SDL_Texture *enemy_base_texture(void)
{
    Services *svc = services_get();
    if (!svc || !svc->texman)
        return NULL;
    return texman_get(svc->texman, TEX_ENEMIES_SHEET);
}

static bool enemy_apply_common(Enemy *e, int hp, float move_speed, float shoot_speed, bool can_fight)
{
    e->health = hp;
    e->move_speed = move_speed;
    e->shoot_speed = shoot_speed;
    e->can_fight = can_fight;
    if (!e->weapon)
        e->weapon = weapon_create_default();
    return true;
}
static bool enemy_create_astro_ant(Enemy *e) { return enemy_apply_common(e, 20, 55.f, 0.8f, true); }
static bool enemy_create_frigate(Enemy *e) { return enemy_apply_common(e, 20, 25.f, 1.2f, true); }
static bool enemy_create_holo_shark(Enemy *e) { return enemy_apply_common(e, 20, 90.f, 0.6f, true); }
static bool enemy_create_nova_nomad(Enemy *e) { return enemy_apply_common(e, 20, 70.f, 0.7f, true); }
static bool enemy_create_plasma_pirate(Enemy *e) { return enemy_apply_common(e, 20, 60.f, 0.5f, true); }
static bool enemy_create_shock_bee(Enemy *e) { return enemy_apply_common(e, 20, 110.f, 0.4f, true); }
static bool enemy_create_shredder_swallow(Enemy *e) { return enemy_apply_common(e, 20, 50.f, 0.9f, true); }
static bool enemy_create_spark_falcon(Enemy *e) { return enemy_apply_common(e, 20, 95.f, 0.5f, true); }
static bool enemy_create_warp_wesp(Enemy *e) { return enemy_apply_common(e, 20, 80.f, 0.6f, true); }

Enemy *enemy_create(World *world, EnemyType type, float x, float y, float angle, int id, int shooter_index)
{
    Enemy *e = calloc(1, sizeof(Enemy));
    if (!e)
        return NULL;
    e->world = world; // set world reference
    e->e.type = ENT_ENEMY;
    e->e.vt = &ENEMY_VT;
    e->type = type;
    e->e.pos.x = x;
    e->e.pos.y = y;
    e->e.angle = angle;
    e->id = id;
    e->shooter_index = shooter_index;
    e->alive = true;
    e->e.size.x = 32;
    e->e.size.y = 32; // default tile size; can tweak per type later
    e->e.texture = enemy_base_texture();
    e->e.angle_offset = (M_PI * 0.5f);
    // dispatch
    bool ok = false;
    switch (type)
    {
    case ENEMY_ASTRO_ANT:
        ok = enemy_create_astro_ant(e);
        break;
    case ENEMY_FRIGATE:
        ok = enemy_create_frigate(e);
        break;
    case ENEMY_HOLO_SHARK:
        ok = enemy_create_holo_shark(e);
        break;
    case ENEMY_NOVA_NOMAD:
        ok = enemy_create_nova_nomad(e);
        break;
    case ENEMY_PLASMA_PIRATE:
        ok = enemy_create_plasma_pirate(e);
        break;
    case ENEMY_SHOCK_BEE:
        ok = enemy_create_shock_bee(e);
        break;
    case ENEMY_SHREDDER_SWALLOW:
        ok = enemy_create_shredder_swallow(e);
        break;
    case ENEMY_SPARK_FALCON:
        ok = enemy_create_spark_falcon(e);
        break;
    case ENEMY_WARP_WESP:
        ok = enemy_create_warp_wesp(e);
        break;
    default:
        ok = enemy_create_astro_ant(e);
        e->type = ENEMY_ASTRO_ANT;
        break;
    }
    if (!ok)
    {
        enemy_destroy(e);
        return NULL;
    }
    return e;
}
void enemy_destroy(Enemy *en)
{
    if (!en)
        return;
    if (en->weapon)
        weapon_destroy(en->weapon);
    free(en);
}
void enemy_ai_update(Enemy *en, struct World *w, float dt)
{
    (void)en;
    (void)w;
    (void)dt;
}
bool enemy_try_shoot(Enemy *en, struct World *w)
{
    (void)en;
    (void)w;
    return false;
}
OBB enemy_collider(const Enemy *en) { return (OBB){{en->e.pos.x, en->e.pos.y}, {en->e.size.x, en->e.size.y}, en->e.angle}; }
