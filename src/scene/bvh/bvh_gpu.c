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

static void print_enclosure(AABB aabb) {
    printf("A = ");
    printf("%f, ", aabb.A.x);
    printf("%f, ", aabb.A.y);
    printf("%f\n", aabb.A.z);

    printf("B = ");
    printf("%f, ", aabb.B.x);
    printf("%f, ", aabb.B.y);
    printf("%f\n", aabb.B.z);
}

bool bvh_gpu_create(BVHGPU* gpu, const BVH* cpu) {
    size_t_array node_object_indexes;
    node_object_indexes.size = 0;
    for (size_t i = cpu->amount_nodes / 2; i < cpu->amount_nodes; i++) {
        node_object_indexes.size += cpu->nodes[i].object_indexes.size;
    }
    printf("Average objects per leaf: %f\n", (double) node_object_indexes.size / (double) ((cpu->amount_nodes + 1) / 2));
    node_object_indexes.data = malloc(node_object_indexes.size * sizeof(size_t));

    if (!compute_alloc((void**) &gpu->object_indexes.data, node_object_indexes.size * sizeof(size_t))) {
        free(node_object_indexes.data);
        return false;
    }

    if (!compute_alloc((void**) &gpu->nodes.data, cpu->amount_nodes * sizeof(NodeGPU))) {
        compute_free((void*) gpu->object_indexes.data);
        gpu->object_indexes.data = NULL;
        free(node_object_indexes.data);
        return false;
    }


    NodeGPU_array nodes;
    nodes.size = cpu->amount_nodes;
    nodes.data = malloc(nodes.size * sizeof(NodeGPU));

    size_t index = 0;
    for (size_t i = 0; i < cpu->amount_nodes; i++) {
        Node node = cpu->nodes[i];
        node_gpu_create(&nodes.data[i], index, node.object_indexes.size, node.aabb);
        if (i >= cpu->amount_nodes / 2) {
            for (size_t j = 0; j < node.object_indexes.size; j++) {
                node_object_indexes.data[index] = node.object_indexes.data[j];
                index++;
            }
        }
        
    }

    gpu->object_indexes.size = node_object_indexes.size;
    if (!compute_memcpy_to(gpu->object_indexes.data, node_object_indexes.data, gpu->object_indexes.size * sizeof(size_t))) {
        free(nodes.data);
        free(node_object_indexes.data);
        return false;
    }


    gpu->nodes.size = cpu->amount_nodes;
    if (!compute_memcpy_to(gpu->nodes.data, nodes.data, gpu->nodes.size * sizeof(NodeGPU))) {
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
}