#include "scene_textbox.h"

#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../core/types.h"
#include "../services/services.h"
#include "../services/renderer.h"
#include "../services/texture_manager.h"
#include "../services/input.h"
#include "../app/app.h"
#include "../game/ui_prompts.h"
#include "../core/log.h"

#define TEXTBOX_LINE_SPACING 28.f
#define TEXTBOX_TOP_MARGIN 130.f
#define TEXTBOX_BOTTOM_MARGIN 106.f
#define TEXTBOX_CONTENT_WIDTH 800.f

typedef struct TextboxSceneState
{
    struct Services *svc;
    const TextboxSceneConfig *config;
    char **lines;
    char *text_buffer;
    int line_count;
    int top_line;
    int suppress_input_frames;
    int prev_move_up;
    int prev_move_down;
    int prev_back;
} TextboxSceneState;

static void textbox_scene_clear_lines(TextboxSceneState *st)
{

    if (!st)
        return;
    if (st->lines)
        free(st->lines);
    st->lines = NULL;
    if (st->text_buffer)
        free(st->text_buffer);
    st->text_buffer = NULL;
    st->line_count = 0;
}

static bool textbox_scene_apply_fallback(TextboxSceneState *st)
{

    if (!st || !st->config || !st->config->fallback_lines || st->config->fallback_count <= 0)
        return false;

    textbox_scene_clear_lines(st);

    int count = st->config->fallback_count;
    size_t total_len = 0;
    for (int i = 0; i < count; ++i)
        total_len += strlen(st->config->fallback_lines[i]) + 1;

    char *buffer = (char *)malloc(total_len);
    if (!buffer)
    {
        LOG_ERROR("textbox_scene", "Failed to allocate fallback buffer");
        return false;
    }

    char **lines = (char **)calloc((size_t)count, sizeof(char *));
    if (!lines)
    {
        LOG_ERROR("textbox_scene", "Failed to allocate fallback lines array");
        free(buffer);
        return false;
    }

    char *cursor = buffer;
    for (int i = 0; i < count; ++i)
    {
        const char *src = st->config->fallback_lines[i];
        size_t len = strlen(src);
        memcpy(cursor, src, len);
        cursor[len] = '\0';
        lines[i] = cursor;
        cursor += len + 1;
    }

    st->text_buffer = buffer;
    st->lines = lines;
    st->line_count = count;
    return true;
}

static bool textbox_scene_load_file(TextboxSceneState *st)
{

    if (!st || !st->config || !st->config->relative_path || !st->config->relative_path[0])
        return false;

    textbox_scene_clear_lines(st);

    char path[512];
#ifdef PLATFORM_VITA
    snprintf(path, sizeof(path), "app0:/assets/%s", st->config->relative_path);
#else
    snprintf(path, sizeof(path), "./assets/%s", st->config->relative_path);
#endif

    FILE *file = fopen(path, "rb");
    if (!file)
        return false;

    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        return false;
    }

    long raw_size = ftell(file);
    if (raw_size < 0)
    {
        fclose(file);
        return false;
    }
    rewind(file);

    size_t size = (size_t)raw_size;
    char *buffer = (char *)malloc(size + 1);
    if (!buffer)
    {
        LOG_ERROR("textbox_scene", "Failed to allocate file buffer");
        fclose(file);
        return false;
    }

    size_t read_bytes = fread(buffer, 1, size, file);
    fclose(file);
    buffer[read_bytes] = '\0';

    size_t write_idx = 0;
    for (size_t i = 0; i < read_bytes; ++i)
    {
        char c = buffer[i];
        if (c == '\r')
            continue;
        buffer[write_idx++] = c;
    }
    buffer[write_idx] = '\0';

    if (write_idx == 0)
    {

        free(buffer);
        return false;
    }

    size_t line_count = 1;
    for (size_t i = 0; i < write_idx; ++i)
        if (buffer[i] == '\n')
            line_count += 1;

    char **lines = (char **)calloc(line_count, sizeof(char *));
    if (!lines)
    {
        LOG_ERROR("textbox_scene", "Failed to allocate lines array");
        free(buffer);
        return false;
    }

    size_t idx = 0;
    char *start = buffer;
    for (size_t i = 0; i < write_idx; ++i)
    {
        if (buffer[i] == '\n')
        {
            buffer[i] = '\0';
            lines[idx++] = start;
            start = buffer + i + 1;
        }
    }
    lines[idx++] = start;

    st->text_buffer = buffer;
    st->lines = lines;
    st->line_count = (int)idx;
    return true;
}

static void textbox_scene_draw_caret(Renderer *r, float cx, float y, double angle_deg)
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

static void textbox_scene_ensure_visible(TextboxSceneState *st, int visible_lines)
{

    if (!st)
        return;
    if (st->top_line < 0)
        st->top_line = 0;
    int max_start = (st->line_count > visible_lines) ? (st->line_count - visible_lines) : 0;
    if (st->top_line > max_start)
        st->top_line = max_start;
}

