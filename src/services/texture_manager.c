#include "texture_manager.h"
#include "../core/types.h"
#include <string.h>
#include <stdbool.h>
#include <SDL2/SDL_image.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

struct BaseSlot {
    SDL_Texture *tex;
    uint16_t flags, tile_w, tile_h;
};
struct TextureManager {
    SDL_Renderer *sdl;
    char assets_root[128];
    struct BaseSlot base[TEX__COUNT];
    int projectile_variant_count;
    /* Explosions meta */
    SDL_Texture *explosions_sheet;
    int explosion_types;
    int explosion_frames;
    int explosion_tile; /* square tile size */
};
static SDL_Texture *gen_nebula(SDL_Renderer *r, uint32_t seed);
static SDL_Texture *gen_projectiles_sheet(SDL_Renderer *r, int *out_variants);
static SDL_Texture *gen_projectiles_sheet_proc(SDL_Renderer *r, uint32_t seed);
static SDL_Texture *gen_explosions_sheet(SDL_Renderer *r, int *out_types, int *out_frames, int *out_tile);
struct TextureSpec {
    const char *name;
    const char *path;
    SDL_Texture *(*proc)(SDL_Renderer *, uint32_t);
    uint16_t flags, tile_w, tile_h;
};
static const struct TextureSpec SPECS[TEX__COUNT] = {
    [TEX_PLANETS_SHEET] = {"planets", "images/planets_atlas.png", NULL, TEXF_SHEET, 128, 128},
    [TEX_PLAYER] = {"player", "images/player.png", NULL, 0, 0, 0},
    [TEX_BG_STARFIELD] = {"bg_starfield", NULL, gen_nebula, 0, 0, 0},
    [TEX_LOGO] = {"logo", "images/logo.png", NULL, 0, 0, 0},
    [TEX_ICONS_SHEET] = {"icons", "images/hud_atlas.png", NULL, TEXF_SHEET, 32, 32},
    [TEX_PROJECTILES_SHEET] = {"projectiles", NULL, gen_projectiles_sheet_proc, TEXF_SHEET, 8, 8},
    [TEX_ENEMIES_SHEET] = {"enemies", "images/enemies_atlas.png", NULL, TEXF_SHEET, 32, 32},
    [TEX_EXPLOSIONS_SHEET] = {"explosions", NULL, NULL, TEXF_SHEET, 0, 0}, /* generated separately (multi-frame) */
    [TEX_UI_BUTTON_CROSS] = {"ui_button_cross", "buttons/Vita_Cross.png", NULL, 0, 0, 0},
    [TEX_UI_BUTTON_CIRCLE] = {"ui_button_circle", "buttons/Vita_Circle.png", NULL, 0, 0, 0},
};
TextureManager *texman_create(SDL_Renderer *sdl, const char *assets_root) {
    TextureManager *m = calloc(1, sizeof(*m));
    if (!m)
        return NULL;
    m->sdl = sdl;
    if (assets_root)
        strncpy(m->assets_root, assets_root, sizeof(m->assets_root) - 1);

    return m;
}
bool texman_load_textures(TextureManager *m, uint32_t seed) {
    SDL_Renderer *sdl = m->sdl;
    for (int i = 0; i < TEX__COUNT; i++) {
        const struct TextureSpec *s = &SPECS[i];
        SDL_Texture *tex = NULL;
        if (s->proc)
            tex = s->proc(sdl, seed);
        else if (s->path) {
            char buf[256];
            snprintf(buf, sizeof(buf), "%s/%s", m->assets_root[0] ? m->assets_root : "./assets", s->path);
            SDL_Surface *surf = IMG_Load(buf);
            if (surf) {
                tex = SDL_CreateTextureFromSurface(sdl, surf);
                SDL_FreeSurface(surf);
}
        }
        if (tex)
            SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
        m->base[i].tex = tex;
        m->base[i].flags = s->flags;
        m->base[i].tile_w = s->tile_w;
        m->base[i].tile_h = s->tile_h;
    }
    if (m->projectile_variant_count <= 0 && m->base[TEX_PROJECTILES_SHEET].tex) {
        int w, h;
        Uint32 f;
        int acc;
        if (SDL_QueryTexture(m->base[TEX_PROJECTILES_SHEET].tex, &f, &acc, &w, &h) == 0 && m->base[TEX_PROJECTILES_SHEET].tile_w > 0)
            m->projectile_variant_count = w / m->base[TEX_PROJECTILES_SHEET].tile_w;
    }
    /* Generate explosions sheet (types x frames atlas) */
    m->explosions_sheet = gen_explosions_sheet(sdl, &m->explosion_types, &m->explosion_frames, &m->explosion_tile);
    if (m->explosions_sheet)
        SDL_SetTextureBlendMode(m->explosions_sheet, SDL_BLENDMODE_BLEND);

    return true;
}
void texman_destroy(TextureManager *m) {
    if (!m)
        return;
    for (int i = 0; i < TEX__COUNT; i++)
        if (m->base[i].tex)
            SDL_DestroyTexture(m->base[i].tex);
    if (m->explosions_sheet)
        SDL_DestroyTexture(m->explosions_sheet);
    free(m);
}
SDL_Texture *texman_get(TextureManager *m, TextureID id) {
    if (!m || id < 0 || id >= TEX__COUNT)
        return NULL;
    return m->base[id].tex;
}
SDL_Rect texman_planet_src_rect(int idx) {
    if (idx < 0)
        idx = 0;
    if (idx > 15)
        idx = 15;
    int cell = 128;
    return (SDL_Rect){(idx % 4) * cell, (idx / 4) * cell, cell, cell
};
}
static SDL_Rect sheet_src(TextureManager *m, TextureID id, int index) {
    SDL_Rect r = {0, 0, 0, 0
};
    if (!m || id < 0 || id >= TEX__COUNT)
        return r;
    struct BaseSlot *b = &m->base[id];
    if (!(b->flags & TEXF_SHEET) || !b->tex)
        return r;
    int w, h;
    Uint32 f;
    int acc;
    if (SDL_QueryTexture(b->tex, &f, &acc, &w, &h) != 0)
        return r;
    if (!b->tile_w || !b->tile_h)
        return r;
    int cols = w / b->tile_w;
    if (cols <= 0)
        return r;
    if (index < 0)
        index = 0;
    int row = index / cols;
    int col = index % cols;
    r.x = col * b->tile_w;
    r.y = row * b->tile_h;
    r.w = b->tile_w;
    r.h = b->tile_h;
    return r;
}
SDL_Rect texman_sheet_src(TextureManager *m, TextureID id, int index) {
    return sheet_src(m, id, index);
}
SDL_Texture *texman_projectiles_texture(TextureManager *m) {
    return m ? m->base[TEX_PROJECTILES_SHEET].tex : NULL;
}
SDL_Rect texman_projectile_src(TextureManager *m, int variant_index) {
    if (!m)
        return (SDL_Rect){0, 0, 0, 0
};
    if (variant_index < 0)
        variant_index = 0;
    if (variant_index >= m->projectile_variant_count)
        variant_index = m->projectile_variant_count - 1;
    return sheet_src(m, TEX_PROJECTILES_SHEET, variant_index);
}
int texman_projectile_variant_count(TextureManager *m) {
    return m ? m->projectile_variant_count : 0;
}
static const Uint8 PROJ_COLORS[][3] = {{255, 80, 80}, {80, 255, 100}, {90, 170, 255}, {255, 200, 40}, {200, 120, 255}, {120, 255, 255}};
bool texman_projectile_variant_color(TextureManager *m, int idx, Uint8 *r, Uint8 *g, Uint8 *b) {
    (void)m;
    int max = (int)(sizeof(PROJ_COLORS) / sizeof(PROJ_COLORS[0]));
    if (idx < 0 || idx >= max)
        return false;
    if (r)
        *r = PROJ_COLORS[idx][0];
    if (g)
        *g = PROJ_COLORS[idx][1];
    if (b)
        *b = PROJ_COLORS[idx][2];
    return true;
}
bool texman_regen_nebula(TextureManager *m, uint32_t seed) {
    if (!m)
        return false;
    SDL_Texture *old = m->base[TEX_BG_STARFIELD].tex;
    SDL_Texture *neu = gen_nebula(m->sdl, seed);
    if (!neu)
        return false;
    m->base[TEX_BG_STARFIELD].tex = neu;
    if (old)
        SDL_DestroyTexture(old);
    return true;
}
TexmanStats texman_stats(TextureManager *m) {
    TexmanStats s = {0
};
    if (!m)
        return s;
    for (int i = 0; i < TEX__COUNT; i++)
        if (m->base[i].tex)
            s.base_loaded++;
    s.projectile_variants = m->projectile_variant_count;
    return s;
}
/* ---------------- Explosions generation ---------------------------------- */
static inline Uint32 px_make(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    return ((Uint32)a << 24) | ((Uint32)r << 16) | ((Uint32)g << 8) | b;
}

