#include "math.h"
#include <math.h>

float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

float lengthf(Vec2 v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}

Vec2 add2(Vec2 a, Vec2 b) {
    return (Vec2){a.x + b.x, a.y + b.y};
}

Vec2 sub2(Vec2 a, Vec2 b) {
    return (Vec2){a.x - b.x, a.y - b.y};
}

Vec2 mul2(Vec2 a, float s) {
    return (Vec2){a.x * s, a.y * s};
}

Vec2 norm2(Vec2 v) {
    float l = lengthf(v);
    return l > 0 ? mul2(v, 1.0f / l) : (Vec2){0, 0};
}

bool collide_circle_circle(Circle a, Circle b) {
    float dx = a.c.x - b.c.x;
    float dy = a.c.y - b.c.y;
    float r = a.r + b.r;
    return (dx * dx + dy * dy) <= r * r;
}

bool collide_circle_obb(Circle c, OBB b) {
    (void)c;
    (void)b;
    return false;
}

bool collide_obb_obb(OBB a, OBB b) {
    (void)a;
    (void)b;
    return false;
}
