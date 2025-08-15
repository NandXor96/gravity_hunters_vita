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
static void enemy_on_collide(Entity *self, Entity *other)
{
    if (!self || self->type != ENT_ENEMY)
        return;
    Enemy *en = (Enemy *)self;
    if (!en->alive)
        return;
    (void)other;
    // Placeholder: could apply small health damage or bounce later
}
static const EntityVTable ENEMY_VT = {enemy_create_entity, enemy_destroy_entity, enemy_update, enemy_render, enemy_on_hit, enemy_on_collide};

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
// Übernimmt ein lokales Polygon in den Collider der Enemy.
// Erwartet rohe Pixelkoordinaten (wie aus JSON exportiert). Diese werden
// unverändert in poly_local gespeichert; die Welttransformation passiert
// später einmal pro Frame in collider_prepare() (collision.c).
static void enemy_set_polygon(Enemy *e, const Vec2 *pts, int count)
{
    if (!e || !pts || count <= 0)
        return;
    if (count > 30)
        count = 30;
    e->e.collider.poly_count = count;
    for (int i = 0; i < count; i++)
        e->e.collider.poly_local[i] = pts[i];
    e->e.collider.poly_world_dirty = 1; // mark world poly_world dirty
    e->e.collider.shape = COLLIDER_SHAPE_POLY;
}
static const Vec2 POLY_ASTRO_ANT[] = {{0, 7}, {0, 11}, {5, 15}, {3, 17}, {8, 26}, {10, 22}, {14, 27}, {19, 26}, {21, 22}, {23, 26}, {28, 18}, {26, 15}, {31, 11}, {31, 7}, {20, 10}, {23, 5}, {25, 6}, {25, 3}, {22, 3}, {19, 7}, {12, 7}, {9, 3}, {6, 3}, {6, 6}, {10, 7}, {10, 11}};
static const Vec2 POLY_FRIGATE[] = {{0, 13}, {0, 24}, {9, 19}, {15, 28}, {21, 19}, {31, 24}, {30, 11}, {28, 9}, {22, 10}, {17, 3}, {14, 3}, {9, 10}, {3, 9}};
static const Vec2 POLY_HOLO_SHARK[] = {{15, 1}, {9, 11}, {0, 20}, {0, 23}, {8, 20}, {10, 22}, {8, 29}, {12, 27}, {15, 30}, {18, 27}, {23, 29}, {21, 22}, {23, 20}, {31, 23}, {31, 20}};
static const Vec2 POLY_NOVA_NOMAD[] = {{8, 1}, {1, 9}, {0, 16}, {2, 22}, {0, 24}, {0, 30}, {2, 30}, {5, 25}, {12, 29}, {19, 29}, {26, 25}, {28, 29}, {31, 30}, {31, 24}, {29, 22}, {31, 13}, {29, 7}, {20, 0}};
static const Vec2 POLY_PLASMA_PIRATE[] = {{2, 13}, {0, 23}, {2, 22}, {5, 27}, {8, 26}, {9, 18}, {13, 26}, {18, 26}, {22, 18}, {23, 26}, {26, 27}, {29, 22}, {31, 23}, {29, 13}, {23, 10}, {17, 3}, {13, 3}, {8, 10}};
static const Vec2 POLY_SHOCK_BEE[] = {{9, 1}, {12, 4}, {10, 6}, {11, 11}, {1, 9}, {0, 13}, {3, 16}, {3, 19}, {9, 17}, {10, 24}, {16, 29}, {21, 24}, {22, 17}, {28, 19}, {31, 10}, {27, 9}, {21, 12}, {21, 6}, {19, 4}, {22, 1}, {18, 4}, {13, 4}};
static const Vec2 POLY_SHREDDER_SWALLOW[] = {{15, 2}, {9, 12}, {7, 9}, {3, 13}, {0, 21}, {3, 18}, {4, 21}, {12, 17}, {14, 19}, {9, 27}, {13, 24}, {16, 28}, {18, 24}, {22, 27}, {17, 19}, {19, 17}, {27, 21}, {28, 18}, {31, 21}, {31, 18}, {24, 9}, {22, 12}};
static const Vec2 POLY_SPARK_FALCON[] = {{15, 1}, {11, 10}, {7, 9}, {0, 14}, {0, 19}, {3, 18}, {3, 22}, {13, 16}, {10, 27}, {13, 24}, {16, 29}, {18, 24}, {21, 27}, {17, 18}, {19, 16}, {21, 19}, {29, 22}, {28, 19}, {31, 19}, {31, 14}, {24, 9}, {20, 10}};
static const Vec2 POLY_WARP_WESP[] = {{13, 2}, {11, 7}, {3, 10}, {0, 14}, {5, 13}, {6, 16}, {11, 13}, {11, 20}, {16, 28}, {20, 20}, {19, 14}, {20, 13}, {25, 16}, {26, 13}, {31, 13}, {21, 7}, {18, 2}};
static bool enemy_create_astro_ant(Enemy *e)
{
    bool ok = enemy_apply_common(e, 20, 55.f, 0.8f, true);
    enemy_set_polygon(e, POLY_ASTRO_ANT, (int)(sizeof(POLY_ASTRO_ANT) / sizeof(POLY_ASTRO_ANT[0])));
    return ok;
}
static bool enemy_create_frigate(Enemy *e)
{
    bool ok = enemy_apply_common(e, 20, 25.f, 1.2f, true);
    enemy_set_polygon(e, POLY_FRIGATE, (int)(sizeof(POLY_FRIGATE) / sizeof(POLY_FRIGATE[0])));
    return ok;
}
static bool enemy_create_holo_shark(Enemy *e)
{
    bool ok = enemy_apply_common(e, 20, 90.f, 0.6f, true);
    enemy_set_polygon(e, POLY_HOLO_SHARK, (int)(sizeof(POLY_HOLO_SHARK) / sizeof(POLY_HOLO_SHARK[0])));
    return ok;
}
static bool enemy_create_nova_nomad(Enemy *e)
{
    bool ok = enemy_apply_common(e, 20, 70.f, 0.7f, true);
    enemy_set_polygon(e, POLY_NOVA_NOMAD, (int)(sizeof(POLY_NOVA_NOMAD) / sizeof(POLY_NOVA_NOMAD[0])));
    return ok;
}
static bool enemy_create_plasma_pirate(Enemy *e)
{
    bool ok = enemy_apply_common(e, 20, 60.f, 0.5f, true);
    enemy_set_polygon(e, POLY_PLASMA_PIRATE, (int)(sizeof(POLY_PLASMA_PIRATE) / sizeof(POLY_PLASMA_PIRATE[0])));
    return ok;
}
static bool enemy_create_shock_bee(Enemy *e)
{
    bool ok = enemy_apply_common(e, 20, 110.f, 0.4f, true);
    enemy_set_polygon(e, POLY_SHOCK_BEE, (int)(sizeof(POLY_SHOCK_BEE) / sizeof(POLY_SHOCK_BEE[0])));
    return ok;
}
static bool enemy_create_shredder_swallow(Enemy *e)
{
    bool ok = enemy_apply_common(e, 20, 50.f, 0.9f, true);
    enemy_set_polygon(e, POLY_SHREDDER_SWALLOW, (int)(sizeof(POLY_SHREDDER_SWALLOW) / sizeof(POLY_SHREDDER_SWALLOW[0])));
    return ok;
}
static bool enemy_create_spark_falcon(Enemy *e)
{
    bool ok = enemy_apply_common(e, 20, 95.f, 0.5f, true);
    enemy_set_polygon(e, POLY_SPARK_FALCON, (int)(sizeof(POLY_SPARK_FALCON) / sizeof(POLY_SPARK_FALCON[0])));
    return ok;
}
static bool enemy_create_warp_wesp(Enemy *e)
{
    bool ok = enemy_apply_common(e, 20, 80.f, 0.6f, true);
    enemy_set_polygon(e, POLY_WARP_WESP, (int)(sizeof(POLY_WARP_WESP) / sizeof(POLY_WARP_WESP[0])));
    return ok;
}

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
    const float PI_F = 3.14159265358979323846f;
    e->e.angle_offset = (PI_F * 0.5f);
    e->e.is_dynamic = true;
    e->e.collider.radius = sqrtf(e->e.size.x * e->e.size.x + e->e.size.y * e->e.size.y) * 0.5f + 1;
    e->e.collider.poly_count = 0; // will be set by specific create_* below
    e->e.collider.shape = COLLIDER_SHAPE_CIRCLE;
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
