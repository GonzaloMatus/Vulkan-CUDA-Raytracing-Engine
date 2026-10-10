#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "scene.h"
#include "../vector/float3.h"
#include "fixed_array.h"
#include "material/material.h"
#include "primitives_gpu.h"
#include "bvh/bvh_gpu.h"

FixedArray(Material);

typedef struct {
    Material_array materials;
    PrimitivesGPU  primitives;
    BVHGPU         bvh;
    bool           is_dirty;
} SceneGPU;

bool scene_gpu_create(SceneGPU* gpu, const Scene* cpu);
void scene_gpu_update(SceneGPU* gpu, const Scene* cpu);
void scene_gpu_destroy(SceneGPU* gpu);