#include "player.h"
#include <stdlib.h>
#include <math.h>
#include "projectile.h"
// Local constant for PI to avoid relying on platform-specific M_PI macro
static const float PI_F = 3.14159265358979323846f;
#include "../core/log.h"
#include "../services/renderer.h"
#include "../services/services.h"
#include "../services/texture_manager.h"
#include "world.h" // for world_fire_projectile
#include "weapon.h"
#include "entity_helpers.h"

// Forward declaration
static void player_trigger_death_effect(Player *p);

static void player_apply_damage(Player *p, int damage)
{

    if (!p || !p->alive)
        return;

    p->health -= damage;
    if (p->health <= 0)
    {
        p->alive = false;
        player_trigger_death_effect(p);
    }
}

static void player_trigger_death_effect(Player *p)
{

    if (!p || p->death_explosion_triggered)
        return;
    if (!p->world)
        return;
    Services *svc = services_get();
    if (!svc || !svc->texman)
        return;
    int types = texman_explosion_type_count(svc->texman);
    int type = 0;
    if (types > 0)
    {
        if (types >= 3)
            type = 2;
        else
            type = types - 1;
    }
    float scale = 1.4f;
    world_add_explosion(p->world, type, p->e.pos.x, p->e.pos.y, scale);
    p->death_explosion_triggered = true;
}

