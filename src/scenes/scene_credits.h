#pragma once
#include "../app/scene.h"

void scene_credits_enter(Scene *s);
void scene_credits_leave(Scene *s);
void scene_credits_handle_input(Scene *s, const struct InputState *in);
void scene_credits_update(Scene *s, float dt);
void scene_credits_render(Scene *s, struct Renderer *r);
