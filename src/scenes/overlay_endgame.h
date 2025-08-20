#pragma once
#include "../app/scene.h"

typedef struct OverlayEndgameState {
    struct Services *svc;
    int selected;
    bool prev_confirm;
} OverlayEndgameState;

void overlay_endgame_enter(Scene* s);
void overlay_endgame_leave(Scene* s);
void overlay_endgame_handle_input(Scene* s, const struct InputState* in);
void overlay_endgame_update(Scene* s, float dt);
void overlay_endgame_render(Scene* s, struct Renderer* r);
/* Setter to provide final stats to the overlay before pushing it */
void overlay_endgame_set_stats(int kills, int deaths, int points);
