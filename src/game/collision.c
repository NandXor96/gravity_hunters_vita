#include "collision.h"
#include "world.h"
#include "player.h"
#include "enemy.h"
#include "planet.h"
#include <math.h>
#include "projectile_system.h"
#include "projectile.h"
#include "../services/renderer.h"

typedef struct Pair {
    Entity *a;
    Entity *b;
} Pair;

// Prüft, ob eine Entity an der Kollision teilnehmen soll (Alive/Active Status)
static int entity_alive(Entity *e) {
    if (!e)
        return 0;
    switch (e->type) {
    case ENT_PLAYER:
        return ((struct Player *)e)->alive;
    case ENT_ENEMY:
        return ((struct Enemy *)e)->alive;
    case ENT_PLANET:
        return 1;
    case ENT_PROJECTILE:
        return ((struct Projectile *)e)->active;
    case ENT_EXPLOSION:
        return 0; // ignore in collisions
    default:
        return 0;
}
}

// Paar-Filter: Entscheidet ob zwei Entities überhaupt getestet werden.
// Unterdrückt identische Pointer, nicht aktive, Planet-Planet, Explosionen,
// Projectile vs Projectile und Friendly-Fire.
static int collision_should_test(Entity *a, Entity *b) {
    if (a == b)
        return 0;
    // Planet vs Planet ignore (both static)
    if (a->type == ENT_PLANET && b->type == ENT_PLANET)
        return 0;
    // Explosion ignore
    if (a->type == ENT_EXPLOSION || b->type == ENT_EXPLOSION)
        return 0;
    if (a->type == ENT_PROJECTILE) {
        Projectile *pa = (Projectile *)a;
        if (b->type == pa->owner_kind)
            return 0; // owner kind skip
    }
    if (b->type == ENT_PROJECTILE) {
        Projectile *pb = (Projectile *)b;
        if (a->type == pb->owner_kind)
            return 0;
    }
    return 1;
}

// Holt alle relevanten Entities aus der World (Player, Enemies, Planets,
// aktive Projektile) in ein flaches Array.
static void collect_entities(struct World *w, Entity **out, int *count, int max) {
    *count = 0;
    if (!w)
        return;
    if (w->player && *count < max)
        out[(*count)++] = (Entity *)w->player;
    for (int i = 0; i < w->enemy_count && *count < max; i++) {
        if (w->enemies[i])
            out[(*count)++] = (Entity *)w->enemies[i];
    }
    for (int i = 0; i < w->planet_count && *count < max; i++) {
        if (w->planets[i])
            out[(*count)++] = (Entity *)w->planets[i];
    }
    // projectiles
    ProjectileSystem *ps = &w->projsys;
    for (int s = 0; s < ps->shooter_count && *count < max; ++s) {
        ShooterPool *pool = &ps->shooters[s];
        for (int i = 0; i < pool->head && *count < max; i++) {
            Projectile *pr = pool->items[i];
            if (pr && pr->active)
                out[(*count)++] = (Entity *)pr;
        }
    }
}

// Schneller Bounding-Circle Broadphase Test (nur Radius / Distanz)
static inline int broadphase_circle(const Entity *a, const Entity *b) {
    float r = a->collider.radius + b->collider.radius;
    float dx = b->pos.x - a->pos.x;
    float dy = b->pos.y - a->pos.y;
    return (dx * dx + dy * dy) <= (r * r);
}

// Transformiert lokales Polygon (poly_local) nach Weltkoordinaten in poly_world[]
// (nur falls Polygon vorhanden + noch nicht transformiert). Rücktransformation
// ist nicht nötig, wir überschreiben pro Frame neu und setzen am Ende das
// Flag wieder auf 0.
static void collider_prepare(Entity *e) {
    if (!e)
        return;
    EntityCollider *c = &e->collider;
    if (!(c->shape & COLLIDER_SHAPE_POLY))
        return;
    if (c->poly_count <= 0)
        return;
    if (!c->poly_world_dirty)
        return; // already world

    // Rotation + Offset (Sprite wird mit angle+angle_offset gerendert -> Collider folgt dem)
    float rot = e->angle + e->angle_offset;
    float s = sinf(rot);
    float co = cosf(rot);
    for (int i = 0; i < c->poly_count; i++) {
        Vec2 lp = c->poly_local[i]; // stored local
        float cx = e->size.x * 0.5f;
        float cy = e->size.y * 0.5f;
        float x = lp.x - cx;
        float y = lp.y - cy;
        float wx = x * co - y * s + e->pos.x;
        float wy = x * s + y * co + e->pos.y;
        c->poly_world[i] = (Vec2){wx, wy}; // world
    }
    c->poly_world_dirty = 0;
}

// SAT Helpers --------------------------------------------------------------
// Projektion aller Punkte eines Polygons auf eine Achse (Normalisierte Achse)
// liefert Min/Max Skalarwerte für SAT Tests.
static void project_axis(const Vec2 *pts, int count, Vec2 axis, float *minOut, float *maxOut) {
    float minp = pts[0].x * axis.x + pts[0].y * axis.y;
    float maxp = minp;
    for (int i = 1; i < count; i++) {
        float p = pts[i].x * axis.x + pts[i].y * axis.y;
        if (p < minp)
            minp = p;
        if (p > maxp)
            maxp = p;
    }
    *minOut = minp;
    *maxOut = maxp;
}

