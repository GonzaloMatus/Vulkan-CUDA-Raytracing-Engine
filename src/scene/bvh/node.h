#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../../vector/float3.h"
#include "../aabb/aabb.h"
#include "object.h"
#include "../dynamic_array.h"

DynamicArray(size_t)

typedef struct node {
    size_t_list object_indexes;
    AABB aabb;
} Node;

#ifndef __cplusplus
void node_create(Node* node);
#endif