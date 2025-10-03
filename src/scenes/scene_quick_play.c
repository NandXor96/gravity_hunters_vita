#include "scene_quick_play.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../core/log.h"
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
#include "overlay_endgame_qp.h"

static uint8_t enemy_difficulty[4] = {0, 110, 150, 210};
static uint8_t enemy_count[3] = {2, 3, 3};

static void on_world_time_over(struct World *w, void *user) {

    (void)w;
    Scene *scene = (Scene *)user;
    if (!scene)
        return;
    SceneQuickPlayState *st = (SceneQuickPlayState *)scene->state;
    if (!st || st->time_over_handled || st->player_death_handled)
        return;
    st->time_over_handled = 1;
    if (st && st->world) {
        overlay_endgame_qp_set_stats(st->world->kills, st->world->score);

}
    app_push_overlay(SCENE_OVERLAY_ENDGAME);
}

/* Module-level settings storage (menu writes these before starting scene) */
static QuickPlaySettings g_qp_settings = {0};

void scene_quick_play_set_settings(const QuickPlaySettings *s) {

    if (!s)
        return;
    g_qp_settings = *s;

}
const QuickPlaySettings *scene_quick_play_get_settings(void) {
    return &g_qp_settings;
}

void scene_quick_play_pause(Scene *s) {

    (void)s;
    app_push_overlay(SCENE_OVERLAY_PAUSE);

}
void scene_quick_play_resume(Scene *s) {
    (void)s;
}

static void spawn_to_maintain(SceneQuickPlayState *st) {

    if (!st || !st->world)
        return;
    World *w = st->world;
    const QuickPlaySettings *qs = scene_quick_play_get_settings();
    int max_enemies = qs->difficulty + 1;
    /* map quickplay difficulty to enemy difficulty byte (0..255) */
    if (w->enemy_count >= max_enemies)
        return; // already at desired population

    uint8_t enemy_diff = 85; /* baseline */
    if (qs)
        enemy_diff = enemy_difficulty[qs->difficulty];

    for (int i = w->enemy_count; i < max_enemies; ++i) {
        Vec2 epos = world_find_free_position(w, 10.f, 150.f, 100.f, 32.f, 400);
        if (epos.x < 0.f)
            break;
    world_spawn_enemy(w, rng_rangei(&w->rng, 0, ENEMY_TYPE_COUNT - 1), epos.x, epos.y, enemy_diff, 0);

}
}

void scene_quick_play_enter(Scene *s) {

    SceneQuickPlayState *st = calloc(1, sizeof(SceneQuickPlayState));
    if (!st) {
        LOG_ERROR("scene_quick_play", "Failed to allocate scene state");
        return;

}
    s->state = st;
    st->svc = app_services();
    u32 seed = (u32)time(NULL);
    // Read quick-play settings (if any) and create world accordingly
    const QuickPlaySettings *qs = scene_quick_play_get_settings();

    /* Map planets_count and planet_size into an approximate max_planet_area parameter.
     * Keep it simple: base 10 + planets_count*6 + planet_size*3 (approx). */
    unsigned int max_planet_area = 9u + (unsigned int)((qs ? (qs->planets_count + 1) : 1) * 2u) + (unsigned int)((qs ? qs->planet_size : 0) * 3u);
    float tl = qs ? (float)qs->time_seconds : 20.f;
    /* map planet_size to an average radius: 0=small,1=medium,2=large */
    float avg_radius = 56.f;
    if (qs) {
        if (qs->planet_size == 0)
            avg_radius = 50.f;
        else if (qs->planet_size == 1)
            avg_radius = 60.f;
        else if (qs->planet_size == 2)
            avg_radius = 70.f;
    }

    st->world = world_create(st->svc, seed);

    if (st->world) {

        /* time limit from settings (seconds) */
        if (tl <= 0.f)
            tl = -1.f; /* treat non-positive as infinite */
        world_set_time_limit(st->world, tl);
        st->world->on_time_over = on_world_time_over;
        st->world->on_time_over_user = s; // pass scene pointer
        /* Populate planets and place player explicitly from the scene */
        world_populate_planets(st->world, max_planet_area, avg_radius);
        world_place_player(st->world, 32.f);
        /* Initial enemy placement: scenes control spawning now. */
        spawn_to_maintain(st);
        st->world->hud = hud_create(st->world->svc, st->world->player);

        app_push_overlay(SCENE_OVERLAY_START_GAME);

    }
}
void scene_quick_play_leave(Scene *s) {
    SceneQuickPlayState *st = (SceneQuickPlayState *)s->state;
    if (st) {
        if (st->world)
            world_destroy(st->world);
        free(st);
}
}
void scene_quick_play_handle_input(Scene *s, const struct InputState *in) {
    if (in->pause)
        scene_quick_play_pause(s);
    SceneQuickPlayState *st = (SceneQuickPlayState *)s->state;
    if (st)
        st->last_input = *in;
}
void scene_quick_play_update(Scene *s, float dt) {
    SceneQuickPlayState *st = (SceneQuickPlayState *)s->state;
    if (!st || !st->world)
        return;

    if (st->world->player)
        player_set_input(st->world->player, &st->last_input);

    if (!st->player_death_handled && !st->time_over_handled)
        spawn_to_maintain(st);

    world_update(st->world, dt);

    Player *player = st->world->player;
    if (player && !player->alive && !st->player_death_handled) {
        st->player_death_handled = 1;
        st->player_death_delay = PLAYER_DEATH_DELAY_SECONDS;
}

    if (st->player_death_handled && !st->time_over_handled) {

        st->player_death_delay -= dt;
        if (st->player_death_delay <= 0.f) {
            st->time_over_handled = 1;
            if (st->world)
                overlay_endgame_qp_set_stats(st->world->kills, st->world->score);
            app_push_overlay(SCENE_OVERLAY_ENDGAME);

    }
        return;
    }
}
void scene_quick_play_render(Scene *s, struct Renderer *r) {
    SceneQuickPlayState *st = (SceneQuickPlayState *)s->state;
    struct Services *svc = st->svc;
    SDL_Texture *bg = texman_get(svc->texman, TEX_BG_STARFIELD);
    renderer_draw_texture(r, bg, NULL, NULL, 0);
    if (st->world)
        world_render(st->world, r);
}
