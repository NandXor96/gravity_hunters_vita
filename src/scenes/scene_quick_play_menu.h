#pragma once
#include "../app/scene.h"
#include "../services/services.h"

typedef struct SceneQuickPlayMenuState {
    struct Services *svc;

    int selected_row;

    int planets_count; // 0..2 -> low/med/high
    int planet_size;   // 0..2 -> small/med/large
    int difficulty;    // 0..2 -> easy/normal/hard
    int time_limit;    // index into time_values
} SceneQuickPlayMenuState;

void scene_quick_play_menu_enter(Scene *s);
void scene_quick_play_menu_leave(Scene *s);
void scene_quick_play_menu_handle_input(Scene *s, const struct InputState *in);
void scene_quick_play_menu_update(Scene *s, float dt);
void scene_quick_play_menu_render(Scene *s, struct Renderer *r);
