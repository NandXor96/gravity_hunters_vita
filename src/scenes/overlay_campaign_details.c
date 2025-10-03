#include "overlay_campaign_details.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "scene_campaign.h"
#include "scene_campaign_menu.h"
#include "../app/app.h"
#include "../services/input.h"
#include "../services/renderer.h"
#include "../services/services.h"
#include "../services/texture_manager.h"
#include "../game/level_loader.h"
#include "../game/ui_prompts.h"
#include "../core/log.h"

static char s_pending_level_filename[CAMPAIGN_LEVEL_MAX_FILENAME] = {0};

static char *overlay_campaign_details_strdup(const char *src) {

    if (!src)
        return NULL;
    size_t len = strlen(src);
    char *dst = (char *)malloc(len + 1);
    if (!dst) {
        LOG_ERROR("overlay_campaign_details", "Failed to allocate string");
        return NULL;

}
    memcpy(dst, src, len);
    dst[len] = '\0';
    return dst;
}

void overlay_campaign_details_prepare(const char *filename) {

    if (filename && filename[0])
        snprintf(s_pending_level_filename, sizeof(s_pending_level_filename), "%s", filename);
    else
        s_pending_level_filename[0] = '\0';

}

static void overlay_campaign_details_load(OverlayCampaignDetailsState *st) {

    if (!st)
        return;

    if (!s_pending_level_filename[0]) {
        st->start_text = overlay_campaign_details_strdup("No level selected.");
        st->load_ok = 0;
        return;

}

    snprintf(st->level_filename, sizeof(st->level_filename), "%s", s_pending_level_filename);

    GameLevel lvl = {0};
    char err[256] = {0};
    if (level_load(st->level_filename, &lvl, err, sizeof(err)) != 0) {
        st->start_text = overlay_campaign_details_strdup(err[0] ? err : "Failed to load level data.");
        st->load_ok = 0;
        level_free(&lvl);
        return;
    }

    st->time_limit = lvl.time_limit;
    st->goal_kills = lvl.goal_kills;
    st->rating[0] = lvl.rating[0];
    st->rating[1] = lvl.rating[1];
    st->rating[2] = lvl.rating[2];
    st->start_text = overlay_campaign_details_strdup(lvl.start_text && lvl.start_text[0] ? lvl.start_text : "Ready?");
    st->load_ok = 1;

    level_free(&lvl);
}

void overlay_campaign_details_enter(Scene *s) {

    OverlayCampaignDetailsState *st = (OverlayCampaignDetailsState *)calloc(1, sizeof(OverlayCampaignDetailsState));
    if (!st) {
        LOG_ERROR("overlay_campaign_details", "Failed to allocate overlay state");
        return;

}

    s->state = st;
    s->is_overlay = true;
    s->blocks_under_input = true;
    s->blocks_under_update = true;

    st->svc = app_services();

    overlay_campaign_details_load(st);

    st->wait_release = 1;
    st->prev_confirm = 1;
    st->prev_back = 1;
}

void overlay_campaign_details_leave(Scene *s) {

    OverlayCampaignDetailsState *st = (OverlayCampaignDetailsState *)s->state;
    if (!st)
        return;
    if (st->start_text)
        free(st->start_text);
    free(st);

}

void overlay_campaign_details_handle_input(Scene *s, const struct InputState *in) {

    OverlayCampaignDetailsState *st = (OverlayCampaignDetailsState *)s->state;
    if (!st || !in)
        return;

    int confirm = in->confirm ? 1 : 0;
    int back = in->back ? 1 : 0;

    if (st->wait_release) {
        if (!confirm && !back)
            st->wait_release = 0;
        st->prev_confirm = confirm;
        st->prev_back = back;
        return;

}

    if (st->load_ok && confirm && !st->prev_confirm) {

        char level_to_load[CAMPAIGN_LEVEL_MAX_FILENAME];
        snprintf(level_to_load, sizeof(level_to_load), "%s", st->level_filename);
        scene_campaign_set_level(level_to_load);
        app_pop_overlay();
        app_set_scene(SCENE_CAMPAIGN);
        return;

    }

    if (back && !st->prev_back) {

        scene_campaign_menu_suppress_input(4);
        app_pop_overlay();
        return;

    }

    st->prev_confirm = confirm;
    st->prev_back = back;
}

