#include "scene_menu.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../services/services.h"
#include "../services/renderer.h"
#include "../services/texture_manager.h"
#include "../services/input.h"
#include "../app/app.h"

static void ensure_items(SceneMenuState *st)
{
    if (st->items)
        return;
    st->count = 1;
    st->items = calloc(st->count, sizeof(MenuItem));
    st->items[0].label = "Press Enter to Start";
    st->items[0].target = SCENE_GAME;
}

void scene_menu_enter(Scene *s)
{
    SceneMenuState *st = calloc(1, sizeof(SceneMenuState));
    s->state = st;
    st->svc = app_services();
    ensure_items(st);
}
void scene_menu_leave(Scene *s)
{
    SceneMenuState *st = (SceneMenuState *)s->state;
    free(st->items);
    free(st);
}
void scene_menu_handle_input(Scene *s, const struct InputState *in)
{
    if (in->confirm)
    {
        app_set_scene(SCENE_GAME);
    }
}
void scene_menu_update(Scene *s, float dt)
{
    (void)s;
    (void)dt;
}
void scene_menu_render(Scene *s, struct Renderer *r)
{
    SceneMenuState *st = (SceneMenuState *)s->state;
    (void)st;
    // Background
    struct Services *svc = app_services();
    SDL_Texture *bg = texman_get(svc->texman, TEX_BG_STARFIELD);
    SDL_FRect dst = {0, 0, (float)svc->display_w, (float)svc->display_h};
    renderer_draw_texture(r, bg, NULL, &dst, 0);
    renderer_draw_text(r, "MENU", 40, 40, (TextStyle){0});
    renderer_draw_text(r, "Press Enter to Start", 40, 70, (TextStyle){0});
}
