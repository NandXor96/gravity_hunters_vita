#pragma once
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "scene.h"

// Forward decl
struct Services;

typedef struct App App;

// Singleton & lifecycle
App* app_get(void);
bool app_create(void); // Initializes SDL, services, base scene (menu)
void app_destroy(void);

// Main loop (one frame)
void app_update(void);

// Scene switching (base scene)
bool app_set_scene(SceneID id);

// Overlays
bool app_push_overlay(SceneID id);
void app_pop_overlay(void);
bool app_has_overlay(void);

// Services accessor
struct Services* app_services(void);
