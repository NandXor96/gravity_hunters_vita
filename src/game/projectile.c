#include "projectile.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../services/services.h"
#include "../services/renderer.h"
#include "../core/types.h"
#include "planet.h"
#include "../services/texture_manager.h"
#include "player.h"
#include "enemy.h"
#include "../core/log.h"

static Entity *projectile_create_entity(void *params) {

    (void)params;
    return NULL;

}
static void projectile_destroy_entity(Entity *e) {
    if (!e)
        return;
    projectile_destroy((Projectile *)e);
}
static void projectile_update_entity(Entity *e, float dt) {
    // Will be overridden via extended update that needs world context (set externally)
    (void)e;
    (void)dt;
}
// --- Trail Helper -------------------------------------------------------
static void trail_init(Trail *t) {
    memset(t, 0, sizeof(*t));
    t->alpha_head = 0.95f;
    t->alpha_tail = 0.05f; // stärkerer Kontrast für sichtbare Transparenz
    t->whiten_factor = 0.95f;
    t->core_half_width = 1.3f; // gewünschte Halbbreite (ca. 3 Pixel Gesamtdicke)
    t->glow_offset = 1.0f;
    t->glow_alpha_factor = 0.25f;
}
static void trail_add_point(Trail *t, Vec2 p) {
    if (t->length < TRAIL_LEN)
        t->length++;
    t->points[t->head] = p;
    t->head = (t->head + 1) % TRAIL_LEN;
}
static void trail_render(const Trail *t, Uint8 base_r, Uint8 base_g, Uint8 base_b, struct Renderer *r) {
    // Mehrere Segmente für Krümmung + Farbverlauf, aber nur eine Linie pro Segment (niedrige Draw Calls)
    int n = t->length;
    if (n <= 1)
        return;
    int headIdx = (t->head - 1 + TRAIL_LEN) % TRAIL_LEN;
    int segments = n - 1; // Anzahl Linien
    for (int s = 0; s < segments; ++s) {
        int newer = (headIdx - s + TRAIL_LEN) % TRAIL_LEN;     // näher am Kopf
        int older = (headIdx - s - 1 + TRAIL_LEN) % TRAIL_LEN; // näher am Schwanz
        Vec2 a = t->points[newer];
        Vec2 b = t->points[older];
        float denom = (float)(segments - 1);
        float tnorm = (segments <= 1) ? 0.f : (float)s / (denom <= 0.f ? 1.f : denom); // 0 Kopf -> 1 Schwanz
        float whiten = tnorm * t->whiten_factor;
        Uint8 rcol = (Uint8)(base_r + (255 - base_r) * whiten);
        Uint8 gcol = (Uint8)(base_g + (255 - base_g) * whiten);
        Uint8 bcol = (Uint8)(base_b + (255 - base_b) * whiten);
        float aF = t->alpha_head + (t->alpha_tail - t->alpha_head) * tnorm;
        if (aF < 0)
            aF = 0;
        if (aF > 1)
            aF = 1;
        Uint8 alpha = (Uint8)(aF * 255.f);
        // Linienbreite approximieren durch parallele Offsets
        float dx = b.x - a.x;
        float dy = b.y - a.y;
        float len2 = dx * dx + dy * dy;
        if (len2 < 0.0001f) {
            SDL_SetRenderDrawColor(r->sdl, rcol, gcol, bcol, alpha);
            SDL_RenderDrawPointF(r->sdl, a.x, a.y);
            continue;
        }
        float inv_len = 1.f / sqrtf(len2);
        dx *= inv_len;
        dy *= inv_len;
        float nx = -dy, ny = dx;
        // Erzeuge Offsets in ganzen Pixeln bis zur gewünschten Halbbreite
        float hw = t->core_half_width;
        int max_off = (int)floorf(hw + 0.01f);
        if (max_off < 0)
            max_off = 0;
        SDL_SetRenderDrawColor(r->sdl, rcol, gcol, bcol, alpha);
        // Center Linie
        SDL_RenderDrawLineF(r->sdl, a.x, a.y, b.x, b.y);
        // Symmetrische Offsets
        for (int o = 1; o <= max_off; ++o) {
            float ox = nx * (float)o;
            float oy = ny * (float)o;
            // leicht geringere Alpha für äußere Linien
            Uint8 oa = (Uint8)(alpha * (1.0f - (float)o / (float)(max_off + 1)));
            SDL_SetRenderDrawColor(r->sdl, rcol, gcol, bcol, oa);
            SDL_RenderDrawLineF(r->sdl, a.x + ox, a.y + oy, b.x + ox, b.y + oy);
            SDL_RenderDrawLineF(r->sdl, a.x - ox, a.y - oy, b.x - ox, b.y - oy);
        }
    }
}

