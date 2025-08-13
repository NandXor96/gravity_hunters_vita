#pragma once
#include "../app/scene.h"

typedef struct SceneCampaignState { struct Services* svc; u32 current_level_seed; int level_index; } SceneCampaignState;

void scene_campaign_enter(Scene* s);
void scene_campaign_leave(Scene* s);
void scene_campaign_handle_input(Scene* s, const struct InputState* in);
void scene_campaign_update(Scene* s, float dt);
void scene_campaign_render(Scene* s, struct Renderer* r);
