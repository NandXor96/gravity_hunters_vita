#include "hud.h"
#include "world.h"
#include "player.h"
#include "weapon.h"
#include "../services/services.h"
#include "../services/renderer.h"
#include <stdlib.h>
#include "../services/texture_manager.h"
#include <stdio.h>

#include "../core/types.h"
#include "../core/log.h"

// Forward static value & render helper
static float hud_value_shot_speed(struct World *w, void *user);
static float hud_value_weapon_cooldown(struct World *w, void *user);
static float hud_value_player_health(struct World *w, void *user);
static void hud_render_bar(Hud *h, HudBar *b, struct Renderer *r);
static void hud_render_stat(Hud *h, HudStat *s, struct Renderer *r);
static void hud_stat_update_time(HudStat *stat, struct World *w, void *user);
static void hud_stat_update_kills(HudStat *stat, struct World *w, void *user);
static void hud_stat_update_points(HudStat *stat, struct World *w, void *user);
int hud_bar_set_icon(Hud *h, int bar_index, SDL_Texture *tex, SDL_Rect src) {
    if (!h)
        return 0;
    if (bar_index < 0 || bar_index >= h->bar_count)
        return 0;
    HudBar *b = &h->bars[bar_index];
    b->icon_tex = tex;
    b->icon_src = src;
    b->has_icon = (tex && src.w > 0 && src.h > 0);
    return 1;
}

