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
#include "../core/types.h"

static char s_pending_level_filename[CAMPAIGN_LEVEL_MAX_FILENAME] = {0};

static char *overlay_campaign_details_strdup(const char *src)
{

    if (!src)
        return NULL;
    size_t len = strlen(src);
    char *dst = (char *)malloc(len + 1);
    if (!dst)
    {
        LOG_ERROR("overlay_campaign_details", "Failed to allocate string");
        return NULL;
    }
    memcpy(dst, src, len);
    dst[len] = '\0';
    return dst;
}

void overlay_campaign_details_prepare(const char *filename)
{

    if (filename && filename[0])
        snprintf(s_pending_level_filename, sizeof(s_pending_level_filename), "%s", filename);
    else
        s_pending_level_filename[0] = '\0';
}

static void overlay_campaign_details_load(OverlayCampaignDetailsState *st)
{

    if (!st)
        return;

    if (st->start_text)
    {
        free(st->start_text);
        st->start_text = NULL;
    }

    if (!s_pending_level_filename[0])
    {
        st->start_text = overlay_campaign_details_strdup("No level selected.");
        st->load_ok = 0;
        return;
    }

    snprintf(st->level_filename, sizeof(st->level_filename), "%s", s_pending_level_filename);

    GameLevel lvl = {0};
    char err[256] = {0};
    if (level_load(st->level_filename, &lvl, err, sizeof(err)) != 0)
    {
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

void overlay_campaign_details_enter(Scene *s)
{

    OverlayCampaignDetailsState *st = (OverlayCampaignDetailsState *)calloc(1, sizeof(OverlayCampaignDetailsState));
    if (!st)
    {
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

void overlay_campaign_details_leave(Scene *s)
{

    OverlayCampaignDetailsState *st = (OverlayCampaignDetailsState *)s->state;
    if (!st)
        return;
    if (st->start_text)
        free(st->start_text);
    free(st);
}

void overlay_campaign_details_handle_input(Scene *s, const struct InputState *in)
{

    OverlayCampaignDetailsState *st = (OverlayCampaignDetailsState *)s->state;
    if (!st || !in)
        return;

    int confirm = in->confirm ? 1 : 0;
    int back = in->back ? 1 : 0;

    if (st->wait_release)
    {
        if (!confirm && !back)
            st->wait_release = 0;
        st->prev_confirm = confirm;
        st->prev_back = back;
        return;
    }

    if (st->load_ok && confirm && !st->prev_confirm)
    {

        char level_to_load[CAMPAIGN_LEVEL_MAX_FILENAME];
        snprintf(level_to_load, sizeof(level_to_load), "%s", st->level_filename);
        scene_campaign_set_level(level_to_load);
        app_pop_overlay();
        app_set_scene(SCENE_CAMPAIGN);
        return;
    }

    if (back && !st->prev_back)
    {

        scene_campaign_menu_suppress_input(4);
        app_pop_overlay();
        return;
    }

    st->prev_confirm = confirm;
    st->prev_back = back;
}

void overlay_campaign_details_update(Scene *s, float dt)
{

    (void)s;
    (void)dt;
}

static float overlay_campaign_details_render_start_text(const OverlayCampaignDetailsState *st, Renderer *r, float x, float y, float wrap_width)
{

    if (!st || !r)
        return y;

    const char *text = st->start_text ? st->start_text : "";
    if (!text[0] || wrap_width <= 0.f)
        return y;

    TextStyle style = {
        .font_path = NULL,
        .size_px = 0,
        .wrap_w = (int)wrap_width};

    SDL_Texture *tex = NULL;
    SDL_Point size = {0, 0};
    if (!renderer_get_text_texture(r, text, style, &tex, &size) || !tex)
        return y;

    SDL_FRect dst = {x, y, (float)size.x, (float)size.y};
    SDL_RenderCopyF(r->sdl, tex, NULL, &dst);
    return y + (float)size.y;
}

static void overlay_campaign_details_render_icon_row(Renderer *r, SDL_Texture *tex, SDL_Rect src, float x, float y, float icon_h, float text_spacing, const char *text)
{

    if (!r || !tex || src.w <= 0 || src.h <= 0)
        return;

    float icon_w = icon_h * ((float)src.w / (float)src.h);
    SDL_FRect dst = {x, y, icon_w, icon_h

    };
    renderer_draw_texture(r, tex, &src, &dst, 0.f);

    if (text && text[0])
    {

        SDL_Point text_size = renderer_measure_text(r, text, (TextStyle){0

                                                             });
        float text_y = y;
        if (text_size.y > 0)
            text_y = y + (icon_h - (float)text_size.y) * 0.5f;
        renderer_draw_text(r, text, dst.x + dst.w + text_spacing, text_y + 3, (TextStyle){0});
    }
}

void overlay_campaign_details_render(Scene *s, struct Renderer *r)
{

    OverlayCampaignDetailsState *st = (OverlayCampaignDetailsState *)s->state;
    if (!st || !r)
        return;
    struct Services *svc = st->svc;
    if (!svc)
        return;

    SDL_FRect backdrop = {0.f, 0.f, (float)svc->display_w, (float)svc->display_h

    };
    renderer_draw_filled_rect(r, backdrop, OVERLAY_BACKDROP_COLOR);

    float panel_w = (float)svc->display_w * 0.75f;
    if (panel_w > 700.f)
        panel_w = 700.f;
    float panel_h = (float)svc->display_h * 0.65f;
    if (panel_h > 480.f)
        panel_h = 480.f;
    float panel_x = ((float)svc->display_w - panel_w) * 0.5f;
    float panel_y = ((float)svc->display_h - panel_h) * 0.5f;
    SDL_FRect panel = {panel_x, panel_y, panel_w, panel_h};
    renderer_draw_filled_rect(r, panel, MENU_COLOR_TEXT_BACKGROUND_DARKER);

    const float title_y = panel_y + 26.f;
    char level_label[CAMPAIGN_LEVEL_MAX_DISPLAY_NAME] = {0};
    campaign_level_format_display_name(st->level_filename, level_label, sizeof(level_label));
    if (!level_label[0] && st->level_filename[0])
    {
        size_t len = strlen(st->level_filename);
        size_t copy_len = len > sizeof(level_label) - 1 ? sizeof(level_label) - 1 : len;
        memcpy(level_label, st->level_filename, copy_len);
        level_label[copy_len] = '\0';
    }

    char display_name[CAMPAIGN_LEVEL_MAX_DISPLAY_NAME + 8] = {0};
    snprintf(display_name, sizeof(display_name), "Level: %s", level_label[0] ? level_label : "?");

    renderer_draw_text_centered(r, display_name, panel_x + panel_w * 0.5f, title_y, (TextStyle){

                                                                                        0

                                                                                    });

    float padding = 32.f;
    float text_x = panel_x + padding;
    float y = panel_y + 72.f;
    const float line_spacing = 28.f;

    float wrap_width = panel_w - (padding * 2);
    float after_text_y = overlay_campaign_details_render_start_text(st, r, text_x, y, wrap_width);
    if (after_text_y > y)
        y = after_text_y;
    y += line_spacing * 0.5f;

    SDL_Texture *icons_sheet = (svc && svc->texman) ? texman_get(svc->texman, TEX_ICONS_SHEET) : NULL;
    SDL_Rect icon_time = icons_sheet ? texman_sheet_src(svc->texman, TEX_ICONS_SHEET, 5) : (SDL_Rect){0};
    SDL_Rect icon_rating = icons_sheet ? texman_sheet_src(svc->texman, TEX_ICONS_SHEET, 4) : (SDL_Rect){0};
    SDL_Rect icon_goal = icons_sheet ? texman_sheet_src(svc->texman, TEX_ICONS_SHEET, 6) : (SDL_Rect){0};

    char time_value[64];
    if (st->time_limit != 0)
    {
        uint32_t limit = (uint32_t)st->time_limit;
        unsigned int minutes = limit / 60;
        unsigned int seconds = limit % 60;
        snprintf(time_value, sizeof(time_value), "%u:%02u", minutes, seconds);
    }
    else
    {
        snprintf(time_value, sizeof(time_value), "");
    }

    char rating_value[128];
    snprintf(rating_value, sizeof(rating_value), "%u|%u|%u", st->rating[0], st->rating[1], st->rating[2]);

    char rating_display[160];
    snprintf(rating_display, sizeof(rating_display), "%s", rating_value);

    char goal_value[64];
    snprintf(goal_value, sizeof(goal_value), "%u", (unsigned int)st->goal_kills);

    const float icon_height = 26.f;
    const float icon_text_spacing = 6.f;

    if (icons_sheet && icon_time.w > 0 && icon_time.h > 0)
    {
        overlay_campaign_details_render_icon_row(r, icons_sheet, icon_time, text_x, y, icon_height, icon_text_spacing, time_value);
        if (st->time_limit == 0)
        {
            // Hacky as fuck - no time! The competition is ending in 2 days :D
            SDL_Rect infinity_symbol = icons_sheet ? texman_sheet_src(svc->texman, TEX_ICONS_SHEET, 7) : (SDL_Rect){0};
            overlay_campaign_details_render_icon_row(r, icons_sheet, infinity_symbol, text_x + icon_time.w, y, icon_height, icon_text_spacing, "");
        }
    }

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
    SDL_Color button_color = st->load_ok ? MENU_COLOR_BUTTON_BASE : MENU_COLOR_BUTTON_INACTIVE;
    SDL_FRect button_rect = {button_x, button_y, button_w, button_h};
    renderer_draw_filled_rect(r, button_rect, button_color);
    renderer_draw_rect_outline(r, button_rect, MENU_COLOR_BUTTON_HIGHLIGHT, 2);
    renderer_draw_text_centered(r, "Start Mission", button_x + button_w * 0.5f, button_y + (button_h / 4) + 2, (TextStyle){0});

    ui_draw_prompts(r, svc, true, true);
}
