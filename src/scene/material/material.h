#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../../vector/float3.h"

typedef struct {
    float3 albedo;
    float3 radiance;
} Material;

#ifndef __cplusplus
void material_create(Material* material, const float3 albedo, const float3 radiance);
#endif