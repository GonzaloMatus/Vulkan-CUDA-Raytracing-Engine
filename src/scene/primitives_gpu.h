#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "fixed_array.h"
#include "primitives.h"

FixedArray(Sphere);
FixedArray(Triangle);
FixedArray(Box);
FixedArray(Cylinder);

typedef struct {
    Sphere_array   spheres;
    Triangle_array triangles;
    Box_array      boxes;
    Cylinder_array cylinders;
} PrimitivesGPU;

bool primitives_gpu_create(PrimitivesGPU* gpu, const Primitives* cpu);
void primitives_gpu_update(PrimitivesGPU* gpu, const Primitives* cpu);
void primitives_gpu_destroy(PrimitivesGPU* gpu);