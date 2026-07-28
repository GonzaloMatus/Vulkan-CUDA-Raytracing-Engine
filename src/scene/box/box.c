#include "box.h"
#include "../../vector/float3.h"

void box_create(Box* box, float3 A, float3 B, int material_index) {
    box->A = A;
    box->B = B;
    box->material_index = material_index;
}

AABB get_box_aabb(Box* box) {
    AABB aabb;
    aabb.A = box->A;
    aabb.B = box->B;
    return aabb;
}