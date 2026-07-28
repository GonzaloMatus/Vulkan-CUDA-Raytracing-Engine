#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../../vector/float3.h"
#include "../aabb/aabb.h"

typedef struct cylinder {
    int material_index;
    float3 C;
    float radius;
    float height;
#ifdef __cplusplus
    __device__ __forceinline__ float get_intersection_distance(const float3 ray_origin, const float3 ray_direction) const {

        const float3 Q = C - ray_origin;
        float closest_distance = FLT_MAX;
        const float b = ray_direction.x * Q.x + ray_direction.y * Q.y;
        const float a = (1 - ray_direction.z * ray_direction.z);
        const float radius_squared =  radius * radius;
        const float c = Q.x * Q.x + Q.y * Q.y - radius_squared;
        const float determinant = (b * b) - (a * c);

        if (determinant >= 0) {
            const float sqrt_d = sqrtf(determinant);
            const float t1 = (b - sqrt_d) / a;
            const float z1 = (ray_direction.z * t1) - Q.z;
            if (t1 > 0.0f && 0 <= z1 && z1 <= height) {
                closest_distance = t1;
            } else {
                const float t2 = (b + sqrt_d) / a;
                const float z2 = (ray_direction.z * t2) - Q.z;
                if (t2 > 0.0f && 0 <= z2 && z2 <= height)
                    closest_distance = t2;
            }
        }

        const float t3 = Q.z / ray_direction.z;
        const float x3 = ray_direction.x * t3 - Q.x;
        const float y3 = ray_direction.y * t3 - Q.y;
        if (0.0 < t3 && x3*x3 + y3*y3 <= radius_squared) {
            closest_distance = min(closest_distance, t3);
        }
        
        const float t4 = (height + Q.z) / ray_direction.z;
        const float x4 = ray_direction.x * t4 - Q.x;
        const float y4 = ray_direction.y * t4 - Q.y;
        if (0.0 < t4 && x4*x4 + y4*y4 <= radius_squared) {
            closest_distance = min(closest_distance, t4);
        }

        if (closest_distance == FLT_MAX) {
            return -1.0;
        }
        return closest_distance;
    }

    __device__ __forceinline__ float3 compute_surface_normal(const float3 p) const {
        const float eps = 1e-4f;
        if (fabsf(C.z + height - p.z) < eps) return make_float3(0.0f, 0.0f, 1.0f);
        if (fabsf(C.z - p.z) < eps) return make_float3(0.0f, 0.0f, -1.0f);

        const float x = (p.x - C.x) / radius;
        const float y = (p.y - C.y) / radius;
        return make_float3(x, y, 0.0f);
    }

#endif

} Cylinder;

#ifndef __cplusplus
void cylinder_create(Cylinder* cylinder, float3 C, float radius, float height, int material_index);

AABB get_cylinder_aabb(Cylinder* cylinder);
#endif