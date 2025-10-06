#include "scene_help.h"

#include "scene_help.h"

#include "scene_textbox.h"

static const char *const HELP_FALLBACK_LINES[] = {
    "Help file missing."
};

static const TextboxSceneConfig HELP_CONFIG = {
    .title = "HELP",
    .relative_path = "help.txt",
    .fallback_lines = HELP_FALLBACK_LINES,
    .fallback_count = (int)(sizeof(HELP_FALLBACK_LINES) / sizeof(HELP_FALLBACK_LINES[0])),
    .use_custom_prompts = true,
    .show_ok_prompt = false,
    .show_back_prompt = true
};

void scene_help_enter(Scene *s) {

    textbox_scene_enter(s, &HELP_CONFIG);

}
void scene_help_leave(Scene *s) {
    textbox_scene_leave(s);
}
void scene_help_handle_input(Scene *s, const struct InputState *in) {
    textbox_scene_handle_input(s, in);
}
void scene_help_update(Scene *s, float dt) {
    textbox_scene_update(s, dt);
}
void scene_help_render(Scene *s, struct Renderer *r) {
    textbox_scene_render(s, r);
}
