#pragma once
#include <stdint.h>
#include "scene.h"
#include "../vector/float3.h"
#include "sphere/sphere.h"
#include "fixed_array.h"
#include "bvh/bvh_gpu.h"

FixedArray(Material);
FixedArray(Box);
FixedArray(Sphere);
FixedArray(Triangle);
FixedArray(Cylinder);
FixedArray(Object);

typedef struct {
    Material_array materials;
    Object_array objects;
    BVHGPU bvh;

    Sphere_array spheres;
    Triangle_array triangles;
    Box_array boxes;
    Cylinder_array cylinders;
} SceneGPU;

bool scene_gpu_create(SceneGPU* gpu, const Scene* cpu);
void scene_gpu_update(SceneGPU* gpu, const Scene* cpu);
void scene_gpu_destroy(SceneGPU* gpu);
