#include "renderer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../core/log.h"

static char *renderer_strdup(const char *src) {
    if (!src)
        return NULL;
    size_t len = strlen(src) + 1;
    char *dst = (char *)malloc(len);
    if (!dst)
        return NULL;
    memcpy(dst, src, len);
    return dst;
}

static void renderer_font_cache_remove(Renderer *r, size_t index) {
    if (!r || index >= r->font_cache_count)
        return;
    RendererFontCacheEntry *entry = &r->font_cache[index];
    if (entry->font) {
        TTF_CloseFont(entry->font);
        entry->font = NULL;
    }
    free(entry->font_path);
    entry->font_path = NULL;

    size_t last = r->font_cache_count - 1;
    if (index != last) {
        r->font_cache[index] = r->font_cache[last];
    }
    r->font_cache[last] = (RendererFontCacheEntry){0};
    r->font_cache_count -= 1;
}

static void renderer_text_cache_remove(Renderer *r, size_t index) {
    if (!r || index >= r->text_cache_count)
        return;
    RendererTextCacheEntry *entry = &r->text_cache[index];
    if (entry->texture) {
        SDL_DestroyTexture(entry->texture);
        entry->texture = NULL;
    }
    free(entry->text);
    entry->text = NULL;
    free(entry->font_path);
    entry->font_path = NULL;

    size_t last = r->text_cache_count - 1;
    if (index != last) {
        r->text_cache[index] = r->text_cache[last];
    }
    r->text_cache[last] = (RendererTextCacheEntry){0};
    r->text_cache_count -= 1;
}

static bool renderer_is_absolute_font_path(const char *path) {
    if (!path)
        return false;
    if (path[0] == '/' || path[0] == '\\')
        return true;
    /* Detect Vita absolute with drive or app prefix */
    return strstr(path, ":/") != NULL || strstr(path, ":\\") != NULL;
}

static char *renderer_build_font_path(Renderer *r, const char *font_name) {
    if (!font_name)
        return NULL;
    if (renderer_is_absolute_font_path(font_name))
        return renderer_strdup(font_name);
    if (!r->font_assets_root)
        return renderer_strdup(font_name);

    size_t base_len = strlen(r->font_assets_root);
    size_t name_len = strlen(font_name);
    size_t total = base_len + name_len + 1;
    char *full_path = (char *)malloc(total);
    if (!full_path)
        return NULL;
    snprintf(full_path, total, "%s%s", r->font_assets_root, font_name);
    return full_path;
}

static RendererFontCacheEntry *renderer_get_font_entry(Renderer *r, const TextStyle *style) {
    if (!r)
        return NULL;

    int size_px = r->default_font_size;
    if (style && style->size_px > 0)
        size_px = style->size_px;

    const char *font_name = r->default_font_name;
    if (style && style->font_path)
        font_name = style->font_path;

    if (!font_name)
        return NULL;

    char *resolved_path = renderer_build_font_path(r, font_name);
    if (!resolved_path)
        return NULL;

    for (size_t i = 0; i < r->font_cache_count; ++i) {
        RendererFontCacheEntry *entry = &r->font_cache[i];
        if (entry->size_px == size_px && entry->font_path && strcmp(entry->font_path, resolved_path) == 0) {
            entry->last_used_frame = r->frame_counter;
            free(resolved_path);
            return entry;
        }
    }

    TTF_Font *font = TTF_OpenFont(resolved_path, size_px);
    if (!font) {
        LOG_ERROR("renderer", "Failed to load font %s (%d): %s", resolved_path, size_px, TTF_GetError());
        free(resolved_path);
        return NULL;
    }

    if (r->font_cache_count >= RENDERER_FONT_CACHE_CAP) {
        size_t evict_index = 0;
        uint64_t oldest = r->font_cache[0].last_used_frame;
        for (size_t i = 1; i < r->font_cache_count; ++i) {
            if (r->font_cache[i].last_used_frame < oldest) {
                oldest = r->font_cache[i].last_used_frame;
                evict_index = i;
            }
        }
        renderer_font_cache_remove(r, evict_index);
    }

    RendererFontCacheEntry entry = {
        .font_path = resolved_path,
        .size_px = size_px,
        .font = font,
        .last_used_frame = r->frame_counter
    };

    RendererFontCacheEntry *target = &r->font_cache[r->font_cache_count++];
    *target = entry;
    return target;
}

static RendererTextCacheEntry *renderer_find_text_entry(Renderer *r, const char *text, const char *font_path, int size_px, int wrap_w) {
    if (!r || !text || !font_path)
        return NULL;

    for (size_t i = 0; i < r->text_cache_count; ++i) {
        RendererTextCacheEntry *entry = &r->text_cache[i];
        if (entry->size_px == size_px && entry->wrap_w == wrap_w && entry->font_path && strcmp(entry->font_path, font_path) == 0 && entry->text && strcmp(entry->text, text) == 0) {
            entry->last_used_frame = r->frame_counter;
            return entry;
        }
    }
    return NULL;
}

