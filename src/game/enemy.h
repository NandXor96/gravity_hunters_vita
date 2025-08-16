#pragma once
#include "entity.h"
#include "weapon.h"
#include "../core/math.h"

struct World; // forward
typedef enum EnemyType
{
    ENEMY_ASTRO_ANT = 0,
    ENEMY_FRIGATE,
    ENEMY_HOLO_SHARK,
    ENEMY_NOVA_NOMAD,
    ENEMY_PLASMA_PIRATE,
    ENEMY_SHOCK_BEE,
    ENEMY_SHREDDER_SWALLOW,
    ENEMY_SPARK_FALCON,
    ENEMY_WARP_WESP,
    ENEMY_TYPE_COUNT
} EnemyType;

typedef struct Enemy
{
    Entity e;
    EnemyType type;
    bool alive;
    bool can_fight;
    float move_speed;
    float shoot_speed;
    int id;
    int shooter_index;
    int health;
    struct Weapon *weapon;
    struct World *world;
    /* Energy/economy for AI shooting */
    int   energy_max;
    float energy; // current energy pool
    float energy_regen_rate; // per second
    float shoot_chance; // probability per decision [0..1]
    /* Cached shot info to skip full direct simulation when neither player nor enemy moved */
    Vec2  last_shot_player_pos;  // last player pos used for aiming
    Vec2  last_shot_enemy_pos;   // last enemy pos used for aiming
    float cached_shot_angle;     // best angle found last time
    float cached_shot_strength;  // best strength found last time
    float cached_shot_best_dist; // minimal distance to player for cached shot
    int   cached_shot_valid;     // boolean-like flag (0/1) if cache is valid
} Enemy;

// Unified create that sets base fields then dispatches to type-specific initializer
Enemy *enemy_create(struct World *world, EnemyType type, float x, float y, float angle, int id, int shooter_index);
// Type-specific internal initializers (return false on failure)
void enemy_destroy(Enemy *en);
void enemy_ai_update(Enemy *en, struct World *w, float dt);
bool enemy_try_shoot(Enemy *en, struct World *w);
OBB enemy_collider(const Enemy *en);
