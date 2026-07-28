#include "triangle.h"
#include "../../vector/float3.h"

void triangle_create(Triangle* triangle, float3 A, float3 B, float3 C, int material_index) {
    triangle->A = A;
    triangle->E1 = subtract(B, A);
    triangle->E2 = subtract(C, A);
    triangle->normal = cross(triangle->E1, triangle->E2);
    normalize(&triangle->normal);
    triangle->material_index = material_index;
}


AABB get_triangle_aabb(Triangle* triangle) {
    AABB aabb;
    const float3 A = triangle->A;
    const float3 B = add(triangle->A, triangle->E1);
    const float3 C = add(triangle->A, triangle->E2);
    
    aabb.A.x = fminf(fminf(A.x, B.x), C.x);
    aabb.A.y = fminf(fminf(A.y, B.y), C.y);
    aabb.A.z = fminf(fminf(A.z, B.z), C.z);

    aabb.B.x = fmaxf(fmaxf(A.x, B.x), C.x);
    aabb.B.y = fmaxf(fmaxf(A.y, B.y), C.y);
    aabb.B.z = fmaxf(fmaxf(A.z, B.z), C.z);
    return aabb;
}