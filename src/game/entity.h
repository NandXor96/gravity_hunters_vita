#pragma once
#include <SDL2/SDL.h>
#include <stdbool.h>
#include <math.h>
#include "../core/types.h"
#include "../core/math.h"


typedef enum {
    ENT_PLAYER,
    ENT_ENEMY,
    ENT_PLANET,
    ENT_PROJECTILE,
    ENT_EXPLOSION
} EntityType;

typedef struct Entity Entity;

struct Renderer; // forward declare
typedef struct EntityVTable {
    Entity *(*create)(void *params); // generic factory (params type depends on entity kind)
    void (*destroy)(Entity *);
    void (*update)(Entity *, float dt);
    void (*render)(Entity *, struct Renderer *);
    void (*on_hit)(Entity *, Entity *hitter);
    void (*on_collide)(Entity *, Entity *other); // physical contact callback (no extra info, bounce now handled per-entity)
} EntityVTable;

// Collider shape flags (circle always implicit, polygon optional later)
typedef enum {
    COLLIDER_SHAPE_CIRCLE = 1,
    COLLIDER_SHAPE_POLY   = 2
} ColliderShapeFlags;

typedef struct EntityCollider {
    float radius;          // bounding circle radius
    int   poly_count;      // 0 => no polygon
    Vec2  poly_world[30];   // world polygon points (rebuilt when dirty)
    Vec2  poly_local[30];   // original local polygon points (unchanged)
    bool  poly_world_dirty; // 1: needs world rebuild, 0: poly_world[] up-to-date
    unsigned int shape; // bitmask of ColliderShapeFlags
} EntityCollider;

struct Entity {
    const EntityVTable *vt;
    EntityType type;
    Vec2 pos;  // position
    Vec2 vel;  // velocity
    Vec2 size; // size (w,h) or extents depending on context
    float angle;
    float angle_offset; // render-only orientation adjustment (radians)
    bool active;
    SDL_Texture *texture;
    EntityCollider collider; // embedded collider
    bool is_dynamic;         // participates in positional resolution if true
};

struct Player *entity_create_player(void);
struct Enemy *entity_create_enemy(int kind);
struct Planet *entity_create_planet(float radius, float mass);
struct Projectile *entity_create_projectile(Entity *owner, Vec2 pos, Vec2 vel);
