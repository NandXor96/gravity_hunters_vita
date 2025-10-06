#pragma once
#include <stdint.h>
#include "entity.h"
#include "weapon.h"
#include "../core/math.h"

struct World; // forward
typedef enum EnemyType {
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

/**
 * @brief ShotCache stores incremental state for the enemy's trajectory search.
 *
 * The AI progressively searches candidate projectile trajectories over several
 * frames to find a firing angle/strength that comes close to the player. This
 * struct holds the cached best candidate and the progressive search state so
 * work can be spread across multiple updates.
 */
typedef struct ShotCache {
    Vec2  last_player_pos;  // last player pos used for aiming
    Vec2  last_enemy_pos;   // last enemy pos used for aiming
    float angle;            // best angle found last time
    float strength;         // best strength found last time
    float best_dist;        // minimal distance to player for cached shot
    bool  valid;            // cache valid
    /* incremental shot-search state (spread work over update cycles) */
    uint16_t   search_total;     // total candidate samples to evaluate
    uint16_t   search_done;      // how many samples evaluated so far
    uint8_t    search_per_frame; // samples to evaluate per update (small)
    uint16_t   improvements;     // number of improvements found
    bool  ready;            // found a trajectory with min_dist <= threshold
    /* deterministic progressive grid-search state (no RNG) */
    float search_radius_ang;   // current angular search radius (radians)
    float search_radius_str;   // current strength search radius (fraction of strength)
    uint8_t   search_grid_n;       // grid divisions per axis (n x n) (small)
    uint16_t  search_idx;          // linear index into current grid
    float search_base_angle;   // center angle for current refinement
    float search_base_strength;// center strength for current refinement
} ShotCache;

/**
 * @brief Per-type AI tuning values.
 *
 * These are lightweight parameters that influence the enemy's aiming jitter
 * and overall difficulty scaling.
 */
typedef struct EnemyAIConfig {
    unsigned char difficulty;         // 0..255 general difficulty
    float base_jitter_angle;          // per-type base angle jitter (radians)
    float base_jitter_strength;       // per-type base strength jitter (fraction)
} EnemyAIConfig;

/**
 * @brief Compact aim state used by enemies.
 */
typedef struct AimState {
    float target_angle;     /* desired facing angle (radians) */
    float rotate_speed;     /* max rotation speed (radians/sec) while aiming */
    float queued_strength;  /* stored strength when an aimed shot is queued */
    uint8_t queued;         /* boolean-like flag: shot queued and awaiting alignment */
} AimState;

/**
 * @brief Enemy instance.
 *
 * This contains runtime state for a single enemy entity. Most gameplay and AI
 * values live here. Fields use compact integer types where practical to save
 * memory on constrained platforms.
 */
typedef struct Enemy {
    Entity e;
    uint8_t type; /* stores EnemyType values (0..ENEMY_TYPE_COUNT-1) in a compact form */
    bool alive;
    float shoot_speed;
    int16_t shooter_index;
    int16_t health;
    struct Weapon *weapon;
    struct World *world;
    /* Energy/economy for AI shooting */
    uint16_t   energy_max;
    float      energy; // current energy pool
    float      energy_regen_rate; // per second
    float      shoot_chance; // probability per decision [0..1]

    ShotCache shot;
    EnemyAIConfig ai;
    int8_t   explosion_type;             // preferred explosion type index
    AimState aim;
    // Configurable thresholds for shot search (default values)
    /* deterministic progressive grid-search state (moved into ShotCache) */
} Enemy;

/**
 * @brief Create a new Enemy instance and initialize it from the type table.
 *
 * @param world  World pointer the enemy will belong to.
 * @param type   EnemyType index selecting per-type parameters.
 * @param x      Spawn X coordinate.
 * @param y      Spawn Y coordinate.
 * @param angle  Initial orientation (radians).
 * @param id     Local id for the enemy (application assigned).
 * @param shooter_index Index into the world's shooter registry used for firing.
 * @return Pointer to a newly allocated Enemy or NULL on failure.
 */
Enemy *enemy_create(struct World *world, EnemyType type, float x, float y, int shooter_index, char difficulty);

/**
 * @brief Destroy an Enemy instance and free its resources.
 *
 * @param en Enemy pointer to destroy (nullable).
 */
void enemy_destroy(Enemy *en);

/**
 * @brief Update per-frame AI for an enemy (decision making, firing, etc.).
 *
 * @param en Enemy to update.
 * @param w  World context (used for targets, rng, time, etc.).
 * @param dt Delta time in seconds.
 */
void enemy_ai_update(Enemy *en, struct World *w, float dt);

/**
 * @brief Attempt to fire the enemy's weapon if conditions allow.
 *
 * Returns true when a projectile was fired and any resource costs applied.
 *
 * @param en Enemy attempting to shoot.
 * @param w  World context.
 * @return true if a shot was fired.
 */
bool enemy_try_shoot(Enemy *en, struct World *w);

/**
 * @brief Build an OBB (or equivalent) for collision queries from an enemy.
 *
 * @param en Enemy to query.
 * @return OBB representing the enemy's collision footprint.
 */
OBB enemy_collider(const Enemy *en);
