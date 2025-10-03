#include "level_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(push,1)
typedef struct {
    char magic[4];
    uint16_t version;
    uint16_t reserved;
    uint32_t time_limit;
    uint16_t goal_kills;
    uint32_t rating[3];
    float player_pos_x;
    float player_pos_y;
    uint32_t player_health;
    uint32_t planets_count;
    uint32_t enemies_count;
} FileHeader;

typedef struct {
    uint8_t type;
    float size;
    float pos_x;
    float pos_y;
} PlanetEntry;

typedef struct {
    uint16_t id;
    uint8_t type;
    float pos_x;
    float pos_y;
    uint8_t difficulty;
    uint32_t health;
    uint8_t spawn_kind;
    uint32_t spawn_arg;
    uint32_t spawn_delay;
} EnemyEntry;
#pragma pack(pop)

static const char *spawn_kind_name(uint8_t k) {

    switch (k) {
        case 0: return "on_start";
        case 1: return "on_death";
    default: return "unknown";

}
}

int level_load(const char *filename, GameLevel *out, char *err, size_t errlen) {

    if (!filename || !out) {
        if (err && errlen) snprintf(err, errlen, "invalid args");
        return 1;

}

    // Build path under assets/levels if filename is not absolute
    char path[512];
    if (filename[0] == '/' || (filename[1] == ':' && filename[2] == '\\')) {
        strncpy(path, filename, sizeof(path)-1);
        path[sizeof(path)-1] = '\0';
    } else {
        snprintf(path, sizeof(path), "assets/levels/%s", filename);
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        if (err && errlen) snprintf(err, errlen, "failed to open '%s'", path);
        return 2;
    }

    FileHeader h;
    if (fread(&h, sizeof(h), 1, f) != 1) {
        if (err && errlen) snprintf(err, errlen, "failed to read header");
        fclose(f); return 3;
    }
    if (memcmp(h.magic, "GHLV", 4) != 0) {
        if (err && errlen) snprintf(err, errlen, "bad magic"); fclose(f); return 4;
    }

    // fill basic header fields
    memset(out, 0, sizeof(*out));
    out->version = h.version;
    out->time_limit = h.time_limit;
    out->goal_kills = h.goal_kills;
    out->rating[0] = h.rating[0]; out->rating[1] = h.rating[1]; out->rating[2] = h.rating[2];
    out->player_pos_x = h.player_pos_x; out->player_pos_y = h.player_pos_y; out->player_health = h.player_health;
    out->planets_count = h.planets_count; out->enemies_count = h.enemies_count;

    // read start_text
    size_t cap = 256;
    char *sbuf = (char*)malloc(cap);
    if (!sbuf) {
        if (err && errlen) snprintf(err, errlen, "alloc failed"); fclose(f); return 5;
    }
    size_t si = 0; int c;
    while ((c = fgetc(f)) != EOF && c != '\0') {
        if (si + 1 >= cap) {
            cap *= 2; char *n = (char*)realloc(sbuf, cap);
            if (!n) { free(sbuf); if (err && errlen) snprintf(err, errlen, "alloc failed"); fclose(f); return 6;
        }
            sbuf = n;
        }
        sbuf[si++] = (char)c;
    }
    sbuf[si] = '\0';
    out->start_text = sbuf;

    // read planets
    out->planets = NULL; out->enemies = NULL;
    if (out->planets_count) {
        out->planets = (LevelPlanet*)malloc(sizeof(LevelPlanet) * out->planets_count);
        if (!out->planets) { if (err && errlen) snprintf(err, errlen, "alloc failed"); fclose(f); return 7;
    }
        for (uint32_t i = 0; i < out->planets_count; ++i) {
            PlanetEntry pe;
            if (fread(&pe, sizeof(pe), 1, f) != 1) { if (err && errlen) snprintf(err, errlen, "failed to read planet"); fclose(f); return 8;
        }
            out->planets[i].type = pe.type; out->planets[i].size = pe.size; out->planets[i].pos_x = pe.pos_x; out->planets[i].pos_y = pe.pos_y;
        }
    }

    // read enemies
    if (out->enemies_count) {
        out->enemies = (LevelEnemy*)malloc(sizeof(LevelEnemy) * out->enemies_count);
        if (!out->enemies) { if (err && errlen) snprintf(err, errlen, "alloc failed"); fclose(f); return 9;
    }
        for (uint32_t i = 0; i < out->enemies_count; ++i) {
            EnemyEntry ee;
            if (fread(&ee, sizeof(ee), 1, f) != 1) { if (err && errlen) snprintf(err, errlen, "failed to read enemy"); fclose(f); return 10;
        }
            out->enemies[i].id = ee.id; out->enemies[i].type = ee.type;
            out->enemies[i].pos_x = ee.pos_x; out->enemies[i].pos_y = ee.pos_y;
            out->enemies[i].difficulty = ee.difficulty; out->enemies[i].health = ee.health;
            out->enemies[i].spawn_kind = ee.spawn_kind; out->enemies[i].spawn_arg = ee.spawn_arg; out->enemies[i].spawn_delay = ee.spawn_delay;
        }
    }

    fclose(f);
    return 0;
}

void level_free(GameLevel *lvl) {

    if (!lvl) return;
    if (lvl->start_text) free(lvl->start_text);
    if (lvl->planets) free(lvl->planets);
    if (lvl->enemies) free(lvl->enemies);
    lvl->start_text = NULL; lvl->planets = NULL; lvl->enemies = NULL;

}