void overlay_campaign_details_update(Scene *s, float dt) {

    (void)s;
    (void)dt;

}

static float overlay_campaign_details_render_multiline(Renderer *r, float x, float y, float line_spacing, const char *text, const float max_line_width) {

    if (!text || !text[0])
        return y;

    const char *ptr = text;
    float current_y = y;

    while (*ptr) {
        // Find explicit newline
        const char *nl = strchr(ptr, '\n');
        size_t len = nl ? (size_t)(nl - ptr) : strlen(ptr);

        // Try to fit as much as possible within max_line_width
        size_t line_end = 0;
        size_t last_space = 0;
        for (size_t i = 0; i < len; ++i) {
            if (ptr[i] == ' ')
                last_space = i;
            char temp[512];
            size_t temp_len = i + 1;
            if (temp_len >= sizeof(temp))
                break;
            memcpy(temp, ptr, temp_len);
            temp[temp_len] = '\0';
            SDL_Point sz = renderer_measure_text(r, temp, (TextStyle){0

});
            if (sz.x > max_line_width) {
                line_end = last_space ? last_space : i;
                break;
            }
        }
        if (line_end == 0 && len > 0)
            line_end = len;

        // Draw the line
        char *line = (char *)malloc(line_end + 1);
        if (!line) {
            LOG_ERROR("overlay_campaign_details", "Failed to allocate line buffer");
            break;
        }
        memcpy(line, ptr, line_end);
        line[line_end] = '\0';
        renderer_draw_text(r, line, x, current_y, (TextStyle) {
            0
        });
        free(line);
        current_y += line_spacing;

        ptr += line_end;
        while (*ptr == ' ') ptr++; // Skip spaces

        // If explicit newline, skip it
        if (nl && ptr >= nl)
            ptr = nl + 1;
    }
    return current_y;
}

static void overlay_campaign_details_render_icon_row(Renderer *r, SDL_Texture *tex, SDL_Rect src, float x, float y, float icon_h, float text_spacing, const char *text) {

    if (!r || !tex || src.w <= 0 || src.h <= 0)
        return;

    float icon_w = icon_h * ((float)src.w / (float)src.h);
    SDL_FRect dst = {x, y, icon_w, icon_h

};
    renderer_draw_texture(r, tex, &src, &dst, 0.f);

    if (text && text[0]) {

        SDL_Point text_size = renderer_measure_text(r, text, (TextStyle){0

    });
        float text_y = y;
        if (text_size.y > 0)
            text_y = y + (icon_h - (float)text_size.y) * 0.5f;
        renderer_draw_text(r, text, dst.x + dst.w + text_spacing, text_y + 3, (TextStyle) {
            0
        });
    }
}

