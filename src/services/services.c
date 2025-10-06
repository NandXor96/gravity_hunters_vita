#include "services.h"
#include <stdlib.h>
#include <stdio.h>
#include "renderer.h"
#include "input.h"
#include "texture_manager.h"
#include "audio.h"
#include "../core/types.h"
#include "../core/log.h"
#include <time.h>

static Services g_services;
TextureManager *g_texman = NULL; /* global for quick projectile sheet src access */
Services *services_get(void) {
    return &g_services;
}

void services_init(Services *s) {

    s->display_w = DISPLAY_W; s->display_h = DISPLAY_H;
  s->window = SDL_CreateWindow("Gravity Hunters", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, s->display_w, s->display_h, 0);
  if (!s->window) LOG_ERROR("services", "Window creation failed: %s", SDL_GetError());
  s->sdl_renderer = SDL_CreateRenderer(s->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  s->renderer = renderer_create(s->sdl_renderer);
  s->input = input_create();
#ifdef PLATFORM_VITA
  const char *assets_root = "app0:/assets";
#else
  const char *assets_root = "./assets";
#endif
  s->texman = texman_create(s->sdl_renderer, assets_root);
  texman_load_textures(s->texman, 1337u);
  Uint64 counter = SDL_GetPerformanceCounter();
  uint32_t nebula_seed = (uint32_t)(counter ^ (Uint64)time(NULL));
  if (nebula_seed == 0)
    nebula_seed = 0xA5A5F00Du ^ (uint32_t)counter;
  if (!texman_regen_nebula(s->texman, nebula_seed))
    LOG_WARN("services", "Failed to regenerate nebula texture");
  else
    LOG_INFO("services", "Generated nebula seed=%08x", nebula_seed);
  g_texman = s->texman;

  s->audio = audio_create(assets_root);

}

void services_shutdown(Services *s) {

    if (s->audio) {
        audio_destroy(s->audio);
        s->audio = NULL;
    }

    if (s->texman) { texman_destroy(s->texman); g_texman=NULL;

}
  if (s->input) input_destroy(s->input);
  if (s->renderer) renderer_destroy(s->renderer);
  if (s->sdl_renderer) SDL_DestroyRenderer(s->sdl_renderer);
  if (s->window) SDL_DestroyWindow(s->window);
}
