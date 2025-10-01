#include "scene_loading.h"
#include <stdlib.h>
#include "../core/log.h"
#include "../services/renderer.h"
#include "../app/app.h"

void scene_loading_enter(Scene *s) {

    SceneLoadingState *st = calloc(1, sizeof(SceneLoadingState));
    if (!st) {
        LOG_ERROR("scene_loading", "Failed to allocate scene state");
        return;

}
    s->state = st;
}
void scene_loading_leave(Scene *s) {
    free(s->state);
}
void scene_loading_update(Scene *s,float dt) {
    (void)s;(void)dt;
}
void scene_loading_render(Scene *s, struct Renderer *r) {
    renderer_draw_text(r,"LOADING", 40,40,(TextStyle){0
}); }
