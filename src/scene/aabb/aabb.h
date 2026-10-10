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
        const float3 safe_rd = make_float3(
            copysignf(fmaxf(fabsf(ray_direction.x), 1e-20f), ray_direction.x),
            copysignf(fmaxf(fabsf(ray_direction.y), 1e-20f), ray_direction.y),
            copysignf(fmaxf(fabsf(ray_direction.z), 1e-20f), ray_direction.z)
        );
        const float3 inv_rd = make_float3(
            1.0f / safe_rd.x,
            1.0f / safe_rd.y,
            1.0f / safe_rd.z
        );

        const float3 t0 = (A - ray_origin) * inv_rd;
        const float3 t1 = (B - ray_origin) * inv_rd;

        const float3 tmin = make_float3(
            fminf(t0.x, t1.x),
            fminf(t0.y, t1.y),
            fminf(t0.z, t1.z)
        );
        const float3 tmax = make_float3(
            fmaxf(t0.x, t1.x),
            fmaxf(t0.y, t1.y),
            fmaxf(t0.z, t1.z)
        );

        const float t_enter = fmaxf(fmaxf(tmin.x, tmin.y), tmin.z);
        const float t_exit  = fminf(fminf(tmax.x, tmax.y), tmax.z);

        // Sin intersección.
        if (t_enter > t_exit) return -FLT_MAX;

        // AABB completamente detrás del rayo.
        if (t_exit < 0.0f) return -FLT_MAX;

        // Rayo empieza dentro del AABB.
        if (t_enter < 0.0f) return 0.0f;

        // Rayo entra desde afuera.
        return t_enter;
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