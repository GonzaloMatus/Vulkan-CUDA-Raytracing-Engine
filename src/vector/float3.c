#include "float3.h"
#include <math.h>
#include <stdbool.h>
#ifndef __cplusplus
float dot(float3 a, float3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

float length_squared(float3 v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

float length(float3 v) {
    return sqrtf(length_squared(v));
}

float3 add(float3 a, float3 b) {
    float3 result = {a.x + b.x, a.y + b.y, a.z + b.z};
    return result;
}

float3 subtract(float3 a, float3 b) {
    float3 result = {a.x - b.x, a.y - b.y, a.z - b.z};
    return result;
}

float3 multiply(float3 v, float scalar) {
    float3 result = {v.x * scalar, v.y * scalar, v.z * scalar};
    return result;
}

float3 divide(float3 v, float scalar) {
    float3 result = {v.x / scalar, v.y / scalar, v.z / scalar};
    return result;
}


float3 cross(float3 a, float3 b) {
    float3 c;
    c.x = a.y * b.z - a.z * b.y;
    c.y = a.z * b.x - a.x * b.z;
    c.z = a.x * b.y - a.y * b.x;
    return c;
}

float3 normalized(float3 v) {
    float len = length(v);
    if (len > 0.0f) {
        return divide(v, len);
    }
    return v;
}

void increase(float3* a, float3 b) {
    a->x += b.x;
    a->y += b.y;
    a->z += b.z;
}

void decrease(float3* a, float3 b) {
    a->x -= b.x;
    a->y -= b.y;
    a->z -= b.z;
}

void scale(float3* v, float scalar) {
    v->x *= scalar;
    v->y *= scalar; 
    v->z *= scalar;
}

void shrink(float3* v, float scalar) {
    v->x /= scalar;
    v->y /= scalar;
    v->z /= scalar;
}

void normalize(float3* v) {
    float len = length(*v);
    if (len > 0.0f) {
        shrink(v, len);
    }
}

bool is_zero_vector(float3 v) {
    return (v.x == 0.0f && v.y == 0.0f && v.z == 0.0f);
}

bool is_near_zero(float3 v, float epsilon) {
    return (fabsf(v.x) < epsilon && fabsf(v.y) < epsilon && fabsf(v.z) < epsilon);
}
#endif