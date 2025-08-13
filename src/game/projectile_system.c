#include "projectile_system.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "planet.h"
#include "player.h"
#include "enemy.h"
#include "weapon.h"

void projectile_system_init(ProjectileSystem *ps, TextureManager *tm)
{
    if (ps)
    {
        *ps = (ProjectileSystem){0};
        ps->texman = tm;
    }
}
static void destroy_projectile(ProjectileSystem *ps, Projectile *p)
{
    if (!p)
        return;
    (void)ps; /* textures now always resident; no release */
    projectile_destroy(p);
}
void projectile_system_shutdown(ProjectileSystem *ps)
{
    if (!ps)
        return;
    for (int s = 0; s < ps->shooter_count; s++)
    {
        ShooterPool *pool = &ps->shooters[s];
        for (int i = 0; i < pool->head; i++)
        {
            destroy_projectile(ps, pool->items[i]);
        }
        pool->head = 0;
    }
}
int projectile_system_register_shooter(ProjectileSystem *ps)
{
    if (!ps)
        return -1;
    if (ps->shooter_count >= MAX_SHOOTERS)
        return -1;
    return ps->shooter_count++;
}
bool projectile_system_fire(ProjectileSystem *ps, float world_time, int shooter_index, Entity *owner, float angle, float strength)
{
    if (!ps || !owner || shooter_index < 0 || shooter_index >= ps->shooter_count)
        return false;
    ShooterPool *pool = &ps->shooters[shooter_index];
    // cooldown handled externally by weapon system; only capacity check here
    if (pool->head >= MAX_PROJECTILES_PER_SHOOTER)
        return false;
    Vec2 dir = {cosf(angle), sinf(angle)};
    if (strength <= 0.f)
        strength = 300.f;
    Vec2 vel = {dir.x * strength, dir.y * strength};
    int damage = 1;
    int variant = 0;
    if (owner->type == ENT_PLAYER) {
        Player *pl = (Player *)owner; if (pl->weapon) { damage = pl->weapon->damage; variant = pl->weapon->projectile_variant; }}
    else if (owner->type == ENT_ENEMY) { Enemy *en = (Enemy *)owner; if (en->weapon) { damage = en->weapon->damage; variant = en->weapon->projectile_variant; }}
    /* Clamp variant */
    int maxv = texman_projectile_variant_count(ps->texman); if (maxv>0) { if (variant < 0) variant = 0; if (variant >= maxv) variant = maxv-1; }
    /* Texture & color */
    SDL_Texture *sheet = texman_projectiles_texture(ps->texman);
    SDL_Rect src = texman_projectile_src(ps->texman, variant);
    Uint8 cr=255,cg=255,cb=255; texman_projectile_variant_color(ps->texman, variant, &cr,&cg,&cb);
    SDL_Texture *proj_tex = sheet; /* store sheet; projectile render can choose src rect */
    Projectile *p = projectile_create(owner, owner->pos, vel, proj_tex, damage, cr, cg, cb);
    if(p) p->variant = variant;
    if (!p)
        return false;
    pool->items[pool->head++] = p;
    pool->last_fire_time = world_time; // keep timestamp for possible future logic (e.g., muzzle flash)
    return true;
}
// physics subsystem removed: gravity handled in projectile.c via planet masses
void projectile_system_update(ProjectileSystem *ps, struct Planet **planets, int planet_count, struct Player *player, struct Enemy **enemies, int enemy_count, float oob_margin_factor, int display_w, int display_h, float dt, float world_time)
{
    if (!ps)
        return;
    float margin_x = oob_margin_factor * (float)display_w;
    float margin_y = oob_margin_factor * (float)display_h;
    ProjectileUpdateCtx ctx; memset(&ctx,0,sizeof(ctx));
    ctx.planets = planets;
    ctx.planet_count = planet_count;
    ctx.player = player;
    ctx.enemies = enemies;
    ctx.enemy_count = enemy_count;
    ctx.min_x = -margin_x;
    ctx.min_y = -margin_y;
    ctx.max_x = (float)display_w + margin_x;
    ctx.max_y = (float)display_h + margin_y;
    ctx.dt = dt;
    for (int s = 0; s < ps->shooter_count; s++)
    {
        ShooterPool *pool = &ps->shooters[s];
        int write = 0;
        for (int i = 0; i < pool->head; i++)
        {
            Projectile *pr = pool->items[i];
            if (!pr)
                continue;
            if (pr->active)
                projectile_update(pr, &ctx);
            if (pr->active)
            {
                pool->items[write++] = pr;
            }
            else
            {
                destroy_projectile(ps, pr);
            }
        }
        pool->head = write;
    }
    (void)world_time;
}
void projectile_system_render(ProjectileSystem *ps, Renderer *r)
{
    if (!ps)
        return;
    for (int s = 0; s < ps->shooter_count; s++)
    {
        ShooterPool *pool = &ps->shooters[s];
        for (int i = 0; i < pool->head; i++)
        {
            Projectile *pr = pool->items[i];
            if (!pr || !pr->active)
                continue;
            if (pr->e.vt && pr->e.vt->render)
                pr->e.vt->render((Entity *)pr, r);
        }
    }
}
