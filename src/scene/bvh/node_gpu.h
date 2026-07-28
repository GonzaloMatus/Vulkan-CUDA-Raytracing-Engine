#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../../vector/float3.h"
#include "../aabb/aabb.h"
#include "object.h"


typedef struct node_gpu {
    size_t start_index;
    size_t amount_objects;
    AABB aabb;
} NodeGPU;

#ifndef __cplusplus
void node_gpu_create(NodeGPU* node, size_t start_index, size_t amount_objects, AABB aabb);
#endif