#include "overlay_pause.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../services/renderer.h"
#include "../services/services.h"
#include "../services/texture_manager.h"
#include "../app/app.h"

void overlay_pause_enter(Scene *s)
{
    OverlayPauseState *st = calloc(1, sizeof(OverlayPauseState));
    s->state = st;
    s->is_overlay = true;
    s->blocks_under_input = true;
    s->blocks_under_update = true;
    st->svc = app_services();
    st->selected = 0;
}
void overlay_pause_leave(Scene *s)
{
    OverlayPauseState *st = (OverlayPauseState *)s->state;
    if (st)
        free(st);
}
void overlay_pause_handle_input(Scene *s, const struct InputState *in)
{
    OverlayPauseState *st = (OverlayPauseState *)s->state;
    if (!st || !in)
        return;
    int up = in->move_up ? 1 : 0;
    int down = in->move_down ? 1 : 0;
    int confirm = in->confirm ? 1 : 0;
    int up_pressed = (up && !st->prev_move_up) || in->speed_up_step;
    int down_pressed = (down && !st->prev_move_down) || in->speed_down_step;
    if (up_pressed)
        st->selected = (st->selected - 1 + 2) % 2;
    if (down_pressed)
        st->selected = (st->selected + 1) % 2;
    if (confirm && !st->prev_confirm)
    {
        if (st->selected == 0)
        {
            /* Continue: pop overlay */
            app_pop_overlay();
            return;
        }
        else if (st->selected == 1)
        {
            /* Exit: return to main menu and remove overlay */
            app_set_scene(SCENE_MENU);
            app_pop_overlay();
            return;
        }
    }
    st->prev_move_up = up;
    st->prev_move_down = down;
    st->prev_confirm = confirm;
}
void overlay_pause_update(Scene *s, float dt)
{
    (void)s;
    (void)dt;
}
void overlay_pause_render(Scene *s, struct Renderer *r)
{
    OverlayPauseState *st = (OverlayPauseState *)s->state;
    if (!st)
        return;
    struct Services *svc = st->svc;
    /* Draw a semi-transparent black rectangle behind the overlay (40% alpha) */
    SDL_FRect dst = {0, 0, (float)svc->display_w, (float)svc->display_h};
    renderer_draw_filled_rect(r, dst, (SDL_Color){0, 0, 0, (Uint8)(0.40f * 255)});

    int cx = svc->display_w / 2;
    int title_y = 80;
    renderer_draw_text_centered(r, "PAUSE", (float)cx, (float)title_y, (TextStyle){0});

    int menu_y = 140;
    int spacing = 56;
    int box_w = 360;
    int box_h = 40;
    const char *labels[2] = {"Continue", "Exit"};
    for (int i = 0; i < 2; ++i)
    {
        int selected = (i == st->selected);
        float bx = (float)(cx - box_w / 2);
        float by = (float)(menu_y + i * spacing);
        SDL_FRect box = {bx, by, (float)box_w, (float)box_h};
        renderer_draw_filled_rect(r, box, (SDL_Color){34, 34, 34, 255});
        if (selected)
        {
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
        renderer_draw_text_centered(r, labels[i], (float)cx, by + 10.f, (TextStyle){0});
    }
}
