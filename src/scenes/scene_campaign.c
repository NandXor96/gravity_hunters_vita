#include "scene_campaign.h"
#include <stdlib.h>
#include <stdio.h>
#include "../services/services.h"
#include "../services/renderer.h"
#include "../app/app.h"
#include "../game/world.h"
#include "../game/level_loader.h"
#include "../game/enemy.h"
#include "../game/player.h"
#include "overlay_endgame.h"

void scene_campaign_enter(Scene *s)
{
    SceneCampaignState *st = calloc(1, sizeof(SceneCampaignState));
    s->state = st;
    st->svc = app_services();
    st->world = NULL;
    st->spawns = NULL;
    st->spawn_count = 0;

    // For now load a fixed example level from assets/levels
    GameLevel lvl;
    char err[256] = {0};
    if (level_load("example_level.lvl", &lvl, err, sizeof(err)) != 0)
    {
        return;
    }

    // debug print
    // level_print(&lvl);

    // create world and populate planets/player based on level
    World *w = world_create(st->svc, 0);
    st->world = w;
    if (lvl.time_mode == 1)
        world_set_time_limit(w, (float)lvl.time_limit);

    /* copy goals/ratings into scene state for end-of-level evaluation */
    st->goal_kills = lvl.goal_kills;
    st->goal_deaths = lvl.goal_deaths;
    st->goal_time = lvl.goal_time;
    st->rating[0] = lvl.rating[0];
    st->rating[1] = lvl.rating[1];
    st->rating[2] = lvl.rating[2];

    // spawn planets
    for (uint32_t i = 0; i < lvl.planets_count; ++i)
    {
        LevelPlanet *p = &lvl.planets[i];
        float px = p->pos_x;
        float py = p->pos_y;
        uint8_t type = p->type;
        world_add_planet(w, px, py, p->size, type);
    }

    // place player
    if (lvl.player_pos_x != 0.0f || lvl.player_pos_y != 0.0f)
    {
        float ppx = lvl.player_pos_x;
        float ppy = lvl.player_pos_y;
        world_add_player(w, ppx, ppy);
    }
    else
    {
        world_place_player(w, 0.0f);
    }

    // create enemy spawn templates from lvl.enemies and register to world
    if (lvl.enemies_count)
    {
        st->spawns = calloc(lvl.enemies_count, sizeof(SpawnEntry));
        st->spawn_count = lvl.enemies_count;
        for (uint32_t i = 0; i < lvl.enemies_count; ++i)
        {
            LevelEnemy *le = &lvl.enemies[i];
            SpawnEntry *se = &st->spawns[i];
            se->template = *le;
            // convert normalized coords to pixels
            se->template.pos_x = le->pos_x;
            se->template.pos_y = le->pos_y;
            se->spawned = 0;
            se->runtime_index = -1;
            se->runtime_ptr = NULL;
            se->target_id = le->spawn_kind == 2 ? le->spawn_arg : 0;
            se->target_runtime_index = -1;
            se->trigger_time = (le->spawn_kind == 1) ? ((float)le->spawn_arg + (float)le->spawn_delay) : 0.0f;
            se->scheduled_time = -1.0f;
            // handle on_start spawns: if a delay is set, schedule it, otherwise spawn immediately
            if (le->spawn_kind == 0)
            {
                if (le->spawn_delay > 0)
                {
                    se->scheduled_time = w->time + (float)le->spawn_delay;
                }
                else
                {
                    if (world_spawn_enemy(w, (int)le->type, se->template.pos_x, se->template.pos_y, le->difficulty))
                    {
                        se->spawned = 1;
                    }
                    else
                    {
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
}

void scene_campaign_leave(Scene *s)
{
    SceneCampaignState *st = (SceneCampaignState *)s->state;
    if (!st)
        return;
    if (st->world)
        world_destroy(st->world);
    if (st->spawns)
        free(st->spawns);
    free(st);
}

void scene_campaign_handle_input(Scene *s, const struct InputState *in)
{
    if (in->pause)
        app_push_overlay(SCENE_OVERLAY_PAUSE);
    SceneCampaignState *st = (SceneCampaignState *)s->state;
    if (st)
        st->last_input = *in;
}

void scene_campaign_update(Scene *s, float dt)
{
    SceneCampaignState *st = (SceneCampaignState *)s->state;
    World *w = st->world;
    if (!st || !st->world)
        return;
    if (st->world->player)
        player_set_input(st->world->player, &st->last_input);
    world_update(st->world, dt);

    // process timer-based spawns
    for (uint32_t i = 0; i < st->spawn_count; ++i)
    {
        SpawnEntry *se = &st->spawns[i];
        if (se->spawned)
            continue;
        // timer-based spawns and scheduled on-death spawns share the same time test
        if ((se->template.spawn_kind == 1 && w->time >= se->trigger_time) || (se->scheduled_time >= 0.0f && w->time >= se->scheduled_time))
        {
            LevelEnemy *le = &se->template;
            if (world_spawn_enemy(w, (int)le->type, le->pos_x, le->pos_y, le->difficulty))
            {
                se->spawned = 1;
                se->scheduled_time = -1.0f;
                // try to immediately bind runtime pointer/index to reduce ambiguity when many spawns occur
                for (int ei = 0; ei < w->enemy_count; ++ei)
                {
                    Enemy *en = w->enemies[ei];
                    if (!en)
                        continue;
                    // skip if already assigned to another spawn
                    bool taken = false;
                    for (uint32_t j = 0; j < st->spawn_count; ++j)
                        if (st->spawns[j].runtime_ptr == en)
                        {
                            taken = true;
                            break;
                        }
                    if (taken)
                        continue;
                    float dx = en->e.pos.x - se->template.pos_x;
                    float dy = en->e.pos.y - se->template.pos_y;
                    if (dx * dx + dy * dy < 64.0f) // within 8 pixels
                    {
                        se->runtime_ptr = en;
                        se->runtime_index = ei;
                        break;
                    }
                }
            }
        }
    }

    // resolve runtime pointers for spawned entries by matching positions and unused enemies
    for (uint32_t i = 0; i < st->spawn_count; ++i)
    {
        SpawnEntry *se = &st->spawns[i];
        if (!se->spawned || se->runtime_ptr)
            continue;
        for (int ei = 0; ei < w->enemy_count; ++ei)
        {
            Enemy *en = w->enemies[ei];
            if (!en)
                continue;
            // skip if already assigned
            bool taken = false;
            for (uint32_t j = 0; j < st->spawn_count; ++j)
                if (st->spawns[j].runtime_ptr == en)
                {
                    taken = true;
                    break;
                }
            if (taken)
                continue;
            float dx = en->e.pos.x - se->template.pos_x;
            float dy = en->e.pos.y - se->template.pos_y;
            float d2 = dx * dx + dy * dy;
            if (d2 < 16.0f)
            { // within 4 pixels
                se->runtime_ptr = en;
                se->runtime_index = ei;
                break;
            }
        }
    }

    // handle on-death spawns: trigger when target spawn's enemy is gone or dead
    for (uint32_t i = 0; i < st->spawn_count; ++i)
    {
        SpawnEntry *se = &st->spawns[i];
        if (se->spawned)
            continue;
        if (se->template.spawn_kind != 2)
            continue;
        // find target spawn entry by id
        uint32_t target_id = se->target_id;
        if (target_id == 0)
            continue;
        SpawnEntry *target_spawn = NULL;
        for (uint32_t j = 0; j < st->spawn_count; ++j)
            if (st->spawns[j].template.id == target_id)
            {
                target_spawn = &st->spawns[j];
                break;
            }
        if (!target_spawn)
            continue;
        bool target_dead = false;
        if (target_spawn->runtime_ptr)
        {
            // check if pointer still present in world enemies
            Enemy *tent = (Enemy *)target_spawn->runtime_ptr;
            bool found = false;
            for (int ei = 0; ei < w->enemy_count; ++ei)
                if (w->enemies[ei] == tent)
                {
                    found = true;
                    break;
                }
            if (!found)
                target_dead = true; // removed => dead
            else if (!tent->alive)
                target_dead = true;
        }
        else if (target_spawn->spawned)
        {
            // spawned but no runtime_ptr resolved; try to find any enemy at that template pos
            bool found_pos = false;
            for (int ei = 0; ei < w->enemy_count; ++ei)
            {
                Enemy *en = w->enemies[ei];
                if (!en)
                    continue;
                float dx = en->e.pos.x - target_spawn->template.pos_x;
                float dy = en->e.pos.y - target_spawn->template.pos_y;
                if (dx * dx + dy * dy < 16.0f)
                {
                    found_pos = true;
                    break;
                }
            }
            if (!found_pos)
                target_dead = true; // can't find target in world
        }
        if (target_dead)
        {
            // schedule spawn using the original spawn_delay (if provided)
            if (se->template.spawn_delay > 0)
            {
                if (se->scheduled_time < 0.0f)
                {
                    se->scheduled_time = w->time + (float)se->template.spawn_delay;
                }
            }
            else
            {
                LevelEnemy *le = &se->template;
                if (world_spawn_enemy(w, (int)le->type, le->pos_x, le->pos_y, le->difficulty))
                {
                    se->spawned = 1;
                }
                else
                {
                    se->scheduled_time = w->time; // retry next frame
                    fprintf(stderr, "[scene_campaign] immediate on-death spawn failed id=%u (scheduled retry at %.2f)\n", le->id, se->scheduled_time);
                }
            }
        }
    }

    /* Check level completion: if all spawn entries have been spawned and there are no
     * active enemies left in the world, treat level as finished and show endgame overlay. */
    if (!st->level_end_handled && st->spawn_count > 0)
    {
        bool all_spawned = true;
        for (uint32_t i = 0; i < st->spawn_count; ++i)
        {
            if (!st->spawns[i].spawned)
            {
                all_spawned = false;
                break;
            }
        }
        if (all_spawned && w->enemy_count == 0)
        {
            st->level_end_handled = 1;
            /* evaluate whether goals are met */
            int goals_met = 1;
            if (st->goal_kills > 0 && w->kills < st->goal_kills)
                goals_met = 0;
            if (st->goal_deaths > 0 && w->deaths > st->goal_deaths)
                goals_met = 0;
            if (st->goal_time > 0 && st->world && st->world->time_limit >= 0.f)
            {
                /* if a level time limit existed, check whether elapsed time <= goal_time */
                if (st->world->time > (float)st->goal_time)
                    goals_met = 0;
            }
            /* provide stats to overlay and campaign-specific result */
            overlay_endgame_set_stats(w->kills, w->deaths, w->score);
            overlay_endgame_set_campaign_result(goals_met, st->rating[0], st->rating[1], st->rating[2]);
            app_push_overlay(SCENE_OVERLAY_ENDGAME_CAMPAIGN);
        }
    }
}
void scene_campaign_render(Scene *s, struct Renderer *r)
{
    SceneCampaignState *st = (SceneCampaignState *)s->state;
    struct Services *svc = st->svc;
    SDL_Texture *bg = texman_get(svc->texman, TEX_BG_STARFIELD);
    renderer_draw_texture(r, bg, NULL, NULL, 0);
    if (st->world)
        world_render(st->world, r);
}
