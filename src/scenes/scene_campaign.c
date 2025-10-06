#include "scene_campaign.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../core/log.h"
#include "../services/services.h"
#include "../services/renderer.h"
#include "../app/app.h"
#include "../game/world.h"
#include "../game/level_loader.h"
#include "../game/enemy.h"
#include "../game/player.h"
#include "overlay_endgame.h"

static void scene_campaign_on_time_over(World *world, void *user);
static void scene_campaign_finish(SceneCampaignState *st);

static char g_campaign_level_to_load[CAMPAIGN_LEVEL_MAX_FILENAME] = "example_level.lvl";

void scene_campaign_set_level(const char *filename) {
    if (filename && filename[0])
        snprintf(g_campaign_level_to_load, sizeof(g_campaign_level_to_load), "%s", filename);
    else
        g_campaign_level_to_load[0] = '\0';
}

const char *scene_campaign_get_level(void) {
    if (g_campaign_level_to_load[0])
        return g_campaign_level_to_load;
    return "example_level.lvl";
}

void scene_campaign_enter(Scene *s) {
    SceneCampaignState *st = calloc(1, sizeof(SceneCampaignState));
    if (!st) {
        LOG_ERROR("scene_campaign", "Failed to allocate scene state");
        return;
    }
    s->state = st;
    st->svc = app_services();
    st->world = NULL;
    st->spawns = NULL;
    st->spawn_count = 0;
    st->level_end_delay = LEVEL_END_DELAY_SECONDS;

    const char *level_to_load = scene_campaign_get_level();

    GameLevel lvl;
    char err[256] = {0};
    if (level_load(level_to_load, &lvl, err, sizeof(err)) != 0) {
        LOG_ERROR("scene_campaign", "Failed to load level '%s': %s", level_to_load ? level_to_load : "?", err);
        return;
    }

    snprintf(st->level_filename, sizeof(st->level_filename), "%s", level_to_load ? level_to_load : "");

    // create world and populate planets/player based on level
    World *w = world_create(st->svc, 0);
    st->world = w;
    if (st->world) {
        st->world->on_time_over = scene_campaign_on_time_over;
        st->world->on_time_over_user = st;
    }
    if (lvl.time_limit > 0)
        world_set_time_limit(w, (float)lvl.time_limit);

    /* copy goals/ratings into scene state for end-of-level evaluation */
    st->goal_kills = lvl.goal_kills;
    st->rating[0] = lvl.rating[0];
    st->rating[1] = lvl.rating[1];
    st->rating[2] = lvl.rating[2];

    // spawn planets
    for (uint32_t i = 0; i < lvl.planets_count; ++i) {
        LevelPlanet *p = &lvl.planets[i];
        float px = p->pos_x;
        float py = p->pos_y;
        uint8_t type = p->type;
        world_add_planet(w, px, py, p->size, type);
    }

    // place player
    if (lvl.player_pos_x != 0.0f || lvl.player_pos_y != 0.0f) {
        float ppx = lvl.player_pos_x;
        float ppy = lvl.player_pos_y;
        world_add_player(w, ppx, ppy);
    }
    else {
        world_place_player(w, 0.0f);
    }

    // create enemy spawn templates from lvl.enemies and register to world
    if (lvl.enemies_count) {
        st->spawns = calloc(lvl.enemies_count, sizeof(SpawnEntry));
        if (!st->spawns) {
            LOG_ERROR("scene_campaign", "Failed to allocate spawn entries");
            level_free(&lvl);
            return;
        }
        st->spawn_count = lvl.enemies_count;
        for (uint32_t i = 0; i < lvl.enemies_count; ++i) {
            LevelEnemy *le = &lvl.enemies[i];
            SpawnEntry *se = &st->spawns[i];
            se->template = *le;
            // convert normalized coords to pixels
            se->template.pos_x = le->pos_x;
            se->template.pos_y = le->pos_y;
            se->spawned = 0;
            se->runtime_index = -1;
            se->runtime_ptr = NULL;
            se->target_id = (le->spawn_kind == 1) ? le->spawn_arg : 0;
            se->target_runtime_index = -1;
            se->scheduled_time = -1.0f;
            // handle on_start spawns: if a delay is set, schedule it, otherwise spawn immediately
            if (le->spawn_kind == 0) {
                if (le->spawn_delay > 0) {
                    se->scheduled_time = w->time + (float)le->spawn_delay;
                }
                else {
                    if (world_spawn_enemy(w, (int)le->type, se->template.pos_x, se->template.pos_y, le->difficulty, le->health)) {
                        se->spawned = 1;
                    }
                    else {
                        // failed to spawn immediately; schedule retry next frame
                        se->scheduled_time = w->time;
                    }
                }
            }
        }
    }

    st->world->hud = hud_create(st->world->svc, st->world->player);

    // keep world reference in scene state; level data copied into spawn structures later
    level_free(&lvl);

    if (st->world) {
        app_push_overlay(SCENE_OVERLAY_START_GAME);
    }
}

