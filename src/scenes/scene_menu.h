#pragma once
#include "../app/scene.h"

typedef struct MenuItem { const char* label; int enabled; SceneID target; } MenuItem;

typedef struct SceneMenuState { struct Services *svc; MenuItem *items; int count; int selected; /* input edge state */ int prev_move_up; int prev_move_down; int prev_confirm; /* frames to suppress input after entering scene */ int suppress_input_frames; } SceneMenuState;

void scene_menu_enter(Scene *s);
void scene_menu_leave(Scene *s);
void scene_menu_handle_input(Scene *s, const struct InputState *in);
void scene_menu_update(Scene *s, float dt);
void scene_menu_render(Scene *s, struct Renderer *r);
