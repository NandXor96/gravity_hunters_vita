#include "scene_quick_play_menu.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../services/services.h"
#include "../services/renderer.h"
#include "../services/input.h"
#include "../app/app.h"
#include "scene_quick_play.h"
#include "../services/texture_manager.h"
#include "../game/ui_prompts.h"
#include "../core/log.h"
#include "../core/types.h"

static const char *planets_labels[] = {"Low", "Medium", "High"};
static const char *size_labels[] = {"Small", "Medium", "Large"};
static const char *difficulty_labels[] = {"Zen", "Easy", "Medium", "Hard"};
static const int time_values[] = {60, 120, 180, 240, 300, 600, -1};
static const char *time_labels[] = {"1m", "2m", "3m", "4m", "5m", "10m", "oo"};

void scene_quick_play_menu_enter(Scene *s)
{

    SceneQuickPlayMenuState *st = calloc(1, sizeof(SceneQuickPlayMenuState));
    if (!st)
    {
        LOG_ERROR("scene_quick_play_menu", "Failed to allocate scene state");
        return;
    }
    s->state = st;
    st->svc = app_services();
    st->selected_row = 0;
    st->planets_count = 1;
    st->planet_size = 1;
    st->difficulty = 2;
    st->time_limit = 1;
}
void scene_quick_play_menu_leave(Scene *s)
{
    SceneQuickPlayMenuState *st = (SceneQuickPlayMenuState *)s->state;
    if (st)
        free(st);
}
void scene_quick_play_menu_handle_input(Scene *s, const struct InputState *in)
{
    SceneQuickPlayMenuState *st = (SceneQuickPlayMenuState *)s->state;
    if (!st || !in)
        return;
    int up = in->menu_up ? 1 : 0;
    int down = in->menu_down ? 1 : 0;
    int left = in->menu_left ? 1 : 0;
    int right = in->menu_right ? 1 : 0;
    int confirm = in->confirm ? 1 : 0;
    int back = in->back ? 1 : 0;
    if (up)
        st->selected_row = (st->selected_row - 1 + 5) % 5;
    if (down)
        st->selected_row = (st->selected_row + 1) % 5;
    if (left)
    {
        if (st->selected_row == 0)
            st->planets_count = (st->planets_count - 1 + 3) % 3;
        if (st->selected_row == 1)
            st->planet_size = (st->planet_size - 1 + 3) % 3;
        if (st->selected_row == 2)
            st->difficulty = (st->difficulty - 1 + 4) % 4;
        if (st->selected_row == 3)
            st->time_limit = (st->time_limit - 1 + 7) % 7;
    }
    if (right)
    {
        if (st->selected_row == 0)
            st->planets_count = (st->planets_count + 1) % 3;
        if (st->selected_row == 1)
            st->planet_size = (st->planet_size + 1) % 3;
        if (st->selected_row == 2)
            st->difficulty = (st->difficulty + 1) % 4;
        if (st->selected_row == 3)
            st->time_limit = (st->time_limit + 1) % 7;
    }
    if (confirm)
    {
        if (st->selected_row == 4)
        {
            QuickPlaySettings qs = {0};
            qs.planets_count = st->planets_count;
            qs.planet_size = st->planet_size;
            qs.difficulty = st->difficulty;
            qs.time_seconds = time_values[st->time_limit];
            scene_quick_play_set_settings(&qs);
            app_set_scene(SCENE_QUICK_PLAY);
            return;
        }
    }
    if (back)
    {
        app_set_scene(SCENE_MENU);
        return;
    }
}
void scene_quick_play_menu_update(Scene *s, float dt)
{
    (void)s;
    (void)dt;
}
void scene_quick_play_menu_render(Scene *s, struct Renderer *r)
{
    SceneQuickPlayMenuState *st = (SceneQuickPlayMenuState *)s->state;
    struct Services *svc = st->svc;
    int cx = svc->display_w / 2;
    int y = 60; // match main menu title height
    SDL_Texture *bg = texman_get(svc->texman, TEX_BG_STARFIELD);

    renderer_draw_texture(r, bg, NULL, NULL, 0);
    renderer_draw_text_centered(r, "QUICK PLAY", (float)cx, (float)y, (TextStyle){0});

    int menu_y = 140; // align with main menu
    int spacing = 56;
    int box_w = 420;
    int box_h = 40;

    // Rows 0..4 are selectboxes, 5 is Start button
    for (int i = 0; i < 5; ++i)
    {
        float by = (float)(menu_y + i * spacing);
        float bx = (float)(cx - box_w / 2);
        SDL_FRect box = {bx, by, (float)box_w, (float)box_h};
    renderer_draw_filled_rect(r, box, MENU_COLOR_BUTTON_BASE);
        int selected = (i == st->selected_row);
        /* outer outline only for Start button when it's selected */
        if (i == 4 && selected)
        {
            renderer_draw_rect_outline(r, box, MENU_COLOR_BUTTON_HIGHLIGHT, 2);
        }
        /* Draw label and value inside the box: label left, value right-aligned */
        float label_x = bx + 12.f;             /* left padding inside box */
        float value_right = bx + box_w - 12.f; /* right padding inside box */
        float text_y = by + 10.f;              /* matches main menu text baseline */
        if (i >= 0 && i <= 3)
        {
            /* choose label and value string for this row */
            const char *label = "";
            const char *val = "";
            if (i == 0)
            {
                label = "Planets";
                val = planets_labels[st->planets_count];
            }
            if (i == 1)
            {
                label = "Planet Size";
                val = size_labels[st->planet_size];
            }
            if (i == 2)
            {
                label = "Difficulty";
                val = difficulty_labels[st->difficulty];
            }
            if (i == 3)
            {
                label = "Time Limit";
                val = time_labels[st->time_limit];
            }

            /* draw label left inside main box */
            renderer_draw_text(r, label, label_x, text_y, (TextStyle){0});

            /* measure value text to create a small inner box and place chevrons */
            SDL_Point m = renderer_measure_text(r, val, (TextStyle){0});
            float val_w = (float)m.x;
            /* fixed inner box width so UI stays consistent */
            float inner_w = 140.f;
            float inner_h = (float)(box_h - 12);
            float inner_x = value_right - inner_w;
            float inner_y = by + (box_h - inner_h) / 2.0f;
            SDL_FRect inner_box = {inner_x, inner_y, inner_w, inner_h};

            /* draw inner box background and outline (highlight if selected) */
            renderer_draw_filled_rect(r, inner_box, MENU_COLOR_SELECTBOX_BACKGROUND);
            /* inner outline only; highlight when selected */
            if (selected)
            {
                renderer_draw_rect_outline(r, inner_box, MENU_COLOR_BUTTON_HIGHLIGHT, 2);
            }
            else
            {
                renderer_draw_rect_outline(r, inner_box, MENU_COLOR_SELECTBOX_OUTLINE, 1);
            }

            /* draw chevrons and value centered inside inner box (fixed width) */
            /* measure chevrons so they sit nicely inside the inner box */
            SDL_Point ml = renderer_measure_text(r, "<", (TextStyle){0});
            SDL_Point mr = renderer_measure_text(r, ">", (TextStyle){0});
            float chev_left_x = inner_x + 6.f;                          /* small padding from left edge */
            float chev_right_x = inner_x + inner_w - (float)mr.x - 6.f; /* ensure '>' fits inside */
            float val_x = inner_x + (inner_w - val_w) / 2.0f;
            renderer_draw_text(r, "<", chev_left_x, text_y, (TextStyle){0});
            renderer_draw_text(r, val, val_x, text_y, (TextStyle){0});
            renderer_draw_text(r, ">", chev_right_x, text_y, (TextStyle){0});
        }
        if (i == 4)
        {
            renderer_draw_text_centered(r, "Start", (float)cx, text_y, (TextStyle){0});
        }
    }

    if (!app_has_overlay())
        ui_draw_prompts(r, svc, true, true);
}
