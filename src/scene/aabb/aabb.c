#include "aabb.h"
#include "../../vector/float3.h"

void aabb_create(AABB* aabb, float3 A, float3 B) {
    aabb->A = (float3) {fminf(A.x, B.x), fminf(A.y, B.y), fminf(A.z, B.z)};
    aabb->B = (float3) {fmaxf(A.x, B.x), fmaxf(A.y, B.y), fmaxf(A.z, B.z)};
}


AABB aabb_intersect(AABB aabb1, AABB aabb2) {
    AABB result;

    result.A.x = fmaxf(aabb1.A.x, aabb2.A.x);
    result.A.y = fmaxf(aabb1.A.y, aabb2.A.y);
    result.A.z = fmaxf(aabb1.A.z, aabb2.A.z);

    result.B.x = fminf(aabb1.B.x, aabb2.B.x);
    result.B.y = fminf(aabb1.B.y, aabb2.B.y);
    result.B.z = fminf(aabb1.B.z, aabb2.B.z);
    return result;
}