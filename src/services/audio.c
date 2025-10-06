#include "audio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../core/log.h"

struct Audio *audio_create(const char *assets_root) {
    if (!assets_root) {
        LOG_ERROR("audio", "Missing assets root path");
        return NULL;
    }

    struct Audio *audio = calloc(1, sizeof(*audio));
    if (!audio) {
        LOG_ERROR("audio", "Failed to allocate audio state");
        return NULL;
    }

    int desired_flags = MIX_INIT_OGG;
    int obtained_flags = Mix_Init(desired_flags);
    if ((obtained_flags & desired_flags) != desired_flags) {
        LOG_ERROR("audio", "Mix_Init failed: %s", Mix_GetError());
        free(audio);
        return NULL;
    }
    audio->mix_init_flags = obtained_flags;

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096) != 0) {
        LOG_ERROR("audio", "Mix_OpenAudio failed: %s", Mix_GetError());
        Mix_Quit();
        free(audio);
        return NULL;
    }
    audio->audio_opened = 1;

    Mix_AllocateChannels(16);

    Mix_VolumeMusic((int)(MIX_MAX_VOLUME * 0.7f));

    char music_path[512];
    snprintf(music_path, sizeof(music_path), "%s/audio/bg_loop.ogg", assets_root);
    char player_shot_path[512];
    snprintf(player_shot_path, sizeof(player_shot_path), "%s/audio/shot.ogg", assets_root);
    char enemy_shot_path[512];
    snprintf(enemy_shot_path, sizeof(enemy_shot_path), "%s/audio/shot2.ogg", assets_root);

    audio->bg_music = Mix_LoadMUS(music_path);
    if (!audio->bg_music) {
        LOG_ERROR("audio", "Failed to load background music '%s': %s", music_path, Mix_GetError());
    } else {
        if (Mix_PlayMusic(audio->bg_music, -1) != 0) {
            LOG_ERROR("audio", "Failed to start background music: %s", Mix_GetError());
        } else {
            LOG_INFO("audio", "Background music started from '%s'", music_path);
        }
    }

    audio->player_shot = Mix_LoadWAV(player_shot_path);
    if (!audio->player_shot) {
        LOG_ERROR("audio", "Failed to load player shot sound '%s': %s", player_shot_path, Mix_GetError());
    } else {
        Mix_VolumeChunk(audio->player_shot, (int)(MIX_MAX_VOLUME * 0.5f));
    }

    audio->enemy_shot = Mix_LoadWAV(enemy_shot_path);
    if (!audio->enemy_shot) {
        LOG_ERROR("audio", "Failed to load enemy shot sound '%s': %s", enemy_shot_path, Mix_GetError());
    } else {
        Mix_VolumeChunk(audio->enemy_shot, (int)(MIX_MAX_VOLUME * 0.4f));
    }

    return audio;
}

void audio_destroy(struct Audio *audio) {
    if (!audio) {
        return;
    }

    Mix_HaltMusic();

    if (audio->player_shot) {
        Mix_FreeChunk(audio->player_shot);
    }

    if (audio->enemy_shot) {
        Mix_FreeChunk(audio->enemy_shot);
    }

    if (audio->bg_music) {
        Mix_FreeMusic(audio->bg_music);
    }

    if (audio->audio_opened) {
        Mix_CloseAudio();
    }

    Mix_Quit();

    free(audio);
}

void audio_play_shot(struct Audio *audio, int is_player) {
    if (!audio)
        return;
    Mix_Chunk *chunk = is_player ? audio->player_shot : audio->enemy_shot;
    if (!chunk)
        return;
    if (Mix_PlayChannel(-1, chunk, 0) == -1) {
        LOG_WARN("audio", "Failed to play shot sound: %s", Mix_GetError());
    }
}
