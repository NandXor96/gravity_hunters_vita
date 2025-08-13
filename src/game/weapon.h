#pragma once
#include <SDL2/SDL.h>
#include <stdint.h>

typedef struct Weapon {
    int   projectile_count;   // number of projectiles per shot
    float cooldown;           // seconds between shots
    float last_fire_time;     // world time of last shot
    float min_speed;          // minimum allowed projectile speed
    float max_speed;          // maximum allowed projectile speed
    int   damage;             // damage per projectile
    int   projectile_variant; // index into projectile sheet / trail color
    // Shot point system (replaces simple cooldown gating)
    int   max_shot_points;    // maximum pool size (e.g., 1000)
    float current_shot_points;// current available points (float for smooth regen)
    int   shot_point_cost;    // cost per shot (e.g., 100)
    float regen_rate;         // points regenerated per second
} Weapon;

Weapon* weapon_create_default(void);
void    weapon_destroy(Weapon* w);
int     weapon_can_fire(const Weapon* w, float world_time);
void    weapon_on_fired(Weapon* w, float world_time);
void    weapon_update(Weapon* w, float dt); // handle regeneration each frame
