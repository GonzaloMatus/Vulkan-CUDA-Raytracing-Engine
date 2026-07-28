#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../../vector/float3.h"
#include "../aabb/aabb.h"

typedef struct sphere {
    int material_index;
    float3 center;
    float radius;
#ifdef __cplusplus
    __device__ __forceinline__ float get_intersection_distance(const float3 ray_origin, const float3 ray_direction) const {
        const float3 oc = center - ray_origin;

        const float b = dot(oc, ray_direction);
        const float c = dot(oc, oc) - radius * radius;

        const float discriminant = b * b - c;
        if (discriminant < 0.0f)
            return -1.0f;

        const float sqrt_d = sqrtf(discriminant);

        const float t0 = b - sqrt_d;
        if (t0 > 0.0f)
            return t0;

        const float t1 = b + sqrt_d;
        if (t1 > 0.0f)
            return t1;

        return -1.0f;
    }

    __device__ __forceinline__ float3 compute_surface_normal(float3 intersection_point) const {
        return (intersection_point - center) / radius;
    }

#endif

} Sphere;

#ifndef __cplusplus
void sphere_create(Sphere* sphere, float x, float y, float z, float radius, int material_index);

AABB get_sphere_aabb(Sphere* sphere);
#endif