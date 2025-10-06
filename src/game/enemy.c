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
#include "planet.h"
#include "enemy_types.h"
#include "entity_helpers.h"

#include "../core/types.h"

/**
 * @brief Simulate a projectile trajectory under planet gravity.
 *
 * Runs a fixed-step simulation of a projectile fired from |origin| with the
 * given |angle| and |strength|. Gravity from all planets in the world is
 * applied. The function returns the minimum distance encountered to
 * |player_pos| during the simulation. The simulation stops early when the
 * projectile hits a planet or leaves the allowed bounds.
 *
 * This helper is used by the enemy shot-search to evaluate candidate
 * trajectories.
 */
static float simulate_projectile_min_dist(struct World *w, Vec2 origin, Vec2 player_pos, float angle, float strength, float hit_radius) {
    if (!w)
        return 1e9f;
    Vec2 pos = origin;
    Vec2 vel = (Vec2){cosf(angle) * strength, sinf(angle) * strength
};
    float t = 0.f;
    /* Work in squared distances to avoid sqrt inside the loop */
    float min_dist2 = 1e30f;
    float hit_r2 = hit_radius * hit_radius;

    /* get OOB bounds from world */
    float min_x, min_y, max_x, max_y;
    world_get_proj_oob_bounds(w, &min_x, &min_y, &max_x, &max_y);

    /* simulate with fixed timestep */
    const float sim_dt = FIXED_DT;
    const float sim_max_time = SIM_MAX_PROJECTILE_TIME;

    while (t < sim_max_time) {

        /* gravity from planets */
        for (int i = 0; i < w->planet_count; ++i) {
            struct Planet *pl = w->planets[i];
            if (!pl)
                continue;
            float dx = pl->e.pos.x - pos.x;
            float dy = pl->e.pos.y - pos.y;
            float dist2 = dx * dx + dy * dy;
            /* extremely close singularity guard */
            if (dist2 < PROJ_EPSILON)
                return 1e9f;

            /* Hit planet: if projectile hits planet body, compute distance to player at this point
             * and if that distance is smaller than current best, update and abort (projectile destroyed).
             */
            if (dist2 <= pl->radius_sq) {
                float pdx = pos.x - player_pos.x;
                float pdy = pos.y - player_pos.y;
                float pd2 = pdx * pdx + pdy * pdy;
                if (pd2 < min_dist2)
                    min_dist2 = pd2;
                /* projectile destroyed by planet; stop simulation */
                return sqrtf(min_dist2);
            }

            /* apply gravity (inverse-square) */
            float inv_dist = 1.0f / sqrtf(dist2);
            float inv_dist2 = inv_dist * inv_dist;
            float accel = (pl->mass * PROJ_GRAVITY_CONST) * inv_dist2;
            vel.x += accel * dx * inv_dist * sim_dt;
            vel.y += accel * dy * inv_dist * sim_dt;
        }

        /* integrate */
        pos.x += vel.x * sim_dt;
        pos.y += vel.y * sim_dt;

        /* If the simulated projectile leaves the world bounds, disqualify (real projectiles are deactivated) */
        if (pos.x < min_x || pos.x > max_x || pos.y < min_y || pos.y > max_y)
            return 1e9f;

        /* distance^2 to player */
        float pdx = pos.x - player_pos.x;
        float pdy = pos.y - player_pos.y;
        float pd2 = pdx * pdx + pdy * pdy;

        if (pd2 < min_dist2)
            min_dist2 = pd2;

        if (min_dist2 <= hit_r2)
            return sqrtf(min_dist2);

        t += sim_dt;
    }
    return sqrtf(min_dist2);
}

/**
 * @brief Render callback for enemy entities.
 *
 * Uses the enemy's texture and world position to draw the sprite. Rotation
 * is applied according to the entity angle and angle_offset fields.
 */
