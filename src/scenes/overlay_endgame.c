#include "overlay_endgame.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../services/renderer.h"
#include "../services/services.h"
#include "../services/texture_manager.h"
#include "../app/app.h"
#include "../services/input.h"

/* forward decl */
static void overlay_endgame_copy_saved_to_state(struct OverlayEndgameState *st);

void overlay_endgame_enter(Scene *s)
{
    OverlayEndgameState *st = calloc(1, sizeof(OverlayEndgameState));
    s->state = st;
    s->is_overlay = true;
    s->blocks_under_input = true;
    s->blocks_under_update = true;
    st->svc = app_services();
    st->selected = 0;
    st->has_level_result = 0;
    overlay_endgame_copy_saved_to_state(st);
}
void overlay_endgame_leave(Scene *s)
{
    OverlayEndgameState *st = (OverlayEndgameState *)s->state;
    if (st)
        free(st);
}
void overlay_endgame_handle_input(Scene *s, const struct InputState *in)
{
    OverlayEndgameState *st = (OverlayEndgameState *)s->state;
    if (!st || !in)
        return;
    int confirm = in->confirm ? 1 : 0;
    if (confirm && !st->prev_confirm)
    {
        /* Ok pressed: go back to main menu and pop overlay */
        app_set_scene(SCENE_MENU);
        app_pop_overlay();
        return;
    }
    st->prev_confirm = confirm;
}
void overlay_endgame_update(Scene *s, float dt)
{
    (void)s; (void)dt;
}
void overlay_endgame_render(Scene *s, struct Renderer *r)
{
    OverlayEndgameState *st = (OverlayEndgameState *)s->state;
    if (!st)
        return;
    struct Services *svc = st->svc;
    /* Draw a semi-transparent black rectangle behind the overlay (50% alpha) */
    SDL_FRect dst = {0, 0, (float)svc->display_w, (float)svc->display_h};
    renderer_draw_filled_rect(r, dst, (SDL_Color){0, 0, 0, (Uint8)(0.50f * 255)});

    int cx = svc->display_w / 2;
    int title_y = 80;

    if (st->has_level_result && !st->goals_all_met)
    {
        renderer_draw_text_centered(r, "LEVEL FAILED", (float)cx, (float)title_y, (TextStyle){0});
        /* don't render stars when failed; optionally show stats */
        int y = 140;
        char buf[128];
        renderer_draw_text_centered(r, "Final Stats", (float)cx, (float)y, (TextStyle){0});
        y += 36;
        snprintf(buf, sizeof(buf), "Kills: %d", st->kills);
        renderer_draw_text_centered(r, buf, (float)cx, (float)y, (TextStyle){0});
        y += 28;
        snprintf(buf, sizeof(buf), "Deaths: %d", st->deaths);
        renderer_draw_text_centered(r, buf, (float)cx, (float)y, (TextStyle){0});
        y += 28;
        snprintf(buf, sizeof(buf), "Points: %d", st->points);
        renderer_draw_text_centered(r, buf, (float)cx, (float)y, (TextStyle){0});
    }
    else if (st->has_level_result && st->goals_all_met)
    {
        renderer_draw_text_centered(r, "LEVEL FINISHED", (float)cx, (float)title_y, (TextStyle){0});
        /* compute stars: compare st->points against rating thresholds; rating[0]..[2]
         * We render 3 symbols: 'O' for obtained, 'o' for not obtained. */
        int stars = 0;
        if (st->points >= (int)st->rating[2])
            stars = 3;
        else if (st->points >= (int)st->rating[1])
            stars = 2;
        else if (st->points >= (int)st->rating[0])
            stars = 1;

        /* Render stars as text centered underneath the title. */
        int y = 140;
        char stars_buf[8] = {0};
        for (int i = 0; i < 3; ++i)
            stars_buf[i] = (i < stars) ? 'O' : 'o';
        renderer_draw_text_centered(r, stars_buf, (float)cx, (float)y, (TextStyle){0});

        /* show points too */
        char buf[128];
        y += 36;
        snprintf(buf, sizeof(buf), "Points: %d", st->points);
        renderer_draw_text_centered(r, buf, (float)cx, (float)y, (TextStyle){0});
    }
    else
    {
        /* fallback: no level result provided (e.g. quickplay). Show generic summary */
        renderer_draw_text_centered(r, "GAME OVER", (float)cx, (float)title_y, (TextStyle){0});
        int y = 140;
        char buf[128];
        renderer_draw_text_centered(r, "Final Stats", (float)cx, (float)y, (TextStyle){0});
        y += 36;
        snprintf(buf, sizeof(buf), "Kills: %d", st->kills);
        renderer_draw_text_centered(r, buf, (float)cx, (float)y, (TextStyle){0});
        y += 28;
        snprintf(buf, sizeof(buf), "Deaths: %d", st->deaths);
        renderer_draw_text_centered(r, buf, (float)cx, (float)y, (TextStyle){0});
        y += 28;
        snprintf(buf, sizeof(buf), "Points: %d", st->points);
        renderer_draw_text_centered(r, buf, (float)cx, (float)y, (TextStyle){0});
    }

    /* single Ok button */
    int box_w = 240;
    int box_h = 44;
    float bx = (float)(cx - box_w / 2);
    float by = (float)(svc->display_h - 120);
    SDL_FRect box = {bx, by, (float)box_w, (float)box_h};
    renderer_draw_filled_rect(r, box, (SDL_Color){34, 34, 34, 255});
    renderer_draw_rect_outline(r, box, (SDL_Color){255, 200, 80, 255}, 2);
    renderer_draw_text_centered(r, "Ok", (float)cx, by + 10.f, (TextStyle){0});
}