Hud *hud_create(struct Services *svc, Player *player) {

    Hud *h = calloc(1, sizeof(Hud));
    if (!h) {
        LOG_ERROR("hud", "Failed to allocate HUD");
        return NULL;
    }
    h->svc = svc;
    h->visible = true;
    h->bar_count = 0;
    h->player = player;
    h->player_bars_initialized = false;
    h->tex_hud = NULL;
    h->shot_speed_bar_index = -1;
    h->weapon_cd_bar_index = -1;
    h->health_bar_index = -1;
    h->stat_count = 0;
    // Build bars immediately if player valid
    if (player) {
        SDL_Color bg = {30, 30, 30, 160};
        SDL_Color fg = {120, 220, 120, 220};
        // width 180, height 14 (previous order was swapped)
        float bar_h = 18.f;
        float icon_w = bar_h; // square icon
        float spacing = 4.f;
        float base_y = (float)svc->display_h - bar_h - 8.f;
        // shift bar right to make space for icon
        h->shot_speed_bar_index = hud_add_bar(h, 8.f + icon_w + spacing, base_y, 180.f, bar_h, fg, bg, hud_value_shot_speed, player);
        // Cooldown bar placed above shot speed bar
        SDL_Color cd_fg = (SDL_Color){220, 160, 60, 220};
        SDL_Color cd_bg = (SDL_Color){40, 40, 40, 160};
        h->weapon_cd_bar_index = hud_add_bar(h, 8.f + icon_w + spacing, base_y - (bar_h + spacing), 180.f, bar_h, cd_fg, cd_bg, hud_value_weapon_cooldown, player);

        // Health bar centered bottom
        SDL_Color hp_fg = (SDL_Color){200, 50, 50, 220};
        SDL_Color hp_bg = (SDL_Color){40, 20, 20, 160};
        float hp_width = 240.f;
        float hp_height = 18.f;
        float hp_x = ((float)svc->display_w - hp_width) * 0.5f + hp_height + 4.f; // leave space for icon
        float hp_y = (float)svc->display_h - hp_height - 8.f;                     // same bottom margin
        h->health_bar_index = hud_add_bar(h, hp_x, hp_y, hp_width, hp_height, hp_fg, hp_bg, hud_value_player_health, player);

        // fetch icon texture from texture manager and attach icons to bars & stats
        if (svc && svc->texman) {
            SDL_Texture *sheet = texman_get(svc->texman, TEX_ICONS_SHEET);
            if (sheet) {
                SDL_Rect icon0 = texman_sheet_src(svc->texman, TEX_ICONS_SHEET, 0);   // speed icon index 2 per user request
                SDL_Rect icon1 = texman_sheet_src(svc->texman, TEX_ICONS_SHEET, 1);   // cooldown icon
                SDL_Rect icon_hp = texman_sheet_src(svc->texman, TEX_ICONS_SHEET, 2); // health icon index 3
                SDL_Rect icon_points = texman_sheet_src(svc->texman, TEX_ICONS_SHEET, 4);
                SDL_Rect icon_time = texman_sheet_src(svc->texman, TEX_ICONS_SHEET, 5);     // time (idx6 user-facing)
                SDL_Rect icon_kills = texman_sheet_src(svc->texman, TEX_ICONS_SHEET, 6);    // kills (idx7 user-facing)
                SDL_Rect icon_infinite = texman_sheet_src(svc->texman, TEX_ICONS_SHEET, 7); // infinite (idx8 user-facing)
                if (h->shot_speed_bar_index >= 0)
                    hud_bar_set_icon(h, h->shot_speed_bar_index, sheet, icon0);
                if (h->weapon_cd_bar_index >= 0)
                    hud_bar_set_icon(h, h->weapon_cd_bar_index, sheet, icon1);
                if (h->health_bar_index >= 0)
                    hud_bar_set_icon(h, h->health_bar_index, sheet, icon_hp);

                // Stats positioning (bottom-right, 2 columns x 2 rows)
                float margin = 6.f;
                float row_h = 18.f;
                float row_gap = 4.f; // match vertical spacing between stacked bars
                float col_gap = 24.f;
                float text_w = 120.f; // width from text origin to right edge
                int cols = 2;
                int rows = 2;
                float col_width = text_w; // icon is drawn to left, no extra width needed here
                float total_width = cols * col_width + (cols - 1) * col_gap;
                float total_height = rows * row_h + (rows - 1) * row_gap;
                float base_x = (float)svc->display_w - margin - total_width;  // left edge of first column text start
                float base_y = (float)svc->display_h - margin - total_height; // top edge of first row
                // Grid order mapping: (row1col0)=Kills, (row0col1)=Points, (row1col1)=Time
                float x_col0 = base_x;
                float x_col1 = base_x + col_width + col_gap;
                float y_row0 = base_y;
                float y_row1 = base_y + row_h + row_gap;
                int idx_kills = hud_add_stat(h, x_col0, y_row1, text_w, row_h, icon_kills, hud_stat_update_kills, NULL);
                int idx_points = hud_add_stat(h, x_col1, y_row0, text_w, row_h, icon_points, hud_stat_update_points, NULL);
                int idx_time = hud_add_stat(h, x_col1, y_row1, text_w, row_h, icon_time, hud_stat_update_time, NULL);
                // Store infinite icon in user_data of time stat if needed during update
                if (idx_time >= 0) {
                    // misuse user_data: keep pointer to infinite icon src index? We'll handle in update using world->time_limit
        }
                // Assign icon textures
                for (int si = 0; si < h->stat_count; ++si) {
                    h->stats[si].icon_tex = sheet;
                    h->stats[si].has_icon = true;
                }
                // Override time icon to infinite variant if world time_limit == -1 at first creation (will also adjust dynamically in update)
                if (h->world && h->world->time_limit < 0.f && idx_time >= 0) {
                    h->stats[idx_time].icon_src = icon_infinite;
                }
            }
        }

        h->player_bars_initialized = true;
    }
    return h;
}
void hud_destroy(Hud *h) {
    if (!h)
        return;
    // h->tex_hud is owned by texture manager; do NOT destroy here
    free(h);
}
void hud_update(Hud *h, struct World *w, float dt) {
    if (h)
        h->world = w;
    (void)dt;
    // update stats dynamic text
    if (h && w) {
        for (int i = 0; i < h->stat_count; i++) {
            HudStat *s = &h->stats[i];
            if (!s->visible)
                continue;
            if (s->update_fn)
                s->update_fn(s, w, s->user_data);
        }
    }
}
// no anchor resolution needed anymore
void hud_render(Hud *h, struct Renderer *r) {
    if (!h || !h->visible)
        return;
    // Static background: full width strip at bottom of screen with fixed height
    if (h->svc){
        float dh = (float)h->svc->display_h;
        float dw = (float)h->svc->display_w;
        float hgt = HUD_BG_HEIGHT;
        if (hgt > dh) hgt = dh; // clamp
        SDL_FRect bg = { 0.f, dh - hgt, dw, hgt};
        SDL_SetRenderDrawColor(r->sdl, 8, 8, 16, HUD_BG_ALPHA);
        SDL_RenderFillRectF(r->sdl, &bg);
        SDL_SetRenderDrawColor(r->sdl, 40, 40, 60, HUD_BG_BORDER_ALPHA);
        SDL_RenderDrawRectF(r->sdl, &bg);
    }
    // Draw bars & stats after background
    for (int i = 0; i < h->bar_count; i++) hud_render_bar(h, &h->bars[i], r);
    for (int i = 0; i < h->stat_count; i++) hud_render_stat(h, &h->stats[i], r);
}

