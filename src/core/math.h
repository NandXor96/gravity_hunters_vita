#pragma once
#include <stdbool.h>
#include "types.h"

float clampf(float v, float lo, float hi);
float lengthf(Vec2 v);
Vec2 add2(Vec2 a, Vec2 b);
Vec2 sub2(Vec2 a, Vec2 b);
Vec2 mul2(Vec2 a, float s);
Vec2 norm2(Vec2 v);

typedef struct Circle
{
    Vec2 c;
    float r;
} Circle;

typedef struct OBB
{
    Vec2 c;
    Vec2 half;
    float angle;
} OBB;

bool collide_circle_circle(Circle a, Circle b);
bool collide_circle_obb(Circle c, OBB b);
bool collide_obb_obb(OBB a, OBB b);
