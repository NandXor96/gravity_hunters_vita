#include "overlay_endgame.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../services/renderer.h"
#include "../services/services.h"
#include "../services/texture_manager.h"
#include "../app/app.h"
#include "../services/input.h"
#include "../game/campaign_levels.h"
#include "../game/campaign_progress.h"
#include "scene_campaign_menu.h"
#include "../core/log.h"
#include "../core/types.h"

/* forward decl */
static void overlay_endgame_copy_saved_to_state(struct OverlayEndgameState *st);
static void overlay_endgame_record_campaign_progress(const OverlayEndgameState *st);

static int s_saved_kills = 0;
static int s_saved_points = 0;
static int s_saved_goals_all_met = -1;
static unsigned int s_saved_rating[3] = {0, 0, 0};
static char s_saved_level_filename[CAMPAIGN_LEVEL_MAX_FILENAME] = {0};
static OverlayEndgameFailReason s_saved_fail_reason = OVERLAY_ENDGAME_FAIL_NONE;

static void overlay_endgame_draw_star_row(Renderer *r, SDL_Texture *sheet, SDL_Rect src_filled, SDL_Rect src_empty, float cx, float top_y, int earned) {

    const int total_slots = 3;
    const float spacing = 10.f;
    if (!r)
        return;

    if (sheet && src_filled.w > 0 && src_filled.h > 0 && src_empty.w > 0 && src_empty.h > 0) {
        float icon_size = (float)src_filled.w;
        float total_width = (float)total_slots * icon_size + (float)(total_slots - 1) * spacing;
        float start_x = cx - total_width * 0.5f;
        for (int i = 0; i < total_slots; ++i) {
            const SDL_Rect *src = (i < earned) ? &src_filled : &src_empty;
            SDL_FRect dst = {start_x + i * (icon_size + spacing), top_y, icon_size, icon_size

};
            renderer_draw_texture(r, sheet, src, &dst, 0.f);
        }
    }
    else {
        char fallback[32];
        snprintf(fallback, sizeof(fallback), "Stars: %d/%d", earned, total_slots);
        renderer_draw_text_centered(r, fallback, cx, top_y, (TextStyle) {
            0
        });
    }
}

