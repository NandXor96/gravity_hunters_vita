#pragma once
#include <stdbool.h>
#include "types.h"

/**
 * @brief Clamp a float value between min and max
 * @param v Value to clamp
 * @param lo Minimum value
 * @param hi Maximum value
 * @return Clamped value
 */
float clampf(float v, float lo, float hi);

/**
 * @brief Calculate the length of a 2D vector
 * @param v Vector
 * @return Length of the vector
 */
float lengthf(Vec2 v);

/**
 * @brief Add two 2D vectors
 * @param a First vector
 * @param b Second vector
 * @return Sum of the vectors
 */
Vec2 add2(Vec2 a, Vec2 b);

/**
 * @brief Subtract two 2D vectors
 * @param a First vector
 * @param b Second vector
 * @return Difference of the vectors
 */
Vec2 sub2(Vec2 a, Vec2 b);

/**
 * @brief Multiply a 2D vector by a scalar
 * @param a Vector
 * @param s Scalar multiplier
 * @return Scaled vector
 */
Vec2 mul2(Vec2 a, float s);

/**
 * @brief Normalize a 2D vector to unit length
 * @param v Vector to normalize
 * @return Normalized vector
 */
Vec2 norm2(Vec2 v);

typedef struct Circle {
    Vec2 c;
    float r;
} Circle;

typedef struct OBB {
    Vec2 c;
    Vec2 half;
    float angle;
} OBB;

/**
 * @brief Check collision between two circles
 * @param a First circle
 * @param b Second circle
 * @return true if circles collide, false otherwise
 */
bool collide_circle_circle(Circle a, Circle b);

/**
 * @brief Check collision between circle and oriented bounding box
 * @param c Circle
 * @param b Oriented bounding box
 * @return true if collision detected, false otherwise
 */
bool collide_circle_obb(Circle c, OBB b);

/**
 * @brief Check collision between two oriented bounding boxes
 * @param a First OBB
 * @param b Second OBB
 * @return true if collision detected, false otherwise
 */
bool collide_obb_obb(OBB a, OBB b);