void overlay_endgame_set_stats(int kills, int deaths, int points)
{
    /* store into global overlay state singleton-ish: try to find existing overlay state
     * if present, set values; otherwise set defaults via static globals. For simplicity,
     * set static globals and also update current scene state if active. */
    Scene *top = NULL; /* we don't have direct access to scene stack here; update current instance if present */
    /* Fallback: update global stat store (per overlay instance on enter it's copied) */
    /* TODO: keep simple: use file-level static hard store */
    extern void overlay_endgame_set_stats_internal(int, int, int);
    overlay_endgame_set_stats_internal(kills, deaths, points);
}

/* Internal static storage and helper to allow overlay_endgame_set_stats to work before overlay is created. */
static int s_saved_kills = 0;
static int s_saved_deaths = 0;
static int s_saved_points = 0;
void overlay_endgame_set_stats_internal(int kills, int deaths, int points)
{
    s_saved_kills = kills;
    s_saved_deaths = deaths;
    s_saved_points = points;
}

void overlay_endgame_set_campaign_result(int goals_all_met, unsigned int rating0, unsigned int rating1, unsigned int rating2)
{
    /* store into saved area; when overlay enters it will copy these */
    extern void overlay_endgame_set_campaign_result_internal(int, unsigned int, unsigned int, unsigned int);
    overlay_endgame_set_campaign_result_internal(goals_all_met, rating0, rating1, rating2);
}

static int s_saved_goals_all_met = 0;
static unsigned int s_saved_rating[3] = {0,0,0};
void overlay_endgame_set_campaign_result_internal(int goals_all_met, unsigned int r0, unsigned int r1, unsigned int r2)
{
    s_saved_goals_all_met = goals_all_met ? 1 : 0;
    s_saved_rating[0] = r0;
    s_saved_rating[1] = r1;
    s_saved_rating[2] = r2;
}

/* When overlay enters, copy saved values into state */
static void overlay_endgame_copy_saved_to_state(OverlayEndgameState *st)
{
    if (!st)
        return;
    st->kills = s_saved_kills;
    st->deaths = s_saved_deaths;
    st->points = s_saved_points;
    st->has_level_result = s_saved_goals_all_met >= 0 ? 1 : 0; // we always set it via setter when needed
    st->goals_all_met = s_saved_goals_all_met;
    st->rating[0] = s_saved_rating[0];
    st->rating[1] = s_saved_rating[1];
    st->rating[2] = s_saved_rating[2];
}

/* Ensure that overlay_endgame_enter copies saved data */
/* We'll patch overlay_endgame_enter above to call overlay_endgame_copy_saved_to_state after creating state. */

