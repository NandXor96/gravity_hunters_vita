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
    // Energy economy now lives mostly on Player; only per-shot cost remains here
    int   energy_cost;        // cost per shot (e.g., 100)
} Weapon;

Weapon *weapon_create_default(void);
void    weapon_destroy(Weapon *w);
int     weapon_can_fire(const Weapon *w, float world_time); // only checks cooldown now
void    weapon_on_fired(Weapon *w, float world_time); // only updates last_fire_time (no energy deduction)
void    weapon_update(Weapon *w, float dt); // currently no-op (placeholder for future weapon-specific logic)
