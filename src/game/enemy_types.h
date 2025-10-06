#pragma once
#include "enemy.h"

static const Vec2 POLY_ASTRO_ANT[] = {{0, 7}, {0, 11}, {5, 15}, {3, 17}, {8, 26}, {10, 22}, {14, 27}, {19, 26}, {21, 22}, {23, 26}, {28, 18}, {26, 15}, {31, 11}, {31, 7}, {20, 10}, {23, 5}, {25, 6}, {25, 3}, {22, 3}, {19, 7}, {12, 7}, {9, 3}, {6, 3}, {6, 6}, {10, 7}, {10, 11}};
static const Vec2 POLY_FRIGATE[] = {{0, 13}, {0, 24}, {9, 19}, {15, 28}, {21, 19}, {31, 24}, {30, 11}, {28, 9}, {22, 10}, {17, 3}, {14, 3}, {9, 10}, {3, 9}};
static const Vec2 POLY_HOLO_SHARK[] = {{15, 1}, {9, 11}, {0, 20}, {0, 23}, {8, 20}, {10, 22}, {8, 29}, {12, 27}, {15, 30}, {18, 27}, {23, 29}, {21, 22}, {23, 20}, {31, 23}, {31, 20}};
static const Vec2 POLY_NOVA_NOMAD[] = {{8, 1}, {1, 9}, {0, 16}, {2, 22}, {0, 24}, {0, 30}, {2, 30}, {5, 25}, {12, 29}, {19, 29}, {26, 25}, {28, 29}, {31, 30}, {31, 24}, {29, 22}, {31, 13}, {29, 7}, {20, 0}};
static const Vec2 POLY_PLASMA_PIRATE[] = {{2, 13}, {0, 23}, {2, 22}, {5, 27}, {8, 26}, {9, 18}, {13, 26}, {18, 26}, {22, 18}, {23, 26}, {26, 27}, {29, 22}, {31, 23}, {29, 13}, {23, 10}, {17, 3}, {13, 3}, {8, 10}};
static const Vec2 POLY_SHOCK_BEE[] = {{9, 1}, {12, 4}, {10, 6}, {11, 11}, {1, 9}, {0, 13}, {3, 16}, {3, 19}, {9, 17}, {10, 24}, {16, 29}, {21, 24}, {22, 17}, {28, 19}, {31, 10}, {27, 9}, {21, 12}, {21, 6}, {19, 4}, {22, 1}, {18, 4}, {13, 4}};
static const Vec2 POLY_SHREDDER_SWALLOW[] = {{15, 2}, {9, 12}, {7, 9}, {3, 13}, {0, 21}, {3, 18}, {4, 21}, {12, 17}, {14, 19}, {9, 27}, {13, 24}, {16, 28}, {18, 24}, {22, 27}, {17, 19}, {19, 17}, {27, 21}, {28, 18}, {31, 21}, {31, 18}, {24, 9}, {22, 12}};
static const Vec2 POLY_SPARK_FALCON[] = {{15, 1}, {11, 10}, {7, 9}, {0, 14}, {0, 19}, {3, 18}, {3, 22}, {13, 16}, {10, 27}, {13, 24}, {16, 29}, {18, 24}, {21, 27}, {17, 18}, {19, 16}, {21, 19}, {29, 22}, {28, 19}, {31, 19}, {31, 14}, {24, 9}, {20, 10}};
static const Vec2 POLY_WARP_WESP[] = {{13, 2}, {11, 7}, {3, 10}, {0, 14}, {5, 13}, {6, 16}, {11, 13}, {11, 20}, {16, 28}, {20, 20}, {19, 14}, {20, 13}, {25, 16}, {26, 13}, {31, 13}, {21, 7}, {18, 2}};

typedef struct EnemyDef {
    EnemyType type;
    int hp;
    int damage;
    float shoot_speed;
    int energy_max;
    float energy_regen_rate;
    float shoot_chance;
    float base_jitter_angle;
    float base_jitter_strength;
    float aim_rotate_speed; /* max rotation speed (radians/sec) while aiming */
    const Vec2 *poly;
    int poly_count;
    int explosion_type;
    float size_x;
    float size_y;
} EnemyDef;

static const EnemyDef ENEMY_DEFS[ENEMY_TYPE_COUNT] = {
    { ENEMY_ASTRO_ANT, 11, 5, 0.8f, 300, 120.f, 0.005f, 0.25f, 0.09f, 1.75f, POLY_ASTRO_ANT, (int)(sizeof(POLY_ASTRO_ANT)/sizeof(POLY_ASTRO_ANT[0])), 0, 32.f, 32.f },
    { ENEMY_FRIGATE,   20, 8, 1.2f, 800, 40.f, 0.002f, 0.04f, 0.02f, 0.6f, POLY_FRIGATE, (int)(sizeof(POLY_FRIGATE)/sizeof(POLY_FRIGATE[0])), 3, 32.f, 32.f },
    { ENEMY_HOLO_SHARK,15, 10, 0.6f, 250, 160.f, 0.005f, 0.09f, 0.04f, 2.0f, POLY_HOLO_SHARK, (int)(sizeof(POLY_HOLO_SHARK)/sizeof(POLY_HOLO_SHARK[0])), 1, 32.f, 32.f },
    { ENEMY_NOVA_NOMAD,22, 10, 0.7f, 500, 80.f, 0.0026f, 0.04f, 0.02f, 1.5f, POLY_NOVA_NOMAD, (int)(sizeof(POLY_NOVA_NOMAD)/sizeof(POLY_NOVA_NOMAD[0])), 0, 32.f, 32.f },
    { ENEMY_PLASMA_PIRATE,20,12,0.5f, 600, 60.f, 0.004f, 0.09f, 0.04f, 0.75f, POLY_PLASMA_PIRATE, (int)(sizeof(POLY_PLASMA_PIRATE)/sizeof(POLY_PLASMA_PIRATE[0])), 3, 32.f, 32.f },
    { ENEMY_SHOCK_BEE, 11,9,0.4f, 200,220.f, 0.007f, 0.25f, 0.09f, 2.0f, POLY_SHOCK_BEE, (int)(sizeof(POLY_SHOCK_BEE)/sizeof(POLY_SHOCK_BEE[0])), 0, 32.f, 32.f },
    { ENEMY_SHREDDER_SWALLOW,15,8,0.9f, 450,70.f,0.0028f,0.05f,0.025f,1.25f, POLY_SHREDDER_SWALLOW, (int)(sizeof(POLY_SHREDDER_SWALLOW)/sizeof(POLY_SHREDDER_SWALLOW[0])), 2, 32.f, 32.f },
    { ENEMY_SPARK_FALCON,15,10,0.5f, 320,140.f,0.0046f,0.045f,0.025f,1.9f, POLY_SPARK_FALCON, (int)(sizeof(POLY_SPARK_FALCON)/sizeof(POLY_SPARK_FALCON[0])), 3, 32.f, 32.f },
    { ENEMY_WARP_WESP, 13,8,0.6f,380,100.f,0.0080f,0.3f,0.11f,2.1f, POLY_WARP_WESP, (int)(sizeof(POLY_WARP_WESP)/sizeof(POLY_WARP_WESP[0])), 0, 32.f, 32.f }
};
