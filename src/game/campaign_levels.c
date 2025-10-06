#include "campaign_levels.h"
#include "campaign_progress.h"

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <sys/stat.h>

#include <strings.h> // strcasecmp
#include "../core/log.h"

#ifndef PATH_MAX
#define PATH_MAX 256
#endif


#define LEVEL_EXTENSION ".lvl"

static int ensure_descriptor_capacity(CampaignLevelList *list, int min_capacity) {

    if (!list)
        return -1;

    if (list->capacity >= min_capacity)
        return 0;

    int new_capacity = (list->capacity > 0) ? (list->capacity * 2) : 8;
    if (new_capacity < min_capacity)
        new_capacity = min_capacity;

    CampaignLevelDescriptor *new_items = (CampaignLevelDescriptor *)realloc(list->items, (size_t)new_capacity * sizeof(*new_items));
    if (!new_items)
        return -1;

    list->items = new_items;
    list->capacity = new_capacity;
    return 0;

}

static int ensure_catalog_capacity(CampaignLevelCatalog *catalog, int min_capacity) {

    if (!catalog)
        return -1;

    if (catalog->capacity >= min_capacity)
        return 0;

    int new_capacity = (catalog->capacity > 0) ? (catalog->capacity * 2) : 8;
    if (new_capacity < min_capacity)
        new_capacity = min_capacity;

    CampaignLevelInfo *new_items = (CampaignLevelInfo *)realloc(catalog->items, (size_t)new_capacity * sizeof(*new_items));
    if (!new_items)
        return -1;

    catalog->items = new_items;
    catalog->capacity = new_capacity;
    return 0;

}

void campaign_level_format_display_name(const char *filename, char *buffer, size_t buffer_size) {

    if (!buffer || buffer_size == 0)
        return;

    buffer[0] = '\0';
    if (!filename || !filename[0])
        return;

    const char *dot = strrchr(filename, '.');
    size_t copy_len = dot && dot != filename ? (size_t)(dot - filename) : strlen(filename);
    if (copy_len >= buffer_size)
        copy_len = buffer_size - 1;

    memcpy(buffer, filename, copy_len);
    buffer[copy_len] = '\0';

    for (size_t i = 0; i < copy_len; ++i) {
        if (buffer[i] == '_')
            buffer[i] = ' ';
    }

    if (buffer[0] == '\0')
        snprintf(buffer, buffer_size, "%s", filename);

}

static int has_level_extension(const char *name) {

    if (!name)
        return 0;
    const char *dot = strrchr(name, '.');
    if (!dot)
        return 0;
    return strcasecmp(dot, LEVEL_EXTENSION) == 0;

}

static int compare_descriptors(const void *lhs, const void *rhs) {

    const CampaignLevelDescriptor *a = (const CampaignLevelDescriptor *)lhs;
    const CampaignLevelDescriptor *b = (const CampaignLevelDescriptor *)rhs;
    int cmp = strcasecmp(a->filename, b->filename);
    if (cmp == 0)
        cmp = strcmp(a->filename, b->filename);
    return cmp;

}

const char *campaign_levels_default_root(void) {

    #ifdef PLATFORM_VITA
    return "app0:/assets/levels";
#else
    return "./assets/levels";
#endif

}

void campaign_levels_free(CampaignLevelList *list) {

    if (!list)
        return;
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;

}

static int entry_is_directory(const char *root, const struct dirent *entry) {

    if (!root)
        return 0;
    char full_path[PATH_MAX];
    size_t len = snprintf(full_path, sizeof(full_path), "%s/%s", root, entry->d_name);
    if (len >= sizeof(full_path))
        return 1; // treat as directory to skip overly long paths
    struct stat st;
    if (stat(full_path, &st) == 0)
        return S_ISDIR(st.st_mode);
    return 0;

}

static void handle_long_name_warning(const char *name) {

    LOG_WARN("campaign_levels", "Skipping level '%s' (name too long, max %d chars)",
            name ? name : "?", CAMPAIGN_LEVEL_MAX_FILENAME - 1);

}

int campaign_levels_scan(CampaignLevelList *list, const char *levels_root) {

    if (!list)
        return -1;

    campaign_levels_free(list);

    const char *root = (levels_root && levels_root[0]) ? levels_root : campaign_levels_default_root();
    if (!root)
        return -1;

    DIR *dir = opendir(root);
    if (!dir) {
        LOG_ERROR("campaign_levels", "Failed to open '%s': %s", root, strerror(errno));
        return -1;

}

    struct dirent *entry = NULL;
    while ((entry = readdir(dir)) != NULL) {
        const char *name = entry->d_name;
        if (!name || name[0] == '.')
            continue;
        if (entry_is_directory(root, entry))
            continue;
        if (!has_level_extension(name))
            continue;
        size_t len = strlen(name);
        if (len >= CAMPAIGN_LEVEL_MAX_FILENAME) {
            handle_long_name_warning(name);
            continue;
        }
    if (ensure_descriptor_capacity(list, list->count + 1) != 0) {
            closedir(dir);
            campaign_levels_free(list);
            return -1;
        }
        CampaignLevelDescriptor *dst = &list->items[list->count++];
        memcpy(dst->filename, name, len + 1);
    }

    closedir(dir);

    if (list->count > 1)
        qsort(list->items, (size_t)list->count, sizeof(*list->items), compare_descriptors);

    return 0;
}

void campaign_level_info_init(CampaignLevelInfo *info, const char *filename) {

    if (!info)
        return;

    memset(info, 0, sizeof(*info));

    if (filename && filename[0])
        snprintf(info->filename, sizeof(info->filename), "%s", filename);
    else
        info->filename[0] = '\0';

    campaign_level_format_display_name(info->filename[0] ? info->filename : filename, info->display_name, sizeof(info->display_name));
    if (!info->display_name[0] && info->filename[0])
        snprintf(info->display_name, sizeof(info->display_name), "%s", info->filename);
    info->unlocked = false;
    info->stars_earned = 0;

}

void campaign_level_catalog_free(CampaignLevelCatalog *catalog) {

    if (!catalog)
        return;
    free(catalog->items);
    catalog->items = NULL;
    catalog->count = 0;
    catalog->capacity = 0;

}

int campaign_level_catalog_from_descriptors(const CampaignLevelList *src, CampaignLevelCatalog *dst) {

    if (!src || !dst)
        return -1;

    campaign_level_catalog_free(dst);

    if (src->count == 0)
        return 0;

    if (ensure_catalog_capacity(dst, src->count) != 0) {
        campaign_level_catalog_free(dst);
        return -1;

}

    for (int i = 0; i < src->count; ++i) {

        CampaignLevelInfo *info = &dst->items[i];
        campaign_level_info_init(info, src->items[i].filename);

    }
    dst->count = src->count;
    CampaignProgress *progress = campaign_progress_data();
    campaign_level_catalog_apply_progress(dst, progress);
    return 0;
}
