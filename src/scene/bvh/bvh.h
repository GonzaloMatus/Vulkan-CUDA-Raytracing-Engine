#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../../vector/float3.h"
#include "object.h"
#include "node.h"
#include "../primitives.h"
#include "../dynamic_array.h"

DynamicArray(Object);

typedef struct {
    Node* nodes;
    size_t amount_nodes;
} BVH;

#ifndef __cplusplus
void bvh_create(BVH* bvh, const Object_list* object_list);
#endif