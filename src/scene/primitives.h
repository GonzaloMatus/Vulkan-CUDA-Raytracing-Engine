#pragma once
#include "dynamic_array.h"
#include <stdbool.h>

#include "sphere/sphere.h"
#include "triangle/triangle.h"
#include "box/box.h"
#include "cylinder/cylinder.h"

DynamicArray(Box);
DynamicArray(Sphere);
DynamicArray(Triangle);
DynamicArray(Cylinder);


typedef struct {
    Sphere_list sphere_list;
    Triangle_list triangle_list;
    Box_list box_list;
    Cylinder_list cylinder_list;
} Primitives;

bool primitives_innit(Primitives* scene);

void primitives_destroy(Primitives* scene);