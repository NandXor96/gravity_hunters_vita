#include "world.h"
#include <stdlib.h>
#include <math.h>
#include "player.h"
#include "enemy.h"
#include "planet.h"
#include "projectile.h"
#include "explosion.h"
#include "hud.h"
#include "collision.h"
#include "../services/renderer.h"
#include "../services/texture_manager.h"
#include "../services/services.h"
#include "../core/rand.h"

static float usable_bottom;

/* Generic placement helper: places a point at least min_dist away from entities.
 * For planets we need variable exclusion radii (planet radius * factor). We pass dynamic radius via param.
 */
static Vec2 world_place_helper(World *w, float min_dist_planets, float min_dist_player, float min_dist_enemies, float size, int max_attempts)
{
    Vec2 v = {-1.f, -1.f};
    float half_size = 0.5 * size;
    if (!w)
        return v;
    Services *svc = services_get();
    for (int attempt = 0; attempt < max_attempts; ++attempt)
    {
        float x = rng_rangef(&w->rng, 40.f + half_size, svc->display_w - 40.f - half_size);
        float y = rng_rangef(&w->rng, 40.f + half_size, usable_bottom - 40.f - half_size);
        bool overlaps = false;
        // planets
        for (int j = 0; j < w->planet_count && !overlaps; ++j)
        {
            Planet *p = w->planets[j];
            if (!p)
                continue;
            float dx = x - p->e.pos.x, dy = y - p->e.pos.y;
            float d = sqrtf(dx * dx + dy * dy);
            float need = p->radius + min_dist_planets + half_size;
            if (d < need)
                overlaps = true;
        }
        if (overlaps)
            continue;
        // player
        if (w->player)
        {
            float dx = x - w->player->e.pos.x, dy = y - w->player->e.pos.y;
            float player_max = fmaxf(w->player->e.size.x, w->player->e.size.y);
            if (sqrtf(dx * dx + dy * dy) < (player_max * 0.5f + min_dist_player + half_size))
                overlaps = true;
        }
        if (overlaps)
            continue;
        // enemies
        for (int j = 0; j < w->enemy_count && !overlaps; ++j)
        {
            Enemy *en = w->enemies[j];
            if (!en)
                continue;
            float dx = x - en->e.pos.x, dy = y - en->e.pos.y;
            float enemy_max = fmaxf(en->e.size.x, en->e.size.y);
            if (sqrtf(dx * dx + dy * dy) < (enemy_max * 0.5f + min_dist_enemies + half_size))
                overlaps = true;
        }
        if (overlaps)
            continue;
        v.x = x;
        v.y = y;
        break;
    }
    return v;
}

/**
 * Create a new world.
 *
 * @param svc The services instance.
 * @param seed The random seed.
 * @param max_planet_area The maximum area for planets.
 * @return A pointer to the newly created world.
 */
