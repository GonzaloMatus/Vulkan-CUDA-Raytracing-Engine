#include "scene_gpu.h"
#include "scene.h"
#include "../cuda_api/cuda_api.h"

static bool copy_list_to_gpu(void **gpu_ptr, size_t *count, const void *cpu_data, size_t elem_size, size_t elem_count) {
    size_t total_size = elem_count * elem_size;
    if (!cuda_alloc(gpu_ptr, total_size)) {
        return false;
    }
    if (!cuda_memcpy_to_device(*gpu_ptr, cpu_data, total_size)) {
        return false;
    }
    *count = elem_count;
    return true;
}

bool scene_gpu_create(SceneGPU* gpu, const Scene* cpu) {
    if (!bvh_gpu_create(&gpu->bvh, &cpu->bvh)) {
        return false;
    }
    
    if (!copy_list_to_gpu((void**)&gpu->objects.data, &gpu->objects.size, cpu->object_list.data, sizeof(Object), cpu->object_list.size))
        return false;

    if (!copy_list_to_gpu((void**)&gpu->materials.data, &gpu->materials.size, cpu->material_list.data, sizeof(Material), cpu->material_list.size))
        return false;

    if (!copy_list_to_gpu((void**)&gpu->spheres.data, &gpu->spheres.size, cpu->primitives.sphere_list.data, sizeof(Sphere), cpu->primitives.sphere_list.size))
        return false;

    if (!copy_list_to_gpu((void**)&gpu->triangles.data, &gpu->triangles.size, cpu->primitives.triangle_list.data, sizeof(Triangle), cpu->primitives.triangle_list.size))
        return false;

    if (!copy_list_to_gpu((void**)&gpu->boxes.data, &gpu->boxes.size, cpu->primitives.box_list.data, sizeof(Box), cpu->primitives.box_list.size))
        return false;
    
    if (!copy_list_to_gpu((void**)&gpu->cylinders.data, &gpu->cylinders.size, cpu->primitives.cylinder_list.data, sizeof(Cylinder), cpu->primitives.cylinder_list.size))
        return false;

    return true;
}

void scene_gpu_destroy(SceneGPU* gpu) {
    bvh_gpu_destroy(&gpu->bvh);
    
    cuda_free((void*)gpu->materials.data);
    gpu->materials.size = 0;

    cuda_free((void*)gpu->spheres.data);
    gpu->spheres.size = 0;

    cuda_free((void*)gpu->triangles.data);
    gpu->triangles.size = 0;

    cuda_free((void*)gpu->boxes.data);
    gpu->boxes.size = 0;

    cuda_free((void*)gpu->cylinders.data);
    gpu->cylinders.size = 0;
}

void scene_gpu_update(SceneGPU* gpu, const Scene* cpu) {
    const size_t material_size = cpu->material_list.size * sizeof(Material);
    cuda_memcpy_to_device((void*)gpu->materials.data, cpu->material_list.data, material_size);
    gpu->materials.size = cpu->material_list.size;

    const size_t sphere_size = cpu->primitives.sphere_list.size * sizeof(Sphere);
    cuda_memcpy_to_device((void*)gpu->spheres.data, cpu->primitives.sphere_list.data, sphere_size);
    gpu->spheres.size = cpu->primitives.sphere_list.size;

    const size_t triangle_size = cpu->primitives.triangle_list.size * sizeof(Triangle);
    cuda_memcpy_to_device((void*)gpu->triangles.data, cpu->primitives.triangle_list.data, triangle_size);
    gpu->triangles.size = cpu->primitives.triangle_list.size;

    const size_t box_size = cpu->primitives.box_list.size * sizeof(Box);
    cuda_memcpy_to_device((void*)gpu->boxes.data, cpu->primitives.box_list.data, box_size);
    gpu->boxes.size = cpu->primitives.box_list.size;

    const size_t cylinder_size = cpu->primitives.cylinder_list.size * sizeof(Cylinder);
    cuda_memcpy_to_device((void*)gpu->cylinders.data, cpu->primitives.cylinder_list.data, cylinder_size);
    gpu->cylinders.size = cpu->primitives.cylinder_list.size;
}