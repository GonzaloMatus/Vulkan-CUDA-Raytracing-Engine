#pragma once
#include <stdint.h>
#include "material/material.h"
#include "primitives.h"
#include "bvh/bvh.h"

DynamicArray(Material);

typedef struct {
    Material_list material_list;
    Primitives primitives;
    Object_list object_list;
    BVH bvh;
} Scene;

bool scene_innit(Scene* scene);

void scene_destroy(Scene* scene);