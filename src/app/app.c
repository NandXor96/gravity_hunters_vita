#include "app.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "scenestack.h"
#include "../services/services.h"
#include "../services/input.h"
#include "../services/renderer.h"
#include "../scenes/scene_menu.h"
#include "../core/time.h"
#include "../core/types.h"
#include "../core/log.h"
#include "../services/texture_manager.h"
#include "../game/campaign_progress.h"

struct App {
    struct Services *services;
    SceneStack stack;
    FrameTimer timer;
    int running;
};

static App *g_app = NULL;

App *app_get(void) {
    return g_app;
}

struct Services *app_services(void) {
    return g_app ? g_app->services : NULL;
}

bool app_create(void) {
    if (g_app) {
        return true;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        LOG_ERROR("app", "SDL_Init failed: %s", SDL_GetError());
        return false;
    }

    if (TTF_Init() != 0) {
        LOG_ERROR("app", "TTF_Init failed: %s", TTF_GetError());
        return false;
    }

    g_app = (App *)calloc(1, sizeof(App));
    if (!g_app) {
        LOG_ERROR("app", "Failed to allocate App structure");
        return false;
    }

    g_app->services = services_get();
    services_init(g_app->services);

    if (!campaign_progress_data()) {
        LOG_WARN("app", "Failed to load campaign progress from %s",
                campaign_progress_default_path());
    }

    scenestack_init(&g_app->stack);
    if (!scenestack_set_base(&g_app->stack, SCENE_MENU)) {
        LOG_ERROR("app", "Failed to create initial menu scene");
    }
    timer_init(&g_app->timer);
    g_app->running = 1;
    return true;
}

void app_destroy(void) {
    if (!g_app) {
        return;
    }
    scenestack_shutdown(&g_app->stack);
    campaign_progress_shutdown();
    services_shutdown(g_app->services);
    SDL_Quit();
    free(g_app);
    g_app = NULL;
}

bool app_set_scene(SceneID id) {
    return scenestack_set_base(&g_app->stack, id);
}

bool app_push_overlay(SceneID id) {
    return scenestack_push(&g_app->stack, id);
}

void app_pop_overlay(void) {
    scenestack_pop(&g_app->stack);
}

bool app_has_overlay(void) {
    return g_app && g_app->stack.overlay_count > 0;
}

void app_update(void) {
    if (!g_app) {
        return;
    }

    double frame_dt = timer_frame_delta(&g_app->timer);
    if (frame_dt > 0.25) {
        frame_dt = 0.25;
    }
    g_app->timer.accumulator += frame_dt;

    if (g_app->timer.accumulator >= FIXED_DT) {
        struct InputState in = {0};
        input_poll(g_app->services->input, &in);
        scenestack_handle_input(&g_app->stack, &in);
        scenestack_update(&g_app->stack, FIXED_DT);
        g_app->timer.accumulator -= FIXED_DT;
    }

    SDL_Renderer *sdlr = g_app->services->sdl_renderer;
    SDL_SetRenderDrawColor(sdlr, 0, 0, 0, 255);
    SDL_RenderClear(sdlr);

    scenestack_render(&g_app->stack, g_app->services->renderer);
    renderer_render(g_app->services->renderer);
}