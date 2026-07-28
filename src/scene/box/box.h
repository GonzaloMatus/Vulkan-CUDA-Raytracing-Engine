#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <float.h>
#include <math.h>
#include "../../vector/float3.h"
#include "../aabb/aabb.h"

typedef struct box {
    int material_index;
    float3 A;
    float3 B;

#ifdef __cplusplus

    __device__ __forceinline__ float get_intersection_distance(const float3 ray_origin, const float3 ray_direction) const {
        const float min_x = fminf(A.x, B.x);
        const float min_y = fminf(A.y, B.y);
        const float min_z = fminf(A.z, B.z);

        const float max_x = fmaxf(A.x, B.x);
        const float max_y = fmaxf(A.y, B.y);
        const float max_z = fmaxf(A.z, B.z);

        float t_low = -FLT_MAX;
        float t_high = FLT_MAX;

        // ----- X slab -----
        const float tx_low  = ((ray_direction.x >= 0.0f ? min_x : max_x) - ray_origin.x) / ray_direction.x;
        const float tx_high = ((ray_direction.x >= 0.0f ? max_x : min_x) - ray_origin.x) / ray_direction.x;
        t_low = fmaxf(t_low, tx_low);
        t_high = fminf(t_high, tx_high);
        
        // ----- Y slab -----
        const float ty_low  = ((ray_direction.y >= 0.0f ? min_y : max_y) - ray_origin.y) / ray_direction.y;
        const float ty_high = ((ray_direction.y >= 0.0f ? max_y : min_y) - ray_origin.y) / ray_direction.y;
        t_low = fmaxf(t_low, ty_low);
        t_high = fminf(t_high, ty_high);

        // ----- Z slab -----
        const float tz_low  = ((ray_direction.z >= 0.0f ? min_z : max_z) - ray_origin.z) / ray_direction.z;
        const float tz_high = ((ray_direction.z >= 0.0f ? max_z : min_z) - ray_origin.z) / ray_direction.z;
        t_low = fmaxf(t_low, tz_low);
        t_high = fminf(t_high, tz_high);

        if (t_low > t_high)
            return -1.0f;

        if (t_low >= 0.0f)
            return t_low;

        if (t_high >= 0.0f)
            return t_high;

        return -1.0f;
    }

    __device__ __forceinline__ float3 compute_surface_normal(float3 p) const {

        const float min_x = fminf(A.x, B.x);
        const float min_y = fminf(A.y, B.y);
        const float min_z = fminf(A.z, B.z);

        const float max_x = fmaxf(A.x, B.x);
        const float max_y = fmaxf(A.y, B.y);
        const float max_z = fmaxf(A.z, B.z);

        const float eps = 1e-4f;

        if (fabsf(p.x - min_x) < eps) return make_float3(-1.f, 0.f, 0.f);
        if (fabsf(p.x - max_x) < eps) return make_float3( 1.f, 0.f, 0.f);

        if (fabsf(p.y - min_y) < eps) return make_float3(0.f, -1.f, 0.f);
        if (fabsf(p.y - max_y) < eps) return make_float3(0.f,  1.f, 0.f);

        if (fabsf(p.z - min_z) < eps) return make_float3(0.f, 0.f, -1.f);
        if (fabsf(p.z - max_z) < eps) return make_float3(0.f, 0.f,  1.f);

        // fallback (no debería ocurrir)
        return make_float3(0.0f, 0.0f, 0.0f);
    }


#endif
} Box;


#ifndef __cplusplus
void box_create(Box* box, float3 A, float3 B, int material_index);

AABB get_box_aabb(Box* box);

#endif