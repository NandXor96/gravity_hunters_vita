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
#include "../scenes/overlay_pause.h"
#include "../scenes/overlay_endgame.h"
#include "../scenes/overlay_endgame_qp.h"
#include "../scenes/scene_loading.h"

static SceneVTable vt_menu = {scene_menu_enter, scene_menu_leave, scene_menu_handle_input, scene_menu_update, scene_menu_render};
static SceneVTable vt_campaign_menu = {scene_campaign_menu_enter, scene_campaign_menu_leave, scene_campaign_menu_handle_input, scene_campaign_menu_update, scene_campaign_menu_render};
static SceneVTable vt_help = {scene_help_enter, scene_help_leave, scene_help_handle_input, scene_help_update, scene_help_render};
static SceneVTable vt_credits = {scene_credits_enter, scene_credits_leave, scene_credits_handle_input, scene_credits_update, scene_credits_render};
static SceneVTable vt_qp_menu = {scene_quick_play_menu_enter, scene_quick_play_menu_leave, scene_quick_play_menu_handle_input, scene_quick_play_menu_update, scene_quick_play_menu_render};
static SceneVTable vt_game = {scene_quick_play_enter, scene_quick_play_leave, scene_quick_play_handle_input, scene_quick_play_update, scene_quick_play_render};
static SceneVTable vt_camp = {scene_campaign_enter, scene_campaign_leave, scene_campaign_handle_input, scene_campaign_update, scene_campaign_render};
static SceneVTable vt_campaign_details = {overlay_campaign_details_enter, overlay_campaign_details_leave, overlay_campaign_details_handle_input, overlay_campaign_details_update, overlay_campaign_details_render};
static SceneVTable vt_pause = {overlay_pause_enter, overlay_pause_leave, overlay_pause_handle_input, overlay_pause_update, overlay_pause_render};
static SceneVTable vt_endgame_qp = {overlay_endgame_qp_enter, overlay_endgame_qp_leave, overlay_endgame_qp_handle_input, overlay_endgame_qp_update, overlay_endgame_qp_render};
static SceneVTable vt_endgame = {overlay_endgame_enter, overlay_endgame_leave, overlay_endgame_handle_input, overlay_endgame_update, overlay_endgame_render};
static SceneVTable vt_endgame_camp = {overlay_endgame_enter, overlay_endgame_leave, overlay_endgame_handle_input, overlay_endgame_update, overlay_endgame_render};
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
    case SCENE_OVERLAY_PAUSE:
        s->vt = &vt_pause;
        s->is_overlay = true;
        s->blocks_under_input = true;
        s->blocks_under_update = true;
        break;
    case SCENE_OVERLAY_ENDGAME:
        /* legacy id -> quickplay endgame overlay */
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
