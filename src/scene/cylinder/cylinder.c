#include "cylinder.h"
#include "../../vector/float3.h"


void cylinder_create(Cylinder* cylinder, const float3 C, float radius, float height, int material_index) {
    cylinder->C = C;
    cylinder->radius = radius;
    cylinder->height = height;
    cylinder->material_index = material_index;
}

AABB get_cylinder_aabb(Cylinder* cylinder) {
    AABB aabb;
    const float3 A = (float3) {cylinder->C.x - cylinder->radius, cylinder->C.y - cylinder->radius, cylinder->C.z};
    const float3 B = (float3) {cylinder->C.x + cylinder->radius, cylinder->C.y + cylinder->radius, cylinder->C.z + cylinder->height};
    
    aabb.A = A;
    aabb.B = B;
    return aabb;
}