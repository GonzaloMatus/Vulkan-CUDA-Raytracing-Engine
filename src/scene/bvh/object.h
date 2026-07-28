#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../../vector/float3.h"
#include "../aabb/aabb.h"
#include "../object_types.h"
#include "../primitives.h"



typedef struct {
    ObjectType type;
    size_t index;
    AABB aabb;
} Object;


#ifndef __cplusplus
void object_create(Object* object, const Primitives* scene, ObjectType type, size_t index);
#endif