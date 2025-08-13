#include "overlay_pause.h"
#include <stdlib.h>
#include "../services/renderer.h"
#include "../app/app.h"

void overlay_pause_enter(Scene *s)
{
    OverlayPauseState *st = calloc(1, sizeof(OverlayPauseState));
    s->state = st;
    s->is_overlay = true;
    s->blocks_under_input = true;
    s->blocks_under_update = true;
}
void overlay_pause_leave(Scene *s) { free(s->state); }
void overlay_pause_handle_input(Scene *s, const struct InputState *in)
{
    (void)s;
    (void)in;
}
void overlay_pause_update(Scene *s, float dt)
{
    (void)s;
    (void)dt;
}
void overlay_pause_render(Scene *s, struct Renderer *r) { renderer_draw_text(r, "PAUSED", 200, 200, (TextStyle){0}); }
