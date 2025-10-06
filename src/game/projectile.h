#pragma once
#include "entity.h"

typedef struct Trail {
    Vec2 points[TRAIL_LEN];
    int  head;
    int  length;
    float alpha_head;
    float alpha_tail;
    float whiten_factor;     // Anteil der Aufhellung an Schwanz (0..1)
    float core_half_width;   // Offset für Kernbreite
    float glow_offset;       // Offset für Glow
    float glow_alpha_factor; // Multiplikator für Glow Alpha relativ zum Segment Alpha
} Trail;

typedef struct Projectile {
    Entity e;               // includes pos & vel
    bool   active;
    int    owner_id;        // optional: shooter index or entity id
    unsigned char owner_kind; // EntityType of owner when fired (for friendly-fire filtering)
    int    damage;
    float  flight_time;     // seconds since fired
    Uint8  color_r, color_g, color_b; // base color
    int    variant;         // projectile sheet index
    Trail  trail;           // optischer Trail
} Projectile;
Projectile *projectile_create(Entity *owner, Vec2 pos, Vec2 vel, SDL_Texture *tex, int damage, Uint8 cr, Uint8 cg, Uint8 cb);
void projectile_destroy(Projectile *p);
void projectile_update_trail(Projectile *p);

// Context for per-projectile update (gravity from planets, collisions, bounds)
struct Planet; struct Player; struct Enemy;
typedef struct ProjectileUpdateCtx {
    struct Planet **planets; int planet_count;
    struct Player *player;
    struct Enemy **enemies; int enemy_count;
    float min_x, max_x, min_y, max_y; // out-of-bounds rectangle
    float dt; // delta time
} ProjectileUpdateCtx;

// Full update: physics, OOB, collisions; deactivates projectile when consumed
void projectile_update(Projectile *p, const ProjectileUpdateCtx *ctx);

// Get flight time scaled damage (increases over time)
int projectile_get_damage(Projectile *p, float factor);