int hud_add_stat(Hud *h, float x, float y, float w, float hgt, SDL_Rect icon_src, void (*update_fn)(HudStat *, struct World *, void *), void *user_data) {
    if (!h || h->stat_count >= HUD_MAX_STATS)
        return -1;
    HudStat *s = &h->stats[h->stat_count++];
    s->rect = (SDL_FRect){x, y, w, hgt};
    s->visible = true;
    s->icon_src = icon_src;
    s->icon_tex = NULL; // assigned later
    s->has_icon = true;
    s->extra_icon_tex = NULL;
    s->has_extra_icon = false;
    s->update_fn = update_fn;
    s->user_data = user_data;
    s->text[0] = '\0';
    return h->stat_count - 1;
}

int hud_add_bar(Hud *h, float x, float y, float length, float height, SDL_Color fill, SDL_Color bg, HudBarValueFn fn, void *user_data) {

    if (!h || h->bar_count >= HUD_MAX_BARS)
        return -1;
    HudBar *b = &h->bars[h->bar_count++];
    b->rect.x = x;
    b->rect.y = y;
    b->rect.w = length;
    b->rect.h = height;
    b->fill_color = fill;
    b->bg_color = bg;
    b->value_fn = fn;
    b->user_data = user_data;
    b->visible = true;
    return h->bar_count - 1;
}

// Shot speed value function (expects user_data = Player*)
static float hud_value_shot_speed(struct World *w, void *user) {
    (void)w;
    Player *p = (Player *)user;
    if (!p || !p->weapon)
        return 0.f;
    float rng = (p->weapon->max_speed - p->weapon->min_speed);
    if (rng <= 0.f)
        return 0.f;
    return (p->current_shot_speed - p->weapon->min_speed) / rng;
}

// Weapon cooldown progress (time since last shot / cooldown), clamped 0..1
static float hud_value_weapon_cooldown(struct World *w, void *user) {
    Player *p = (Player *)user;
    if (!p || !p->weapon || !w)
        return 0.f;
    float maxp = (float)p->energy_max;
    if (maxp <= 0.f)
        return 0.f;
    float v = p->energy / maxp;
    if (v < 0.f)
        v = 0.f;
    else if (v > 1.f)
        v = 1.f;
    return v;
}

// Player health normalized
static float hud_value_player_health(struct World *w, void *user) {
    (void)w;
    Player *p = (Player *)user;
    if (!p)
        return 0.f;
    if (p->max_health <= 0)
        return 0.f;
    float v = (float)p->health / (float)p->max_health;
    if (v < 0.f)
        v = 0.f;
    else if (v > 1.f)
        v = 1.f;
    return v;
}