static void enemy_render(Entity *e, struct Renderer *r) {
    if (!e || e->type != ENT_ENEMY)
        return;
    Enemy *en = (Enemy *)e;
    if (!en->alive)
        return;
    // Use sprite sheet index = enemy type
    SDL_Rect src = {0, 0, 0, 0};
    if (en->e.texture) {
        Services *svc = services_get();
        if (svc && svc->texman) {
            src = texman_sheet_src(svc->texman, TEX_ENEMIES_SHEET, (int)en->type);
        }
    }
    float w = en->e.size.x;
    float h = en->e.size.y;
    SDL_FRect dst = {en->e.pos.x - w * 0.5f, en->e.pos.y - h * 0.5f, w, h};
    // Convert angle to degrees; align similar to player (assume artwork faces up) -> apply angle_offset´
    float angle_deg = (en->e.angle + en->e.angle_offset) * (180.f / M_PI);
    renderer_draw_texture(r, en->e.texture, (src.w > 0 ? &src : NULL), &dst, angle_deg);
}
/**
 * @brief Per-frame entity update wrapper.
 *
 * Handles energy regeneration and dispatches to the AI update path.
 */
static void enemy_update(Entity *e, float dt) {
    Enemy *enemy = (Enemy *)e;
    if (!enemy || !enemy->alive)
        return;
    /* energy regeneration */
    entity_regenerate_energy(&enemy->energy, enemy->energy_regen_rate, enemy->energy_max, dt);
    /* AI update (shooting etc.) */
    /* Smooth rotation toward aim target if a shot is pending.
     * If aim_rotate_speed is zero, snap immediately and fire.
     */
    if (enemy->aim.queued) {
        float cur = enemy->e.angle;
        float target = enemy->aim.target_angle;
        float diff = atan2f(sinf(target - cur), cosf(target - cur));
        float max_step = enemy->aim.rotate_speed * dt;
        if (max_step <= 0.f || fabsf(diff) <= max_step) {
            enemy->e.angle = target;
        }
        else {
            enemy->e.angle += (diff > 0.f ? 1.f : -1.f) * max_step;
        }
        /* If aligned within tolerance, perform the pending shot here (synchronous). */
        float remain = atan2f(sinf(target - enemy->e.angle), cosf(target - enemy->e.angle));
        if (fabsf(remain) <= 0.05f) {
            bool fired = world_fire_projectile(enemy->world, enemy->shooter_index, (Entity *)enemy, enemy->aim.target_angle, enemy->aim.queued_strength);
            if (fired) {
                weapon_on_fired(enemy->weapon, enemy->world->time);
                enemy->energy -= (float)enemy->weapon->energy_cost;
                if (enemy->energy < 0.f)
                    enemy->energy = 0.f;
            }
            enemy->aim.queued = 0;
        }
        enemy->e.collider.poly_world_dirty = 1; // mark world poly dirty
    }
    enemy_ai_update(enemy, enemy->world, dt);
}

/**
 * @brief Collision / hit event handler for enemies.
 *
 * Applies damage from projectiles, handles scoring, death, and explosion
 * creation.
 */
