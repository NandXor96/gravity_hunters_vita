#pragma once
#include "../app/scene.h"
#include "../services/input.h"

typedef struct OverlayPauseState {
	struct Services *svc;
	int selected;      /* 0 = Continue, 1 = Exit */
	int prev_move_up;
	int prev_move_down;
	int prev_confirm;
} OverlayPauseState;

void overlay_pause_enter(Scene *s);
void overlay_pause_leave(Scene *s);
void overlay_pause_handle_input(Scene *s, const struct InputState *in);
void overlay_pause_update(Scene *s, float dt);
void overlay_pause_render(Scene *s, struct Renderer *r);
