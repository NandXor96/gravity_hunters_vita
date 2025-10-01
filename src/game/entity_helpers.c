#include "entity_helpers.h"

void entity_regenerate_energy(float *energy, float regen_rate, int energy_max, float dt) {

    if (regen_rate > 0.f) {
        *energy += regen_rate * dt;
        if (*energy > (float)energy_max)
            *energy = (float)energy_max;

}
}
