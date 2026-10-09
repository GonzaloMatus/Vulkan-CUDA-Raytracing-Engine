#include "scene_gpu.h"
#include "scene.h"
#include "../compute/compute.h"

static bool copy_list_to_gpu(void** gpu_ptr, size_t* count,
                             const void* cpu_data, size_t elem_size, size_t elem_count) {
    if (elem_count == 0) {
        *gpu_ptr = NULL;
        *count   = 0;
        return true;
    }
    size_t total_size = elem_count * elem_size;
    if (!compute_alloc(gpu_ptr, total_size)) return false;
    if (!compute_memcpy_to(*gpu_ptr, cpu_data, total_size)) return false;
    *count = elem_count;
    return true;
}

bool scene_gpu_create(SceneGPU* gpu, const Scene* cpu) {
    if (!bvh_gpu_create(&gpu->bvh, &cpu->bvh)) return false;

    if (!copy_list_to_gpu((void**)&gpu->objects.data, &gpu->objects.size,
                          cpu->object_list.data, sizeof(Object), cpu->object_list.size)) return false;
    if (!copy_list_to_gpu((void**)&gpu->materials.data, &gpu->materials.size,
                          cpu->material_list.data, sizeof(Material), cpu->material_list.size)) return false;
    if (!copy_list_to_gpu((void**)&gpu->spheres.data, &gpu->spheres.size,
                          cpu->primitives.sphere_list.data, sizeof(Sphere), cpu->primitives.sphere_list.size)) return false;
    if (!copy_list_to_gpu((void**)&gpu->triangles.data, &gpu->triangles.size,
                          cpu->primitives.triangle_list.data, sizeof(Triangle), cpu->primitives.triangle_list.size)) return false;
    if (!copy_list_to_gpu((void**)&gpu->boxes.data, &gpu->boxes.size,
                          cpu->primitives.box_list.data, sizeof(Box), cpu->primitives.box_list.size)) return false;
    if (!copy_list_to_gpu((void**)&gpu->cylinders.data, &gpu->cylinders.size,
                          cpu->primitives.cylinder_list.data, sizeof(Cylinder), cpu->primitives.cylinder_list.size)) return false;

    gpu->is_dirty = true;
    return true;
}

void scene_gpu_destroy(SceneGPU* gpu) {
    bvh_gpu_destroy(&gpu->bvh);

    compute_free((void*)gpu->materials.data); gpu->materials.size = 0;
    compute_free((void*)gpu->spheres.data);   gpu->spheres.size   = 0;
    compute_free((void*)gpu->triangles.data); gpu->triangles.size = 0;
    compute_free((void*)gpu->boxes.data);     gpu->boxes.size     = 0;
    compute_free((void*)gpu->cylinders.data); gpu->cylinders.size = 0;
}

void scene_gpu_update(SceneGPU* gpu, const Scene* cpu) {
    compute_memcpy_to((void*)gpu->materials.data, cpu->material_list.data,
                      cpu->material_list.size * sizeof(Material));
    gpu->materials.size = cpu->material_list.size;

    compute_memcpy_to((void*)gpu->spheres.data, cpu->primitives.sphere_list.data,
                      cpu->primitives.sphere_list.size * sizeof(Sphere));
    gpu->spheres.size = cpu->primitives.sphere_list.size;

    compute_memcpy_to((void*)gpu->triangles.data, cpu->primitives.triangle_list.data,
                      cpu->primitives.triangle_list.size * sizeof(Triangle));
    gpu->triangles.size = cpu->primitives.triangle_list.size;

    compute_memcpy_to((void*)gpu->boxes.data, cpu->primitives.box_list.data,
                      cpu->primitives.box_list.size * sizeof(Box));
    gpu->boxes.size = cpu->primitives.box_list.size;

    compute_memcpy_to((void*)gpu->cylinders.data, cpu->primitives.cylinder_list.data,
                      cpu->primitives.cylinder_list.size * sizeof(Cylinder));
    gpu->cylinders.size = cpu->primitives.cylinder_list.size;
}