World *world_create(struct Services *svc, u32 seed, unsigned int max_planet_area)
{
    World *w = calloc(1, sizeof(World));
    w->svc = svc;
    w->seed = seed;
    w->proj_oob_margin_factor = 0.2f; // default as requested
    projectile_system_init(&w->projsys, svc->texman);
    w->explosion_count = 0;

    rng_seed(&w->rng, seed);

    // Add Planets
    float display_area = svc->display_w * svc->display_h;
    float planets_area = 0;
    uint8_t tex_idx = rng_rangei(&w->rng, 0, 15);

    // Reserve bottom area for HUD (must match HUD_BG_HEIGHT in hud.c). If HUD changes, update here or refactor into shared header.
    /* Reserve bottom area for HUD (use centralized HUD_BG_HEIGHT to keep layout consistent) */
    usable_bottom = svc->display_h - HUD_BG_HEIGHT;
    if (usable_bottom < 0.f)
        usable_bottom = 0.f;

    // Planets placement
    int attempts = 0;
    float min_radius = 40.f;
    float max_radius = 80.f;
    while (planets_area < display_area * max_planet_area / 100 && attempts < 100)
    {
        float radius = rng_rangef(&w->rng, 40.0f, max_radius);
        Vec2 pos = world_place_helper(w, 1.2 * radius, 0, 0, 2 * radius, 500);
        if (pos.x < 0.f)
        {
            printf("Couldn't place planet with radius %f\n", radius);
            printf("%f < %f\n", planets_area, display_area * max_planet_area / 100);
            if(max_radius > min_radius)
                max_radius = radius - 2;
            attempts++;
            continue;
        }
        float mass = M_PI * radius * radius;
        SDL_Texture *tex = texman_get(w->svc->texman, TEX_PLANETS_SHEET);
        SDL_Rect src = texman_sheet_src(w->svc->texman, TEX_PLANETS_SHEET, (tex_idx++) % 16);
        world_add_planet(w, pos.x, pos.y, radius, mass, tex);
        if (w->planet_count > 0)
        {
            Planet *np = w->planets[w->planet_count - 1];
            np->src = src;
        }
        planets_area += mass;
    }

    printf("Planets placed: %d\n", w->planet_count);
    printf("%f < %f\n", planets_area, display_area * max_planet_area / 100);

    // Player placement
    {
        Vec2 ppos = world_place_helper(w, 32.f, 0, 0, 32.f, 300);
        if (ppos.x < 0.f)
            ppos = (Vec2){svc->display_w * 0.5f, usable_bottom * 0.5f};
        world_add_player(w, ppos.x, ppos.y);
        if (!w->hud)
            w->hud = hud_create(svc, w->player);
    }

    // Enemies placement (3 initial)
    for (int i = 0; i < 3; ++i)
    {
        Vec2 epos = world_place_helper(w, 32.f, 150.f, 100.f, 32.f, 400);
        if (epos.x < 0.f)
            break;
        world_spawn_enemy(w, rng_rangei(&w->rng, 0, ENEMY_TYPE_COUNT - 1), epos.x, epos.y);
    }

    // Time limit defaults
    w->time_limit = -1.f; // infinite by default
    w->time_over_triggered = 0;
    w->on_time_over = NULL;
    w->on_time_over_user = NULL;

    return w;
}
void world_destroy(World *w)
{
    if (!w)
        return;
    /* Destroy explosions (if we add storage later) */
    projectile_system_shutdown(&w->projsys);
    {
        for (int i = 0; i < w->planet_count; i++)
            if (w->planets[i])
                planet_destroy(w->planets[i]);
        free(w->planets);
    }
    if (w->player)
        player_destroy(w->player);
    for (int i = 0; i < w->explosion_count; i++)
        if (w->explosions[i])
            explosion_destroy(w->explosions[i]);
    if (w->hud)
        hud_destroy(w->hud);
    free(w);
}
void world_update(World *w, float dt)
{
    if (!w)
        return;
    w->time += dt;
    // Time limit check
    if (w->time_limit >= 0.f && !w->time_over_triggered)
    {
        if (w->time >= w->time_limit)
        {
            w->time_over_triggered = 1;
            if (w->on_time_over)
                w->on_time_over(w, w->on_time_over_user);
        }
    }

    // Update dynamic entities

    // Player
    if (w->player && w->player->e.vt && w->player->e.vt->update)
        w->player->e.vt->update((Entity *)w->player, dt);

    // Enemies update & deferred removal compaction
    for (int i = 0; i < w->enemy_count; ++i)
    {
        Enemy *en = w->enemies[i];
        if (!en)
            continue;
        if (!en->alive && en->e.vt && en->e.vt->destroy)
        {
            // unregister shooter before destroying enemy so projectile_system can reuse the slot
            if (en->shooter_index >= 0)
                projectile_system_unregister_shooter(&w->projsys, en->shooter_index);
            en->e.vt->destroy((Entity *)en);
            w->enemies[i] = NULL; // mark for removal
            continue;
        }
        if (en->e.vt && en->e.vt->update)
            en->e.vt->update((Entity *)en, dt);
    }
    // Compact enemy list (single pass)
    {
        int write = 0;
        for (int i = 0; i < w->enemy_count; ++i)
        {
            Enemy *en = w->enemies[i];
            if (!en)
                continue;
            en->id = write; // update id to new index
            w->enemies[write++] = en;
        }
        w->enemy_count = write;
        for (int i = write; i < MAX_ENEMIES; ++i)
            w->enemies[i] = NULL;
    }
    // Spawn new enemies
    for (int i = w->enemy_count; i < 3; ++i)
    {
        Vec2 epos = world_place_helper(w, 32.f, 150.f, 100.f, 32.f, 400);
        if (epos.x < 0.f)
            break;
        world_spawn_enemy(w, rng_rangei(&w->rng, 0, ENEMY_TYPE_COUNT - 1), epos.x, epos.y);
    }

    // Planets
    for (int i = 0; i < w->planet_count; ++i)
    {
        Planet *pl = w->planets[i];
        if (pl && pl->e.vt && pl->e.vt->update)
            pl->e.vt->update((Entity *)pl, dt);
    }

    // Explosions aktualisieren und ggfs. entfernen
    int write = 0;
    for (int i = 0; i < w->explosion_count; i++)
    {
        Explosion *ex = w->explosions[i];
        if (!ex)
            continue;
        if (ex->e.vt && ex->e.vt->update)
            ex->e.vt->update((Entity *)ex, dt);
        if (ex->active)
            w->explosions[write++] = ex;
        else
            explosion_destroy(ex);
    }
    w->explosion_count = write;
    // gravity sources added once at planet creation; no per-frame rebuild
    projectile_system_update(&w->projsys, w->planets, w->planet_count, w->player, w->enemies, w->enemy_count, w->proj_oob_margin_factor, w->svc->display_w, w->svc->display_h, dt, w->time);
    // Run generic collision system (Phase1: player/enemy/planet)
    collision_run(w, dt);
    if (w->hud)
        hud_update(w->hud, w, dt);
}
static void world_render_entities(World *w, struct Renderer *r)
{
    // Planets
    for (int i = 0; i < w->planet_count; i++)
    {
        Planet *p = w->planets[i];
        if (!p)
            continue;
        if (p->e.vt && p->e.vt->render)
            p->e.vt->render((Entity *)p, r);
    }

    // Projectiles
    projectile_system_render(&w->projsys, r);

    // Player
    if (w->player && w->player->e.vt && w->player->e.vt->render)
        w->player->e.vt->render((Entity *)w->player, r);

    // Enemies
    for (int i = 0; i < w->enemy_count; i++)
    {
        Enemy *en = w->enemies[i];
        if (!en)
            continue;
        if (en->e.vt && en->e.vt->render)
            en->e.vt->render((Entity *)en, r);
    }

    // Explosions
    for (int i = 0; i < w->explosion_count; i++)
    {
        Explosion *ex = w->explosions[i];
        if (!ex)
            continue;
        if (ex->e.vt && ex->e.vt->render)
            ex->e.vt->render((Entity *)ex, r);
    }
}
void world_render(World *w, struct Renderer *r)
{
    if (!w)
        return;
    world_render_entities(w, r);
    if (w->hud)
        hud_render(w->hud, r);
    // optional collider debug draw (compile-time)
    #ifdef DEBUG_COLLISION
    collision_debug_draw(w, r);
    #endif
}
bool world_add_planet(World *w, float x, float y, float radius, float mass, SDL_Texture *tex)
{
    if (!w)
        return false;
    // default src (full texture) until assigned by caller
    SDL_Rect dummy = {0, 0, 0, 0};
    Planet *p = planet_create(x, y, radius, mass, tex, dummy);
    if (!p)
        return false;
    p->world = w; /* back-reference for spawning effects */
    Planet **new_arr = realloc(w->planets, sizeof(Planet *) * (w->planet_count + 1));
    if (!new_arr)
    {
        planet_destroy(p);
        return false;
    }
    w->planets = new_arr;
    w->planets[w->planet_count++] = p;
    return true;
}
bool world_add_player(World *w, float x, float y)
{
    SDL_Texture *tex = texman_get(w->svc->texman, TEX_PLAYER);
    if (!w || !tex)
        return false;
    if (w->player)
        return false; // Player already exists
    w->player = player_create(tex);
    if (!w->player)
        return false;
    w->player->e.pos.x = x;
    w->player->e.pos.y = y;
    int si = world_register_shooter(w);
    w->player->shooter_index = si;
    w->player->world = w;
    return true;
}

