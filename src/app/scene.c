#include "scene.h"
#include <stdlib.h>
#include "../scenes/scene_menu.h"
#include "../scenes/scene_quick_play.h"
#include "../scenes/scene_campaign.h"
#include "../scenes/overlay_pause.h"
#include "../scenes/scene_loading.h"

static SceneVTable vt_menu = {scene_menu_enter, scene_menu_leave, scene_menu_handle_input, scene_menu_update, scene_menu_render};
static SceneVTable vt_game = {scene_quick_play_enter, scene_quick_play_leave, scene_quick_play_handle_input, scene_quick_play_update, scene_quick_play_render};
static SceneVTable vt_camp = {scene_campaign_enter, scene_campaign_leave, scene_campaign_handle_input, scene_campaign_update, scene_campaign_render};
static SceneVTable vt_pause = {overlay_pause_enter, overlay_pause_leave, overlay_pause_handle_input, overlay_pause_update, overlay_pause_render};
static SceneVTable vt_load = {scene_loading_enter, scene_loading_leave, NULL, scene_loading_update, scene_loading_render};

Scene *scene_create(SceneID id)
{
    Scene *s = calloc(1, sizeof(Scene));
    s->id = id;
    switch (id)
    {
    case SCENE_MENU:
        s->vt = &vt_menu;
        break;
    case SCENE_QUICK_PLAY:
        s->vt = &vt_game;
        break;
    case SCENE_CAMPAIGN:
        s->vt = &vt_camp;
        break;
    case SCENE_OVERLAY_PAUSE:
        s->vt = &vt_pause;
        s->is_overlay = true;
        s->blocks_under_input = true;
        s->blocks_under_update = true;
        break;
    case SCENE_LOADING:
        s->vt = &vt_load;
        break;
    default:
        break;
    }
    return s;
}
void scene_destroy(Scene *s)
{
    if (!s)
        return; // leave already called by stack logic before destroy
    if (s->state && s->vt && s->vt->leave)
    { /* defensive: ensure not called twice by nulling vt->leave */
        s->vt->leave(s);
    }
    free(s);
}
