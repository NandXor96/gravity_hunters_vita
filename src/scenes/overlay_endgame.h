#pragma once
#include "../app/scene.h"

typedef enum OverlayEndgameFailReason {
    OVERLAY_ENDGAME_FAIL_NONE = 0,
    OVERLAY_ENDGAME_FAIL_TIME,
    OVERLAY_ENDGAME_FAIL_DEATH,
    OVERLAY_ENDGAME_FAIL_SCORE
} OverlayEndgameFailReason;

typedef struct OverlayEndgameState {
    struct Services *svc;
    int selected;
    bool prev_confirm;
    /* final stats */
    int kills;
    int points;
    /* campaign-specific: whether all goals were met and rating thresholds */
    int has_level_result; /* 0 = not provided, 1 = provided */
    int goals_all_met;    /* boolean */
    unsigned int rating[3];
    OverlayEndgameFailReason fail_reason;
} OverlayEndgameState;

void overlay_endgame_enter(Scene *s);
void overlay_endgame_leave(Scene *s);
void overlay_endgame_handle_input(Scene *s, const struct InputState *in);
void overlay_endgame_update(Scene *s, float dt);
void overlay_endgame_render(Scene *s, struct Renderer *r);
/* Setter to provide final stats to the overlay before pushing it (quickplay/campaign) */
void overlay_endgame_set_stats(int kills, int points);
/* Campaign-only: provide whether all goals were met and three rating thresholds
 * Call this before pushing the overlay for campaign scenes. */
void overlay_endgame_set_campaign_result(int goals_all_met, unsigned int rating0, unsigned int rating1, unsigned int rating2);
void overlay_endgame_set_campaign_level(const char *filename);
void overlay_endgame_set_failure_reason(OverlayEndgameFailReason reason);