static void draw_explosion_frame(uint32_t *pixels, int pitch_pixels, int tile, int type, int frame, int total_frames) {

    int size = tile;
    int center = size / 2;
    float frame_progress = (float)frame / (float)(total_frames - 1);
    float max_radius = center * 0.9f;
    struct Config {
        float r, g, b, density, inner, fade;

} cfgs[4] = {
        {1.0f, 0.6f, 0.1f, 1.2f, 0.9f, 0.4f}, // orange
        {0.2f, 0.4f, 1.0f, 1.0f, 0.8f, 0.4f}, // blau
        {0.3f, 1.0f, 0.2f, 0.8f, 0.7f, 0.4f}, // grün
        {1.0f, 0.2f, 0.1f, 1.4f, 1.0f, 0.4f}  // rot
    };
    int idx = type % 4;
    struct Config c = cfgs[idx];
    srand((unsigned)(type * 1337));
    float ring_radius = max_radius * (0.1f + frame_progress * 0.9f);
    float ring_thickness = max_radius * (0.5f - frame_progress * 0.3f);
    int glowing_spots[20][2];
    float spot_int[20];
    for (int i = 0; i < 20; i++) {
        float ang = (float)rand() / RAND_MAX * 6.2831853f;
        float rad = (float)rand() / RAND_MAX * max_radius * 0.8f;
        glowing_spots[i][0] = center + (int)(cosf(ang) * rad);
        glowing_spots[i][1] = center + (int)(sinf(ang) * rad);
        spot_int[i] = 0.3f + ((float)rand() / RAND_MAX) * 0.7f;
    }
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            float dx = (float)x - center;
            float dy = (float)y - center;
            float dist = sqrtf(dx * dx + dy * dy);
            float intensity = 0.f;
            if (dist >= ring_radius - ring_thickness && dist <= ring_radius + ring_thickness) {
                float ring_factor = 1.f - fabsf(dist - ring_radius) / ring_thickness;
                ring_factor = powf(ring_factor, 0.8f);
                intensity += ring_factor * c.inner * (1.f - frame_progress * 0.6f);
    }
            if (frame < total_frames / 3 && dist < ring_radius * 0.7f) {
                float inner_factor = 1.f - dist / (ring_radius * 0.7f);
                intensity += inner_factor * c.inner * (1.f - (float)frame / (float)(total_frames / 3));
            }
            for (int i = 0; i < 20; i++) {
                float sx = (float)x - glowing_spots[i][0];
                float sy = (float)y - glowing_spots[i][1];
                float sd = sqrtf(sx * sx + sy * sy);
                float spot_r = max_radius * 0.15f * (1.f + frame_progress * 0.8f);
                if (sd < spot_r) {
                    float sf = powf(1.f - sd / spot_r, 1.2f);
                    intensity += spot_int[i] * sf * c.density * (1.f - frame_progress * 0.5f);
            }
            }
            float noise_seed = x * 0.1f + y * 0.07f;
            float particle_noise = 0.5f + 0.5f * sinf(noise_seed * 3.0f) * cosf(noise_seed * 2.5f);
            if (particle_noise > 0.6f && dist < ring_radius * 1.3f && dist > ring_radius * 0.2f) {
                float p_int = (particle_noise - 0.6f) / 0.4f;
                float df = 1.f - fabsf(dist - ring_radius) / (ring_radius * 0.8f);
                if (df < 0)
                    df = 0;
                p_int *= df * c.density * (1.f - frame_progress * 0.7f);
                intensity += p_int * 0.4f;
            }
            if (frame > total_frames / 4 && dist > ring_radius * 0.7f && dist < max_radius * 1.4f) {
                float smoke_factor = 1.f - (dist - ring_radius * 0.7f) / (max_radius * 0.7f);
                if (smoke_factor < 0)
                    smoke_factor = 0;
                float smoke_noise = 0.5f + 0.5f * sinf(x * 0.03f + y * 0.05f) * cosf(x * 0.04f - y * 0.06f);
                if (smoke_noise > 0.4f) {
                    float si = (smoke_noise - 0.4f) / 0.6f;
                    intensity += smoke_factor * si * 0.3f * ((float)(frame - total_frames / 4) / (float)(total_frames - total_frames / 4));
            }
            }
            if (intensity > 0.01f) {
                if (intensity > 1.f)
                    intensity = 1.f;
                float sat_boost = 1.f + intensity * 0.2f;
                float heat = intensity * (1.f - frame_progress * 0.4f);
                Uint8 r8 = (Uint8)(255 * intensity * c.r * sat_boost * (0.8f + heat * 0.2f));
                Uint8 g8 = (Uint8)(255 * intensity * c.g * sat_boost * (0.6f + heat * 0.4f));
                Uint8 b8 = (Uint8)(255 * intensity * c.b * sat_boost * (0.4f + heat * 0.6f));
                Uint8 a8 = (Uint8)(255 * intensity * (1.f - frame_progress * 0.3f));
                pixels[y * pitch_pixels + x] = px_make(r8, g8, b8, a8);
            }
        }
    }
}
static SDL_Texture *gen_explosions_sheet(SDL_Renderer *r, int *out_types, int *out_frames, int *out_tile) {
    int types = 4;
    int frames = 12; /* 0..11 original logic */
    int tile = 64;   /* each explosion frame 64x64 */
    int w = tile * frames;
    int h = tile * types;
    SDL_Texture *tex = SDL_CreateTexture(r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
    if (!tex)
        return NULL;
    void *pixels;
    int pitch;
    if (SDL_LockTexture(tex, NULL, &pixels, &pitch) != 0) {
        SDL_DestroyTexture(tex);
        return NULL;
}
    int pitch_pixels = pitch / 4;
    uint32_t *px = (uint32_t *)pixels;
    for (int t = 0; t < types; t++) {
        for (int f = 0; f < frames; f++) {
            uint32_t *frame_base = px + (t * tile * pitch_pixels) + (f * tile);
            draw_explosion_frame(frame_base, pitch_pixels, tile, t, f, frames);
    }
    }
    SDL_UnlockTexture(tex);
    if (out_types)
        *out_types = types;
    if (out_frames)
        *out_frames = frames;
    if (out_tile)
        *out_tile = tile;
    return tex;
}
SDL_Texture *texman_explosions_texture(TextureManager *m) {
    return m ? m->explosions_sheet : NULL;
}
int texman_explosion_type_count(TextureManager *m) {
    return m ? m->explosion_types : 0;
}
int texman_explosion_frames(TextureManager *m) {
    return m ? m->explosion_frames : 0;
}
SDL_Rect texman_explosion_src(TextureManager *m, int explosion_type, int frame) {
    SDL_Rect r = {0, 0, 0, 0
};
    if (!m || !m->explosions_sheet)
        return r;
    if (m->explosion_types <= 0 || m->explosion_frames <= 0)
        return r;
    if (explosion_type < 0)
        explosion_type = 0;
    if (explosion_type >= m->explosion_types)
        explosion_type = m->explosion_types - 1;
    if (frame < 0)
        frame = 0;
    if (frame >= m->explosion_frames)
        frame = m->explosion_frames - 1;
    r.x = frame * m->explosion_tile;
    r.y = explosion_type * m->explosion_tile;
    r.w = r.h = m->explosion_tile;
    return r;
}
static SDL_Texture *gen_projectiles_sheet(SDL_Renderer *r, int *out_variants) {
    int tile = 8;
    int variants = (int)(sizeof(PROJ_COLORS) / sizeof(PROJ_COLORS[0]));
    int w = tile * variants, h = tile;
    SDL_Texture *tex = SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
    if (!tex)
        return NULL;
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    SDL_Texture *prev = SDL_GetRenderTarget(r);
    SDL_SetRenderTarget(r, tex);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    for (int v = 0; v < variants; v++) {
        Uint8 rc = PROJ_COLORS[v][0], gc = PROJ_COLORS[v][1], bc = PROJ_COLORS[v][2];
        for (int y = 0; y < tile; y++) {
            for (int x = 0; x < tile; x++) {
                float fx = (x + 0.5f) - tile * 0.5f;
                float fy = (y + 0.5f) - tile * 0.5f;
                float d2 = fx * fx + fy * fy;
                float rad = (tile * 0.5f) - 0.5f;
                if (d2 <= rad * rad)
                    SDL_SetRenderDrawColor(r, rc, gc, bc, 255);
                else
                    SDL_SetRenderDrawColor(r, 0, 0, 0, 0);
                SDL_RenderDrawPoint(r, v * tile + x, y);
}
        }
    }
    SDL_SetRenderTarget(r, prev);
    if (out_variants)
        *out_variants = variants;
    return tex;
}
static SDL_Texture *gen_projectiles_sheet_proc(SDL_Renderer *r, uint32_t seed) {
    (void)seed;
    int v = 0;
    return gen_projectiles_sheet(r, &v);
}
/* Nebula algorithm exactly as specified in nebule.c (multi-center falloff + trig noise + limited stars) */
static inline float fast_exp(float x) {
    union {
        uint32_t i;
        float f;
} u;
    u.i = (uint32_t)(12102203 * x + 1065353216);
    return u.f;
}
static inline Uint32 make_color(Uint8 r8, Uint8 g8, Uint8 b8, Uint8 a8) {
    return ((Uint32)a8 << 24) | ((Uint32)r8 << 16) | ((Uint32)g8 << 8) | b8;
}
static SDL_Texture *gen_nebula(SDL_Renderer *r, uint32_t seed) {
    const int w = DISPLAY_W, h = DISPLAY_H;
    SDL_Texture *tex = SDL_CreateTexture(r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, w, h);
    if (!tex)
        return NULL;
    Uint32 *px = (Uint32 *)malloc((size_t)w * h * sizeof(Uint32));
    if (!px) {
        SDL_DestroyTexture(tex);
        return NULL;
}
    /* follow nebule.c: if seed provided use it, else random */
    int nebula_seed = seed ? (int)seed : rand();
    srand(nebula_seed);
    int num_centers = 5 + (rand() % 4); /* 5-8 centers */
    struct Center {
        float x, y, intensity, inv_radius;
        int color_type;
    } centers[10];
    for (int i = 0; i < num_centers; i++) {
        centers[i].x = (float)(rand() % w);
        centers[i].y = (float)(rand() % h);
        centers[i].intensity = 0.4f + (rand() / (float)RAND_MAX) * 0.5f;
        centers[i].color_type = rand() % 5;
        float base_radius = 120.f + (float)(rand() % 80); /* fixed per center */
        centers[i].inv_radius = 1.f / base_radius;
    }
    float *sin_lut = (float *)malloc(sizeof(float) * w);
    float *cos_lut_x = (float *)malloc(sizeof(float) * w);
    float *cos_lut_y = (float *)malloc(sizeof(float) * h);
    if (!sin_lut || !cos_lut_x || !cos_lut_y) {
        free(px);
        free(sin_lut);
        free(cos_lut_x);
        free(cos_lut_y);
        SDL_DestroyTexture(tex);
        return NULL;
    }
    for (int x = 0; x < w; x++) {
        sin_lut[x] = sinf(x * 0.015f);
        cos_lut_x[x] = cosf(x * 0.018f);
    }
    for (int y = 0; y < h; y++) {
        cos_lut_y[y] = cosf(y * 0.014f);
    }
    for (int y = 0; y < h; y++) {
        float y_component = y * 0.012f;
        for (int x = 0; x < w; x++) {
            float total_intensity = 0.f;
            float red = 0.f, green = 0.f, blue = 0.f;
            float noise_factor = 1.0f + 0.25f * sin_lut[x] * sinf(y_component) * cos_lut_x[x] * cos_lut_y[y];
            for (int c = 0; c < num_centers; c++) {
                float dx = x - centers[c].x;
                float dy = y - centers[c].y;
                float dist = sqrtf(dx * dx + dy * dy);
                float falloff = fast_exp(-dist * centers[c].inv_radius) * centers[c].intensity;
                falloff *= noise_factor;
                total_intensity += falloff;
                switch (centers[c].color_type) {
                case 0: /* blue */
                    blue += falloff * 0.85f;
                    green += falloff * 0.15f;
                    red += falloff * 0.05f;
                    break;
                case 1: /* purple */
                    red += falloff * 0.70f;
                    blue += falloff * 0.80f;
                    green += falloff * 0.10f;
                    break;
                case 2: /* dark red */
                    red += falloff * 0.90f;
                    green += falloff * 0.05f;
                    blue += falloff * 0.20f;
                    break;
                case 3: /* cyan */
                    blue += falloff * 0.80f;
                    green += falloff * 0.60f;
                    red += falloff * 0.10f;
                    break;
                case 4: /* violet */
                    red += falloff * 0.50f;
                    blue += falloff * 0.90f;
                    green += falloff * 0.05f;
                    break;
    }
            }
            float base_intensity = 0.08f;
            red = fminf((red + base_intensity) * 85.0f, 120.0f);
            green = fminf((green + base_intensity * 0.5f) * 60.0f, 80.0f);
            blue = fminf((blue + base_intensity) * 100.0f, 140.0f);
            float alpha_intensity = total_intensity + base_intensity;
            Uint8 alpha = (Uint8)fminf(alpha_intensity * 200.0f + 40.0f, 200.0f);
            px[y * (size_t)w + x] = make_color((Uint8)red, (Uint8)green, (Uint8)blue, alpha);
        }
    }
    free(sin_lut);
    free(cos_lut_x);
    free(cos_lut_y);
    for (int i = 0; i < 600; i++) {
        int sx = rand() % w;
        int sy = rand() % h;
        Uint8 bright = (Uint8)(120 + rand() % 86);
        px[sy * (size_t)w + sx] = make_color(bright, bright, bright, 255);
    }
    SDL_UpdateTexture(tex, NULL, px, w * (int)sizeof(Uint32));
    free(px);
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    return tex;
}
