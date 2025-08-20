#include "scene_quick_play.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../services/services.h"
#include "../services/renderer.h"
#include "../services/texture_manager.h"
#include "../services/input.h"
#include "../app/app.h"
#include "../game/world.h"
#include "../game/planet.h"
#include "../game/player.h"
#include "../core/rand.h"
#include "../game/enemy.h"

static void on_world_time_over(struct World* w, void* user){
    (void)w;
    Scene *scene = (Scene*)user;
    if(!scene) return;
    SceneQuickPlayState *st = (SceneQuickPlayState*)scene->state;
    if(!st || st->time_over_handled) return;
    st->time_over_handled = 1;
    scene_quick_play_display_text(scene, "Time Over");
}

void scene_quick_play_pause(Scene *s)
{
    (void)s;
    app_push_overlay(SCENE_OVERLAY_PAUSE);
}
void scene_quick_play_resume(Scene *s) { (void)s; }
void scene_quick_play_display_text(Scene *s, const char *msg)
{
    SceneQuickPlayState *st = (SceneQuickPlayState *)s->state;
    st->show_hud_text = true;
    strncpy(st->hud_text, msg, sizeof(st->hud_text) - 1);
}
void scene_quick_play_display_hud(Scene *s, bool show)
{
    SceneQuickPlayState *st = (SceneQuickPlayState *)s->state;
    st->show_hud_text = show;
}

static void spawn_to_maintain(SceneQuickPlayState *st)
{
    if (!st || !st->world) return;
    World *w = st->world;
    for (int i = w->enemy_count; i < 3; ++i)
    {
        Vec2 epos = world_find_free_position(w, 32.f, 150.f, 100.f, 32.f, 400);
        if (epos.x < 0.f)
            break;
        world_spawn_enemy(w, rng_rangei(&w->rng, 0, ENEMY_TYPE_COUNT - 1), epos.x, epos.y);
    }
}


void scene_quick_play_enter(Scene *s)
{
    SceneQuickPlayState *st = calloc(1, sizeof(SceneQuickPlayState));
    s->state = st;
    st->svc = app_services();
    // Create world with time-based random seed so Quick Play varies per run
    u32 seed = (u32)time(NULL);
    printf("Quick Play seed: %u\n", seed);
    st->world = world_create(st->svc, seed, 14);
    if(st->world){
        // Example: 20 second limit; change as needed. Use -1 for infinite.
        world_set_time_limit(st->world, 20.f);
        st->world->on_time_over = on_world_time_over;
        st->world->on_time_over_user = s; // pass scene pointer
        /* Populate planets and place player explicitly from the scene */
        world_populate_planets(st->world, 14);
        world_place_player(st->world, 32.f);
        /* Initial enemy placement: scenes control spawning now. */
        for (int i = 0; i < 3; ++i)
        {
            Vec2 epos = world_find_free_position(st->world, 32.f, 150.f, 100.f, 32.f, 400);
            if (epos.x < 0.f)
                break;
            world_spawn_enemy(st->world, rng_rangei(&st->world->rng, 0, ENEMY_TYPE_COUNT - 1), epos.x, epos.y);
        }
    }
}
void scene_quick_play_leave(Scene *s)
{
    SceneQuickPlayState *st = (SceneQuickPlayState *)s->state;
    if (st)
    {
        if (st->world)
            world_destroy(st->world);
        free(st);
    }
}
void scene_quick_play_handle_input(Scene *s, const struct InputState *in)
{
    if (in->pause)
        scene_quick_play_pause(s);
    SceneQuickPlayState *st = (SceneQuickPlayState *)s->state;
    if (st)
        st->last_input = *in;
}
void scene_quick_play_update(Scene *s, float dt)
{
    SceneQuickPlayState *st = (SceneQuickPlayState *)s->state;
    if (st && st->world)
    {
        if (st->world->player)
            player_set_input(st->world->player, &st->last_input);
        world_update(st->world, dt);
    /* Keep a small desired population for Quick Play (scene-managed). */
    spawn_to_maintain(st);
    }
}
void scene_quick_play_render(Scene *s, struct Renderer *r)
{
    SceneQuickPlayState *st = (SceneQuickPlayState *)s->state;
    struct Services *svc = st->svc;
    SDL_Texture *bg = texman_get(svc->texman, TEX_BG_STARFIELD);
    renderer_draw_texture(r, bg, NULL, NULL, 0);
    if (st->world)
        world_render(st->world, r);
}