// Render a single bar (background, fill, border)
static void hud_render_bar(Hud *h, HudBar *b, struct Renderer *r) {
    if (!b || !b->visible || !b->value_fn)
        return;
    // Icon (reserve space at left if present)
    float icon_offset = 0.f;
    if (b->has_icon && b->icon_tex) {
        float icon_size = b->rect.h; // square
        SDL_FRect idst = {b->rect.x - icon_size - 4.f, b->rect.y, icon_size, b->rect.h};
        renderer_draw_texture(r, b->icon_tex, &b->icon_src, &idst, 0.f);
    }
    SDL_FRect base = b->rect; // absolute coordinates
    // background
    SDL_SetRenderDrawColor(r->sdl, b->bg_color.r, b->bg_color.g, b->bg_color.b, b->bg_color.a);
    SDL_RenderFillRectF(r->sdl, &base);
    float v = b->value_fn(h->world, b->user_data);
    if (v < 0.f)
        v = 0.f;
    else if (v > 1.f)
        v = 1.f;
    SDL_FRect fill = base;
    fill.w = base.w * v;
    SDL_Color fill_color = b->fill_color;
    bool is_energy_bar = (h && h->weapon_cd_bar_index >= 0 && &h->bars[h->weapon_cd_bar_index] == b);
    if (is_energy_bar && v < 0.2f) {
        fill_color = (SDL_Color){210, 140, 40, b->fill_color.a};
    }
    SDL_SetRenderDrawColor(r->sdl, fill_color.r, fill_color.g, fill_color.b, fill_color.a);
    SDL_RenderFillRectF(r->sdl, &fill);

    SDL_SetRenderDrawColor(r->sdl, 0, 0, 0, 200);
    SDL_RenderDrawRectF(r->sdl, &base);
}

static void hud_render_stat(Hud *h, HudStat *s, struct Renderer *r) {

    if (!s || !s->visible)
        return;
    float icon_size = s->rect.h;
    float icon_x = s->rect.x - icon_size - 4.f;
    if (s->has_icon && s->icon_tex) {
        SDL_FRect idst = {icon_x, s->rect.y, icon_size, s->rect.h};
        renderer_draw_texture(r, s->icon_tex, &s->icon_src, &idst, 0.f);
    }
    // text baseline
    if (s->has_extra_icon && s->extra_icon_tex) {
        SDL_FRect edst = {s->rect.x, s->rect.y + HUD_STAT_EXTRA_ICON_OFFSET_Y, icon_size, s->rect.h};
        renderer_draw_texture(r, s->extra_icon_tex, &s->extra_icon_src, &edst, 0.f);
    } else {
        renderer_draw_text(r, s->text, s->rect.x, s->rect.y + HUD_STAT_TEXT_OFFSET_Y, (TextStyle) {
            0
        });
    }
}

// Stat update helpers
static void hud_stat_update_kills(HudStat *stat, struct World *w, void *user) {
    (void)user;
    if (!stat || !w)
        return;
    snprintf(stat->text, sizeof(stat->text), "%d", w->kills);
}
static void hud_stat_update_points(HudStat *stat, struct World *w, void *user) {
    (void)user;
    if (!stat || !w)
        return;
    snprintf(stat->text, sizeof(stat->text), "%d", w->score);
}
static void hud_stat_update_time(HudStat *stat, struct World *w, void *user) {
    (void)user;
    if (!stat || !w)
        return;
    if (w->time_limit < 0.f) {
        // show base time icon + infinity as extra icon; hide text
        stat->text[0] = '\0';
        if (w->svc && w->svc->texman && stat->icon_tex) {
            // main icon stays time (sheet index 5), extra icon is infinity (index 7)
            stat->icon_src = texman_sheet_src(w->svc->texman, TEX_ICONS_SHEET, 5);
            stat->extra_icon_tex = stat->icon_tex;
            stat->extra_icon_src = texman_sheet_src(w->svc->texman, TEX_ICONS_SHEET, 7);
            stat->has_extra_icon = true;
        }
    }
    else {
        // timed: show countdown with time icon (index 5)
        stat->has_extra_icon = false;
        stat->extra_icon_tex = NULL;
        if (w->svc && w->svc->texman && stat->icon_tex)
            stat->icon_src = texman_sheet_src(w->svc->texman, TEX_ICONS_SHEET, 5);
        float remaining = w->time_limit - w->time;
        if (remaining < 0.f)
            remaining = 0.f;
        int sec = (int)remaining;
        int mm = sec / 60;
        int ss = sec % 60;
        snprintf(stat->text, sizeof(stat->text), "%02d:%02d", mm, ss);
    }
}
