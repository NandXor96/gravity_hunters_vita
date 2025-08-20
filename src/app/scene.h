#pragma once
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "../core/types.h"

typedef enum {
  SCENE_MENU,
  SCENE_QUICK_PLAY_MENU,
  SCENE_QUICK_PLAY,
  SCENE_CAMPAIGN,
  SCENE_OVERLAY_PAUSE,
  SCENE_OVERLAY_ENDGAME,
  SCENE_LOADING
} SceneID;

struct InputState;
struct Renderer;

typedef struct Scene Scene;

typedef struct SceneVTable {
  void (*enter)(Scene*);
  void (*leave)(Scene*);
  void (*handle_input)(Scene*, const struct InputState*);
  void (*update)(Scene*, float dt);
  void (*render)(Scene*, struct Renderer*);
} SceneVTable;

struct Scene {
  const SceneVTable* vt;
  void* state;
  bool is_overlay;
  bool blocks_under_update;
  bool blocks_under_input;
  SceneID id;
};

Scene* scene_create(SceneID id);
void   scene_destroy(Scene* s);
