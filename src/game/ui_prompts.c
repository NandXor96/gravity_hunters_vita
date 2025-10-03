#include "ui_prompts.h"

#include <SDL2/SDL.h>

#include "../services/renderer.h"
#include "../services/services.h"
#include "../services/texture_manager.h"

static float ui_prompt_icon_width(SDL_Texture *tex, float target_height) {

    if (!tex || target_height <= 0.f)
        return 0.f;
    int w = 0, h = 0;
    if (SDL_QueryTexture(tex, NULL, NULL, &w, &h) != 0 || h <= 0)
        return target_height;
    return target_height * ((float)w / (float)h);

}

static float ui_prompt_text_y(float top, float height, SDL_Point size) {

    float baseline = top;
    if (size.y > 0)
        baseline += (height - (float)size.y) * 0.5f;
    return baseline;

}

void ui_draw_prompts(struct Renderer *r, struct Services *svc, bool show_ok, bool show_back) {

    if (!r || !svc)
        return;
    if (!show_ok && !show_back)
        return;

    SDL_Texture *tex_ok = NULL;
    SDL_Texture *tex_back = NULL;
    if (svc->texman) {
        if (show_ok)
            tex_ok = texman_get(svc->texman, TEX_UI_BUTTON_CROSS);
        if (show_back)
            tex_back = texman_get(svc->texman, TEX_UI_BUTTON_CIRCLE);
    }

    const char *ok_label = "Ok";
    const char *back_label = "Back";

    const float icon_height = 42.f;
    const float padding = 7.f;
    const float icon_text_spacing = 0.f;
    const float prompt_spacing = 26.f;

    SDL_Point ok_size = show_ok ? renderer_measure_text(r, ok_label, (TextStyle){0}) : (SDL_Point){0, 0};
    SDL_Point back_size = show_back ? renderer_measure_text(r, back_label, (TextStyle){0}) : (SDL_Point){0, 0};

    if (show_ok)
        ok_size.x += 5;
    if (show_back)
        back_size.x += 5;

    float ok_icon_width = show_ok ? ui_prompt_icon_width(tex_ok, icon_height) : 0.f;
    float back_icon_width = show_back ? ui_prompt_icon_width(tex_back, icon_height) : 0.f;

    float ok_block_width = 0.f;
    if (show_ok) {
        ok_block_width = ok_icon_width;
        if (ok_size.x > 0) {
            if (ok_block_width > 0.f)
                ok_block_width += icon_text_spacing + (float)ok_size.x;
            else
                ok_block_width = (float)ok_size.x;
        }
        if (ok_block_width <= 0.f)
            ok_block_width = icon_height; /* graceful fallback */
    }

    float back_block_width = 0.f;
    if (show_back) {
        back_block_width = back_icon_width;
        if (back_size.x > 0) {
            if (back_block_width > 0.f)
                back_block_width += icon_text_spacing + (float)back_size.x;
            else
                back_block_width = (float)back_size.x;
        }
        if (back_block_width <= 0.f)
            back_block_width = icon_height;
    }

    float total_width = 0.f;
    if (show_ok)
        total_width += ok_block_width;
    if (show_back) {
        if (show_ok)
            total_width += prompt_spacing;
        total_width += back_block_width;
    }

    float start_x = (float)svc->display_w - padding - total_width;
    float y = (float)svc->display_h - padding - icon_height;
    float current_x = start_x;

    if (show_ok && ok_icon_width > 0.f && tex_ok) {
        SDL_FRect icon_rect = {current_x, y, ok_icon_width, icon_height};
        renderer_draw_texture(r, tex_ok, NULL, &icon_rect, 0.f);
        current_x += ok_icon_width + icon_text_spacing;
    }
    if (show_ok) {
        float ok_text_y = ui_prompt_text_y(y + 2, icon_height, ok_size);
        renderer_draw_text(r, ok_label, current_x, ok_text_y, (TextStyle){0});
    }

    if (show_back) {

        current_x = show_ok ? (start_x + ok_block_width + prompt_spacing) : start_x;
        if (back_icon_width > 0.f && tex_back) {
            SDL_FRect icon_rect = {current_x, y, back_icon_width, icon_height};
            renderer_draw_texture(r, tex_back, NULL, &icon_rect, 0.f);
            current_x += back_icon_width + icon_text_spacing;
        }
        float back_text_y = ui_prompt_text_y(y + 2, icon_height, back_size);
        renderer_draw_text(r, back_label, current_x, back_text_y, (TextStyle){0});
    }
}
