#include "material.h"
#include "../../vector/float3.h"


void material_create(Material* material, const float3 albedo, const float3 radiance) {
    material->albedo = albedo;
    material->radiance = radiance;
}
