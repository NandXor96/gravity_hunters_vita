#include "overlay_start_game.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include "../services/renderer.h"
#include "../services/services.h"
#include "../app/app.h"

void overlay_start_game_enter(Scene *s) {
    OverlayStartGameState *st = calloc(1, sizeof(OverlayStartGameState));
    if (!st) {
        return;
    }

    st->svc = app_services();
    st->duration = 3.0f;
    st->elapsed = 0.0f;
    st->current_value = 3;

    s->state = st;
    s->is_overlay = true;
    s->blocks_under_input = true;
    s->blocks_under_update = true;
}

void overlay_start_game_leave(Scene *s) {
    if (!s || !s->state) {
        return;
    }
    OverlayStartGameState *st = (OverlayStartGameState *)s->state;
    free(st);
    s->state = NULL;
}

void overlay_start_game_handle_input(Scene *s, const struct InputState *in) {
    (void)s;
    (void)in;
}

void overlay_start_game_update(Scene *s, float dt) {
    OverlayStartGameState *st = (OverlayStartGameState *)s->state;
    if (!st) {
        return;
    }

    st->elapsed += dt;
    if (st->elapsed >= st->duration) {
        app_pop_overlay();
        return;
    }

    float remaining = st->duration - st->elapsed;
    int display = (int)ceilf(remaining);
    if (display < 1) {
        display = 1;
    }
    st->current_value = display;
}

void overlay_start_game_render(Scene *s, struct Renderer *r) {
    OverlayStartGameState *st = (OverlayStartGameState *)s->state;
    if (!st || !r || !st->svc) {
        return;
    }

    const float width = (float)st->svc->display_w;
    const float height = (float)st->svc->display_h;

    SDL_FRect bg = {0.0f, 0.0f, width, height};
    renderer_draw_filled_rect(r, bg, OVERLAY_BACKDROP_COLOR);

    char buffer[8];
    snprintf(buffer, sizeof(buffer), "%d", st->current_value);

    const float center_x = width * 0.5f;
    const float center_y = height * 0.5f - 20.0f;

    TextStyle number_style = {
        .font_path = NULL,
        .size_px = 120,
        .wrap_w = 0
    };

    SDL_Point number_size = renderer_measure_text(r, buffer, number_style);

    const float padding = 20.0f;
    const float number_top = center_y - (float)number_size.y * 0.5f;
    const float number_bottom = number_top + (float)number_size.y;
    float max_width = (float)number_size.x * 2.5f;

    SDL_FRect panel = {
        center_x - (max_width * 0.5f) - padding,
        number_top - padding,
        max_width + padding * 2.0f,
        (number_bottom - number_top) + padding
    };

    renderer_draw_filled_rect(r, panel, MENU_COLOR_TEXT_BACKGROUND);

    renderer_draw_text_centered(r, buffer, center_x, number_top, number_style);
}
