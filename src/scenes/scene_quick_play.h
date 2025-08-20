#pragma once
#include "../app/scene.h"
#include "../services/input.h" // for InputState
struct World;
struct Services;
struct Renderer;

typedef struct SceneQuickPlayState
{
    struct Services *svc;
    struct World *world;
    bool show_hud_text;
    char hud_text[256];
    struct InputState last_input; // store last input for updates
    int time_over_handled; // guard
} SceneQuickPlayState;

void scene_quick_play_pause(Scene *s);
void scene_quick_play_resume(Scene *s);
void scene_quick_play_display_text(Scene *s, const char *msg);
void scene_quick_play_display_hud(Scene *s, bool show);

void scene_quick_play_enter(Scene *s);
void scene_quick_play_leave(Scene *s);
void scene_quick_play_handle_input(Scene *s, const struct InputState *in);
void scene_quick_play_update(Scene *s, float dt);
void scene_quick_play_render(Scene *s, struct Renderer *r);
