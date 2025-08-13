#include "renderer.h"
#include <stdlib.h>
#include <string.h>

Renderer *renderer_create(SDL_Renderer *sdl)
{
    Renderer *r = calloc(1, sizeof(Renderer));
    r->sdl = sdl;

// Load Font
#ifdef PLATFORM_VITA
    char *base_path = "app0:/assets/fonts/";
#else
    char *base_path = "./assets/fonts/";
#endif

    char *font = "comic_shanns2.ttf";
    char *full_path = malloc(strlen(base_path) + strlen("comic_shanns2.ttf") + 1);
    strcpy(full_path, base_path);
    strcat(full_path, "comic_shanns2.ttf");

    r->font = TTF_OpenFont(full_path, 25);
    free(full_path);

    SDL_SetRenderDrawBlendMode(r->sdl, SDL_BLENDMODE_BLEND);

    return r;
}
void renderer_destroy(Renderer *r)
{
    TTF_CloseFont(r->font);
    free(r);
}
void renderer_render(Renderer *r) { SDL_RenderPresent(r->sdl); }
void renderer_draw_texture(Renderer *r, SDL_Texture *tex, const SDL_Rect *src, const SDL_FRect *dst, float angle_deg)
{
    (void)angle_deg;
    if (!tex)
    {
        SDL_FRect rect = dst ? *dst : (SDL_FRect){0, 0, 32, 32};
        SDL_SetRenderDrawColor(r->sdl, 80, 80, 160, 255);
        SDL_RenderFillRectF(r->sdl, &rect);
    }
    else
    {
        SDL_RenderCopyExF(r->sdl, tex, src, dst, angle_deg, NULL, 0);
    }
}
void renderer_draw_text(Renderer *r, const char *text, float x, float y, TextStyle style)
{
    if (r->font)
    {
        SDL_Surface *surface = TTF_RenderText_Blended(r->font, text, (SDL_Color){255, 255, 255, 255});
        if (surface)
        {
            SDL_Texture *texture = SDL_CreateTextureFromSurface(r->sdl, surface);
            if (texture)
            {
                SDL_Rect dst = {x, y, surface->w, surface->h};
                SDL_RenderCopy(r->sdl, texture, NULL, &dst);
                SDL_DestroyTexture(texture);
            }
            SDL_FreeSurface(surface);
        }
    }
}
void renderer_draw_textbox(Renderer *r, const char *text, SDL_FRect box, TextboxStyle style)
{
    (void)text;
    (void)style;
    SDL_SetRenderDrawColor(r->sdl, 200, 200, 200, 255);
    SDL_RenderDrawRectF(r->sdl, &box);
}
SDL_Point renderer_measure_text(Renderer *r, const char *text, TextStyle style)
{
    (void)r;
    (void)style;
    int w = (int)(text ? (int)strlen(text) * 8 : 0);
    return (SDL_Point){w, 12};
}

// unneeded because of pic0.png
void renderer_show_splashscreen(Renderer *r)
{
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