void scene_campaign_leave(Scene *s) {
    SceneCampaignState *st = (SceneCampaignState *)s->state;
    if (!st)
        return;
    if (st->world) {
        st->world->on_time_over = NULL;
        st->world->on_time_over_user = NULL;
        world_destroy(st->world);
    }
    if (st->spawns)
        free(st->spawns);
    free(st);
}

static void scene_campaign_on_time_over(World *world, void *user) {
    (void)world;
    SceneCampaignState *st = (SceneCampaignState *)user;
    if (!st)
        return;
    st->time_limit_reached = 1;
}

static void scene_campaign_finish(SceneCampaignState *st) {
    if (!st || !st->world || st->level_end_handled)
        return;

    World *w = st->world;
    int goals_met = 1;
    if (st->goal_kills > 0 && w->kills < st->goal_kills)
        goals_met = 0;
    if (st->level_failed)
        goals_met = 0;
    if (st->time_limit_reached)
        goals_met = 0;

    int stars = 0;
    if (w->score >= (int)st->rating[2])
        stars = 3;
    else if (w->score >= (int)st->rating[1])
        stars = 2;
    else if (w->score >= (int)st->rating[0])
        stars = 1;

    OverlayEndgameFailReason fail_reason = OVERLAY_ENDGAME_FAIL_NONE;
    if (st->level_failed)
        fail_reason = OVERLAY_ENDGAME_FAIL_DEATH;
    else if (st->time_limit_reached)
        fail_reason = OVERLAY_ENDGAME_FAIL_TIME;
    else if (!goals_met || stars <= 0)
        fail_reason = OVERLAY_ENDGAME_FAIL_SCORE;

    overlay_endgame_set_failure_reason(fail_reason);
    overlay_endgame_set_stats(w->kills, w->score);
    overlay_endgame_set_campaign_result(goals_met, st->rating[0], st->rating[1], st->rating[2]);
    overlay_endgame_set_campaign_level(st->level_filename);
    st->level_end_handled = 1;
    app_push_overlay(SCENE_OVERLAY_ENDGAME_CAMPAIGN);
}

void scene_campaign_handle_input(Scene *s, const struct InputState *in) {
    if (in->pause)
        app_push_overlay(SCENE_OVERLAY_PAUSE);
    SceneCampaignState *st = (SceneCampaignState *)s->state;
    if (st)
        st->last_input = *in;
}

