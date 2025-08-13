#include "weapon.h"
#include <stdlib.h>

Weapon *weapon_create_default(void)
{
    Weapon *w = calloc(1, sizeof(Weapon));
    if (!w)
        return NULL;
    w->projectile_count = 2;
    w->cooldown = 0.20f;
    w->last_fire_time = 0.f;
    w->min_speed = 100.f;
    w->max_speed = 500.f;
    w->damage = 10;
    w->projectile_variant = 1; /* default green variant (matches color table index) */
    // Shot point system defaults
    w->max_shot_points = 1000;
    w->current_shot_points = (float)w->max_shot_points;
    w->shot_point_cost = 100; // cost per shot
    w->regen_rate = 100.f; // points per second
    return w;
}
void weapon_destroy(Weapon *w) { free(w); }
int weapon_can_fire(const Weapon *w, float world_time) {
    if(!w) return 0;
    // Need enough shot points AND respect minimal interval (cooldown)
    if(w->current_shot_points < (float)w->shot_point_cost) return 0;
    if(w->cooldown > 0.f) {
        float since = world_time - w->last_fire_time;
        if(since < w->cooldown) return 0;
    }
    return 1;
}
void weapon_on_fired(Weapon *w, float world_time)
{
    if (!w) return;
    w->last_fire_time = world_time; // keep for potential effects
    // Deduct cost
    w->current_shot_points -= (float)w->shot_point_cost;
    if(w->current_shot_points < 0.f) w->current_shot_points = 0.f;
}
void weapon_update(Weapon *w, float dt){
    if(!w) return;
    if(w->regen_rate > 0.f){
        w->current_shot_points += w->regen_rate * dt;
        float maxf = (float)w->max_shot_points;
        if(w->current_shot_points > maxf) w->current_shot_points = maxf;
    }
}