static void player_render(Entity *e, struct Renderer *r)
{

    Player *p = (Player *)e;
    if (!p || !p->alive)
        return;
    SDL_FRect dst = {p->e.pos.x - p->e.size.x * 0.5f, p->e.pos.y - p->e.size.y * 0.5f, p->e.size.x, p->e.size.y

    };
    float angle_deg = (p->e.angle + p->e.angle_offset) * (180.0f / PI_F);
    renderer_draw_texture(r, p->e.texture, NULL, &dst, angle_deg);
}
static void player_update(Entity *e, float dt)
{
    Player *p = (Player *)e;
    if (!p)
        return;
    if (!p->alive)
    {
        player_trigger_death_effect(p);
        return;
    }
    // Track original position to detect movement for polygon dirty flag
    float oldx = p->e.pos.x;
    float oldy = p->e.pos.y;
    // reset per-frame boost damage flag
    p->boost_damage_flag = 0;
    // energy regeneration (moved from weapon)
    entity_regenerate_energy(&p->energy, p->energy_regen_rate, p->energy_max, dt);
#ifdef PLATFORM_PC
    // movement (WASD) for Debug
    float speed = 120.0f * dt;
    if (p->input.move_left)
        p->e.pos.x -= speed;
    if (p->input.move_right)
        p->e.pos.x += speed;
    if (p->input.move_up)
        p->e.pos.y -= speed;
    if (p->input.move_down)
        p->e.pos.y += speed;
#endif
    // adjust projectile speed via step events (auto-repeat)
    if (p->weapon)
    {
        float base_step_small = 5.f;   // units per step with small modifier
        float base_step_normal = 10.f; // default
        float base_step_large = 30.f;  // with large modifier
        float step = p->input.mod_large ? base_step_large : (p->input.mod_small ? base_step_small : base_step_normal);
        if (p->input.speed_up_step)
            p->current_shot_speed += step;
        if (p->input.speed_down_step)
            p->current_shot_speed -= step;
        if (p->current_shot_speed < p->weapon->min_speed)
            p->current_shot_speed = p->weapon->min_speed;
        if (p->current_shot_speed > p->weapon->max_speed)
            p->current_shot_speed = p->weapon->max_speed;
    }

    // rotation: if analog stick active use its angle, else apply step events
    if (p->input.stick_active)
    {
        // Adjust stick angle by +90deg so that right on stick yields facing right on screen.
        // (Observed: previous mapping made right=up, meaning we were effectively -90deg off.)
        float adjusted = p->input.stick_angle + (PI_F * 0.5f);
        float target = adjusted - p->e.angle_offset;
        // Normalize to [-PI, PI] for numerical stability
        if (target > PI_F)
            target -= 2.f * PI_F;
        if (target < -PI_F)
            target += 2.f * PI_F;
        p->e.angle = target;
        p->e.collider.poly_world_dirty = 1; // mark world poly dirty
    }
    else
    {
        float base_step_small = 0.01f;
        float base_step_normal = 0.02f;
        float base_step_large = 0.05f;
        float step = p->input.mod_large ? base_step_large : (p->input.mod_small ? base_step_small : base_step_normal);
        if (p->input.turn_left_step)
        {
            p->e.angle -= step;
            p->e.collider.poly_world_dirty = 1; // mark world poly dirty
        }
        if (p->input.turn_right_step)
        {
            p->e.angle += step;
            p->e.collider.poly_world_dirty = 1; // mark world poly dirty
        }
    }
    // Boost
    int raw_boost = p->input.boost ? 1 : 0;
    int edge = raw_boost && !p->prev_boost;
    // boost_cost: voller Abzug beim Initial-Press (edge) + gleicher Wert pro Sekunde während Halten.
    // Wenn beim Edge nicht genug Energie für boost_cost vorhanden: kein Boost (auch kein Halten-Effekt).
    // Während Halten nur weiter beschleunigen/verstetigen solange genug Energie für die aktuelle Frame-Drain vorhanden ist.
    float dx = cosf(p->e.angle);
    float dy = sinf(p->e.angle);
    if (edge)
    {
        if (p->energy >= (0.2 * p->energy_max))
        {
            if (p->energy < 0.f)
                p->energy = 0.f;
            p->boosting_session = 1;
            // kleiner Sofort-Impuls (10% Zielgeschwindigkeit)
            float tap_speed = p->boost_strength * 0.10f;
            p->e.vel.x += dx * tap_speed;
            p->e.vel.y += dy * tap_speed;
        }
        else
        {
            p->boosting_session = 0; // kein Boost starten
        }
    }
    // Beenden der Session wenn Taste losgelassen
    if (!raw_boost)
        p->boosting_session = 0;
    // Laufender Boost: nur wenn Session aktiv und noch Energie vorhanden
    if (p->boosting_session && raw_boost)
    {
        // Per-Frame Drain berechnen
        float drain = (float)p->boost_cost * dt; // cost per second -> dt Anteil
        if (p->energy >= drain && drain > 0.f)
        {
            p->energy -= drain;
            if (p->energy < 0.f)
                p->energy = 0.f;
            // Vorwärtsgeschwindigkeit in Richtung Ziel annähern
            float vx = p->e.vel.x;
            float vy = p->e.vel.y;
            float proj = vx * dx + vy * dy; // aktuelle Vorwärtskomponente
            float target = p->boost_strength;
            if (target < 0.f)
                target = 0.f;
            // sanfte Annäherung (Beschleunigung proportional zum Ziel)
            float accel = target * 4.f; // ~0.3s auf 95%
            if (proj < target)
            {
                float add = accel * dt;
                if (proj + add > target)
                    add = target - proj;
                if (add > 0.f)
                {
                    p->e.vel.x += dx * add;
                    p->e.vel.y += dy * add;
                }
            }
        }
        else
        {
            // Nicht genug Energie für weiteren Frame -> Boost endet sofort
            p->boosting_session = 0;
        }
    }
    p->prev_boost = raw_boost;

    // Apply velocity to position

    float new_x = p->e.pos.x + p->e.vel.x * dt;
    float new_y = p->e.pos.y + p->e.vel.y * dt;

    if (new_x >= p->e.size.x * 0.25f && new_x <= DISPLAY_W - p->e.size.x * 0.25f && new_y >= p->e.size.y * 0.25f && new_y <= DISPLAY_H - p->e.size.y * 0.25f)
    {
        p->e.pos.x = new_x;
        p->e.pos.y = new_y;
    }

    // Mark for rebuild AFTER movement for next collision pass
    if (p->e.pos.x != oldx || p->e.pos.y != oldy)
        p->e.collider.poly_world_dirty = 1;
    // Friction (exponential-ish linear damp)
    float friction_k = 4.0f; // tweakable
    p->e.vel.x -= p->e.vel.x * friction_k * dt;
    p->e.vel.y -= p->e.vel.y * friction_k * dt;
    if (fabsf(p->e.vel.x) < 0.5f)
        p->e.vel.x = 0.f;
    if (fabsf(p->e.vel.y) < 0.5f)
        p->e.vel.y = 0.f;

    // shooting disabled in same frame as boost edge? keep it simple: still allow
    if (p->world && p->input.fire)
        player_shoot(p, p->world, -1.f);
}
static void player_destroy_entity(Entity *e)
{
    player_destroy((Player *)e);
}
static void player_on_hit_entity(Entity *e, Entity *hitter)
{
    if (!e || e->type != ENT_PLAYER)
        return;
    Player *p = (Player *)e;
    if (hitter && hitter->type == ENT_PROJECTILE)
    {
        struct Projectile *pr = (struct Projectile *)hitter; // forward declared via projectile.h through inclusion chain
        int dmg = 1;
        if (pr)
            dmg = projectile_get_damage(pr, 0.5f);
        // Friendly fire filter: ignore projectiles fired by players
        if (pr && pr->owner_kind == ENT_PLAYER)
            return; // do not deactivate friendly projectile
        player_apply_damage(p, dmg);
        if (pr)
            pr->active = false; // deactivate projectile only on valid hit
        /* Add a small explosion effect for player hits. Use explosion type 1
         * if available; fall back to 0 if the texture manager provides fewer
         * types. Position at the projectile hit while player still alive,
         * otherwise at the player's position. Scale mirrors enemy logic. */
        if (p->world && p->alive)
        {
            Services *svc = services_get();
            if (svc && svc->texman)
            {
                int types = texman_explosion_type_count(svc->texman);
                int type = 1;
                if (types <= 1)
                    type = 0;
                else if (type >= types)
                    type = type % types;
                float x = hitter ? hitter->pos.x : p->e.pos.x;
                float y = hitter ? hitter->pos.y : p->e.pos.y;
                float scale = 0.5f;
                world_add_explosion(p->world, type, x, y, scale);
            }
        }
    }
}

