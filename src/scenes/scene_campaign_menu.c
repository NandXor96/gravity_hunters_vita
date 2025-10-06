#include "scene_campaign_menu.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "../core/log.h"
#include "../core/types.h"
#include "../services/services.h"
#include "../services/renderer.h"
#include "../services/texture_manager.h"
#include "../services/input.h"
#include "../app/app.h"
#include "scene_campaign.h"
#include "overlay_campaign_details.h"
#include "../game/ui_prompts.h"

#define CAMPAIGN_MENU_VISIBLE_ROWS 6

static SceneCampaignMenuState *s_active_state = NULL;
static int s_focus_last_unlocked_request = 0;

static void scene_campaign_menu_draw_caret(Renderer *r, float cx, float y, double angle_deg)
{

    if (!r)
        return;

    SDL_Texture *texture = NULL;
    SDL_Point size = {0, 0};
    if (!renderer_get_text_texture(r, "^", (TextStyle){0}, &texture, &size) || !texture)
        return;

    SDL_Rect dst = {
        (int)(cx - size.x * 0.5f),
        (int)y,
        size.x,
        size.y};
    SDL_Point center = {size.x / 2, size.y / 2};
    SDL_RenderCopyEx(r->sdl, texture, NULL, &dst, angle_deg, &center, SDL_FLIP_NONE);
}

static void scene_campaign_menu_ensure_visible(SceneCampaignMenuState *st)
{

    if (!st)
        return;

    int count = st->catalog.count;
    if (count <= 0)
    {
        st->first_visible_index = 0;
        return;
    }

    if (st->selected_index < 0)
        st->selected_index = 0;
    if (st->selected_index >= count)
        st->selected_index = count - 1;

    int max_start = (count > CAMPAIGN_MENU_VISIBLE_ROWS) ? (count - CAMPAIGN_MENU_VISIBLE_ROWS) : 0;
    if (st->first_visible_index > max_start)
        st->first_visible_index = max_start;
    if (st->first_visible_index < 0)
        st->first_visible_index = 0;

    if (st->selected_index < st->first_visible_index)
    {

        st->first_visible_index = st->selected_index;
    }
    else if (st->selected_index >= st->first_visible_index + CAMPAIGN_MENU_VISIBLE_ROWS)
    {
        st->first_visible_index = st->selected_index - CAMPAIGN_MENU_VISIBLE_ROWS + 1;
        if (st->first_visible_index > max_start)
            st->first_visible_index = max_start;
    }
}

static void scene_campaign_menu_apply_focus_last_unlocked(SceneCampaignMenuState *st)
{

    if (!st || !s_focus_last_unlocked_request)
        return;

    int last_unlocked = -1;
    for (int i = 0; i < st->catalog.count; ++i)
    {
        if (st->catalog.items[i].unlocked)
            last_unlocked = i;
    }

    if (last_unlocked >= 0)
        st->selected_index = last_unlocked;

    scene_campaign_menu_ensure_visible(st);
    s_focus_last_unlocked_request = 0;
}

static void scene_campaign_menu_refresh(SceneCampaignMenuState *st)
{

    if (!st)
        return;

    if (campaign_levels_scan(&st->descriptors, NULL) != 0)
    {
        LOG_ERROR("scene_campaign_menu", "Failed to scan campaign levels");
        campaign_level_catalog_free(&st->catalog);
        st->selected_index = 0;
        st->first_visible_index = 0;
        return;
    }

    if (campaign_level_catalog_from_descriptors(&st->descriptors, &st->catalog) != 0)
    {
        LOG_ERROR("scene_campaign_menu", "Failed to build campaign catalog");
        campaign_level_catalog_free(&st->catalog);
        st->selected_index = 0;
        st->first_visible_index = 0;
        return;
    }

    if (st->cheat_unlocked)
    {
        for (int i = 0; i < st->catalog.count; ++i)
            st->catalog.items[i].unlocked = true;
    }

    if (st->selected_index >= st->catalog.count)
        st->selected_index = (st->catalog.count > 0) ? st->catalog.count - 1 : 0;

    if (s_focus_last_unlocked_request)
        scene_campaign_menu_apply_focus_last_unlocked(st);
    else
        scene_campaign_menu_ensure_visible(st);
}

void scene_campaign_menu_enter(Scene *s)
{

    SceneCampaignMenuState *st = calloc(1, sizeof(SceneCampaignMenuState));
    if (!st)
    {
        LOG_ERROR("scene_campaign_menu", "Failed to allocate scene state");
        return;
    }
    s->state = st;
    st->svc = app_services();
    st->selected_index = 0;
    st->suppress_input_frames = 4;
    st->prev_button_triangle = 1;
    st->cheat_press_count = 0;
    st->cheat_unlocked = 0;
    scene_campaign_menu_refresh(st);
    s_active_state = st;
}

