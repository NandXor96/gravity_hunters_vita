#include "overlay_endgame_qp.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../services/renderer.h"
#include "../services/services.h"
#include "../services/texture_manager.h"
#include "../app/app.h"
#include "../services/input.h"
#include "../core/log.h"
#include "../core/types.h"

/* Stored stats populated via overlay_endgame_qp_set_stats() */
static int g_kills_qp = 0;
static int g_points_qp = 0;

void overlay_endgame_qp_enter(Scene *s) {

    OverlayEndgameStateQP *st = calloc(1, sizeof(OverlayEndgameStateQP));
    if (!st) {
        LOG_ERROR("overlay_endgame_qp", "Failed to allocate overlay state");
        return;

}
    s->state = st;
    s->is_overlay = true;
    s->blocks_under_input = true;
    s->blocks_under_update = true;
    st->svc = app_services();
    st->selected = 0;
}
void overlay_endgame_qp_leave(Scene *s) {
    OverlayEndgameStateQP *st = (OverlayEndgameStateQP *)s->state;
    if (st)
        free(st);
}
void overlay_endgame_qp_handle_input(Scene *s, const struct InputState *in) {
    OverlayEndgameStateQP *st = (OverlayEndgameStateQP *)s->state;
    if (!st || !in)
        return;
    int confirm = in->confirm ? 1 : 0;
    if (confirm && !st->prev_confirm) {
        /* Ok pressed: go back to main menu and pop overlay */
        app_set_scene(SCENE_MENU);
        app_pop_overlay();
        return;
}
    st->prev_confirm = confirm;
}
void overlay_endgame_qp_update(Scene *s, float dt) {
    (void)s; (void)dt;
}
void overlay_endgame_qp_render(Scene *s, struct Renderer *r) {
    OverlayEndgameStateQP *st = (OverlayEndgameStateQP *)s->state;
    if (!st)
        return;
    struct Services *svc = st->svc;
    /* Draw a semi-transparent black rectangle behind the overlay (50% alpha) */
    SDL_FRect dst = {0, 0, (float)svc->display_w, (float)svc->display_h
};
    renderer_draw_filled_rect(r, dst, OVERLAY_BACKDROP_COLOR);

    int cx = svc->display_w / 2;
    int title_y = 80;

    int stats_title_y = 140;
    int kills_y = stats_title_y + 36;
    int points_y = kills_y + 28;

    float panel_top = (float)title_y - 32.f;
    float panel_bottom = (float)points_y + 55.f;
    float panel_width = (float)svc->display_w * 0.65f;
    float max_width = (float)svc->display_w - 80.f;
    if (max_width > 0.f && panel_width > max_width)
        panel_width = max_width;
    if ((float)svc->display_w >= 360.f && panel_width < 320.f)
        panel_width = 320.f;
    if (panel_width < 0.f)
        panel_width = 0.f;
    SDL_FRect panel_rect = {
        (float)cx - panel_width * 0.5f,
        panel_top,
        panel_width,
        panel_bottom - panel_top
    };
    if (panel_rect.h < 0.f)
        panel_rect.h = 0.f;
    renderer_draw_filled_rect(r, panel_rect, MENU_COLOR_TEXT_BACKGROUND);

    renderer_draw_text_centered(r, "GAME OVER", (float)cx, (float)title_y, (TextStyle){0});

    /* Use stored stats provided by scene before pushing overlay */
    char buf[128];
    int y = stats_title_y;
    renderer_draw_text_centered(r, "Final Stats", (float)cx, (float)y, (TextStyle){0});
    y += 36;
    snprintf(buf, sizeof(buf), "Kills: %d", g_kills_qp);
    renderer_draw_text_centered(r, buf, (float)cx, (float)y, (TextStyle){0});
    y += 28;
    snprintf(buf, sizeof(buf), "Points: %d", g_points_qp);
    renderer_draw_text_centered(r, buf, (float)cx, (float)y, (TextStyle){0});

    /* single Ok button */
    int box_w = 240;
    int box_h = 44;
    float bx = (float)(cx - box_w / 2);
    float by = (float)(svc->display_h - 120);
    SDL_FRect box = {bx, by, (float)box_w, (float)box_h};
    renderer_draw_filled_rect(r, box, MENU_COLOR_BUTTON_BASE);
    renderer_draw_rect_outline(r, box, MENU_COLOR_BUTTON_HIGHLIGHT, 2);
    renderer_draw_text_centered(r, "Ok", (float)cx, by + 10.f, (TextStyle){0});
}

void overlay_endgame_qp_set_stats(int kills, int points) {

    g_kills_qp = kills;
    g_points_qp = points;

}