static Entity *player_create_entity(void *params)
{

    SDL_Texture *tex = (SDL_Texture *)params;
    return (Entity *)player_create(tex);
}

static void player_on_collide_entity(Entity *self, Entity *other)
{

    if (!self || self->type != ENT_PLAYER)
        return;
    Player *p = (Player *)self;
    if (!p->alive)
        return;
    if (!other)
        return;
    if (other->type == ENT_PLANET || other->type == ENT_ENEMY)
    {
        float vx = p->e.vel.x;
        float vy = p->e.vel.y;
        float speed = sqrtf(vx * vx + vy * vy);
        // Separation normal: from other center to player center (avoid using velocity dir)
        float dx = p->e.pos.x - other->pos.x;
        float dy = p->e.pos.y - other->pos.y;
        float dist = sqrtf(dx * dx + dy * dy);
        float nx, ny;
        if (dist > 1e-4f)
        {
            nx = dx / dist;
            ny = dy / dist;
        }
        else if (speed > 1e-4f)
        {
            nx = vx / speed;
            ny = vy / speed;
        }
        else
        {
            nx = 1.f;
            ny = 0.f;
        }
        // Approx penetration using circles (player radius + other radius - dist) if other has radius
        float pr = p->e.collider.radius;
        float orad = other->collider.radius > 0.f ? other->collider.radius : fmaxf(other->size.x, other->size.y) * 0.5f;
        float penetration = (pr + orad) - dist; // can be negative (no overlap in circle approximation)
        if (penetration > 0.f)
        {
            // Only push out along n; limit push to avoid overshoot
            float push = penetration < 8.f ? penetration : 8.f;
            // If velocity points outward already, reduce push (prevents 'stick')
            float vdotn = vx * nx + vy * ny;
            if (vdotn > 0.f)
                push *= 0.3f;
            p->e.pos.x += nx * push;
            p->e.pos.y += ny * push;
            p->e.collider.poly_world_dirty = 1; // mark moved
            // Velocity response: only modify inward component
            if (vdotn < 0.f)
            {
                float restitution = 0.25f;                  // damped bounce
                float remove = (1.f + restitution) * vdotn; // vdotn negative
                p->e.vel.x -= nx * remove;
                p->e.vel.y -= ny * remove;
            }
            // Damage only if inward impact speed large enough (use inward component NOT total speed)
            const float impact_threshold = 60.f; // tune: lower so damage actually occurs
            float inward_speed = -vdotn;         // positive if moving into object
            if (p->boost_damage_flag == 0 && inward_speed > impact_threshold)
            {
                int dmg = (int)(inward_speed * 0.02f) + 1;
                if (dmg < 1)
                    dmg = 1;
                player_apply_damage(p, dmg);
                p->boost_damage_flag = 1; // only set when damage applied
            }
        }
    }
}
static const EntityVTable PLAYER_VT = {player_create_entity, player_destroy_entity, player_update, player_render, player_on_hit_entity, player_on_collide_entity};