void overlay_endgame_enter(Scene *s) {

    OverlayEndgameState *st = calloc(1, sizeof(OverlayEndgameState));
    if (!st) {
        LOG_ERROR("overlay_endgame", "Failed to allocate overlay state");
        return;

}
    s->state = st;
    s->is_overlay = true;
    s->blocks_under_input = true;
    s->blocks_under_update = true;
    st->svc = app_services();
    st->selected = 0;
    st->has_level_result = 0;
    overlay_endgame_copy_saved_to_state(st);
    overlay_endgame_record_campaign_progress(st);
}
void overlay_endgame_leave(Scene *s) {
    OverlayEndgameState *st = (OverlayEndgameState *)s->state;
    if (st)
        free(st);
}
void overlay_endgame_handle_input(Scene *s, const struct InputState *in) {
    OverlayEndgameState *st = (OverlayEndgameState *)s->state;
    if (!st || !in)
        return;
    int confirm = in->confirm ? 1 : 0;
    if (confirm && !st->prev_confirm) {
        if (st->has_level_result) {
            scene_campaign_menu_focus_last_unlocked();
            app_set_scene(SCENE_CAMPAIGN_MENU);
}
        else {
            app_set_scene(SCENE_MENU);
        }
        app_pop_overlay();
        return;
    }
    st->prev_confirm = confirm;
}
void overlay_endgame_update(Scene *s, float dt) {
    (void)s;
    (void)dt;
}
void overlay_endgame_render(Scene *s, struct Renderer *r) {
    OverlayEndgameState *st = (OverlayEndgameState *)s->state;
    if (!st)
        return;
    struct Services *svc = st->svc;
    /* Draw a semi-transparent black rectangle behind the overlay (50% alpha) */
    SDL_FRect dst = {0, 0, (float)svc->display_w, (float)svc->display_h
};
    renderer_draw_filled_rect(r, dst, OVERLAY_BACKDROP_COLOR);

    int cx = svc->display_w / 2;
    int title_y = 80;

    SDL_Texture *icons_sheet = texman_get(svc->texman, TEX_ICONS_SHEET);
    SDL_Rect star_src_filled = icons_sheet ? texman_sheet_src(svc->texman, TEX_ICONS_SHEET, 4) : (SDL_Rect){0};
    SDL_Rect star_src_empty = icons_sheet ? texman_sheet_src(svc->texman, TEX_ICONS_SHEET, 8) : (SDL_Rect){0};
    SDL_Rect kill_icon_src = icons_sheet ? texman_sheet_src(svc->texman, TEX_ICONS_SHEET, 6) : (SDL_Rect){0};

    int stars = 0;
    if (st->points >= (int)st->rating[2])
        stars = 3;
    else if (st->points >= (int)st->rating[1])
        stars = 2;
    else if (st->points >= (int)st->rating[0])
        stars = 1;

    int cleared = st->goals_all_met ? 1 : 0;
    int earned_stars = cleared ? stars : 0;
    int success = (cleared && earned_stars) ? 1 : 0;
    const char *reason_text = NULL;
    if (!success) {
        switch (st->fail_reason) {
        case OVERLAY_ENDGAME_FAIL_KILLS:
            reason_text = "Not enough kills";
            break;
        case OVERLAY_ENDGAME_FAIL_DEATH:
            reason_text = "You died";
            break;
        case OVERLAY_ENDGAME_FAIL_SCORE:
            reason_text = "Your score is too low";
            break;
        default:
            break;
        }
    }

    float content_y = (float)title_y + 40.f;
    if (reason_text)
        content_y = (float)title_y + 56.f;

    float star_height = (float)((star_src_filled.h > 0) ? star_src_filled.h : 32);
    float points_y = content_y + star_height + 10.f;

    float panel_top = (float)title_y - 32.f;
    float panel_bottom = points_y + 55.f;
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

    renderer_draw_text_centered(r, success ? "LEVEL CLEARED" : "LEVEL FAILED", (float)cx, (float)title_y, (TextStyle){0});

    if (reason_text)
        renderer_draw_text_centered(r, reason_text, (float)cx, (float)(title_y + 24.f), (TextStyle){0});

    overlay_endgame_draw_star_row(r, icons_sheet, star_src_filled, star_src_empty, (float)cx, content_y, earned_stars);

    char points_buf[32];
    snprintf(points_buf, sizeof(points_buf), "%d", st->points);
    renderer_draw_text_centered(r, points_buf, (float)cx, points_y, (TextStyle){0});

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

void overlay_endgame_set_stats(int kills, int points) {

    s_saved_kills = kills;
    s_saved_points = points;

}

void overlay_endgame_set_campaign_result(int goals_all_met, unsigned int rating0, unsigned int rating1, unsigned int rating2) {

    s_saved_goals_all_met = goals_all_met ? 1 : 0;
    s_saved_rating[0] = rating0;
    s_saved_rating[1] = rating1;
    s_saved_rating[2] = rating2;

}

void overlay_endgame_set_campaign_level(const char *filename) {

    if (!filename || !filename[0]) {
        s_saved_level_filename[0] = '\0';
        return;

}
    snprintf(s_saved_level_filename, sizeof(s_saved_level_filename), "%s", filename);
}

/* When overlay enters, copy saved values into state */
static void overlay_endgame_copy_saved_to_state(OverlayEndgameState *st) {
    if (!st)
        return;

    st->kills = s_saved_kills;
    st->points = s_saved_points;

    st->has_level_result = (s_saved_goals_all_met >= 0) ? 1 : 0;
    st->goals_all_met = (s_saved_goals_all_met > 0) ? 1 : 0;
    st->fail_reason = s_saved_fail_reason;

    if (st->has_level_result) {
        st->rating[0] = s_saved_rating[0];
        st->rating[1] = s_saved_rating[1];
        st->rating[2] = s_saved_rating[2];
    }
    else {
        st->rating[0] = st->rating[1] = st->rating[2] = 0;
    }
}

void overlay_endgame_set_failure_reason(OverlayEndgameFailReason reason) {

    s_saved_fail_reason = reason;
}

static void overlay_endgame_record_campaign_progress(const OverlayEndgameState *st) {

    if (!st)
        return;
    if (!st->has_level_result || !st->goals_all_met)
        return;
    if (!s_saved_level_filename[0])
        return;

    int stars = 0;
    if (st->points >= (int)st->rating[2])
        stars = 3;
    else if (st->points >= (int)st->rating[1])
        stars = 2;
    else if (st->points >= (int)st->rating[0])
        stars = 1;

    if (stars <= 0)
        return;

    CampaignProgress *progress = campaign_progress_data();
    if (!progress)
        return;

    int updated = campaign_progress_update(progress, s_saved_level_filename, (uint8_t)stars, st->points);
    if (updated > 0) {
        if (campaign_progress_save(progress, NULL) != 0) {
            LOG_WARN("overlay_endgame", "Failed to persist campaign progress");
        }
    }

}