void scene_campaign_menu_leave(Scene *s)
{

    SceneCampaignMenuState *st = (SceneCampaignMenuState *)s->state;
    if (!st)
        return;
    if (s_active_state == st)
        s_active_state = NULL;
    campaign_level_catalog_free(&st->catalog);
    campaign_levels_free(&st->descriptors);
    free(st);
}

void scene_campaign_menu_focus_last_unlocked(void)
{

    s_focus_last_unlocked_request = 1;
    if (s_active_state)
        scene_campaign_menu_apply_focus_last_unlocked(s_active_state);
}

void scene_campaign_menu_suppress_input(int frames)
{

    if (!s_active_state)
        return;
    if (frames < 0)
        frames = 0;
    s_active_state->suppress_input_frames = frames;
    /* Ensure edge detectors treat held buttons as already pressed */
    s_active_state->prev_move_up = 1;
    s_active_state->prev_move_down = 1;
    s_active_state->prev_confirm = 1;
    s_active_state->prev_back = 1;
    s_active_state->prev_button_triangle = 1;
}

void scene_campaign_menu_handle_input(Scene *s, const struct InputState *in)
{

    SceneCampaignMenuState *st = (SceneCampaignMenuState *)s->state;
    if (!st || !in)
        return;

    if (st->suppress_input_frames > 0)
    {
        st->suppress_input_frames -= 1;
        st->prev_move_up = in->move_up ? 1 : 0;
        st->prev_move_down = in->move_down ? 1 : 0;
        st->prev_confirm = in->confirm ? 1 : 0;
        st->prev_back = in->back ? 1 : 0;
        st->prev_button_triangle = in->button_triangle ? 1 : 0;
        return;
    }

    int up = in->move_up ? 1 : 0;
    int down = in->move_down ? 1 : 0;
    int confirm = in->confirm ? 1 : 0;
    int back = in->back ? 1 : 0;
    int button_triangle = in->button_triangle ? 1 : 0;
    int up_pressed = in->menu_up ? 1 : 0;
    int down_pressed = in->menu_down ? 1 : 0;

    if (st->catalog.count > 0)
    {

        if (button_triangle && !st->prev_button_triangle)
        {
            if (!st->cheat_unlocked)
            {
                st->cheat_press_count += 1;
                if (st->cheat_press_count >= 5)
                {
                    st->cheat_unlocked = 1;
                    st->cheat_press_count = 0;
                    scene_campaign_menu_refresh(st);
                    LOG_INFO("scene_campaign_menu", "Cheat activated: all campaign levels temporarily unlocked");
                }
            }
        }
        if (up_pressed)
        {
            st->selected_index = (st->selected_index - 1 + st->catalog.count) % st->catalog.count;
            scene_campaign_menu_ensure_visible(st);
        }
        if (down_pressed)
        {
            st->selected_index = (st->selected_index + 1) % st->catalog.count;
            scene_campaign_menu_ensure_visible(st);
        }
    }

    if (confirm && !st->prev_confirm)
    {

        if (st->catalog.count > 0)
        {
            CampaignLevelInfo *info = &st->catalog.items[st->selected_index];
            if (info->unlocked)
            {
                overlay_campaign_details_prepare(info->filename);
                if (app_push_overlay(SCENE_OVERLAY_CAMPAIGN_DETAILS))
                    return;
                // Fallback: if overlay couldn't be pushed, start directly
                scene_campaign_set_level(info->filename);
                app_set_scene(SCENE_CAMPAIGN);
                return;
            }
        }
    }

    if (back && !st->prev_back)
    {

        app_set_scene(SCENE_MENU);
        return;
    }

    st->prev_move_up = up;
    st->prev_move_down = down;
    st->prev_confirm = confirm;
    st->prev_back = back;
    st->prev_button_triangle = button_triangle;
}

void scene_campaign_menu_update(Scene *s, float dt)
{

    (void)s;
    (void)dt;
}

