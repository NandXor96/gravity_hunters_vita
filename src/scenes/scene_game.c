#include "scene_game.h"
#include <stdlib.h>
#include <string.h>
#include "../services/services.h"
#include "../services/renderer.h"
#include "../services/texture_manager.h"
#include "../services/input.h"
#include "../app/app.h"
#include "../game/world.h"
#include "../game/planet.h"
#include "../game/player.h"

static void on_world_time_over(struct World* w, void* user){
    (void)w;
    Scene *scene = (Scene*)user;
    if(!scene) return;
    SceneGameState *st = (SceneGameState*)scene->state;
    if(!st || st->time_over_handled) return;
    st->time_over_handled = 1;
    scene_game_display_text(scene, "Time Over");
}

void scene_game_pause(Scene *s)
{
    (void)s;
    app_push_overlay(SCENE_OVERLAY_PAUSE);
}
void scene_game_resume(Scene *s) { (void)s; }
void scene_game_display_text(Scene *s, const char *msg)
{
    SceneGameState *st = (SceneGameState *)s->state;
    st->show_hud_text = true;
    strncpy(st->hud_text, msg, sizeof(st->hud_text) - 1);
}
void scene_game_display_hud(Scene *s, bool show)
{
    SceneGameState *st = (SceneGameState *)s->state;
    st->show_hud_text = show;
}

void scene_game_enter(Scene *s)
{
    SceneGameState *st = calloc(1, sizeof(SceneGameState));
    s->state = st;
    st->svc = app_services();
    // Create world with random seed (placeholder fixed seed for now)
    st->world = world_create(st->svc, 1245u, 15);
    if(st->world){
        // Example: 5 minute limit; change as needed. Use -1 for infinite.
        world_set_time_limit(st->world, 20.f); // currently infinite; adjust externally later.
        st->world->on_time_over = on_world_time_over;
        st->world->on_time_over_user = s; // pass scene pointer
    }
}
void scene_game_leave(Scene *s)
{
    SceneGameState *st = (SceneGameState *)s->state;
    if (st)
    {
        if (st->world)
            world_destroy(st->world);
        free(st);
    }
}
void scene_game_handle_input(Scene *s, const struct InputState *in)
{
    if (in->pause)
        scene_game_pause(s);
    SceneGameState *st = (SceneGameState *)s->state;
    if (st)
        st->last_input = *in;
}
void scene_game_update(Scene *s, float dt)
{
    SceneGameState *st = (SceneGameState *)s->state;
    if (st && st->world)
    {
        if (st->world->player)
            player_set_input(st->world->player, &st->last_input);
        world_update(st->world, dt);
    }
}
void scene_game_render(Scene *s, struct Renderer *r)
{
    SceneGameState *st = (SceneGameState *)s->state;
    struct Services *svc = st->svc;
    /* Ensure backbuffer is cleared each frame to avoid ghost artifacts */
    SDL_SetRenderDrawColor(svc->sdl_renderer, 0,0,0,255);
    SDL_RenderClear(svc->sdl_renderer);
    SDL_Texture *bg = texman_get(svc->texman, TEX_BG_STARFIELD);
    SDL_FRect dst = {0, 0, (float)svc->display_w, (float)svc->display_h};
    renderer_draw_texture(r, bg, NULL, &dst, 0);
    if (st->world)
        world_render(st->world, r);
}
