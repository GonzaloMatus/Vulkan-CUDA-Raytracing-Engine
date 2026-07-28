#pragma once
#include <stdint.h>
#include "../fixed_array.h"
#include "node_gpu.h"

FixedArray(NodeGPU);
FixedArray(size_t);

typedef struct {
    size_t_array object_indexes;
    NodeGPU_array nodes;
} BVHGPU;

bool bvh_gpu_create(BVHGPU* gpu, const BVH* cpu);
void bvh_gpu_destroy(BVHGPU* gpu);
