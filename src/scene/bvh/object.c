#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "../../vector/float3.h"
#include "object.h"
#include "../aabb/aabb.h"
#include "../object_types.h"

static void print_enclosure(AABB aabb) {
    printf("A = ");
    printf("%f ", aabb.A.x);
    printf("%f ", aabb.A.y);
    printf("%f\n", aabb.A.z);

    printf("B = ");
    printf("%f ", aabb.B.x);
    printf("%f ", aabb.B.y);
    printf("%f\n", aabb.B.z);
}

void object_create(Object* object, const Primitives* primitives, ObjectType type, size_t index) {
    object->index = index;
    object->type = type;
    //printf("Object:  Type = %i \n", type);
    switch (type) {
        case Spherical: {
            Sphere sphere = primitives->sphere_list.data[index];
            object->aabb = get_sphere_aabb(&sphere);
            break;
        }
        case Triangular: {
            Triangle triangle = primitives->triangle_list.data[index];
            object->aabb = get_triangle_aabb(&triangle);
            break;
        }
        case Boxed: {
            Box box = primitives->box_list.data[index];
            object->aabb = get_box_aabb(&box);
            break;
        }
        case Cylindrical: {
            Cylinder cylinder = primitives->cylinder_list.data[index];
            object->aabb = get_cylinder_aabb(&cylinder);
            break;
        }
    }
}