static RendererTextCacheEntry *renderer_prepare_text_entry(Renderer *r, RendererFontCacheEntry *font_entry, const char *text, int wrap_w) {
    if (!r || !font_entry || !text || text[0] == '\0')
        return NULL;

    RendererTextCacheEntry *existing = renderer_find_text_entry(r, text, font_entry->font_path, font_entry->size_px, wrap_w);
    if (existing)
        return existing;

    SDL_Color color = {255, 255, 255, 255};
    SDL_Surface *surface = NULL;
    if (wrap_w > 0)
        surface = TTF_RenderUTF8_Blended_Wrapped(font_entry->font, text, color, wrap_w);
    else
        surface = TTF_RenderUTF8_Blended(font_entry->font, text, color);

    if (!surface)
        return NULL;

    SDL_Texture *texture = SDL_CreateTextureFromSurface(r->sdl, surface);
    if (!texture) {
        LOG_ERROR("renderer", "Failed to create texture for text '%s': %s", text, SDL_GetError());
        SDL_FreeSurface(surface);
        return NULL;
    }
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

    RendererTextCacheEntry new_entry = {
        .text = renderer_strdup(text),
        .font_path = renderer_strdup(font_entry->font_path),
        .size_px = font_entry->size_px,
        .wrap_w = wrap_w,
        .texture = texture,
        .width = surface->w,
        .height = surface->h,
        .last_used_frame = r->frame_counter
    };

    SDL_FreeSurface(surface);

    if (!new_entry.text || !new_entry.font_path) {
        SDL_DestroyTexture(texture);
        free(new_entry.text);
        free(new_entry.font_path);
        return NULL;
    }

    if (r->text_cache_count >= RENDERER_TEXT_CACHE_CAP) {
        size_t evict_index = 0;
        uint64_t oldest = r->text_cache[0].last_used_frame;
        for (size_t i = 1; i < r->text_cache_count; ++i) {
            if (r->text_cache[i].last_used_frame < oldest) {
                oldest = r->text_cache[i].last_used_frame;
                evict_index = i;
            }
        }
        renderer_text_cache_remove(r, evict_index);
    }

    RendererTextCacheEntry *target = &r->text_cache[r->text_cache_count++];
    *target = new_entry;
    return target;
}

Renderer *renderer_create(SDL_Renderer *sdl) {

    Renderer *r = calloc(1, sizeof(Renderer));
    if (!r) {
        LOG_ERROR("renderer", "Failed to allocate Renderer");
        return NULL;
    }
    r->sdl = sdl;

#ifdef PLATFORM_VITA
    const char *base_path = "app0:/assets/fonts/";
#else
    const char *base_path = "./assets/fonts/";
#endif
    const char *font_filename = "comic_shanns2.ttf";

    r->font_assets_root = renderer_strdup(base_path);
    if (!r->font_assets_root) {
        LOG_ERROR("renderer", "Failed to allocate font base path");
        renderer_destroy(r);
        return NULL;
    }

    r->default_font_name = renderer_strdup(font_filename);
    if (!r->default_font_name) {
        LOG_ERROR("renderer", "Failed to allocate font filename");
        renderer_destroy(r);
        return NULL;
    }

    r->default_font_size = 25;

    RendererFontCacheEntry *default_font = renderer_get_font_entry(r, &(TextStyle){0});
    if (!default_font || !default_font->font) {
        LOG_ERROR("renderer", "Failed to load default font: %s", TTF_GetError());
        renderer_destroy(r);
        return NULL;
    }
    r->font = default_font->font;

    SDL_SetRenderDrawBlendMode(r->sdl, SDL_BLENDMODE_BLEND);

    return r;
}
void renderer_destroy(Renderer *r) {
    if (!r)
        return;

    for (size_t i = 0; i < r->text_cache_count; ++i) {
        RendererTextCacheEntry *entry = &r->text_cache[i];
        if (entry->texture)
            SDL_DestroyTexture(entry->texture);
        free(entry->text);
        free(entry->font_path);
    }

    for (size_t i = 0; i < r->font_cache_count; ++i) {
        RendererFontCacheEntry *entry = &r->font_cache[i];
        if (entry->font)
            TTF_CloseFont(entry->font);
        free(entry->font_path);
    }

    free(r->font_assets_root);
    free(r->default_font_name);

    free(r);
}
void renderer_render(Renderer *r) {
    if (!r)
        return;
    r->frame_counter += 1;
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
    if (!r || !text || text[0] == '\0')
        return;

    SDL_Texture *texture = NULL;
    SDL_Point size = {0, 0};
    if (!renderer_get_text_texture(r, text, style, &texture, &size) || !texture)
        return;

    SDL_FRect dst = {
        .x = x,
        .y = y,
        .w = (float)size.x,
        .h = (float)size.y
    };
    SDL_RenderCopyF(r->sdl, texture, NULL, &dst);
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
    if (!r || !text || text[0] == '\0')
        return (SDL_Point){0, 0};

    RendererFontCacheEntry *font_entry = renderer_get_font_entry(r, &style);
    if (!font_entry || !font_entry->font)
        return (SDL_Point){0, 0};

    RendererTextCacheEntry *entry = renderer_find_text_entry(r, text, font_entry->font_path, font_entry->size_px, style.wrap_w);
    if (entry)
        return (SDL_Point){entry->width, entry->height};

    int w = 0;
    int h = 0;
    if (TTF_SizeUTF8(font_entry->font, text, &w, &h) != 0) {
        w = (int)strlen(text) * (font_entry->size_px / 2);
        h = font_entry->size_px;
    }
    return (SDL_Point){w, h};
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

bool renderer_get_text_texture(Renderer *r, const char *text, TextStyle style, SDL_Texture **out_texture, SDL_Point *out_size) {
    if (out_texture)
        *out_texture = NULL;
    if (out_size)
        *out_size = (SDL_Point){0, 0};

    if (!r || !text || text[0] == '\0')
        return false;

    RendererFontCacheEntry *font_entry = renderer_get_font_entry(r, &style);
    if (!font_entry || !font_entry->font)
        return false;

    RendererTextCacheEntry *entry = renderer_prepare_text_entry(r, font_entry, text, style.wrap_w);
    if (!entry || !entry->texture)
        return false;

    entry->last_used_frame = r->frame_counter;

    if (out_texture)
        *out_texture = entry->texture;
    if (out_size)
        *out_size = (SDL_Point){entry->width, entry->height};
    return true;
}