static void enemy_on_hit(Entity *e, Entity *hitter) {
    if (!e || e->type != ENT_ENEMY)
        return;
    Enemy *en = (Enemy *)e;
    if (!en->alive)
        return;
    if (hitter && hitter->type == ENT_PROJECTILE) {
        struct Projectile *pr = (struct Projectile *)hitter;

        // Friendly fire filter: ignore projectiles fired by other enemies
        if (pr && pr->owner_kind == ENT_ENEMY)
            return;

        // Decrease health by projectile damage
        int dmg = 0;
        if (pr)
            dmg = projectile_get_damage(pr, 1.f);

        en->health -= dmg;

        // When health is zero or below
        if (en->health <= 0) {
            en->alive = false; // die

            // Increase world kills
            if (en->world) {
                en->world->kills++;
                en->world->score += pr->flight_time * 25;
                en->world->score += 100; // Add a score of 100 for each kill
            }
        }

        // Deactivate projectile
        if (pr)
            pr->active = false;

        // Add Explosion
        if (en->world) {
            Services *svc = services_get();
            if (svc && svc->texman) {
                int types = texman_explosion_type_count(svc->texman);
                if (types > 0) {
                    int type = en->explosion_type;
                    if (type < 0)
                        type = 0;
                    if (type >= types)
                        type = type % types; /* clamp/modulo to available range */
                    float x;
                    float y;
                    float scale;
                    // Use projectile position on hit where player don't die
                    if (en->alive) {
                        x = hitter->pos.x;
                        y = hitter->pos.y;
                        scale = 0.5f;
                    }
                    // Use enemy position if killed
                    else {
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

/**
 * @brief Entity factory stub for enemy VTable.
 *
 * The world spawns enemies via its own API; this generic factory is not
 * used and returns NULL.
 */
static Entity *enemy_create_entity(void *params) {
    (void)params; // new path shouldn't use generic factory directly
    return NULL;  // not used; enemies spawned via world_spawn_enemy
}
static void enemy_destroy_entity(Entity *e) {
    enemy_destroy((Enemy *)e);
}
/**
 * @brief Physical collision callback (contact) for enemies.
 *
 * Currently a placeholder — no damage or response applied here.
 */
static void enemy_on_collide(Entity *self, Entity *other) {
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
static SDL_Texture *enemy_base_texture(void) {
    Services *svc = services_get();
    if (!svc || !svc->texman)
        return NULL;
    return texman_get(svc->texman, TEX_ENEMIES_SHEET);
}

/**
 * @brief Progressive shot-search worker.
 *
 * Evaluates a small batch of candidate firing parameters each call and updates
 * the cached best solution in the enemy's ShotCache. The search is
 * deterministic and spreads work over multiple frames.
 */
static void enemy_update_shot_search(Enemy *en, World *w) {
    if (!en || !w || !w->player || !en->weapon)
        return;
    // If cache invalid or player/enemy moved, reset progressive search
    Vec2 player_pos = w->player->e.pos;
    Vec2 origin = en->e.pos;
    if (!en->shot.valid || en->shot.last_player_pos.x != player_pos.x || en->shot.last_player_pos.y != player_pos.y ||
        en->shot.last_enemy_pos.x != origin.x || en->shot.last_enemy_pos.y != origin.y) {
        en->shot.last_player_pos = player_pos;
        en->shot.last_enemy_pos = origin;
        en->shot.angle = atan2f(player_pos.y - origin.y, player_pos.x - origin.x);
        en->shot.strength = (en->weapon->min_speed + en->weapon->max_speed) * 0.5f;
        en->shot.best_dist = simulate_projectile_min_dist(w, origin, player_pos, en->shot.angle, en->shot.strength, ENEMY_SHOT_HIT_RADIUS);
        en->shot.valid = true;
        en->shot.search_done = 0;
        en->shot.improvements = 0;
        en->shot.ready = (en->shot.best_dist <= ENEMY_SHOT_HIT_RADIUS) ? true : false;
        // initialize deterministic grid search parameters
        en->shot.search_base_angle = en->shot.angle;
        en->shot.search_base_strength = en->shot.strength;
        en->shot.search_radius_ang = 0.6f; // ±0.6 rad initial sweep
        en->shot.search_radius_str = 0.5f; // ±50% strength
        en->shot.search_grid_n = 5;        // 5x5 grid
        en->shot.search_idx = 0;
}

    // progressive sampling: few iterations per call
    int to_run = en->shot.search_per_frame;
    int n = en->shot.search_grid_n > 1 ? en->shot.search_grid_n : 3;
    int cells = n * n;
    while (to_run-- > 0 && en->shot.search_done < en->shot.search_total && en->shot.best_dist > ENEMY_SHOT_HIT_RADIUS) {
        if (en->shot.search_idx >= cells) {
            // refinement step: move center to best found, shrink radius, reset grid
            en->shot.search_base_angle = en->shot.angle;
            en->shot.search_base_strength = en->shot.strength;
            en->shot.search_radius_ang *= 0.5f;
            en->shot.search_radius_str *= 0.5f;
            en->shot.search_idx = 0;
            // increase grid resolution every couple of refinements to allow finer search
            if (en->shot.search_grid_n < 11)
                en->shot.search_grid_n += 2;
            n = en->shot.search_grid_n;
            cells = n * n;
        }
        int idx = en->shot.search_idx++;
        int ix = idx % n;
        int iy = idx / n;
        // map ix,iy [0..n-1] to normalized range [-1,1]
        float nx = (n == 1) ? 0.f : ((float)ix / (float)(n - 1)) * 2.f - 1.f;
        float ny = (n == 1) ? 0.f : ((float)iy / (float)(n - 1)) * 2.f - 1.f;
        // candidate offsets
        float cand_angle = en->shot.search_base_angle + nx * en->shot.search_radius_ang;
        float strength_range = (en->weapon->max_speed - en->weapon->min_speed);
        float base_str = en->shot.search_base_strength;
        float cand_strength = base_str * (1.0f + ny * en->shot.search_radius_str);
        // clamp
        if (cand_strength < en->weapon->min_speed)
            cand_strength = en->weapon->min_speed;
        if (cand_strength > en->weapon->max_speed)
            cand_strength = en->weapon->max_speed;
        float cand_dist = simulate_projectile_min_dist(w, origin, player_pos, cand_angle, cand_strength, ENEMY_SHOT_HIT_RADIUS);
        en->shot.search_done++;
        if (cand_dist < en->shot.best_dist) {
            en->shot.best_dist = cand_dist;
            en->shot.angle = cand_angle;
            en->shot.strength = cand_strength;
            en->shot.improvements++;
            if (cand_dist <= ENEMY_SHOT_HIT_RADIUS) {
                en->shot.ready = true;
                /* keep candidate in shot cache; do not queue firing here. The
                 * decision to fire (and thus queue aiming) happens in
                 * enemy_try_shoot.
                 */
            }
        }
    }
}
/**
 * @brief Copy a local polygon into the enemy's collider.
 *
 * The provided points are stored in the local polygon array; the world-space
 * polygon will be computed later during collision preparation.
 */
static void enemy_set_polygon(Enemy *e, const Vec2 *pts, int count) {
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

/**
 * @brief Initialize an Enemy instance from a type definition.
 *
 * Populates runtime fields using the per-type data in `ENEMY_DEFS`.
 */
static bool enemy_init_from_def(Enemy *e, EnemyType t) {
    if (!e)
        return false;
    if ((int)t < 0 || t >= ENEMY_TYPE_COUNT)
        return false;
    const EnemyDef *d = &ENEMY_DEFS[t];
    /* Inline common initialization (previously enemy_apply_common) */
    e->health = d->hp;
    e->shoot_speed = d->shoot_speed;
    if (!e->weapon) {
        e->weapon = weapon_create_default();
        e->weapon->projectile_variant = 3;
    }
    e->energy_max = d->energy_max;
    e->energy = 0;
    e->energy_regen_rate = d->energy_regen_rate;
    e->shoot_chance = d->shoot_chance;
    /* initialize cached shot state */
    e->shot.last_player_pos = (Vec2){0.f, 0.f};
    e->shot.last_enemy_pos = (Vec2){0.f, 0.f};
    e->shot.angle = 0.f;
    e->shot.strength = 0.f;
    e->shot.best_dist = 1e9f;
    e->shot.valid = false;
    /* shot search defaults */
    e->shot.search_total = 100; /* total samples to find good trajectory */
    e->shot.search_done = 0;
    e->shot.search_per_frame = 3; /* do a few samples per update */
    e->shot.improvements = 0;
    e->shot.ready = false;
    /* difficulty defaults (mid) */
    e->explosion_type = 0; /* default; overwritten below by def */
    if (d->poly && d->poly_count > 0)
        enemy_set_polygon(e, d->poly, d->poly_count);
    e->ai.base_jitter_angle = d->base_jitter_angle;
    e->ai.base_jitter_strength = d->base_jitter_strength;
    e->explosion_type = d->explosion_type;
    e->e.size.x = d->size_x;
    e->e.size.y = d->size_y;
    e->e.collider.radius = sqrtf(e->e.size.x * e->e.size.x + e->e.size.y * e->e.size.y) * 0.5f + 1; /* keep +1 for margin */
    /* Aim smoothing defaults: read per-type rotate speed (radians/sec). 0 disables smoothing. */
    e->aim.target_angle = 0.f;
    e->aim.rotate_speed = d->aim_rotate_speed; /* per-type tuning */
    e->aim.queued = 0;
    e->aim.queued_strength = 0.f;
    return true;
}
/* individual enemy_create_* functions removed; initialization is table-driven via ENEMY_DEFS and enemy_init_from_def */

Enemy *enemy_create(World *world, EnemyType type, float x, float y, int shooter_index, char difficulty) {

    Enemy *e = calloc(1, sizeof(Enemy));
    if (!e)
        return NULL;
    e->world = world; // set world reference
    e->e.type = ENT_ENEMY;
    e->e.vt = &ENEMY_VT;
    e->type = type;
    e->e.pos.x = x;
    e->e.pos.y = y;
    e->e.angle = rng_rangef(&world->rng, 0, 2 * M_PI);
    e->shooter_index = shooter_index;
    e->alive = true;
    e->e.size.x = 32;
    e->e.size.y = 32; // default tile size; can tweak per type later
    e->e.texture = enemy_base_texture();
    e->e.angle_offset = (M_PI * 0.5f);
    e->e.is_dynamic = true;
    e->e.collider.radius = sqrtf(e->e.size.x * e->e.size.x + e->e.size.y * e->e.size.y) * 0.5f + 1;
    e->e.collider.poly_count = 0; // will be set by specific create_* below
    e->e.collider.shape = COLLIDER_SHAPE_CIRCLE;
    e->ai.difficulty = difficulty;
    /* initialize from table-driven defs */
    if (!enemy_init_from_def(e, type)) {
        enemy_destroy(e);
        return NULL;
    }
    return e;
}
void enemy_destroy(Enemy *en) {
    if (!en)
        return;
    if (en->weapon)
        weapon_destroy(en->weapon);
    free(en);
}
void enemy_ai_update(Enemy *en, struct World *w, float dt) {
    if (!en || !w)
        return;
    if (!en->weapon)
        return;
    enemy_update_shot_search(en, w);
    // Simple decision: roll per-update chance
    // difficulty influences fire probability (0..255) -> direct proportional
    float diff_factor = ((float)en->ai.difficulty) / 255.0f; // 0 => no shooting, 1 => full
    float effective_chance = en->shoot_chance * diff_factor;
    if (effective_chance > 1.f)
        effective_chance = 1.f;
    float roll = rng_rangef(&w->rng, 0.f, 1.f);
    if (roll < effective_chance) {
        enemy_try_shoot(en, w);
    }
}
bool enemy_try_shoot(Enemy *en, struct World *w) {
    if (!en || !w || !en->weapon)
        return false;
    // cooldown check
    if (!weapon_can_fire(en->weapon, w->time))
        return false;
    // energy check
    if (en->energy < (float)en->weapon->energy_cost)
        return false;
    // Only fire when a cached shot exists (background search in enemy_update_shot_search)
    if (!en->shot.valid)
        return false;
    if (en->aim.queued)
        return false; // already aiming, do not fire again

    Vec2 origin = en->e.pos;
    // apply jitter based on difficulty: higher difficulty -> less jitter
    float diff_factor = ((float)en->ai.difficulty) / 255.0f; // 0..1
    float jitter_scale = 1.0f - diff_factor;                 // 1 => max jitter at diff 0, 0 => no jitter
    float jitter_angle = 0;
    float jitter_strength = 1;
    bool jitter_type = (bool)rng_rangei(&w->rng, 0, 1);
    if (jitter_type)
        jitter_angle = rng_rangef(&w->rng, -en->ai.base_jitter_angle * jitter_scale, en->ai.base_jitter_angle * jitter_scale);
    else
        jitter_strength = 1.0f + rng_rangef(&w->rng, -en->ai.base_jitter_strength * jitter_scale, en->ai.base_jitter_strength * jitter_scale);
    float final_angle = en->shot.angle + jitter_angle;
    float final_strength = en->shot.strength * jitter_strength;
    /* Set aim target and mark pending; actual firing will occur in enemy_update
     * when the enemy has rotated into alignment. This keeps rendering/rotation
     * smooth and avoids sudden angle snaps.
     */
    en->aim.target_angle = final_angle;
    en->aim.queued_strength = final_strength;
    en->aim.queued = 1;
    return true; /* indicate request accepted; actual shot may occur shortly in update() */
}
