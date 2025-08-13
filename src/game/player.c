#include "player.h"
#include <stdlib.h>
#include <math.h>
#include "projectile.h"
// Local constant for PI to avoid relying on platform-specific M_PI macro
static const float PI_F = 3.14159265358979323846f;
#include "../services/renderer.h"
#include "world.h" // for world_fire_projectile
#include "weapon.h"

static void player_render(Entity *e, struct Renderer *r)
{
    Player *p = (Player *)e;
    SDL_FRect dst = {p->e.pos.x - p->e.size.x * 0.5f, p->e.pos.y - p->e.size.y * 0.5f, p->e.size.x, p->e.size.y};
    float angle_deg = (p->e.angle + p->e.angle_offset) * (180.0f / PI_F);
    renderer_draw_texture(r, p->e.texture, NULL, &dst, angle_deg);
}
static void player_update(Entity *e, float dt)
{
    Player *p = (Player*)e;
    if(!p) return;
    // weapon regen update
    if(p->weapon) weapon_update(p->weapon, dt);
    // movement (WASD)
    float speed = 120.0f * dt;
    if (p->input.move_left)  p->e.pos.x -= speed;
    if (p->input.move_right) p->e.pos.x += speed;
    if (p->input.move_up)    p->e.pos.y -= speed;
    if (p->input.move_down)  p->e.pos.y += speed;
    // adjust projectile speed via step events (auto-repeat)
    if(p->weapon){
        float base_step_small = 5.f;   // units per step with small modifier
        float base_step_normal = 10.f;  // default
        float base_step_large = 30.f;   // with large modifier
        float step = p->input.mod_large ? base_step_large : (p->input.mod_small ? base_step_small : base_step_normal);
        if(p->input.speed_up_step)   p->current_shot_speed += step;
        if(p->input.speed_down_step) p->current_shot_speed -= step;
        if(p->current_shot_speed < p->weapon->min_speed) p->current_shot_speed = p->weapon->min_speed;
        if(p->current_shot_speed > p->weapon->max_speed) p->current_shot_speed = p->weapon->max_speed;
    }
    // rotation: if analog stick active use its angle, else apply step events
    if(p->input.stick_active){
        // Adjust stick angle by +90deg so that right on stick yields facing right on screen.
        // (Observed: previous mapping made right=up, meaning we were effectively -90deg off.)
        float adjusted = p->input.stick_angle + (PI_F * 0.5f);
        float target = adjusted - p->e.angle_offset;
        // Normalize to [-PI, PI] for numerical stability
        if(target >  PI_F) target -= 2.f * PI_F;
        if(target < -PI_F) target += 2.f * PI_F;
        p->e.angle = target;
    } else {
        float base_step_small = 0.01f;
        float base_step_normal = 0.02f;
        float base_step_large = 0.05f;
        float step = p->input.mod_large ? base_step_large : (p->input.mod_small ? base_step_small : base_step_normal);
        if(p->input.turn_left_step)  p->e.angle -= step;
        if(p->input.turn_right_step) p->e.angle += step;
    }
    // shooting: fire on press edge OR while held if weapon rate allows (auto-fire)
    if(p->world && p->input.fire){
        if(!p->fire_was_down){
            player_shoot(p, p->world, -1.f);
        } else {
            // held: attempt auto fire respecting weapon_can_fire (enforces cooldown & points)
            player_shoot(p, p->world, -1.f);
        }
    }
    p->fire_was_down = p->input.fire;
}
static void player_destroy_entity(Entity *e) { player_destroy((Player *)e); }
static void player_on_hit_entity(Entity *e, Entity *hitter)
{
    if(!e|| e->type!=ENT_PLAYER) return;
    Player* p = (Player*)e;
    if(!p->alive) return;
    if(hitter && hitter->type == ENT_PROJECTILE){
        struct Projectile* pr = (struct Projectile*)hitter; // forward declared via projectile.h through inclusion chain
        int dmg = 1;
        if(pr) dmg = pr->damage;
        // Friendly fire filter: ignore projectiles fired by players
        if(pr && pr->owner_kind == ENT_PLAYER) return; // do not deactivate friendly projectile
        p->health -= dmg;
        if(p->health <= 0){ p->alive = false; }
        if(pr) pr->active = false; // deactivate projectile only on valid hit
    }
}

static Entity *player_create_entity(void *params)
{
    SDL_Texture *tex = (SDL_Texture *)params;
    return (Entity *)player_create(tex);
}

static const EntityVTable PLAYER_VT = {player_create_entity, player_destroy_entity, player_update, player_render, player_on_hit_entity};

Player *player_create(SDL_Texture *tex)
{
    Player *p = calloc(1, sizeof(Player));
    p->e.texture = tex;
    p->alive = true;
    p->e.size = (Vec2){32, 27};
    p->e.vt = &PLAYER_VT;
    p->e.type = ENT_PLAYER;
    p->e.angle_offset = (PI_F * 0.5f); // sprite artwork points up
    p->shooter_index = -1;
    p->world = NULL;
    p->rotation_speed = 2.5f;
    p->health = 10;
    p->max_health = 10;
    p->weapon = weapon_create_default();
    p->current_shot_speed = p->weapon ? (p->weapon->min_speed + p->weapon->max_speed)*0.5f : 300.f;
    p->fire_was_down = false;
    return p;
}
void player_destroy(Player *p) { if(!p) return; if(p->weapon) weapon_destroy(p->weapon); free(p); }
void player_fly(Player *p, const InputState *in, float dt)
{
    // deprecated: logic moved to player_update
    (void)p; (void)in; (void)dt;
}
bool player_shoot(Player *p, struct World *w, float strength)
{
    if (!p || !w) return false;
    if (p->shooter_index < 0) return false;
    if(!p->weapon) return false;
    if(!weapon_can_fire(p->weapon, w->time)) return false;
    if (strength <= 0.f) strength = p->current_shot_speed;
    bool ok = world_fire_projectile(w, p->shooter_index, (Entity*)p, p->e.angle, strength);
    if(ok) weapon_on_fired(p->weapon, w->time);
    return ok;
}
void player_on_kill(Player *p) { (void)p; }
OBB player_collider(const Player *p) { return (OBB){{p->e.pos.x, p->e.pos.y}, {p->e.size.x, p->e.size.y}, p->e.angle}; }
void player_set_input(Player *p, const InputState *in){ if(p&&in) p->input = *in; }
