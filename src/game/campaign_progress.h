#ifndef GH_CAMPAIGN_PROGRESS_H
#define GH_CAMPAIGN_PROGRESS_H

#include <stdbool.h>
#include <stdint.h>

#include "campaign_levels.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CAMPAIGN_PROGRESS_MAX_STARS 3

typedef struct CampaignProgressEntry {
    char filename[CAMPAIGN_LEVEL_MAX_FILENAME];
    uint8_t stars;
    int32_t score;
} CampaignProgressEntry;

// Container storing all progress entries; zero-initialize before use.
typedef struct CampaignProgress {
    CampaignProgressEntry *entries;
    int count;
    int capacity;
    bool dirty;
} CampaignProgress;

// Returns default save path for current platform.
const char *campaign_progress_default_path(void);

int campaign_progress_load(CampaignProgress *progress, const char *path);
int campaign_progress_save(CampaignProgress *progress, const char *path);
void campaign_progress_free(CampaignProgress *progress);

const CampaignProgressEntry *campaign_progress_find(const CampaignProgress *progress, const char *filename);
int campaign_progress_update(CampaignProgress *progress, const char *filename, uint8_t stars, int32_t score);

CampaignProgress *campaign_progress_data(void);
void campaign_progress_shutdown(void);

void campaign_level_catalog_apply_progress(CampaignLevelCatalog *catalog, const CampaignProgress *progress);

#ifdef __cplusplus
}
#endif

#endif // GH_CAMPAIGN_PROGRESS_H
