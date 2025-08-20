#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>

typedef struct Renderer {
    SDL_Renderer *sdl;
    TTF_Font *font;
} Renderer;

typedef struct TextStyle
{
    const char *font_path;
    int size_px;
    int wrap_w;
} TextStyle;

typedef struct TextboxStyle
{
    int padding_px;
    int border_px;
} TextboxStyle;

Renderer *renderer_create(SDL_Renderer *sdl);
void renderer_destroy(Renderer *r);
void renderer_render(Renderer *r);

void renderer_draw_texture(Renderer *r, SDL_Texture *tex, const SDL_Rect *src, const SDL_FRect *dst, float angle_deg);
void renderer_draw_text(Renderer *r, const char *text, float x, float y, TextStyle style);
void renderer_draw_textbox(Renderer *r, const char *text, SDL_FRect box, TextboxStyle style);
void renderer_draw_filled_rect(Renderer *r, SDL_FRect rect, SDL_Color color);
void renderer_draw_text_centered(Renderer *r, const char *text, float cx, float y, TextStyle style);
void renderer_draw_rect_outline(Renderer *r, SDL_FRect rect, SDL_Color color, int border_px);
void renderer_show_splashscreen(Renderer *r);
SDL_Point renderer_measure_text(Renderer *r, const char *text, TextStyle style);