// SAT Overlap-Test Polygon-Polygon (nur bool Ergebnis, keine Penetration)
static int sat_poly_poly(const Vec2 *A, int nA, const Vec2 *B, int nB) {
    // test all edges of A
    for (int i = 0; i < nA; i++) {
        int j = (i + 1) % nA;
        Vec2 e = {A[j].x - A[i].x, A[j].y - A[i].y};
        Vec2 axis = {-e.y, e.x};
        float len = sqrtf(axis.x * axis.x + axis.y * axis.y);
        if (len < 1e-6f)
            continue;
        axis.x /= len;
        axis.y /= len;
        float minA, maxA, minB, maxB;
        project_axis(A, nA, axis, &minA, &maxA);
        project_axis(B, nB, axis, &minB, &maxB);
        if (maxA < minB || maxB < minA)
            return 0;
    }
    // test all edges of B
    for (int i = 0; i < nB; i++) {
        int j = (i + 1) % nB;
        Vec2 e = {B[j].x - B[i].x, B[j].y - B[i].y};
        Vec2 axis = {-e.y, e.x};
        float len = sqrtf(axis.x * axis.x + axis.y * axis.y);
        if (len < 1e-6f)
            continue;
        axis.x /= len;
        axis.y /= len;
        float minA, maxA, minB, maxB;
        project_axis(A, nA, axis, &minA, &maxA);
        project_axis(B, nB, axis, &minB, &maxB);
        if (maxA < minB || maxB < minA)
            return 0;
    }
    return 1; // overlap
}
// Approximativer Kreis-gegen-Polygon Test:
// 1) Punkt-in-Polygon (Kreismittelpunkt)
// 2) Abstand von Mittelpunkt zu allen Kanten <= Radius
static int sat_circle_poly(Entity *circleE, Entity *polyE) {
    // simple: treat circle as center point expanded; do coarse circle vs AABB already passed.
    // We'll approximate with point-in-poly_world for now plus edge distance check.
    EntityCollider *c = &polyE->collider;
    if (c->poly_count <= 2)
        return 1;
    float cx = circleE->pos.x;
    float cy = circleE->pos.y;
    float r = circleE->collider.radius;
    // First: if center inside polygon -> collision
    int inside = 0;
    for (int i = 0, j = c->poly_count - 1; i < c->poly_count; j = i++) {
        Vec2 pi = c->poly_world[i], pj = c->poly_world[j];
        if (((pi.y > cy) != (pj.y > cy)) && (cx < (pj.x - pi.x) * (cy - pi.y) / (pj.y - pi.y + 1e-6f) + pi.x))
            inside = !inside;
    }
    if (inside)
        return 1;
    // Edge distance check
    float r2 = r * r;
    for (int i = 0; i < c->poly_count; i++) {
        int j = (i + 1) % c->poly_count;
        Vec2 a = c->poly_world[i], b = c->poly_world[j];
        float vx = b.x - a.x, vy = b.y - a.y;
        float wx = cx - a.x, wy = cy - a.y;
        float len2 = vx * vx + vy * vy;
        float t = len2 > 1e-6f ? (wx * vx + wy * vy) / len2 : 0.f;
        if (t < 0.f)
            t = 0.f;
        else if (t > 1.f)
            t = 1.f;
        float px = a.x + vx * t, py = a.y + vy * t;
        float dx = cx - px, dy = cy - py;
        if (dx * dx + dy * dy <= r2)
            return 1;
    }
    return 0;
}

// Schneller Kreis-Kreistest basierend auf (rA+rB)^2 vs Distanz^2
static int circle_circle(Entity *a, Entity *b) {
    float rA = a->collider.radius;
    float rB = b->collider.radius;
    float dx = b->pos.x - a->pos.x;
    float dy = b->pos.y - a->pos.y;
    float dist2 = dx * dx + dy * dy;
    float rSum = rA + rB;
    if (dist2 > rSum * rSum)
        return 0;
    float dist = dist2 > 0.f ? sqrtf(dist2) : 0.f;
    return 1; // overlap detected; we don't need direction for simple resolve now
}
static void apply_separation(Entity *e, Vec2 n, float amount) {
    e->pos.x += n.x * amount;
    e->pos.y += n.y * amount;
}
// NOTE: Global physical separation (bounce) removed.
// Future: Provide collision normal & penetration to on_collide so entities can implement custom bounce.
// Kollisionsreaktions-Dispatch:
//  - Projectile + Nicht-Projektil -> on_hit nur beim Ziel, Projektil deaktiviert
//  - Sonst: beidseitiger on_collide Callback
static void dispatch_pair(Entity *a, Entity *b) {
    int aProj = (a->type == ENT_PROJECTILE);
    int bProj = (b->type == ENT_PROJECTILE);
    if (aProj ^ bProj) {
        Entity *projE = aProj ? a : b;
        Entity *other = aProj ? b : a;
        Projectile *p = (Projectile *)projE;
        if (!p->active)
            return;
        // Friendly fire already filtered earlier.
        if (other->vt && other->vt->on_hit)
            other->vt->on_hit(other, projE); // ONLY one on_hit (target)
        p->active = false;                   // deactivate projectile without triggering its own on_hit
        return;                              // never call on_collide for projectile interactions
}
    if (a->vt && a->vt->on_collide)
        a->vt->on_collide(a, b);
    if (b->vt && b->vt->on_collide)
        b->vt->on_collide(b, a);
}

