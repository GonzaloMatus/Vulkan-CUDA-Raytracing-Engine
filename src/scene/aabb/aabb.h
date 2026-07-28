#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <float.h>
#include <math.h>
#include "../../vector/float3.h"

typedef struct aabb {
    float3 A;
    float3 B;

#ifdef __cplusplus

    __device__ __forceinline__ float get_intersection_distance(const float3 ray_origin, const float3 ray_direction) const {

        float t_low = -FLT_MAX;
        float t_high = FLT_MAX;

        // ----- X slab -----
        const float tx_low  = ((ray_direction.x >= 0.0f ? A.x : B.x) - ray_origin.x) / ray_direction.x;
        const float tx_high = ((ray_direction.x >= 0.0f ? B.x : A.x) - ray_origin.x) / ray_direction.x;
        t_low = fmaxf(t_low, tx_low);
        t_high = fminf(t_high, tx_high);
        
        // ----- Y slab -----
        const float ty_low  = ((ray_direction.y >= 0.0f ? A.y : B.y) - ray_origin.y) / ray_direction.y;
        const float ty_high = ((ray_direction.y >= 0.0f ? B.y : A.y) - ray_origin.y) / ray_direction.y;
        t_low = fmaxf(t_low, ty_low);
        t_high = fminf(t_high, ty_high);

        // ----- Z slab -----
        const float tz_low  = ((ray_direction.z >= 0.0f ? A.z : B.z) - ray_origin.z) / ray_direction.z;
        const float tz_high = ((ray_direction.z >= 0.0f ? B.z : A.z) - ray_origin.z) / ray_direction.z;
        t_low = fmaxf(t_low, tz_low);
        t_high = fminf(t_high, tz_high);

        if (t_low > t_high)
            return -FLT_MAX;

        if (t_low >= 0.0f)
            return t_low;

        return t_high;
    }

    __device__ __forceinline__ float3 compute_surface_normal(float3 p) const {
        const float eps = 1e-4f;

        if (fabsf(p.x - A.x) < eps) return make_float3(-1.f, 0.f, 0.f);
        if (fabsf(p.x - B.x) < eps) return make_float3( 1.f, 0.f, 0.f);

        if (fabsf(p.y - A.y) < eps) return make_float3(0.f, -1.f, 0.f);
        if (fabsf(p.y - B.y) < eps) return make_float3(0.f,  1.f, 0.f);

        if (fabsf(p.z - A.z) < eps) return make_float3(0.f, 0.f, -1.f);
        if (fabsf(p.z - B.z) < eps) return make_float3(0.f, 0.f,  1.f);

        // fallback (no debería ocurrir)
        return make_float3(0.0f, 0.0f, 0.0f);
    }

    __device__ __forceinline__ bool has_inside(float3 p) const {
        return (A.x <= p.x && p.x <= B.x && A.y <= p.y && p.y <= B.y && A.z <= p.z && p.z <= B.z);
    }

#endif
} AABB;


#ifndef __cplusplus
void aabb_create(AABB* aabb, float3 A, float3 B);

AABB aabb_intersect(AABB aabb1, AABB aabb2);
#endif