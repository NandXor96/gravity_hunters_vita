#pragma once

/**
 * @brief Helper functions for common entity operations.
 */

/**
 * @brief Regenerate energy for an entity over time.
 *
 * @param energy Pointer to current energy value.
 * @param regen_rate Energy regeneration rate per second.
 * @param energy_max Maximum energy capacity.
 * @param dt Delta time in seconds.
 */
void entity_regenerate_energy(float *energy, float regen_rate, int energy_max, float dt);