// Haupt-Einstieg: Führt vollständigen Kollisions-Durchlauf aus.
// dt derzeit ungenutzt (Reserviert für zukünftige CCD / zeitabhängige Filter).
void collision_run(struct World *w, float dt) {
    (void)dt;
    if (!w)
        return; // dt kept for future (CCD etc.)
    Entity *list[256];
    int count = 0;
    collect_entities(w, list, &count, 256);
    for (int i = 0; i < count; i++) {
        Entity *e = list[i];
        if (e->collider.radius <= 0.f) {
            float rad = fmaxf(e->size.x, e->size.y) * 0.5f;
            if (rad <= 0.f) rad = 1.f;
            e->collider.radius = rad;
        }
        // Build polygon if marked dirty (movement/rotation set flag earlier in frame)
        if (e->collider.poly_world_dirty)
            collider_prepare(e);
    }
    for (int i = 0; i < count; i++) {
        for (int j = i + 1; j < count; j++) {
            Entity *a = list[i];
            Entity *b = list[j];
            if (!collision_should_test(a, b))
                continue;
            // Schneller Circle Broadphase
            if (!broadphase_circle(a, b))
                continue; // no possible collision
            int collided = 0;
            unsigned int sfA = a->collider.shape;
            unsigned int sfB = b->collider.shape;
            if ((sfA & COLLIDER_SHAPE_POLY) && (sfB & COLLIDER_SHAPE_POLY)) {
                collided = sat_poly_poly(a->collider.poly_world, a->collider.poly_count, b->collider.poly_world, b->collider.poly_count);
            }
            else if ((sfA & COLLIDER_SHAPE_POLY) && !(sfB & COLLIDER_SHAPE_POLY)) {
                collided = sat_circle_poly(b, a); // b is circle
            }
            else if ((sfB & COLLIDER_SHAPE_POLY) && !(sfA & COLLIDER_SHAPE_POLY)) {
                collided = sat_circle_poly(a, b); // a is circle
            }
            else {
                collided = circle_circle(a, b);
            }
            if (!collided)
                continue;
            dispatch_pair(a, b);
        }
    }
}

#ifdef DEBUG_COLLISION
// Debug Rendering (compile-time)
// Zeigt: Bounding-Circle (cyan) + Polygon-Umriss (rot) + Mittelpunkt (magenta)
void collision_debug_draw(struct World *w, struct Renderer *r) {
    if (!w || !r) return;
    Entity *list[256]; int count = 0; collect_entities(w, list, &count, 256);
    SDL_SetRenderDrawBlendMode(r->sdl, SDL_BLENDMODE_BLEND);
    for (int i = 0; i < count; ++i) {
        Entity *e = list[i];
        float rC = e->collider.radius;
        if (rC > 1.f) {
            SDL_SetRenderDrawColor(r->sdl, 0, 200, 255, 180);
            Vec2 prev = {e->pos.x + rC, e->pos.y};
            const int SEG = 32;
            for (int s = 1; s <= SEG; ++s) {
                float ang = (float)s / SEG * 6.28318530718f;
                Vec2 cur = {e->pos.x + cosf(ang) * rC, e->pos.y + sinf(ang) * rC};
                SDL_RenderDrawLineF(r->sdl, prev.x, prev.y, cur.x, cur.y);
                prev = cur;
            }
        }
        // Polygon falls vorhanden (verwende aktuelle world-points; wenn dirty==0 dann noch nicht transformiert -> jetzt transformieren)
        EntityCollider *c = &e->collider;
        if ((c->shape & COLLIDER_SHAPE_POLY) && c->poly_count > 1) {
            if (c->poly_world_dirty) collider_prepare(e);
            SDL_SetRenderDrawColor(r->sdl, 255, 40, 40, 220);
            for (int p = 0; p < c->poly_count; ++p) {
                int q = (p + 1) % c->poly_count;
                Vec2 a = c->poly_world[p];
                Vec2 b = c->poly_world[q];
                SDL_RenderDrawLineF(r->sdl, a.x, a.y, b.x, b.y);
            }
            SDL_SetRenderDrawColor(r->sdl, 255, 0, 255, 220);
            SDL_RenderDrawLineF(r->sdl, e->pos.x - 2, e->pos.y, e->pos.x + 2, e->pos.y);
            SDL_RenderDrawLineF(r->sdl, e->pos.x, e->pos.y - 2, e->pos.x, e->pos.y + 2);
        }
    }
}
#endif
