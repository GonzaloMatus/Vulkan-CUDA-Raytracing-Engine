#include "sphere.h"
#include <stdio.h>

void sphere_create(Sphere* sphere, float x, float y, float z, float radius, int material_index) {
    sphere->center.x = x;
    sphere->center.y = y;
    sphere->center.z = z;
    sphere->radius = fabsf(radius);
    sphere->material_index = material_index;
}


AABB get_sphere_aabb(Sphere* sphere) {
    AABB aabb;
    aabb.A = subtract(sphere->center, (float3) {sphere->radius, sphere->radius, sphere->radius});
    aabb.B = add(sphere->center, (float3) {sphere->radius, sphere->radius, sphere->radius});
    return aabb;
}