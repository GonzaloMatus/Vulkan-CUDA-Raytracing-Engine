#include "primitives_gpu.h"
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

bool primitives_gpu_create(PrimitivesGPU* gpu, const Primitives* cpu) {
    if (!copy_list_to_gpu((void**)&gpu->spheres.data, &gpu->spheres.size,
                          cpu->sphere_list.data, sizeof(Sphere),
                          cpu->sphere_list.size)) return false;

    if (!copy_list_to_gpu((void**)&gpu->triangles.data, &gpu->triangles.size,
                          cpu->triangle_list.data, sizeof(Triangle),
                          cpu->triangle_list.size)) return false;

    if (!copy_list_to_gpu((void**)&gpu->boxes.data, &gpu->boxes.size,
                          cpu->box_list.data, sizeof(Box),
                          cpu->box_list.size)) return false;

    if (!copy_list_to_gpu((void**)&gpu->cylinders.data, &gpu->cylinders.size,
                          cpu->cylinder_list.data, sizeof(Cylinder),
                          cpu->cylinder_list.size)) return false;

    return true;
}

void primitives_gpu_update(PrimitivesGPU* gpu, const Primitives* cpu) {
    compute_memcpy_to((void*)gpu->spheres.data, cpu->sphere_list.data,
                      cpu->sphere_list.size * sizeof(Sphere));
    gpu->spheres.size = cpu->sphere_list.size;

    compute_memcpy_to((void*)gpu->triangles.data, cpu->triangle_list.data,
                      cpu->triangle_list.size * sizeof(Triangle));
    gpu->triangles.size = cpu->triangle_list.size;

    compute_memcpy_to((void*)gpu->boxes.data, cpu->box_list.data,
                      cpu->box_list.size * sizeof(Box));
    gpu->boxes.size = cpu->box_list.size;

    compute_memcpy_to((void*)gpu->cylinders.data, cpu->cylinder_list.data,
                      cpu->cylinder_list.size * sizeof(Cylinder));
    gpu->cylinders.size = cpu->cylinder_list.size;
}

void primitives_gpu_destroy(PrimitivesGPU* gpu) {
    compute_free((void*)gpu->spheres.data);   gpu->spheres.size   = 0;
    compute_free((void*)gpu->triangles.data); gpu->triangles.size = 0;
    compute_free((void*)gpu->boxes.data);     gpu->boxes.size     = 0;
    compute_free((void*)gpu->cylinders.data); gpu->cylinders.size = 0;
}