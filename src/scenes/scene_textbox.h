#pragma once

#include <stdbool.h>

#include "../app/scene.h"

typedef struct TextboxSceneConfig {
    const char *title;
    const char *relative_path; /* relative to assets root */
    const char *const *fallback_lines;
    int fallback_count;
    bool use_custom_prompts;
    bool show_ok_prompt;
    bool show_back_prompt;
} TextboxSceneConfig;

void textbox_scene_enter(Scene *s, const TextboxSceneConfig *config);
void textbox_scene_leave(Scene *s);
void textbox_scene_handle_input(Scene *s, const struct InputState *in);
void textbox_scene_update(Scene *s, float dt);
void textbox_scene_render(Scene *s, struct Renderer *r);