Player *player_create(SDL_Texture *tex)
{

    Player *p = calloc(1, sizeof(Player));
    if (!p)
    {
        LOG_ERROR("player", "Failed to allocate Player");
        return NULL;
    }
    p->e.texture = tex;
    p->alive = true;
    p->e.size = (Vec2){PLAYER_WIDTH, PLAYER_HEIGHT};
    p->e.vt = &PLAYER_VT;
    p->e.is_dynamic = true;
    p->e.collider.radius = sqrtf(p->e.size.x * p->e.size.x + p->e.size.y * p->e.size.y) * 0.5f + 1;
    // (19) Basic polygon (from assets/images/player_polygon/polygons.json)
    static const Vec2 poly_pts[] = {{17, 1}, {14, 5}, {14, 10}, {3, 21}, {3, 27}, {13, 20}, {14, 21}, {10, 26}, {10, 30}, {14, 27}, {16, 29}, {20, 27}, {24, 30}, {24, 26}, {20, 23}, {21, 20}, {31, 27}, {31, 21}, {20, 10}, {20, 5}}; // raw pixel coords
    int poly_total = (int)(sizeof(poly_pts) / sizeof(poly_pts[0]));
    p->e.collider.poly_count = (poly_total > 30) ? 30 : poly_total;
    // Lokale Polygonpunkte (Pixelkoordinaten), dienen für präzisere Kollision.
    for (int i = 0; i < p->e.collider.poly_count; i++)
    {
        p->e.collider.poly_local[i] = poly_pts[i];
    }
    p->e.collider.poly_world_dirty = 1; // will build world copy
    p->e.collider.shape = COLLIDER_SHAPE_POLY;
    p->e.type = ENT_PLAYER;
    p->e.angle_offset = (PI_F * 0.5f); // sprite artwork points up
    p->shooter_index = -1;
    p->world = NULL;
    p->rotation_speed = 2.5f;
    p->health = 100;
    p->max_health = 100;
    p->weapon = weapon_create_default();
    p->current_shot_speed = p->weapon ? (p->weapon->min_speed + p->weapon->max_speed) * 0.5f : 300.f;
    p->fire_was_down = false;
    // energy system init (mirroring old weapon defaults)
    p->energy_max = 1000;
    p->energy = (float)p->energy_max;
    p->energy_regen_rate = 100.f;
    // boost defaults
    p->boost_strength = 100.f;
    p->boost_cost = 400;
    p->boost_damage_flag = 0;
    p->death_explosion_triggered = false;
    // Initialize boost state tracking fields (moved from static variables)
    p->prev_boost = 0;
    p->boosting_session = 0;
    return p;
}
void player_destroy(Player *p)
{
    if (!p)
        return;
    if (p->weapon)
        weapon_destroy(p->weapon);
    free(p);
}
void player_fly(Player *p, const InputState *in, float dt)
{
    // deprecated: logic moved to player_update
    (void)p;
    (void)in;
    (void)dt;
}
bool player_shoot(Player *p, struct World *w, float strength)
{
    if (!p || !w)
        return false;
    if (p->shooter_index < 0)
        return false;
    if (!p->weapon)
        return false;
    // Cooldown check only
    if (!weapon_can_fire(p->weapon, w->time))
        return false;
    // Energy pool check
    if (p->energy < (float)p->weapon->energy_cost)
        return false;
    if (strength <= 0.f)
        strength = p->current_shot_speed;
    bool ok = world_fire_projectile(w, p->shooter_index, (Entity *)p, p->e.angle, strength);
    if (ok)
    {
        weapon_on_fired(p->weapon, w->time); // only updates last_fire_time now
        p->energy -= (float)p->weapon->energy_cost;
        if (p->energy < 0.f)
            p->energy = 0.f;
    }
    return ok;
}
void player_on_kill(Player *p)
{
    (void)p;
}
OBB player_collider(const Player *p)
{
    return (OBB){{p->e.pos.x, p->e.pos.y}, {p->e.size.x, p->e.size.y}, p->e.angle};
}
void player_set_input(Player *p, const InputState *in)
{
    if (p && in)
        p->input = *in;
}
