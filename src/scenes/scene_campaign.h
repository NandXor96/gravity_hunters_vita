#pragma once

#include "../app/scene.h"
#include "../game/level_loader.h"
#include "../game/world.h"
#include "../services/input.h"
#include "../game/campaign_levels.h"

// Timing constants
#define LEVEL_END_DELAY_SECONDS 1.0f

typedef struct SpawnEntry {
	LevelEnemy template;
	int spawned; // boolean
	int runtime_index; // index in world->enemies after spawn, -1 if none
	void *runtime_ptr; // pointer to spawned Enemy instance (opaque)
	uint32_t target_id; // for on_death
	int target_runtime_index; // -1 if unresolved
	float scheduled_time; // for delayed spawns (-1 = none)
} SpawnEntry;

typedef struct SceneCampaignState {
	struct Services *svc;
	u32 current_level_seed;
	int level_index;
	World *world;
	SpawnEntry *spawns;
	uint32_t spawn_count;
	int debug_printed;
    struct InputState last_input;
	/* level goals/ratings copied from GameLevel so overlay can evaluate results */
	uint16_t goal_kills;
	unsigned int rating[3];
	float level_end_delay;
	int level_end_handled;
	int level_failed;
	int player_death_handled;
	int time_limit_reached;
	char level_filename[CAMPAIGN_LEVEL_MAX_FILENAME];
} SceneCampaignState;

void scene_campaign_enter(Scene *s);
void scene_campaign_leave(Scene *s);
void scene_campaign_handle_input(Scene *s, const struct InputState *in);
void scene_campaign_update(Scene *s, float dt);
void scene_campaign_render(Scene *s, struct Renderer *r);

void scene_campaign_set_level(const char *filename);
const char *scene_campaign_get_level(void);
