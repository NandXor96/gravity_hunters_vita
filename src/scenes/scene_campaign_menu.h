#pragma once

#include "../app/scene.h"
#include "../game/campaign_levels.h"

typedef struct SceneCampaignMenuState {
    struct Services *svc;
    CampaignLevelList descriptors;
    CampaignLevelCatalog catalog;
    int selected_index;
    int first_visible_index;
    int suppress_input_frames;
    int prev_move_up;
    int prev_move_down;
    int prev_confirm;
    int prev_back;
} SceneCampaignMenuState;

void scene_campaign_menu_enter(Scene *s);
void scene_campaign_menu_leave(Scene *s);
void scene_campaign_menu_handle_input(Scene *s, const struct InputState *in);
void scene_campaign_menu_update(Scene *s, float dt);
void scene_campaign_menu_render(Scene *s, struct Renderer *r);
void scene_campaign_menu_suppress_input(int frames);
void scene_campaign_menu_focus_last_unlocked(void);
