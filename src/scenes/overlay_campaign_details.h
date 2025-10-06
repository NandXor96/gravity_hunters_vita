#pragma once

#include <stdint.h>
#include "../app/scene.h"
#include "../game/campaign_levels.h"

struct Services;

typedef struct OverlayCampaignDetailsState {
    struct Services *svc;
    char level_filename[CAMPAIGN_LEVEL_MAX_FILENAME];
    char *start_text;
    uint32_t time_limit;
    uint32_t rating[3];
    uint16_t goal_kills;
    int load_ok;
    int wait_release;
    int prev_confirm;
    int prev_back;
} OverlayCampaignDetailsState;

void overlay_campaign_details_prepare(const char *filename);

void overlay_campaign_details_enter(Scene *s);
void overlay_campaign_details_leave(Scene *s);
void overlay_campaign_details_handle_input(Scene *s, const struct InputState *in);
void overlay_campaign_details_update(Scene *s, float dt);
void overlay_campaign_details_render(Scene *s, struct Renderer *r);
