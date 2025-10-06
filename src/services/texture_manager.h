#pragma once
#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdbool.h>

/* Unified, spec-driven always-loaded texture manager (replaces legacy). */

typedef enum {
	TEX_PLANETS_SHEET,
	TEX_PLAYER,
	TEX_BG_STARFIELD,
	TEX_LOGO,
	TEX_ICONS_SHEET,
	TEX_PROJECTILES_SHEET,
	TEX_ENEMIES_SHEET, /* enemies atlas */
	TEX_EXPLOSIONS_SHEET, /* procedural multi-type multi-frame explosions */
	TEX_UI_BUTTON_CROSS,
	TEX_UI_BUTTON_CIRCLE,
	TEX__COUNT
} TextureID;

typedef struct TextureManager TextureManager;

TextureManager *texman_create(SDL_Renderer *sdl, const char* assets_root);
bool 			texman_load_textures(TextureManager*, uint32_t seed);
void            texman_destroy(TextureManager*);
SDL_Texture *texman_get(TextureManager*, TextureID id);
SDL_Rect        texman_planet_src_rect(int idx);
SDL_Rect        texman_sheet_src(TextureManager*, TextureID id, int index);

/* Projectile sheet API */
SDL_Texture *texman_projectiles_texture(TextureManager*);
SDL_Rect     texman_projectile_src(TextureManager*, int variant_index);
int          texman_projectile_variant_count(TextureManager*);
bool         texman_projectile_variant_color(TextureManager*, int variant_index, Uint8 *r, Uint8 *g, Uint8 *b);

/* Explosions sheet (procedural) */
SDL_Texture *texman_explosions_texture(TextureManager*);
int          texman_explosion_type_count(TextureManager*);
int          texman_explosion_frames(TextureManager*);
SDL_Rect     texman_explosion_src(TextureManager*, int explosion_type, int frame);

/* Nebula regeneration */
bool texman_regen_nebula(TextureManager*, uint32_t seed);

typedef struct TexmanStats { int base_loaded; int projectile_variants; } TexmanStats;
TexmanStats texman_stats(TextureManager*);
