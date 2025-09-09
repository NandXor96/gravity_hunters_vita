#include "scene_menu.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../services/services.h"
#include "../services/renderer.h"
#include "../services/texture_manager.h"
#include "../services/input.h"
#include "../app/app.h"

typedef struct MenuEntry
{
    const char *label;
    int enabled;      /* 0 = disabled placeholder, 1 = active */
    int target_scene; /* SCENE_* for quick wiring, or -1 for none */
} MenuEntry;

static const MenuEntry DEFAULT_MENU[] = {
    {"Campaign", 1, SCENE_CAMPAIGN},
    {"Quick Play", 1, SCENE_QUICK_PLAY_MENU},
    {"Zen Mode", 0, -1},
    {"Help", 0, -1},
    {"Credits", 0, -1}};

static void scene_menu_setup_state(SceneMenuState *st)
{
    if (st->items)
        return; // already initialized
    st->count = (int)(sizeof(DEFAULT_MENU) / sizeof(DEFAULT_MENU[0]));
    st->items = calloc(st->count, sizeof(MenuItem));
    for (int i = 0; i < st->count; ++i)
    {
        st->items[i].label = DEFAULT_MENU[i].label;
        st->items[i].target = DEFAULT_MENU[i].target_scene;
        st->items[i].enabled = DEFAULT_MENU[i].enabled;
    }
    /* start selection on first item (no item pre-highlighted as special) */
    st->selected = 0;
}

void scene_menu_enter(Scene *s)
{
    SceneMenuState *st = calloc(1, sizeof(SceneMenuState));
    s->state = st;
    st->svc = app_services();
    scene_menu_setup_state(st);
    // ignore any already-held input for a few frames to avoid accidental activation
    st->suppress_input_frames = 4;
}
void scene_menu_leave(Scene *s)
{
    SceneMenuState *st = (SceneMenuState *)s->state;
    if (st)
    {
        free(st->items);
        free(st);
    }
}

void scene_menu_handle_input(Scene *s, const struct InputState *in)
{
    SceneMenuState *st = (SceneMenuState *)s->state;
    if (!st || !in)
        return;
    // if we just entered the menu, suppress currently-held inputs for a few frames
    if (st->suppress_input_frames > 0)
    {
        st->suppress_input_frames -= 1;
        // still update prev_* to reflect held state so edges are calculated after suppression
        st->prev_move_up = in->move_up ? 1 : 0;
        st->prev_move_down = in->move_down ? 1 : 0;
        st->prev_confirm = in->confirm ? 1 : 0;
        return;
    }
    /* Navigate with up/down (DPAD or arrow keys map into input state) */
    /* Edge-detect movement using move_up/move_down booleans (these are held for WASD/Arrows) */
    int up = in->move_up ? 1 : 0;
    int down = in->move_down ? 1 : 0;
    int confirm = in->confirm ? 1 : 0;
    /* Allow arrow keys / D-Pad to navigate: speed_up_step / speed_down_step are generated for arrows/D-Pad */
    int up_pressed = (up && !st->prev_move_up) || in->speed_up_step;
    int down_pressed = (down && !st->prev_move_down) || in->speed_down_step;
    if (up_pressed)
    {
        st->selected = (st->selected - 1 + st->count) % st->count;
    }
    if (down_pressed)
    {
        st->selected = (st->selected + 1) % st->count;
    }
    if (confirm && !st->prev_confirm)
    {
        MenuItem *mi = &st->items[st->selected];
        if (mi->enabled && mi->target >= 0)
        {
            /* Switching scene may destroy this state immediately; return to avoid
             * accessing freed memory below. */
            app_set_scene(mi->target);
            return;
        }
        /* disabled placeholders do nothing */
    }
    st->prev_move_up = up;
    st->prev_move_down = down;
    st->prev_confirm = confirm;
}

void scene_menu_update(Scene *s, float dt)
{
    (void)s;
    (void)dt;
}

void scene_menu_render(Scene *s, struct Renderer *r)
{
    SceneMenuState *st = (SceneMenuState *)s->state;
    if (!st)
        return;
    struct Services *svc = app_services();
    SDL_Texture *bg = texman_get(svc->texman, TEX_BG_STARFIELD);
    SDL_FRect dst = {0, 0, (float)svc->display_w, (float)svc->display_h};
    renderer_draw_texture(r, bg, NULL, &dst, 0);

    /* Title */
    int cx = svc->display_w / 2;
    int y = 60;
    renderer_draw_text_centered(r, "GRAVITY HUNTERS", (float)cx, (float)y, (TextStyle){0});

    /* Menu buttons: boxed centered column */
    int menu_y = 140;
    int spacing = 56;
    int box_w = 420;
    int box_h = 40;
    for (int i = 0; i < st->count; ++i)
    {
        const char *label = st->items[i].label;
        int enabled = st->items[i].enabled;
        int selected = (i == st->selected);
        float bx = (float)(cx - box_w / 2);
        float by = (float)(menu_y + i * spacing);
        SDL_FRect box = {bx, by, (float)box_w, (float)box_h};
        /* Draw opaque base for all buttons (keep uniform appearance regardless of enabled)
         * Slightly lighter so buttons read as interactive surfaces. */
        renderer_draw_filled_rect(r, box, (SDL_Color){34, 34, 34, 255});

        /* Selected: draw outline to indicate selection — always visible, even for disabled items. */
        if (selected)
        {
            /* Force opaque drawing for the outline so it appears clearly for all items.
             * Save/restore the renderer blend mode around the outline draw. */
            if (st->svc && st->svc->sdl_renderer)
            {
                SDL_BlendMode prev = SDL_BLENDMODE_BLEND;
                SDL_GetRenderDrawBlendMode(st->svc->sdl_renderer, &prev);
                SDL_SetRenderDrawBlendMode(st->svc->sdl_renderer, SDL_BLENDMODE_NONE);
                renderer_draw_rect_outline(r, box, (SDL_Color){255, 200, 80, 255}, 2);
                SDL_SetRenderDrawBlendMode(st->svc->sdl_renderer, prev);
            }
            else
            {
                renderer_draw_rect_outline(r, box, (SDL_Color){255, 200, 80, 255}, 1);
            }
        }
        /* No debug logging in release menu render. */
        renderer_draw_text_centered(r, label, (float)cx, by + 10.f, (TextStyle){0});
        /* no diagnostic marker */
    }
}
