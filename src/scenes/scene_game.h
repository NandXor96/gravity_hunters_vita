#pragma once
#include "../app/scene.h"
#include "../services/input.h" // for InputState
struct World;
struct Services;
struct Renderer;

typedef struct SceneGameState
{
    struct Services *svc;
    struct World *world;
    bool show_hud_text;
    char hud_text[256];
    struct InputState last_input; // store last input for updates
    int time_over_handled; // guard
} SceneGameState;

void scene_game_pause(Scene *s);
void scene_game_resume(Scene *s);
void scene_game_display_text(Scene *s, const char *msg);
void scene_game_display_hud(Scene *s, bool show);

void scene_game_enter(Scene *s);
void scene_game_leave(Scene *s);
void scene_game_handle_input(Scene *s, const struct InputState *in);
void scene_game_update(Scene *s, float dt);
void scene_game_render(Scene *s, struct Renderer *r);