bool world_spawn_enemy(World *w, int kind, float x, float y)
{
    if (!w)
        return false;
    if (w->enemy_count >= MAX_ENEMIES)
        return false;
    int shooter_index = world_register_shooter(w);
    int id = w->enemy_count;
    float angle = 0.0f;
    Enemy *en = enemy_create(w, (EnemyType)kind, x, y, angle, id, shooter_index);
    if (!en)
        return false;
    w->enemies[w->enemy_count++] = en;
    return true;
}
int world_register_shooter(World *w) { return projectile_system_register_shooter(&w->projsys); }
bool world_fire_projectile(World *w, int shooter_index, Entity *owner, float angle, float strength)
{
    if (!w)
        return false;
    return projectile_system_fire(&w->projsys, w->time, shooter_index, owner, angle, strength);
}
bool world_add_explosion(World *w, int type, float x, float y, float scale)
{
    if (!w)
        return false;
    if (w->explosion_count >= MAX_EXPLOSIONS)
        return false;
    Explosion *ex = explosion_create(type, x, y, scale);
    if (!ex)
        return false;
    w->explosions[w->explosion_count++] = ex;
    return true;
}
void world_set_time_limit(World *w, float seconds)
{
    if (!w)
        return;
    w->time_limit = seconds;
    w->time_over_triggered = 0;
}

void world_get_proj_oob_bounds(World *w, float *out_min_x, float *out_min_y, float *out_max_x, float *out_max_y)
{
    if (!w || !out_min_x || !out_min_y || !out_max_x || !out_max_y)
        return;
    int display_w = w->svc ? w->svc->display_w : 960;
    int display_h = w->svc ? w->svc->display_h : 544;
    float margin_x = w->proj_oob_margin_factor * (float)display_w;
    float margin_y = w->proj_oob_margin_factor * (float)display_h;
    *out_min_x = -margin_x;
    *out_min_y = -margin_y;
    *out_max_x = (float)display_w + margin_x;
    *out_max_y = (float)display_h + margin_y;
}
