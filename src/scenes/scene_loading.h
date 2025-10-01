#pragma once
#include "../app/scene.h"

typedef struct SceneLoadingState { struct Services *svc; const int* list; int count; int loaded; } SceneLoadingState;
void scene_loading_enter(Scene *s);
void scene_loading_leave(Scene *s);
void scene_loading_update(Scene *s, float dt);
void scene_loading_render(Scene *s, struct Renderer *r);
