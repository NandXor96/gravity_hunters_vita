#include "campaign_progress.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>

#ifdef PLATFORM_VITA
#include <psp2/io/stat.h>
#endif

#define PROGRESS_MAGIC "GHSV"
#define PROGRESS_VERSION 1
#define PROGRESS_MAX_ENTRIES 2048

static CampaignProgress g_progress = {0};
static bool g_progress_loaded = false;

static int ensure_entry_capacity(CampaignProgress *progress, int min_capacity) {

    if (!progress)
        return -1;
    if (progress->capacity >= min_capacity)
        return 0;

    int new_capacity = (progress->capacity > 0) ? (progress->capacity * 2) : 8;
    if (new_capacity < min_capacity)
        new_capacity = min_capacity;

    CampaignProgressEntry *items = (CampaignProgressEntry *)realloc(progress->entries, (size_t)new_capacity * sizeof(*items));
    if (!items)
        return -1;

    progress->entries = items;
    progress->capacity = new_capacity;
    return 0;

}

static int ensure_parent_directories(const char *path) {

    if (!path || !*path)
        return -1;

    size_t len = strlen(path);
    if (len == 0)
        return -1;

    char buffer[512];
    if (len >= sizeof(buffer))
        return -1;
    memcpy(buffer, path, len + 1);

    for (size_t i = 1; i < len; ++i) {
        if (buffer[i] == '/') {
            char saved = buffer[i];
            buffer[i] = '\0';
            if (buffer[i - 1] != ':') {
#ifdef PLATFORM_VITA
                int res = sceIoMkdir(buffer, 0777);
                if (res < 0) {
                    SceIoStat stat_info;
                    memset(&stat_info, 0, sizeof(stat_info));
                    if (sceIoGetstat(buffer, &stat_info) < 0) {
                        buffer[i] = saved;
                        return -1;
                    }
                }
#else
                if (mkdir(buffer, 0777) != 0 && errno != EEXIST) {
                    buffer[i] = saved;
                    return -1;
                }
#endif
            }
            buffer[i] = saved;
        }
    }
    return 0;
}

const char *campaign_progress_default_path(void) {

    #ifdef PLATFORM_VITA
    return "ux0:/data/GravityHunters/save.bin";
#else
    return "./assets/levels/savegame.bin";
#endif

}

void campaign_progress_free(CampaignProgress *progress) {

    if (!progress)
        return;
    free(progress->entries);
    progress->entries = NULL;
    progress->count = 0;
    progress->capacity = 0;
    progress->dirty = false;

}

static int write_empty_save(const char *path) {

    if (!path)
        return -1;
    if (ensure_parent_directories(path) != 0)
        return -1;
    FILE *f = fopen(path, "wb");
    if (!f)
        return -1;
    const char magic[4] = PROGRESS_MAGIC;
    uint16_t version = PROGRESS_VERSION;
    uint16_t reserved = 0;
    uint32_t count = 0;
    fwrite(magic, sizeof(char), 4, f);
    fwrite(&version, sizeof(version), 1, f);
    fwrite(&reserved, sizeof(reserved), 1, f);
    fwrite(&count, sizeof(count), 1, f);
    fclose(f);
    return 0;

}

int campaign_progress_load(CampaignProgress *progress, const char *path) {

    if (!progress)
        return -1;

    campaign_progress_free(progress);

    const char *resolved = path ? path : campaign_progress_default_path();
    FILE *f = fopen(resolved, "rb");
    if (!f) {
        if (errno == ENOENT) {
            write_empty_save(resolved);
            return 0;

}
        return -1;
    }

    char magic[4];
    if (fread(magic, sizeof(char), 4, f) != 4 || memcmp(magic, PROGRESS_MAGIC, 4) != 0) {
        fclose(f);
        return -1;
    }

    uint16_t version = 0;
    uint16_t reserved = 0;
    uint32_t count = 0;
    if (fread(&version, sizeof(version), 1, f) != 1) {
        fclose(f);
        return -1;
    }
    if (fread(&reserved, sizeof(reserved), 1, f) != 1) {
        fclose(f);
        return -1;
    }
    if (fread(&count, sizeof(count), 1, f) != 1) {
        fclose(f);
        return -1;
    }

    if (version != PROGRESS_VERSION || count > PROGRESS_MAX_ENTRIES) {

        fclose(f);
        return -1;

    }

    for (uint32_t i = 0; i < count; ++i) {

        char name[CAMPAIGN_LEVEL_MAX_FILENAME] = {0

    };
        uint8_t stars = 0;
        int32_t score = 0;
        if (fread(name, sizeof(char), CAMPAIGN_LEVEL_MAX_FILENAME, f) != CAMPAIGN_LEVEL_MAX_FILENAME) {
            campaign_progress_free(progress);
            fclose(f);
            return -1;
        }
        if (fread(&stars, sizeof(uint8_t), 1, f) != 1) {
            campaign_progress_free(progress);
            fclose(f);
            return -1;
        }
        if (fread(&score, sizeof(int32_t), 1, f) != 1) {
            campaign_progress_free(progress);
            fclose(f);
            return -1;
        }
        if (stars > CAMPAIGN_PROGRESS_MAX_STARS)
            stars = CAMPAIGN_PROGRESS_MAX_STARS;
        if (ensure_entry_capacity(progress, progress->count + 1) != 0) {
            campaign_progress_free(progress);
            fclose(f);
            return -1;
        }
        CampaignProgressEntry *entry = &progress->entries[progress->count++];
        memcpy(entry->filename, name, CAMPAIGN_LEVEL_MAX_FILENAME);
        entry->filename[CAMPAIGN_LEVEL_MAX_FILENAME - 1] = '\0';
        entry->stars = stars;
        entry->score = score;
    }
    fclose(f);
    progress->dirty = false;
    return 0;
}

