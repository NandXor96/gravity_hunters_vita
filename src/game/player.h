#pragma once
#include "entity.h"
#include "../core/math.h"
#include "../services/input.h"

// Player entity dimensions
#define PLAYER_WIDTH  34.0f
#define PLAYER_HEIGHT 35.0f

struct World; // forward
typedef struct Player {
    Entity e;
    int id;
    bool alive;
    float respawn_timer;
    int shooter_index;
    struct World *world;  // back-reference
    InputState input;     // last input snapshot
    bool fire_was_down;   // previous frame fire state for edge-trigger
    float rotation_speed; // radians per second
    int health;
    int max_health; // for HUD normalization
    struct Weapon *weapon;
    float current_shot_speed; // runtime adjustable shot speed clamped to weapon min/max
    // Energy system (moved from Weapon)
    float energy;          // current energy (was current_shot_points)
    int   energy_max;      // max energy (was max_shot_points)
    float energy_regen_rate; // per second (was regen_rate)
    // Boost
    float boost_strength;  // additive velocity impulse (pixels/sec)
    int   boost_cost;      // energy per boost
    int   boost_damage_flag; // frame damage flag (reset each update)
    bool  death_explosion_triggered; // guard to avoid spawning multiple death effects
    // Boost state tracking (moved from static variables to fix reentrancy issue)
    int   prev_boost;      // previous frame boost input state for edge detection
    int   boosting_session; // 0 = no active boost hold, 1 = active since valid edge
} Player;
Player *player_create(SDL_Texture *tex);
void player_destroy(Player *p);
void player_fly(Player *p, const InputState *in, float dt);
bool player_shoot(Player *p, struct World *w, float strength);
void player_set_input(Player *p, const InputState *in);
void player_on_kill(Player *p);
OBB player_collider(const Player *p);
