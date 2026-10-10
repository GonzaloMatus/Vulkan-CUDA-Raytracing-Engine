#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../../vector/float3.h"
#include "object.h"
#include "node.h"
#include "../dynamic_array.h"

DynamicArray(Object);

typedef struct {
    Object_list objects;
    Node*       nodes;
    size_t      amount_nodes;
} BVH;

#ifndef __cplusplus
void bvh_create(BVH* bvh);
void bvh_destroy(BVH* bvh);
#endif