int campaign_progress_save(CampaignProgress *progress, const char *path) {

    if (!progress)
        return -1;

    const char *resolved = path ? path : campaign_progress_default_path();

    FILE *f = fopen(resolved, "wb");
    if (!f)
        return -1;

    const char magic[4] = PROGRESS_MAGIC;
    uint16_t version = PROGRESS_VERSION;
    uint16_t reserved = 0;
    uint32_t count = (uint32_t)progress->count;

    fwrite(magic, sizeof(char), 4, f);
    fwrite(&version, sizeof(version), 1, f);
    fwrite(&reserved, sizeof(reserved), 1, f);
    fwrite(&count, sizeof(count), 1, f);

    for (int i = 0; i < progress->count; ++i) {
        const CampaignProgressEntry *entry = &progress->entries[i];
        char namebuf[CAMPAIGN_LEVEL_MAX_FILENAME] = {0

};
        strncpy(namebuf, entry->filename, CAMPAIGN_LEVEL_MAX_FILENAME - 1);
        uint8_t stars = entry->stars;
        int32_t score = entry->score;
        fwrite(namebuf, sizeof(char), CAMPAIGN_LEVEL_MAX_FILENAME, f);
        fwrite(&stars, sizeof(uint8_t), 1, f);
        fwrite(&score, sizeof(int32_t), 1, f);
    }

    fclose(f);
    progress->dirty = false;
    return 0;
}

static CampaignProgressEntry *campaign_progress_find_mut(CampaignProgress *progress, const char *filename) {

    if (!progress || !filename)
        return NULL;
    for (int i = 0; i < progress->count; ++i) {
        if (strncmp(progress->entries[i].filename, filename, CAMPAIGN_LEVEL_MAX_FILENAME) == 0)
            return &progress->entries[i];

}
    return NULL;
}

const CampaignProgressEntry *campaign_progress_find(const CampaignProgress *progress, const char *filename) {

    if (!progress || !filename)
        return NULL;
    for (int i = 0; i < progress->count; ++i) {
        if (strncmp(progress->entries[i].filename, filename, CAMPAIGN_LEVEL_MAX_FILENAME) == 0)
            return &progress->entries[i];

}
    return NULL;
}

int campaign_progress_update(CampaignProgress *progress, const char *filename, uint8_t stars, int32_t score) {

    if (!progress || !filename || !filename[0])
        return 0;
    if (stars > CAMPAIGN_PROGRESS_MAX_STARS)
        stars = CAMPAIGN_PROGRESS_MAX_STARS;

    CampaignProgressEntry *entry = campaign_progress_find_mut(progress, filename);
    if (!entry) {
        if (stars == 0)
            return 0;
        if (ensure_entry_capacity(progress, progress->count + 1) != 0)
            return -1;
        entry = &progress->entries[progress->count++];
        memset(entry, 0, sizeof(*entry));
        strncpy(entry->filename, filename, CAMPAIGN_LEVEL_MAX_FILENAME - 1);
        entry->stars = stars;
        entry->score = score;
        progress->dirty = true;
        return 1;

}

    if (stars < entry->stars)
        return 0;
    if (stars == entry->stars && score <= entry->score)
        return 0;
    if (stars == 0)
        return 0;

    entry->stars = stars;
    entry->score = score;
    progress->dirty = true;
    return 1;
}

static int campaign_progress_ensure_loaded(void) {

    if (g_progress_loaded)
        return 0;
    if (campaign_progress_load(&g_progress, NULL) != 0)
        return -1;
    g_progress_loaded = true;
    return 0;

}

CampaignProgress *campaign_progress_data(void) {

    if (campaign_progress_ensure_loaded() != 0)
        return NULL;
    return &g_progress;

}

void campaign_progress_shutdown(void) {

    if (!g_progress_loaded)
        return;
    if (g_progress.dirty)
        campaign_progress_save(&g_progress, NULL);
    campaign_progress_free(&g_progress);
    g_progress_loaded = false;

}

void campaign_level_catalog_apply_progress(CampaignLevelCatalog *catalog, const CampaignProgress *progress) {

    if (!catalog)
        return;

    bool previous_completed = true; // first level unlocked by default

    for (int i = 0; i < catalog->count; ++i) {
        CampaignLevelInfo *info = &catalog->items[i];
        info->unlocked = previous_completed;
        info->stars_earned = 0;

        const CampaignProgressEntry *entry = progress ? campaign_progress_find(progress, info->filename) : NULL;
        if (entry) {
            info->stars_earned = entry->stars;
            if (info->stars_earned > 0)
                info->unlocked = true;

}

        bool completed = info->stars_earned > 0;
        previous_completed = completed;
    }
}
