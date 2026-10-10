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

    if (!copy_list_to_gpu((void**)&gpu->materials.data, &gpu->materials.size,
                          cpu->material_list.data, sizeof(Material),
                          cpu->material_list.size)) return false;

    if (!primitives_gpu_create(&gpu->primitives, &cpu->primitives)) return false;

    gpu->is_dirty = true;
    return true;
}

void scene_gpu_destroy(SceneGPU* gpu) {
    bvh_gpu_destroy(&gpu->bvh);
    compute_free((void*)gpu->materials.data); gpu->materials.size = 0;
    primitives_gpu_destroy(&gpu->primitives);
}

void scene_gpu_update(SceneGPU* gpu, const Scene* cpu) {
    compute_memcpy_to((void*)gpu->materials.data, cpu->material_list.data,
                      cpu->material_list.size * sizeof(Material));
    gpu->materials.size = cpu->material_list.size;

    primitives_gpu_update(&gpu->primitives, &cpu->primitives);
}