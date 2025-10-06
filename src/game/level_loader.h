#ifndef GH_LEVEL_LOADER_H
#define GH_LEVEL_LOADER_H

#include <stdint.h>
#include <stddef.h>

// Simple runtime representation of a level loaded from binary .lvl files.
// The loader reads files produced by the tools/level_compiler.py format.

typedef struct {
    uint8_t type;
    float size;
    float pos_x;
    float pos_y;
} LevelPlanet;

typedef struct {
    uint16_t id;        // as stored in file (JSON id)
    uint8_t type;
    float pos_x;
    float pos_y;
    uint8_t difficulty;
    uint32_t health;
    uint8_t spawn_kind; // 0=on_start,1=on_death
    uint32_t spawn_arg; // stores ID for on_death
    uint32_t spawn_delay;
} LevelEnemy;

typedef struct {
    // header fields
    uint16_t version;
    uint32_t time_limit;
    uint16_t goal_kills;
    uint32_t rating[3];
    float player_pos_x;
    float player_pos_y;
    uint32_t player_health;

    // counts
    uint32_t planets_count;
    uint32_t enemies_count;

    // allocated arrays / strings (must be freed with level_free)
    char *start_text;
    LevelPlanet *planets;
    LevelEnemy *enemies;
} GameLevel;

// Loads a level from the assets/levels folder (relative name like "1.lvl" or a path).
// On success returns 0 and fills 'out'. On error returns non-zero and places a message into err (if provided).
int level_load(const char *filename, GameLevel *out, char *err, size_t errlen);

// Frees memory previously allocated in GameLevel by level_load.
void level_free(GameLevel *lvl);

#endif // GH_LEVEL_LOADER_H
