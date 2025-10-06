#pragma once
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "scene.h"

struct Services;

typedef struct App App;

/**
 * @brief Get the application singleton instance
 * @return Pointer to the App instance
 */
App *app_get(void);

/**
 * @brief Create and initialize the application
 * @return true if successful, false otherwise
 */
bool app_create(void);

/**
 * @brief Destroy and cleanup the application
 */
void app_destroy(void);

/**
 * @brief Update the application for one frame
 */
void app_update(void);

/**
 * @brief Set the base scene
 * @param id Scene ID to set
 * @return true if successful, false otherwise
 */
bool app_set_scene(SceneID id);

/**
 * @brief Push an overlay scene onto the stack
 * @param id Scene ID to push
 * @return true if successful, false otherwise
 */
bool app_push_overlay(SceneID id);

/**
 * @brief Pop the top overlay scene
 */
void app_pop_overlay(void);

/**
 * @brief Check if there are any active overlays
 * @return true if overlays are active, false otherwise
 */
bool app_has_overlay(void);

/**
 * @brief Get the services instance
 * @return Pointer to the Services instance
 */
struct Services *app_services(void);