static void projectile_render_entity(Entity *e, struct Renderer *r) {

    if (!e)
        return;
    Projectile *p = (Projectile *)e;
    if (!p->active)
        return;
    trail_render(&p->trail, p->color_r, p->color_g, p->color_b, r);
    float sz = p->e.size.x > 0 ? p->e.size.x : 8.f;
    SDL_FRect dst = {p->e.pos.x - sz * 0.5f, p->e.pos.y - sz * 0.5f, sz, sz};
    if (p->e.texture) {
        /* Assume projectile texture is full sheet; fetch src rect via variant */
        Services *services = services_get();
        struct TextureManager *texman = services ? services->texman : NULL;
        SDL_Rect src = {0,0,0,0};
        if (texman)
            src = texman_projectile_src(texman, p->variant);
        renderer_draw_texture(r, p->e.texture, (src.w?&src:NULL), &dst, 0);
    }
}
static void projectile_on_hit_entity(Entity *e, Entity *hitter) {
    (void)hitter;
    if (!e)
        return;
    ((Projectile *)e)->active = false;
}
static const EntityVTable PROJECTILE_VT = {projectile_create_entity, projectile_destroy_entity, projectile_update_entity, projectile_render_entity, projectile_on_hit_entity, NULL};

Projectile *projectile_create(Entity *owner, Vec2 pos, Vec2 vel, SDL_Texture *tex, int damage, Uint8 cr, Uint8 cg, Uint8 cb) {

    Projectile *p = calloc(1, sizeof(Projectile));
    if (!p) {
        LOG_ERROR("projectile", "Failed to allocate projectile");
        return NULL;

}
    p->e.vt = &PROJECTILE_VT;
    p->e.pos = pos;
    p->e.vel = vel;
    p->e.texture = tex;
    p->e.type = ENT_PROJECTILE;
    p->e.size = (Vec2){8, 8};
    p->damage = damage;
    p->owner_kind = owner ? (unsigned char)owner->type : 0;
    p->e.angle_offset = 0.f; // symmetrical or already aligned
    p->active = true;
    p->flight_time = 0.f;
    p->color_r = cr;
    p->color_g = cg;
    p->color_b = cb;
    p->variant = 0; // will be updated by caller if needed
    trail_init(&p->trail);
    // Seed initial trail point so first rendered segment starts exactly am Mittelpunkt
    trail_add_point(&p->trail, p->e.pos);
    // collider config
    p->e.collider.radius = 3; // Should be 4, but 3 provides a nicer gameplay
    p->e.collider.poly_count = 0;
    p->e.collider.shape = COLLIDER_SHAPE_CIRCLE;
    p->e.is_dynamic = false; // do not push others
    return p;
}
void projectile_destroy(Projectile *p) {
    free(p);
}
void projectile_update_trail(Projectile *p) {
    if (!p)
        return;
    trail_add_point(&p->trail, (Vec2){p->e.pos.x, p->e.pos.y});
}

// --- Update Helpers ----------------------------------------------------
// Apply gravity contribution from a single planet
static inline void projectile_apply_gravity_from_planet(Projectile *p, const struct Planet *pl, float dt) {
    float dx = pl->e.pos.x - p->e.pos.x;
    float dy = pl->e.pos.y - p->e.pos.y;
    // Cheap broad-phase: if either axis diff already exceeds a cutoff where gravity negligible? (optional)
    float dist2 = dx * dx + dy * dy;
    if (dist2 < PROJ_EPSILON)
        return; // extremely close singularity guard
    // Inlined gravity (a = G * m / r^2) along direction vector
    // Compute 1/r and reuse
    float inv_dist = 1.0f / sqrtf(dist2);
    float inv_dist2 = inv_dist * inv_dist; // = 1/r^2
    float accel = (pl->mass * PROJ_GRAVITY_CONST) * inv_dist2;
    p->e.vel.x += accel * dx * inv_dist * dt; // dx/r * accel
    p->e.vel.y += accel * dy * inv_dist * dt;
}
// Test collision with a single planet using current position
static void projectile_integrate(Projectile *p, float dt) {
    p->e.pos.x += p->e.vel.x * dt;
    p->e.pos.y += p->e.vel.y * dt;
}
// Check if projectile is out of bounds and deactivate it
static bool projectile_check_oob(Projectile *p, const ProjectileUpdateCtx *ctx) {
    if (p->e.pos.x < ctx->min_x || p->e.pos.x > ctx->max_x || p->e.pos.y < ctx->min_y || p->e.pos.y > ctx->max_y) {
        p->active = false;
        return true;
    }
    return false;
}
// planet collision now handled in combined gravity pass
void projectile_update(Projectile *p, const ProjectileUpdateCtx *ctx) {
    if (!p || !p->active)
        return;
    p->flight_time += ctx->dt;
    // Apply gravity from planets only (collision handled by generic system after integration)
    for (int i = 0; i < ctx->planet_count && p->active; ++i){
        struct Planet *pl = ctx->planets[i];
        if (!pl) continue;
        projectile_apply_gravity_from_planet(p, pl, ctx->dt);
    }
    if (!p->active)
        return;
    projectile_integrate(p, ctx->dt);
    projectile_update_trail(p);
    if (projectile_check_oob(p, ctx))
        return;
    // Collision with player/enemies/planets now in collision system
}

int projectile_get_damage(Projectile *p, float factor) {
    if (!p)
        return 0;

    int additional_damage = (int)floor(p->flight_time * PROJ_DAMAGE_INCREASE_PER_SECOND * factor);

    if (additional_damage <= 0)
        return p->damage;

    else if(additional_damage > p->damage) // Cap the additional damage to prevent excessive values
        additional_damage = p->damage;

    return p->damage + additional_damage;
}