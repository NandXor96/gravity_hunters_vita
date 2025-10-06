#pragma once
#include "../app/scene.h"
#include "../services/input.h"

struct Services;

typedef struct OverlayStartGameState {
    struct Services *svc;
    float elapsed;
    float duration;
    int current_value;
} OverlayStartGameState;

void overlay_start_game_enter(Scene *s);
void overlay_start_game_leave(Scene *s);
void overlay_start_game_handle_input(Scene *s, const struct InputState *in);
void overlay_start_game_update(Scene *s, float dt);
void overlay_start_game_render(Scene *s, struct Renderer *r);
