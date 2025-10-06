#include "explosion.h"
#include "planet.h"
#include "entity.h"
#include "../services/services.h"
#include "../services/texture_manager.h"
#include "../services/renderer.h"
#include <stdlib.h>

static Entity *explosion_create_entity(void* params) {

    (void)params; return NULL;

}
static void explosion_destroy_entity(Entity *e) {
    if (!e) return; free(e);
}
static void explosion_on_hit(Entity *e, Entity *hitter) {
    (void)e; (void)hitter;
}

static void explosion_update(Entity *e, float dt) {

    Explosion *ex = (Explosion*)e; if (!ex) return; if (!ex->active) return;
    ex->timer += dt;
    if (ex->timer >= ex->frame_time) {
        ex->timer -= ex->frame_time;
        ex->frame++;
        Services *svc = services_get();
        if (!svc || !svc->texman || ex->frame >= ex->frame_count) {
            ex->active = false; /* done */

}
    }
}
static void explosion_render(Entity *e, struct Renderer *r) {
    Explosion *ex = (Explosion*)e; if (!ex || !ex->active) return; Services *svc = services_get(); if (!svc || !svc->texman) return;
    SDL_Texture *sheet = texman_explosions_texture(svc->texman); if (!sheet) return;
    SDL_Rect src = texman_explosion_src(svc->texman, ex->type, ex->frame);
    float w = src.w * ex->scale; float h = src.h * ex->scale;
    SDL_FRect dst = { ex->e.pos.x - w*0.5f, ex->e.pos.y - h*0.5f, w, h
};
    renderer_draw_texture(r, sheet, &src, &dst, 0.f);
}
static const EntityVTable EXPLOSION_VT = { explosion_create_entity, explosion_destroy_entity, explosion_update, explosion_render, explosion_on_hit, NULL };

Explosion *explosion_create(int type, float x, float y, float scale) {

    Services *svc = services_get(); if (!svc || !svc->texman) return NULL;
    Explosion *ex = calloc(1, sizeof(Explosion)); if (!ex) return NULL;
    ex->e.vt = &EXPLOSION_VT; ex->e.type = ENT_EXPLOSION; ex->e.pos.x = x; ex->e.pos.y = y; ex->active = true;
    ex->type = type;
    ex->frame = 0; ex->frame_time = 0.04f; ex->timer = 0.f; ex->scale = scale <= 0.f ? 1.f : scale;
    ex->frame_count = texman_explosion_frames(svc->texman);
    if (ex->frame_count <= 0) ex->active = false;
    return ex;

}
void explosion_destroy(Explosion *ex) {
    free(ex);
}

void explosion_spawn_at_planet_center(struct Planet *planet) {

    if (!planet) return; Services *svc = services_get(); if (!svc || !svc->texman) return;
    int types = texman_explosion_type_count(svc->texman); if (types <= 0) return;
    int type = rand() % types;
    float scale = (planet->radius * 1.4f) / 64.f; /* assuming tile 64 */
    Explosion *ex = explosion_create(type, planet->e.pos.x, planet->e.pos.y, scale);
    if (!ex) return;
    /* For now we just render unmanaged explosions by attaching to a simple global list; if no system exists yet, minimal static array */
    /* TODO: integrate into world entity list; placeholder global below */

}
