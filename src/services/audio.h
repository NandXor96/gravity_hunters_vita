#pragma once

#include <SDL2/SDL_mixer.h>

struct Audio {
    Mix_Music *bg_music;
    Mix_Chunk *player_shot;
    Mix_Chunk *enemy_shot;
    int mix_init_flags;
    int audio_opened;
};

struct Audio *audio_create(const char *assets_root);
void audio_destroy(struct Audio *audio);
void audio_play_shot(struct Audio *audio, int is_player);
