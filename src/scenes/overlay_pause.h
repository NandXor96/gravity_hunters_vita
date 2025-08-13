#pragma once
#include "../app/scene.h"

typedef struct OverlayPauseState { struct Services* svc; int selected; } OverlayPauseState;

void overlay_pause_enter(Scene* s);
void overlay_pause_leave(Scene* s);
void overlay_pause_handle_input(Scene* s, const struct InputState* in);
void overlay_pause_update(Scene* s, float dt);
void overlay_pause_render(Scene* s, struct Renderer* r);
