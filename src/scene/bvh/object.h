#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../../vector/float3.h"
#include "../aabb/aabb.h"
#include "../object_types.h"
#include "../primitives.h"

#ifdef __cplusplus
#include "../primitives_gpu.h"
#endif

typedef struct {
    ObjectType type;
    size_t index;
    AABB aabb;
#ifdef __cplusplus
    __device__ __forceinline__ float get_intersection_distance(
        const float3 ray_origen, const float3 ray_direction,
        const PrimitivesGPU& prims) const
    {
        switch (type) {
            case Spherical:
                return prims.spheres.data[index].get_intersection_distance(ray_origen, ray_direction);
            case Triangular:
                return prims.triangles.data[index].get_intersection_distance(ray_origen, ray_direction);
            case Boxed:
                return prims.boxes.data[index].get_intersection_distance(ray_origen, ray_direction);
            case Cylindrical:
                return prims.cylinders.data[index].get_intersection_distance(ray_origen, ray_direction);
        }
        return -1.0f;
    }
#endif
} Object;

#ifndef __cplusplus
void object_create(Object* object, const Primitives* scene, ObjectType type, size_t index);
#endif