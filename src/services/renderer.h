#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>

typedef struct Renderer {
    SDL_Renderer *sdl;
    TTF_Font *font;
} Renderer;

typedef struct TextStyle {
    const char *font_path;
    int size_px;
    int wrap_w;
} TextStyle;

typedef struct TextboxStyle {
    int padding_px;
    int border_px;
} TextboxStyle;

/**
 * @brief Create a new renderer instance
 * @param sdl SDL renderer
 * @return Pointer to created renderer
 */
Renderer *renderer_create(SDL_Renderer *sdl);

/**
 * @brief Destroy renderer and free resources
 * @param r Renderer to destroy
 */
void renderer_destroy(Renderer *r);

/**
 * @brief Present rendered frame to screen
 * @param r Renderer
 */
void renderer_render(Renderer *r);

/**
 * @brief Draw a texture
 * @param r Renderer
 * @param tex Texture to draw
 * @param src Source rectangle (NULL for full texture)
 * @param dst Destination rectangle
 * @param angle_deg Rotation angle in degrees
 */
void renderer_draw_texture(Renderer *r, SDL_Texture *tex, const SDL_Rect *src, const SDL_FRect *dst, float angle_deg);

/**
 * @brief Draw text at position
 * @param r Renderer
 * @param text Text to draw
 * @param x X position
 * @param y Y position
 * @param style Text style
 */
void renderer_draw_text(Renderer *r, const char *text, float x, float y, TextStyle style);

/**
 * @brief Draw text in a box with background
 * @param r Renderer
 * @param text Text to draw
 * @param box Bounding box
 * @param style Textbox style
 */
void renderer_draw_textbox(Renderer *r, const char *text, SDL_FRect box, TextboxStyle style);

/**
 * @brief Draw a filled rectangle
 * @param r Renderer
 * @param rect Rectangle bounds
 * @param color Fill color
 */
void renderer_draw_filled_rect(Renderer *r, SDL_FRect rect, SDL_Color color);

/**
 * @brief Draw centered text
 * @param r Renderer
 * @param text Text to draw
 * @param cx Center X position
 * @param y Y position
 * @param style Text style
 */
void renderer_draw_text_centered(Renderer *r, const char *text, float cx, float y, TextStyle style);

/**
 * @brief Draw rectangle outline
 * @param r Renderer
 * @param rect Rectangle bounds
 * @param color Border color
 * @param border_px Border width in pixels
 */
void renderer_draw_rect_outline(Renderer *r, SDL_FRect rect, SDL_Color color, int border_px);

/**
 * @brief Show splashscreen
 * @param r Renderer
 */
void renderer_show_splashscreen(Renderer *r);

/**
 * @brief Measure text dimensions
 * @param r Renderer
 * @param text Text to measure
 * @param style Text style
 * @return Text dimensions as SDL_Point
 */
SDL_Point renderer_measure_text(Renderer *r, const char *text, TextStyle style);
