#include "planet.h"
#include <stdlib.h>
#include "../services/renderer.h"
#include "projectile.h"
#include "explosion.h"
#include "world.h"
#include "../services/texture_manager.h"
#include "../services/services.h"
#include "../core/log.h"

static void planet_render(Entity *e, struct Renderer *r) {

    Planet *p = (Planet *)e;
    SDL_FRect dst = {p->e.pos.x - p->radius, p->e.pos.y - p->radius, p->radius * 2.0f, p->radius * 2.0f

};
    renderer_draw_texture(r, p->e.texture, &p->src, &dst, 0.0f);
}
static void planet_update(Entity *e, float dt) {
    (void)e;
    (void)dt;
}
static void planet_destroy_entity(Entity *e) {
    planet_destroy((Planet *)e);
}
static void planet_on_hit_entity(Entity *e, Entity *hitter) {
    (void)e;
    if (hitter && hitter->type == ENT_PROJECTILE){
        Projectile *pr = (Projectile*)hitter;
        if (pr) pr->active = false; // destroy projectile on impact
        Planet *p = (Planet*)e;
        if (p->world){
            Services *svc = services_get();
            if (svc && svc->texman){
                int types = texman_explosion_type_count(svc->texman);
                if (types>0){
                    int type = rand() % types;
                    world_add_explosion(p->world, type, hitter->pos.x, hitter->pos.y, 0.5);
}
            }
        }
    }
}
static Entity *planet_create_entity(void *params) {
    struct { float x; float y; float radius; float mass; SDL_Texture *tex; SDL_Rect src;
} *pr = params;
    return (Entity *)planet_create(pr->x, pr->y, pr->radius, pr->mass, pr->tex, pr->src);
}
static const EntityVTable PLANET_VT = {planet_create_entity, planet_destroy_entity, planet_update, planet_render, planet_on_hit_entity, NULL};

Planet *planet_create(float x, float y, float radius, float mass, SDL_Texture *tex, SDL_Rect src) {

    Planet *p = calloc(1, sizeof(Planet));
    if (!p) {
        LOG_ERROR("planet", "Failed to allocate planet");
        return NULL;

}
    p->radius = radius;
    p->radius_sq = radius*radius;
    p->mass = mass;
    p->e.texture = tex;
    p->src = src;
    p->e.size.x = radius*2;
    p->e.size.y = radius*2;
    p->e.vt = &PLANET_VT;
    p->e.type = ENT_PLANET;
    p->e.pos.x = x; p->e.pos.y = y;
    p->e.is_dynamic = false;
    p->e.collider.radius = radius;
    p->e.collider.poly_count = 0;
    p->e.collider.shape = COLLIDER_SHAPE_CIRCLE;
    p->world = NULL; /* set by world_add_planet */
    (void)mass; // mass currently unused (no gravity system)
    return p;
}
void planet_destroy(Planet *p) {
    free(p);
}
Circle planet_collider(const Planet *p) {
    return (Circle){{p->e.pos.x, p->e.pos.y
}, p->radius}; }
