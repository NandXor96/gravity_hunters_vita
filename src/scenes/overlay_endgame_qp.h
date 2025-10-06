#pragma once
#include "../app/scene.h"

typedef struct OverlayEndgameStateQP {
    struct Services *svc;
    int selected;
    bool prev_confirm;
} OverlayEndgameStateQP;

void overlay_endgame_qp_enter(Scene *s);
void overlay_endgame_qp_leave(Scene *s);
void overlay_endgame_qp_handle_input(Scene *s, const struct InputState *in);
void overlay_endgame_qp_update(Scene *s, float dt);
void overlay_endgame_qp_render(Scene *s, struct Renderer *r);
/* Setter to provide final stats to the overlay before pushing it */
void overlay_endgame_qp_set_stats(int kills, int points);
