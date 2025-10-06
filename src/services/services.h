#pragma once
#include <SDL2/SDL.h>
struct Renderer; struct TextureManager; struct Input; struct Audio;

typedef struct Services {
  SDL_Window *window;
  SDL_Renderer *sdl_renderer;
  struct Renderer *renderer;
  struct TextureManager *texman;
  struct Input *input;
  struct Audio *audio;
  int display_w, display_h;
} Services;

Services *services_get(void);
void services_init(Services*);
void services_shutdown(Services*);
