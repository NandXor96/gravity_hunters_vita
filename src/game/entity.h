#pragma once
#include <SDL2/SDL.h>
#include <stdbool.h>
#include <math.h>
#include "../core/types.h"
#include "../core/math.h"

typedef enum
{
    ENT_PLAYER,
    ENT_ENEMY,
    ENT_PLANET,
    ENT_PROJECTILE,
    ENT_EXPLOSION
} EntityType;

typedef struct Entity Entity;

struct Renderer; // forward declare
typedef struct EntityVTable
{
    Entity *(*create)(void *params); // generic factory (params type depends on entity kind)
    void (*destroy)(Entity *);
    void (*update)(Entity *, float dt);
    void (*render)(Entity *, struct Renderer *);
    void (*on_hit)(Entity *, Entity *hitter);
} EntityVTable;

struct Entity
{
    const EntityVTable *vt;
    EntityType type;
    Vec2 pos;  // position
    Vec2 vel;  // velocity
    Vec2 size; // size (w,h) or extents depending on context
    float angle;
    float angle_offset; // render-only orientation adjustment (radians)
    bool active;
    SDL_Texture *texture;
};

struct Player *entity_create_player(void);
struct Enemy *entity_create_enemy(int kind);
struct Planet *entity_create_planet(float radius, float mass);
struct Projectile *entity_create_projectile(Entity *owner, Vec2 pos, Vec2 vel);
