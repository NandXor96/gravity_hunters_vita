#include "weapon.h"
#include <stdlib.h>

Weapon *weapon_create_default(void) {

    Weapon *wpn = calloc(1, sizeof(Weapon));
    if (!wpn)
        return NULL;
    wpn->projectile_count = 2;
    wpn->cooldown = 0.20f;
    wpn->last_fire_time = 0.f;
    wpn->min_speed = 100.f;
    wpn->max_speed = 500.f;
    wpn->damage = 10;
    wpn->projectile_variant = 1; /* default green variant (matches color table index) */
    wpn->energy_cost = 100; // per shot (player holds pool)
    return wpn;

}
void weapon_destroy(Weapon *w) {
    free(w);
}
int weapon_can_fire(const Weapon *w, float world_time) {
    if (!w) return 0;
    if (w->cooldown > 0.f) {
        float since = world_time - w->last_fire_time;
        if (since < w->cooldown) return 0;
}
    return 1;
}
void weapon_on_fired(Weapon *w, float world_time) {
    if (!w) return;
    w->last_fire_time = world_time; // pool deduction handled by Player
}
void weapon_update(Weapon *w, float dt) {
    (void)w; (void)dt;
}
