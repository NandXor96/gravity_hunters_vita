#include "scene_credits.h"

#include "scene_textbox.h"

static const char *const CREDITS_FALLBACK_LINES[] = {
    "Credits missing.",
};

static const TextboxSceneConfig CREDITS_CONFIG = {
    .title = "CREDITS",
    .relative_path = "credits.txt",
    .fallback_lines = CREDITS_FALLBACK_LINES,
    .fallback_count = (int)(sizeof(CREDITS_FALLBACK_LINES) / sizeof(CREDITS_FALLBACK_LINES[0])),
    .use_custom_prompts = true,
    .show_ok_prompt = false,
    .show_back_prompt = true
};

void scene_credits_enter(Scene *s) {

    textbox_scene_enter(s, &CREDITS_CONFIG);

}
void scene_credits_leave(Scene *s) {
    textbox_scene_leave(s);
}
void scene_credits_handle_input(Scene *s, const struct InputState *in) {
    textbox_scene_handle_input(s, in);
}
void scene_credits_update(Scene *s, float dt) {
    textbox_scene_update(s, dt);
}
void scene_credits_render(Scene *s, struct Renderer *r) {
    textbox_scene_render(s, r);
}
