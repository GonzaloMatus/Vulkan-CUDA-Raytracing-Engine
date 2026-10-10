#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <float.h>
#include <math.h>
#include "../../vector/float3.h"
#include "bvh.h"
#include "../primitives.h"
#include "bvh_gpu.h"
#include "../../compute/compute.h"

bool bvh_gpu_create(BVHGPU* gpu, const BVH* cpu) {
    // --- Objetos ---
    if (cpu->objects.size > 0) {
        if (!compute_alloc((void**)&gpu->objects.data,
                           cpu->objects.size * sizeof(Object))) {
            return false;
        }
        if (!compute_memcpy_to(gpu->objects.data, cpu->objects.data,
                               cpu->objects.size * sizeof(Object))) {
            return false;
        }
        gpu->objects.size = cpu->objects.size;
    } else {
        gpu->objects.data = NULL;
        gpu->objects.size = 0;
    }

    // --- Nodos e índices de objetos ---
    size_t total_objects = 0;
    for (size_t i = 0; i < cpu->amount_nodes; i++) {
        total_objects += cpu->nodes[i].object_indexes.size;
    }
    printf("BVH GPU: %zu nodes, %zu objects in leaves\n",
           cpu->amount_nodes, total_objects);

    if (!compute_alloc((void**)&gpu->object_indexes.data,
                       total_objects * sizeof(size_t))) {
        return false;
    }

    if (!compute_alloc((void**)&gpu->nodes.data,
                       cpu->amount_nodes * sizeof(NodeGPU))) {
        return false;
    }

    size_t_array node_object_indexes;
    node_object_indexes.size = total_objects;
    node_object_indexes.data = malloc(total_objects * sizeof(size_t));

    NodeGPU_array nodes;
    nodes.size = cpu->amount_nodes;
    nodes.data = malloc(nodes.size * sizeof(NodeGPU));

    size_t index = 0;
    for (size_t i = 0; i < cpu->amount_nodes; i++) {
        const Node* node = &cpu->nodes[i];
        const size_t count = node->object_indexes.size;

        node_gpu_create(&nodes.data[i], index, count, node->aabb);

        for (size_t j = 0; j < count; j++) {
            node_object_indexes.data[index] = node->object_indexes.data[j];
            index++;
        }
    }

    gpu->object_indexes.size = total_objects;
    if (!compute_memcpy_to(gpu->object_indexes.data, node_object_indexes.data,
                           total_objects * sizeof(size_t))) {
        free(nodes.data);
        free(node_object_indexes.data);
        return false;
    }

    gpu->nodes.size = cpu->amount_nodes;
    if (!compute_memcpy_to(gpu->nodes.data, nodes.data,
                           gpu->nodes.size * sizeof(NodeGPU))) {
        free(nodes.data);
        free(node_object_indexes.data);
        return false;
    }

    free(nodes.data);
    free(node_object_indexes.data);
    return true;
}

void bvh_gpu_destroy(BVHGPU* gpu) {
    compute_free((void*)gpu->nodes.data);
    gpu->nodes.size = 0;
    compute_free((void*)gpu->object_indexes.data);
    gpu->object_indexes.size = 0;
    compute_free((void*)gpu->objects.data);
    gpu->objects.size = 0;
}