void scene_campaign_update(Scene *s, float dt) {
    SceneCampaignState *st = (SceneCampaignState *)s->state;
    World *w = st->world;
    if (!st || !st->world)
        return;
    if (st->world->player)
        player_set_input(st->world->player, &st->last_input);
    world_update(st->world, dt);

    if (!st->level_end_handled) {
        Player *player = st->world->player;
        if (player && !player->alive) {
            if (!st->player_death_handled) {
                st->player_death_handled = 1;
                st->level_failed = 1;
                st->level_end_delay = LEVEL_END_DELAY_SECONDS;
            }
            st->level_end_delay -= dt;
            if (st->level_end_delay > 0.0f)
                return;
            st->level_end_delay = 0.0f;
            scene_campaign_finish(st);
            return;
        }
    }

    if (st->time_limit_reached && !st->level_end_handled) {
        st->level_end_delay -= dt;
        if (st->level_end_delay > 0.0f)
            return;
        st->level_end_delay = 0.0f;
        scene_campaign_finish(st);
        return;
    }

    // process timer-based spawns
    for (uint32_t i = 0; i < st->spawn_count; ++i) {
        SpawnEntry *se = &st->spawns[i];
        if (se->spawned)
            continue;
        // timer-based spawns and scheduled on-death spawns share the same time test
    if (se->scheduled_time >= 0.0f && w->time >= se->scheduled_time) {
            LevelEnemy *le = &se->template;
            if (world_spawn_enemy(w, (int)le->type, le->pos_x, le->pos_y, le->difficulty, le->health)) {
                se->spawned = 1;
                se->scheduled_time = -1.0f;
                // try to immediately bind runtime pointer/index to reduce ambiguity when many spawns occur
                for (int ei = 0; ei < w->enemy_count; ++ei) {
                    Enemy *en = w->enemies[ei];
                    if (!en)
                        continue;
                    // skip if already assigned to another spawn
                    bool taken = false;
                    for (uint32_t j = 0; j < st->spawn_count; ++j) {
                        if (st->spawns[j].runtime_ptr == en) {
                            taken = true;
                            break;
                        }
                    }
                    if (taken) {
                        continue;
                    }
                    float dx = en->e.pos.x - se->template.pos_x;
                    float dy = en->e.pos.y - se->template.pos_y;
                    if (dx * dx + dy * dy < 64.0f) {
                        se->runtime_ptr = en;
                        se->runtime_index = ei;
                        break;
                    }
                }
            }
        }
    }

    // resolve runtime pointers for spawned entries by matching positions and unused enemies
    for (uint32_t i = 0; i < st->spawn_count; ++i) {
        SpawnEntry *se = &st->spawns[i];
        if (!se->spawned || se->runtime_ptr)
            continue;
        for (int ei = 0; ei < w->enemy_count; ++ei) {
            Enemy *en = w->enemies[ei];
            if (!en)
                continue;
            // skip if already assigned
            bool taken = false;
            for (uint32_t j = 0; j < st->spawn_count; ++j) {
                if (st->spawns[j].runtime_ptr == en) {
                    taken = true;
                    break;
                }
            }
            if (taken) {
                continue;
            }
            float dx = en->e.pos.x - se->template.pos_x;
            float dy = en->e.pos.y - se->template.pos_y;
            float d2 = dx * dx + dy * dy;
            if (d2 < 16.0f) {
                // within 4 pixels
                se->runtime_ptr = en;
                se->runtime_index = ei;
                break;
            }
        }
    }

    // handle on-death spawns: trigger when target spawn's enemy is gone or dead
    for (uint32_t i = 0; i < st->spawn_count; ++i) {
        SpawnEntry *se = &st->spawns[i];
        if (se->spawned)
            continue;
    if (se->template.spawn_kind != 1)
            continue;
        // find target spawn entry by id
        uint32_t target_id = se->target_id;
        if (target_id == 0)
            continue;
        SpawnEntry *target_spawn = NULL;
        for (uint32_t j = 0; j < st->spawn_count; ++j) {
            if (st->spawns[j].template.id == target_id) {
                target_spawn = &st->spawns[j];
                break;
            }
        }
        if (!target_spawn) {
            continue;
        }
        bool target_dead = false;
        if (target_spawn->runtime_ptr) {
            // check if pointer still present in world enemies
            Enemy *tent = (Enemy *)target_spawn->runtime_ptr;
            bool found = false;
            for (int ei = 0; ei < w->enemy_count; ++ei) {
                if (w->enemies[ei] == tent) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                target_dead = true;
            }
            else if (!tent->alive)
                target_dead = true;
        }
        else if (target_spawn->spawned) {
            // spawned but no runtime_ptr resolved; try to find any enemy at that template pos
            bool found_pos = false;
            for (int ei = 0; ei < w->enemy_count; ++ei) {
                Enemy *en = w->enemies[ei];
                if (!en)
                    continue;
                float dx = en->e.pos.x - target_spawn->template.pos_x;
                float dy = en->e.pos.y - target_spawn->template.pos_y;
                if (dx * dx + dy * dy < 16.0f) {
                    found_pos = true;
                    break;
                }
            }
            if (!found_pos)
                target_dead = true; // can't find target in world
        }
        if (target_dead) {
            // schedule spawn using the original spawn_delay (if provided)
            if (se->template.spawn_delay > 0) {
                if (se->scheduled_time < 0.0f) {
                    se->scheduled_time = w->time + (float)se->template.spawn_delay;
                }
            }
            else {
                LevelEnemy *le = &se->template;
                if (world_spawn_enemy(w, (int)le->type, le->pos_x, le->pos_y, le->difficulty, le->health)) {
                    se->spawned = 1;
                }
                else {
                    se->scheduled_time = w->time; // retry next frame
                    LOG_WARN("scene_campaign", "Immediate on-death spawn failed id=%u (scheduled retry at %.2f)", le->id, se->scheduled_time);
                }
            }
        }
    }

    /* Check level completion: if all spawn entries have been spawned and there are no
     * active enemies left in the world, treat level as finished and show endgame overlay. */
    if (!st->level_end_handled && st->spawn_count > 0) {
        bool all_spawned = true;
        for (uint32_t i = 0; i < st->spawn_count; ++i) {
            if (!st->spawns[i].spawned) {
                all_spawned = false;
                break;
            }
        }
        if (all_spawned && w->enemy_count == 0) {
            st->level_end_delay -= dt;
            if (st->level_end_delay > 0.0f) {
                return;
            }
            st->level_end_delay = 0.0f;
            scene_campaign_finish(st);
            return;
        }
    }
}
void scene_campaign_render(Scene *s, struct Renderer *r) {
    SceneCampaignState *st = (SceneCampaignState *)s->state;
    struct Services *svc = st->svc;
    SDL_Texture *bg = texman_get(svc->texman, TEX_BG_STARFIELD);
    renderer_draw_texture(r, bg, NULL, NULL, 0);
    if (st->world)
        world_render(st->world, r);
}
