#include "node_gpu.h"

void node_gpu_create(NodeGPU* node, size_t start_index, size_t amount_objects, AABB aabb) {
    node->start_index = start_index;
    node->amount_objects = amount_objects;
    node->aabb = aabb;
}