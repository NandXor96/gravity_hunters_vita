#ifndef GH_CAMPAIGN_LEVELS_H
#define GH_CAMPAIGN_LEVELS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Maximum length (including null terminator) for stored campaign level filenames.
// Keep in sync with asset naming conventions; 128 bytes covers typical use.
#define CAMPAIGN_LEVEL_MAX_FILENAME 24
#define CAMPAIGN_LEVEL_MAX_DISPLAY_NAME 20

typedef struct CampaignLevelDescriptor {
    char filename[CAMPAIGN_LEVEL_MAX_FILENAME];
} CampaignLevelDescriptor;

// Containers must be zero-initialized (e.g., CampaignLevelList list = {0};)
// before use to ensure internal pointers start in a safe state.
typedef struct CampaignLevelList {
    CampaignLevelDescriptor *items;
    int count;
    int capacity;
} CampaignLevelList;

typedef struct CampaignLevelInfo {
    char filename[CAMPAIGN_LEVEL_MAX_FILENAME];
    char display_name[CAMPAIGN_LEVEL_MAX_DISPLAY_NAME];
    bool unlocked;
    uint8_t stars_earned;
} CampaignLevelInfo;

typedef struct CampaignLevelCatalog {
    CampaignLevelInfo *items;
    int count;
    int capacity;
} CampaignLevelCatalog;

// Returns the default filesystem root where campaign levels are stored.
// On Vita this resolves to "app0:/assets/levels", otherwise "./assets/levels".
const char *campaign_levels_default_root(void);

// Scans the levels directory (or a custom root) for *.lvl files, sorts them
// alphabetically, and populates the provided list structure. Existing contents
// in the list are released before population.
// Parameters:
//   list        - output container; must be non-null.
//   levels_root - optional override path; when NULL the default root is used.
// Returns 0 on success, negative on failure (e.g., directory missing).
int campaign_levels_scan(CampaignLevelList *list, const char *levels_root);

// Releases any memory owned by the list and resets it to empty.
void campaign_levels_free(CampaignLevelList *list);

// Initializes a CampaignLevelInfo struct with defaults and copies the provided
// filename (truncated to fit). Display name defaults to the filename without
// extension, unlocked=false, stars_earned=0.
void campaign_level_info_init(CampaignLevelInfo *info, const char *filename);

// Releases memory owned by the catalog and resets it to empty.
void campaign_level_catalog_free(CampaignLevelCatalog *catalog);

// Populates the catalog from a descriptor list, copying filenames and applying
// default metadata for each entry. Existing catalog contents are cleared first.
// Returns 0 on success, negative on allocation failure or bad parameters.
int campaign_level_catalog_from_descriptors(const CampaignLevelList *src, CampaignLevelCatalog *dst);

// Converts a level filename into a human-readable display string.
// Strips the extension and replaces underscores with spaces.
void campaign_level_format_display_name(const char *filename, char *buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif // GH_CAMPAIGN_LEVELS_H