void textbox_scene_enter(Scene *s, const TextboxSceneConfig *config)
{

    if (!s)
        return;
    TextboxSceneState *st = (TextboxSceneState *)calloc(1, sizeof(TextboxSceneState));
    if (!st)
    {
        LOG_ERROR("textbox_scene", "Failed to allocate scene state");
        return;
    }
    s->state = st;
    st->svc = app_services();
    st->config = config;
    st->suppress_input_frames = 4;

    if (!textbox_scene_load_file(st))
        textbox_scene_apply_fallback(st);
    if (st->line_count <= 0)
        textbox_scene_apply_fallback(st);
}

void textbox_scene_leave(Scene *s)
{

    TextboxSceneState *st = (TextboxSceneState *)s->state;
    if (!st)
        return;
    textbox_scene_clear_lines(st);
    free(st);
    s->state = NULL;
}

void textbox_scene_handle_input(Scene *s, const struct InputState *in)
{

    TextboxSceneState *st = (TextboxSceneState *)s->state;
    if (!st || !in)
        return;

    if (st->suppress_input_frames > 0)
    {
        st->suppress_input_frames -= 1;
        st->prev_move_up = in->move_up ? 1 : 0;
        st->prev_move_down = in->move_down ? 1 : 0;
        st->prev_back = in->back ? 1 : 0;
        return;
    }

    int up = in->move_up ? 1 : 0;
    int down = in->move_down ? 1 : 0;
    int back = in->back ? 1 : 0;
    int up_pressed = in->menu_up ? 1 : 0;
    int down_pressed = in->menu_down ? 1 : 0;

    struct Services *svc = st->svc;
    int display_h = svc ? svc->display_h : DISPLAY_H;
    float content_height = (float)display_h - TEXTBOX_TOP_MARGIN - TEXTBOX_BOTTOM_MARGIN;
    if (content_height < TEXTBOX_LINE_SPACING)
        content_height = TEXTBOX_LINE_SPACING;
    int visible_lines = (int)(content_height / TEXTBOX_LINE_SPACING);
    if (visible_lines < 1)
        visible_lines = 1;

    if (up_pressed)
    {

        st->top_line -= 1;
        textbox_scene_ensure_visible(st, visible_lines);
    }
    if (down_pressed)
    {
        st->top_line += 1;
        textbox_scene_ensure_visible(st, visible_lines);
    }

    if (back && !st->prev_back)
    {

        app_set_scene(SCENE_MENU);
        return;
    }

    st->prev_move_up = up;
    st->prev_move_down = down;
    st->prev_back = back;
}

void textbox_scene_update(Scene *s, float dt)
{

    (void)s;
    (void)dt;
}

void textbox_scene_render(Scene *s, struct Renderer *r)
{

    TextboxSceneState *st = (TextboxSceneState *)s->state;
    if (!st || !r)
        return;

    struct Services *svc = st->svc;
    if (!svc)
        return;

    SDL_Texture *bg = texman_get(svc->texman, TEX_BG_STARFIELD);
    renderer_draw_texture(r, bg, NULL, NULL, 0);

    int display_w = svc->display_w;
    int display_h = svc->display_h;
    int cx = display_w / 2;

    const char *title = (st->config && st->config->title) ? st->config->title : "";
    renderer_draw_text_centered(r, title, (float)cx, 60.f, (TextStyle){0});

    float content_w = TEXTBOX_CONTENT_WIDTH;
    float max_w = (float)display_w * 0.9f;
    if (content_w > max_w)
        content_w = max_w;
    float content_x = ((float)display_w - content_w) * 0.5f;
    float content_y = TEXTBOX_TOP_MARGIN;
    float content_h = (float)display_h - TEXTBOX_TOP_MARGIN - TEXTBOX_BOTTOM_MARGIN;
    if (content_h < TEXTBOX_LINE_SPACING)
        content_h = TEXTBOX_LINE_SPACING * 2.f;

    SDL_FRect box = {content_x - 16.f, content_y - 20.f, content_w + 32.f, content_h + 40.f};
    renderer_draw_filled_rect(r, box, MENU_COLOR_TEXT_BACKGROUND);

    int visible_lines = (int)(content_h / TEXTBOX_LINE_SPACING);
    if (visible_lines < 1)
        visible_lines = 1;
    textbox_scene_ensure_visible(st, visible_lines);

    int end_line = st->top_line + visible_lines;
    if (end_line > st->line_count)
        end_line = st->line_count;

    float y = content_y;
    for (int i = st->top_line; i < end_line; ++i)
    {
        const char *line = (st->lines && i >= 0 && i < st->line_count) ? st->lines[i] : NULL;
        if (line && line[0] != '\0')
            renderer_draw_text(r, line, content_x, y, (TextStyle){0});
        y += TEXTBOX_LINE_SPACING;
    }

    if (st->top_line > 0)
        textbox_scene_draw_caret(r, (float)cx, content_y - 36.f, 0.0);

    if (end_line < st->line_count)
        textbox_scene_draw_caret(r, (float)cx, content_y + content_h + 8.f, 180.0);

    if (!app_has_overlay())
    {
        bool show_ok = true;
        bool show_back = true;
        if (st->config && st->config->use_custom_prompts)
        {
            show_ok = st->config->show_ok_prompt;
            show_back = st->config->show_back_prompt;
        }
        if (show_ok || show_back)
            ui_draw_prompts(r, svc, show_ok, show_back);
    }
}
