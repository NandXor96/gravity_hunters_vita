#pragma once
#include "scene.h"

typedef struct SceneStack {
  Scene* base;            // base scene
  Scene* overlays[8];
  int overlay_count;
} SceneStack;

void scenestack_init(SceneStack* st);
void scenestack_shutdown(SceneStack* st);
bool scenestack_set_base(SceneStack* st, SceneID id);
bool scenestack_push(SceneStack* st, SceneID id);
void scenestack_pop(SceneStack* st);
void scenestack_handle_input(SceneStack* st, const struct InputState* in);
void scenestack_update(SceneStack* st, float dt);
void scenestack_render(SceneStack* st, struct Renderer* r);
