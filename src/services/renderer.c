#include "renderer.h"
#include <stdlib.h>
#include <string.h>
#include "../core/log.h"

Renderer *renderer_create(SDL_Renderer *sdl) {

    Renderer *r = calloc(1, sizeof(Renderer));
    if (!r) {
        LOG_ERROR("renderer", "Failed to allocate Renderer");
        return NULL;

}
    r->sdl = sdl;

// Load Font
#ifdef PLATFORM_VITA
    char *base_path = "app0:/assets/fonts/";
#else
    char *base_path = "./assets/fonts/";
#endif

    const char *font_filename = "comic_shanns2.ttf";
    size_t path_len = strlen(base_path) + strlen(font_filename) + 1;
    char *full_path = malloc(path_len);
    if (!full_path) {
        LOG_ERROR("renderer", "Failed to allocate font path");
        free(r);
        return NULL;
    }
    snprintf(full_path, path_len, "%s%s", base_path, font_filename);

    r->font = TTF_OpenFont(full_path, 25);
    if (!r->font) {
        LOG_ERROR("renderer", "Failed to load font: %s", TTF_GetError());
    }
    free(full_path);

    SDL_SetRenderDrawBlendMode(r->sdl, SDL_BLENDMODE_BLEND);

    return r;
}
void renderer_destroy(Renderer *r) {
    TTF_CloseFont(r->font);
    free(r);
}
void renderer_render(Renderer *r) {
    SDL_RenderPresent(r->sdl);
}
void renderer_draw_texture(Renderer *r, SDL_Texture *tex, const SDL_Rect *src, const SDL_FRect *dst, float angle_deg) {
    (void)angle_deg;
    if (!tex) {
        SDL_FRect rect = dst ? *dst : (SDL_FRect){0, 0, 32, 32
};
        SDL_SetRenderDrawColor(r->sdl, 80, 80, 160, 255);
        SDL_RenderFillRectF(r->sdl, &rect);
    }
    else {
        SDL_RenderCopyExF(r->sdl, tex, src, dst, angle_deg, NULL, 0);
    }
}
void renderer_draw_text(Renderer *r, const char *text, float x, float y, TextStyle style) {
    if (r->font) {
        SDL_Surface *surface = TTF_RenderText_Blended(r->font, text, (SDL_Color){255, 255, 255, 255
});
        if (surface) {
            SDL_Texture *texture = SDL_CreateTextureFromSurface(r->sdl, surface);
            if (texture) {
                SDL_Rect dst = {x, y, surface->w, surface->h
        };
                SDL_RenderCopy(r->sdl, texture, NULL, &dst);
                SDL_DestroyTexture(texture);
            }
            SDL_FreeSurface(surface);
        }
    }
}
void renderer_draw_textbox(Renderer *r, const char *text, SDL_FRect box, TextboxStyle style) {
    (void)text;
    (void)style;
    SDL_SetRenderDrawColor(r->sdl, 200, 200, 200, 255);
    SDL_RenderDrawRectF(r->sdl, &box);
}
void renderer_draw_filled_rect(Renderer *r, SDL_FRect rect, SDL_Color color) {
    SDL_SetRenderDrawColor(r->sdl, color.r, color.g, color.b, color.a);
    SDL_RenderFillRectF(r->sdl, &rect);
}

void renderer_draw_text_centered(Renderer *r, const char *text, float cx, float y, TextStyle style) {

    SDL_Point p = renderer_measure_text(r, text, style);
    float x = cx - (p.x * 0.5f);
    renderer_draw_text(r, text, x, y, style);

}

void renderer_draw_rect_outline(Renderer *r, SDL_FRect rect, SDL_Color color, int border_px) {

    (void)border_px; // keep API but draw single-pixel outline using SDL
    if (!r)
        return;
    SDL_SetRenderDrawColor(r->sdl, color.r, color.g, color.b, color.a);
    SDL_RenderDrawRectF(r->sdl, &rect);

}
SDL_Point renderer_measure_text(Renderer *r, const char *text, TextStyle style) {
    (void)style;
    if (!r || !text || !r->font)
        return (SDL_Point){0, 0
};
    int w = 0, h = 0;
    /* Use TTF to get accurate glyph metrics for the current font */
    if (TTF_SizeUTF8(r->font, text, &w, &h) != 0) {
        /* fallback heuristic */
        w = (int)strlen(text) * 8;
        h = 12;
    }
    return (SDL_Point) {
        w, h
    };
}

// unneeded because of pic0.png
void renderer_show_splashscreen(Renderer *r) {
    #ifdef PLATFORM_VITA
    SDL_Surface *surf = IMG_Load("app0:/sce_sys/pic0.png");
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(r->sdl, surf);
        SDL_FreeSurface(surf);
        if (tex) {
            // Render the splash texture
            SDL_RenderCopy(r->sdl, tex, NULL, NULL);
            renderer_render(r);
            SDL_DestroyTexture(tex);
}
    }
    #endif
}
