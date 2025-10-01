#include "scenestack.h"
#include <stdlib.h>
#include <string.h>

void scenestack_init(SceneStack *st) {
    memset(st, 0, sizeof(*st));
}

/**
 * @brief Destroy a scene and set its pointer to NULL
 * @param s Pointer to scene pointer
 */
static void destroy_scene(Scene **s) {
    if (*s) {
        scene_destroy(*s);
        *s = NULL;
    }
}

void scenestack_shutdown(SceneStack *st) {
    for (int i = 0; i < st->overlay_count; i++) {
        destroy_scene(&st->overlays[i]);
    }
    destroy_scene(&st->base);
}

bool scenestack_set_base(SceneStack *st, SceneID id) {
    destroy_scene(&st->base);
    st->base = scene_create(id);
    if (!st->base) {
        return false;
    }
    if (st->base->vt && st->base->vt->enter) {
        st->base->vt->enter(st->base);
    }
    return true;
}

bool scenestack_push(SceneStack *st, SceneID id) {
    if (st->overlay_count >= (int)(sizeof(st->overlays) / sizeof(st->overlays[0]))) {
        return false;
    }
    Scene *s = scene_create(id);
    if (!s) {
        return false;
    }
    s->is_overlay = true;
    st->overlays[st->overlay_count++] = s;
    if (s->vt && s->vt->enter) {
        s->vt->enter(s);
    }
    return true;
}

void scenestack_pop(SceneStack *st) {
    if (st->overlay_count <= 0) {
        return;
    }
    Scene *top = st->overlays[--st->overlay_count];
    scene_destroy(top);
}

void scenestack_handle_input(SceneStack *st, const struct InputState *in) {
    for (int i = st->overlay_count - 1; i >= 0; i--) {
        Scene *s = st->overlays[i];
        if (s->vt && s->vt->handle_input) {
            s->vt->handle_input(s, in);
        }
        if (s->blocks_under_input) {
            return;
        }
    }
    if (st->base && st->base->vt && st->base->vt->handle_input) {
        st->base->vt->handle_input(st->base, in);
    }
}

void scenestack_update(SceneStack *st, float dt) {
    if (st->base) {
        if (st->overlay_count == 0) {
            if (st->base->vt && st->base->vt->update) {
                st->base->vt->update(st->base, dt);
            }
        } else {
            bool blocked = false;
            for (int i = st->overlay_count - 1; i >= 0; i--) {
                Scene *s = st->overlays[i];
                if (s->vt && s->vt->update) {
                    s->vt->update(s, dt);
                }
                if (s->blocks_under_update) {
                    blocked = true;
                    break;
                }
            }
            if (!blocked && st->base->vt && st->base->vt->update) {
                st->base->vt->update(st->base, dt);
            }
        }
    }
}

void scenestack_render(SceneStack *st, struct Renderer *r) {
    if (st->base && st->base->vt && st->base->vt->render) {
        st->base->vt->render(st->base, r);
    }
    for (int i = 0; i < st->overlay_count; i++) {
        Scene *s = st->overlays[i];
        if (s->vt && s->vt->render) {
            s->vt->render(s, r);
        }
    }
}
