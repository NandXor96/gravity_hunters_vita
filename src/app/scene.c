#include "scene.h"
#include <stdlib.h>
#include "../scenes/scene_menu.h"
#include "../scenes/scene_campaign_menu.h"
#include "../scenes/scene_help.h"
#include "../scenes/scene_credits.h"
#include "../scenes/scene_quick_play_menu.h"
#include "../scenes/scene_quick_play.h"
#include "../scenes/scene_campaign.h"
#include "../scenes/overlay_campaign_details.h"
#include "../scenes/overlay_start_game.h"
#include "../scenes/overlay_pause.h"
#include "../scenes/overlay_endgame.h"
#include "../scenes/overlay_endgame_qp.h"
#include "../scenes/scene_loading.h"

/**
 * @brief Macro to reduce verbosity in VTable initialization.
 * Automatically creates a static SceneVTable with standard naming convention.
 */
#define SCENE_VTABLE(vtable_name, prefix) \
    static SceneVTable vtable_name = { \
        prefix##_enter, \
        prefix##_leave, \
        prefix##_handle_input, \
        prefix##_update, \
        prefix##_render \
    }

/**
 * @brief Macro for VTables with custom function pointers (e.g., NULL handle_input).
 */
#define SCENE_VTABLE_CUSTOM(vtable_name, enter_fn, leave_fn, input_fn, update_fn, render_fn) \
    static SceneVTable vtable_name = { \
        enter_fn, \
        leave_fn, \
        input_fn, \
        update_fn, \
        render_fn \
    }

// Standard VTable definitions using consistent naming
SCENE_VTABLE(vt_menu, scene_menu);
SCENE_VTABLE(vt_campaign_menu, scene_campaign_menu);
SCENE_VTABLE(vt_help, scene_help);
SCENE_VTABLE(vt_credits, scene_credits);
SCENE_VTABLE(vt_qp_menu, scene_quick_play_menu);
SCENE_VTABLE(vt_game, scene_quick_play);
SCENE_VTABLE(vt_camp, scene_campaign);
SCENE_VTABLE(vt_campaign_details, overlay_campaign_details);
SCENE_VTABLE(vt_start_game, overlay_start_game);
SCENE_VTABLE(vt_pause, overlay_pause);
SCENE_VTABLE(vt_endgame_qp, overlay_endgame_qp);
SCENE_VTABLE(vt_endgame, overlay_endgame);
SCENE_VTABLE(vt_endgame_camp, overlay_endgame);

// Loading scene has NULL handle_input, so uses custom macro
SCENE_VTABLE_CUSTOM(vt_load, scene_loading_enter, scene_loading_leave, NULL, scene_loading_update, scene_loading_render);

Scene *scene_create(SceneID id) {
    Scene *s = calloc(1, sizeof(Scene));
    s->id = id;
    switch (id) {
    case SCENE_MENU:
        s->vt = &vt_menu;
        break;
    case SCENE_CAMPAIGN_MENU:
        s->vt = &vt_campaign_menu;
        break;
    case SCENE_HELP:
        s->vt = &vt_help;
        break;
    case SCENE_CREDITS:
        s->vt = &vt_credits;
        break;
    case SCENE_QUICK_PLAY_MENU:
        s->vt = &vt_qp_menu;
        break;
    case SCENE_QUICK_PLAY:
        s->vt = &vt_game;
        break;
    case SCENE_CAMPAIGN:
        s->vt = &vt_camp;
        break;
    case SCENE_OVERLAY_CAMPAIGN_DETAILS:
        s->vt = &vt_campaign_details;
        s->is_overlay = true;
        s->blocks_under_input = true;
        s->blocks_under_update = true;
        break;
    case SCENE_OVERLAY_START_GAME:
        s->vt = &vt_start_game;
        s->is_overlay = true;
        s->blocks_under_input = true;
        s->blocks_under_update = true;
        break;
    case SCENE_OVERLAY_PAUSE:
        s->vt = &vt_pause;
        s->is_overlay = true;
        s->blocks_under_input = true;
        s->blocks_under_update = true;
        break;
    case SCENE_OVERLAY_ENDGAME:
        s->vt = &vt_endgame_qp;
        s->is_overlay = true;
        s->blocks_under_input = true;
        s->blocks_under_update = true;
        break;
    case SCENE_OVERLAY_ENDGAME_CAMPAIGN:
        s->vt = &vt_endgame_camp;
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

void scene_destroy(Scene *s) {
    if (!s) {
        return;
    }
    if (s->state && s->vt && s->vt->leave) {
        s->vt->leave(s);
    }
    free(s);
}
