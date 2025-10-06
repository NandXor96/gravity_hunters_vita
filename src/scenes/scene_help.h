#pragma once
#include "../app/scene.h"

void scene_help_enter(Scene *s);
void scene_help_leave(Scene *s);
void scene_help_handle_input(Scene *s, const struct InputState *in);
void scene_help_update(Scene *s, float dt);
void scene_help_render(Scene *s, struct Renderer *r);
