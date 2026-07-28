#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "../../vector/float3.h"
#include "../aabb/aabb.h"

typedef struct triangle {
    int material_index;
    float3 A;
    float3 E1;
    float3 E2;
    float3 normal;

#ifdef __cplusplus

    __device__ __forceinline__ float get_intersection_distance(const float3 ray_origin, const float3 ray_direction) const {
        const float3 P = cross(ray_direction, E2);
        const float det = dot(E1, P);

        if (fabsf(det) < 1e-8f) return -1.0f;

        const float inv_det = 1.0f / det;
        const float3 T = ray_origin - A;

        const float u = dot(T, P) * inv_det;
        if (u < 0.0f || u > 1.0f) return -1.0f;

        const float3 Q = cross(T, E1);
        const float v = dot(ray_direction, Q) * inv_det;
        if (v < 0.0f || (u + v) > 1.0f) return -1.0f;

        const float t = dot(E2, Q) * inv_det;
        return (t > 0.0f) ? t : -1.0f;
    }

#endif
} Triangle;


#ifndef __cplusplus
void triangle_create(Triangle* triangle, float3 A, float3 B, float3 C, int material_index);

AABB get_triangle_aabb(Triangle* triangle);
#endif