void scene_campaign_menu_render(Scene *s, struct Renderer *r)
{

    SceneCampaignMenuState *st = (SceneCampaignMenuState *)s->state;
    if (!st)
        return;

    struct Services *svc = st->svc;
    if (!svc)
        return;

    SDL_Texture *bg = texman_get(svc->texman, TEX_BG_STARFIELD);
    renderer_draw_texture(r, bg, NULL, NULL, 0);

    int cx = svc->display_w / 2;
    int title_y = 60;
    renderer_draw_text_centered(r, "CAMPAIGN", (float)cx, (float)title_y, (TextStyle){0

                                                                          });

    int menu_y = 140;
    int spacing = 56;
    int box_w = 420;
    int box_h = 40;

    SDL_Texture *icons_sheet = texman_get(svc->texman, TEX_ICONS_SHEET);
    SDL_Rect star_src_filled = icons_sheet ? texman_sheet_src(svc->texman, TEX_ICONS_SHEET, 4) : (SDL_Rect){0};
    SDL_Rect star_src_empty = icons_sheet ? texman_sheet_src(svc->texman, TEX_ICONS_SHEET, 8) : (SDL_Rect){0};
    const int star_slots = 3;
    const float star_spacing = 6.f;
    const float star_margin_right = 16.f;

    if (st->catalog.count == 0)
    {

        renderer_draw_text_centered(r, "No campaign levels found", (float)cx, (float)(menu_y + 20), (TextStyle){0

                                                                                                    });
        if (!app_has_overlay())
            ui_draw_prompts(r, svc, true, true);
        return;
    }

    int visible_rows = CAMPAIGN_MENU_VISIBLE_ROWS;
    if (visible_rows > st->catalog.count)
        visible_rows = st->catalog.count;

    int start_index = st->first_visible_index;
    if (start_index < 0)
        start_index = 0;
    if (start_index > st->catalog.count - visible_rows)
        start_index = st->catalog.count - visible_rows;

    const float arrow_tip_gap = 24.f;
    SDL_Point caret_size = renderer_measure_text(r, "^", (TextStyle){0});
    int caret_height = caret_size.y > 0 ? caret_size.y : 0;

    if (start_index > 0)
        scene_campaign_menu_draw_caret(r, (float)cx, (float)(menu_y - arrow_tip_gap), 0.0);

    int end_index = start_index + visible_rows;
    if (end_index > st->catalog.count)
        end_index = st->catalog.count;

    float last_box_bottom = (float)(menu_y + (visible_rows - 1) * spacing + box_h);

    for (int row = 0, i = start_index; i < end_index; ++i, ++row)
    {

        CampaignLevelInfo *info = &st->catalog.items[i];
        float by = (float)(menu_y + row * spacing);
        float bx = (float)(cx - box_w / 2);
        SDL_FRect box = {bx, by, (float)box_w, (float)box_h

        };
        SDL_Color base_color = info->unlocked ? MENU_COLOR_BUTTON_BASE : MENU_COLOR_BUTTON_DISABLED;
        renderer_draw_filled_rect(r, box, base_color);

        int selected = (i == st->selected_index);
        if (selected)
        {
            renderer_draw_rect_outline(r, box, MENU_COLOR_BUTTON_HIGHLIGHT, 2);
        }

        float label_x = bx + 16.f;
        float text_y = by + 10.f;
        const char *display = info->display_name[0] ? info->display_name : info->filename;
        renderer_draw_text(r, display, label_x, text_y, (TextStyle){0});

        int earned = (int)info->stars_earned;
        if (earned < 0)
            earned = 0;
        if (earned > star_slots)
            earned = star_slots;

        bool have_star_icons = icons_sheet && star_src_filled.w > 0 && star_src_filled.h > 0 && star_src_empty.w > 0 && star_src_empty.h > 0;
        float star_size = have_star_icons ? (float)star_src_filled.w : 0.f;
        if (have_star_icons && star_size <= 0.f)
            star_size = (float)star_src_empty.w;

        float star_block_width = star_slots > 0 ? (star_slots * star_size + (star_slots - 1) * star_spacing) : 0.f;
        float star_x = bx + box_w - star_block_width - star_margin_right;
        float star_y = by + (float)box_h * 0.5f;

        if (have_star_icons)
        {

            star_y -= star_size * 0.5f;
            for (int s_idx = 0; s_idx < star_slots; ++s_idx)
            {
                const SDL_Rect *src = (s_idx < earned) ? &star_src_filled : &star_src_empty;
                SDL_FRect dst = {
                    star_x + s_idx * (star_size + star_spacing),
                    star_y,
                    star_size,
                    star_size

                };
                renderer_draw_texture(r, icons_sheet, src, &dst, 0.f);
            }
        }
        else
        {
            char fallback_buf[32];
            snprintf(fallback_buf, sizeof(fallback_buf), "Stars: %d", earned);
            SDL_Point stars_size = renderer_measure_text(r, fallback_buf, (TextStyle){0});
            star_x = bx + box_w - (float)stars_size.x - star_margin_right;
            renderer_draw_text(r, fallback_buf, star_x, text_y, (TextStyle){0});
            star_block_width = (float)stars_size.x;
        }
    }

    if (end_index < st->catalog.count)
    {

        float bottom_arrow_y;
        if (caret_height > 0)
            bottom_arrow_y = last_box_bottom + arrow_tip_gap - (float)caret_height;
        else
            bottom_arrow_y = (float)(menu_y + visible_rows * spacing + 8);
        scene_campaign_menu_draw_caret(r, (float)cx, bottom_arrow_y, 180.0);
    }

    if (!app_has_overlay())
        ui_draw_prompts(r, svc, true, true);
}
