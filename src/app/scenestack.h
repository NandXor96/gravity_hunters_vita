#pragma once
#include "scene.h"

typedef struct SceneStack {
    Scene *base;
    Scene *overlays[8];
    int overlay_count;
} SceneStack;

/**
 * @brief Initialize a scene stack
 * @param st Scene stack to initialize
 */
void scenestack_init(SceneStack *st);

/**
 * @brief Shutdown and cleanup a scene stack
 * @param st Scene stack to shutdown
 */
void scenestack_shutdown(SceneStack *st);

/**
 * @brief Set the base scene for the stack
 * @param st Scene stack
 * @param id Scene ID to set as base
 * @return true if successful, false otherwise
 */
bool scenestack_set_base(SceneStack *st, SceneID id);

/**
 * @brief Push an overlay scene onto the stack
 * @param st Scene stack
 * @param id Scene ID to push as overlay
 * @return true if successful, false otherwise
 */
bool scenestack_push(SceneStack *st, SceneID id);

/**
 * @brief Pop the top overlay scene from the stack
 * @param st Scene stack
 */
void scenestack_pop(SceneStack *st);

/**
 * @brief Handle input for all scenes in the stack
 * @param st Scene stack
 * @param in Input state
 */
void scenestack_handle_input(SceneStack *st, const struct InputState *in);

/**
 * @brief Update all active scenes in the stack
 * @param st Scene stack
 * @param dt Delta time
 */
void scenestack_update(SceneStack *st, float dt);

/**
 * @brief Render all scenes in the stack
 * @param st Scene stack
 * @param r Renderer
 */
void scenestack_render(SceneStack *st, struct Renderer *r);
