#include <math.h>
#include "scene.h"
#include "primitives.h"

void set_materials(Scene* scene) {
    Material material;
    float3 albedo;
    float3 radiance;
    // White
    albedo = (float3) {0.85,0.85,0.82};
    radiance = (float3) {0.0f, 0.0f, 0.0f};
    material_create(&material, albedo, radiance);
    Material_list_push(&scene->material_list, material);

    // Red
    albedo = (float3) {0.85,0.20,0.20};
    radiance = (float3) {0.0f, 0.0f, 0.0f};
    material_create(&material, albedo, radiance);
    Material_list_push(&scene->material_list, material);

    // Green
    albedo = (float3) {0.25,0.75,0.35};
    radiance = (float3) {0.0f, 0.0f, 0.0f};
    material_create(&material, albedo, radiance);
    Material_list_push(&scene->material_list, material);

    // Blue
    albedo = (float3) {0.20,0.35,0.90};
    radiance = (float3) {0.0f, 0.0f, 0.0f};
    material_create(&material, albedo, radiance);
    Material_list_push(&scene->material_list, material);

    // Gold
    albedo = (float3) {0.90,0.75,0.25};
    radiance = (float3) {0.0f, 0.0f, 0.0f};
    material_create(&material, albedo, radiance);
    Material_list_push(&scene->material_list, material);

    // White
    albedo = (float3) {0.9f, 0.9f, 0.9f};
    radiance = (float3) {0.0f, 0.0f, 0.0f};
    material_create(&material, albedo, radiance);
    Material_list_push(&scene->material_list, material);

    // Yellowish with Yellow Light
    albedo = (float3) {0.9f, 0.8f, 0.1f};
    radiance = (float3) {32.0f, 16.0f, 8.0f};
    material_create(&material, albedo, radiance);
    Material_list_push(&scene->material_list, material);

    // Pink Light
    albedo = (float3) {0.5f, 0.1f, 0.5f};
    radiance = (float3) {0.0f, 0.0f, 0.0f};
    material_create(&material, albedo, radiance);
    Material_list_push(&scene->material_list, material);

    // Black
    albedo = (float3) {0.1f, 0.1f, 0.1f};
    radiance = (float3) {0.0f, 0.0f, 0.0f};
    material_create(&material, albedo, radiance);
    Material_list_push(&scene->material_list, material);
}

void object_list_fill(Object_list* object_list, Primitives* primitives) {
    Object object;
    for (size_t i = 0; i < primitives->sphere_list.size; i++) {
        object_create(&object, primitives, Spherical, i);
        Object_list_push(object_list, object);
    }

    for (size_t i = 0; i < primitives->triangle_list.size; i++) {
        object_create(&object, primitives, Triangular, i);
        Object_list_push(object_list, object);
    }

    for (size_t i = 0; i < primitives->box_list.size; i++) {
        object_create(&object, primitives, Boxed, i);
        Object_list_push(object_list, object);
    }

    for (size_t i = 0; i < primitives->cylinder_list.size; i++) {
        object_create(&object, primitives, Cylindrical, i);
        Object_list_push(object_list, object);
    }
}

bool scene_innit(Scene* scene) {
    scene->material_list = Material_list_create(16);
    set_materials(scene);

    primitives_innit(&scene->primitives);

    // Reservar capacidad para el millón de objetos (más margen)
    const size_t amount_objects = scene->primitives.sphere_list.size +
                                  scene->primitives.triangle_list.size +
                                  scene->primitives.box_list.size +
                                  scene->primitives.cylinder_list.size;
    scene->object_list = Object_list_create(amount_objects + 100);

    object_list_fill(&scene->object_list, &scene->primitives);

    // La construcción del BVH con 1M de objetos puede tardar unos segundos.
    bvh_create(&scene->bvh, &scene->object_list);

    return true;
}

void scene_destroy(Scene* scene) {
    Material_list_free(&scene->material_list);
    primitives_destroy(&scene->primitives);
    // No olvides liberar object_list y bvh si no lo haces ya
}