void overlay_campaign_details_render(Scene *s, struct Renderer *r) {

    OverlayCampaignDetailsState *st = (OverlayCampaignDetailsState *)s->state;
    if (!st || !r)
        return;
    struct Services *svc = st->svc;
    if (!svc)
        return;

    SDL_FRect backdrop = {0.f, 0.f, (float)svc->display_w, (float)svc->display_h

};
    renderer_draw_filled_rect(r, backdrop, (SDL_Color) {
        0, 0, 0, 160
    });

    float panel_w = (float)svc->display_w * 0.75f;
    if (panel_w > 700.f)
        panel_w = 700.f;
    float panel_h = (float)svc->display_h * 0.65f;
    if (panel_h > 480.f)
        panel_h = 480.f;
    float panel_x = ((float)svc->display_w - panel_w) * 0.5f;
    float panel_y = ((float)svc->display_h - panel_h) * 0.5f;
    SDL_FRect panel = {panel_x, panel_y, panel_w, panel_h};
    renderer_draw_filled_rect(r, panel, (SDL_Color) {
        24, 24, 24, 235
    });

    const float title_y = panel_y + 26.f;
    char display_name[CAMPAIGN_LEVEL_MAX_FILENAME + 3] = {0};
    size_t len = strlen(st->level_filename);

    snprintf(display_name, sizeof(display_name), "Level: %.*s", (int)(len - 4), st->level_filename);

    renderer_draw_text_centered(r, display_name, panel_x + panel_w * 0.5f, title_y, (TextStyle) {

        0

    });

    float padding = 32.f;
    float text_x = panel_x + padding;
    float y = panel_y + 72.f;
    const float line_spacing = 28.f;

    y = overlay_campaign_details_render_multiline(r, text_x, y, line_spacing, st->start_text ? st->start_text : "", panel_w - (padding * 2));
    y += line_spacing * 0.5f;

    SDL_Texture *icons_sheet = (svc && svc->texman) ? texman_get(svc->texman, TEX_ICONS_SHEET) : NULL;
    SDL_Rect icon_time = icons_sheet ? texman_sheet_src(svc->texman, TEX_ICONS_SHEET, 5) : (SDL_Rect){0};
    SDL_Rect icon_rating = icons_sheet ? texman_sheet_src(svc->texman, TEX_ICONS_SHEET, 4) : (SDL_Rect){0};
    SDL_Rect icon_goal = icons_sheet ? texman_sheet_src(svc->texman, TEX_ICONS_SHEET, 6) : (SDL_Rect){0};

    char time_value[64];
    unsigned int minutes = st->time_limit / 60;
    unsigned int seconds = st->time_limit % 60;
    snprintf(time_value, sizeof(time_value), "%u:%02u", minutes, seconds);

    char rating_value[128];
    snprintf(rating_value, sizeof(rating_value), "%u|%u|%u", st->rating[0], st->rating[1], st->rating[2]);

    char rating_display[160];
    snprintf(rating_display, sizeof(rating_display), "%s", rating_value);

    char goal_value[64];
    snprintf(goal_value, sizeof(goal_value), "%u", (unsigned int)st->goal_kills);

    const float icon_height = 26.f;
    const float icon_text_spacing = 6.f;

    if (icons_sheet && icon_time.w > 0 && icon_time.h > 0)
        overlay_campaign_details_render_icon_row(r, icons_sheet, icon_time, text_x, y, icon_height, icon_text_spacing, time_value);

    y += line_spacing;

    if (icons_sheet && icon_rating.w > 0 && icon_rating.h > 0)
        overlay_campaign_details_render_icon_row(r, icons_sheet, icon_rating, text_x, y, icon_height, icon_text_spacing, rating_display);

    y += line_spacing;

    if (icons_sheet && icon_goal.w > 0 && icon_goal.h > 0)
        overlay_campaign_details_render_icon_row(r, icons_sheet, icon_goal, text_x, y, icon_height, icon_text_spacing, goal_value);

    float button_w = 260.f;
    float button_h = 48.f;
    float button_x = panel_x + panel_w * 0.5f - button_w * 0.5f;
    float button_y = panel_y + panel_h + 16.f;
    SDL_Color button_color = st->load_ok ? (SDL_Color){34, 34, 34, 255} : (SDL_Color){60, 60, 60, 255};
    SDL_FRect button_rect = {button_x, button_y, button_w, button_h};
    renderer_draw_filled_rect(r, button_rect, button_color);
    renderer_draw_rect_outline(r, button_rect, (SDL_Color) {
        255, 200, 80, 255
    }, 2);
    renderer_draw_text_centered(r, "Start Mission", button_x + button_w * 0.5f, button_y + (button_h / 4) + 2, (TextStyle){0});

    ui_draw_prompts(r, svc, true, true);
}
