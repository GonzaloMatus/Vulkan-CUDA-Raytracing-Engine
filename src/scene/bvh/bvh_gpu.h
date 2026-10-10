#pragma once
#include <stdint.h>
#include "../fixed_array.h"
#include "node_gpu.h"
#include "object.h"

FixedArray(NodeGPU);
FixedArray(size_t);
FixedArray(Object);

typedef struct {
    Object_array  objects;       // <-- movido aquí
    size_t_array  object_indexes;
    NodeGPU_array nodes;
} BVHGPU;

bool bvh_gpu_create(BVHGPU* gpu, const BVH* cpu);
void bvh_gpu_destroy(BVHGPU* gpu);