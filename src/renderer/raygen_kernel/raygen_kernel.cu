#include <cuda_runtime.h>
#include <cuda.h>
#include <stdio.h>
#include <cuda_fp16.h>
#include <math.h>
#include <float.h>

#include "../../vector/float3.h"
#include "../../camera/camera.h"
#include "../../scene/scene_gpu.h"
#include "../cuda_renderer/cuda_renderer.h"
#include "../../scene/sphere/sphere.h"
#include "../../scene/cylinder/cylinder.h"
#include "../../scene/material/material.h"
#include "../../scene/box/box.h"
#include "../../scene/object_types.h"


#define CUDA_CHECK(x) do { \
    cudaError_t err = x; \
    if (err != cudaSuccess) { \
        printf("CUDA error %s:%d: %s\n", __FILE__, __LINE__, cudaGetErrorString(err)); \
    } \
} while (0)

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    __half x, y, z, w;
} half4;

typedef struct IntersectionObjectData {
    float closest_object_distance;
    uint32_t closest_object_index;
    Material material;
    float3 normal;
    float3 intersection;
} IntersectionObjectData;

__constant__ SceneGPU scene;


__device__ __forceinline__ uint32_t wang_hash(uint32_t& seed) {
    seed = (seed ^ 61u) ^ (seed >> 16);
    seed *= 9u;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2du;
    seed = seed ^ (seed >> 15);
    return seed;
}

__device__ __forceinline__ float rand01(uint32_t& seed) {
    return (wang_hash(seed) & 0x00FFFFFF) * (1.0f / 16777216.0f);
}

//
// -----------------------------------------------------------------------------
// Ray direction
// -----------------------------------------------------------------------------
//

__device__ static float3 compute_ray_direction(Camera camera, float u, float v) {
    const float u_offset = u - 0.5f;
    const float v_offset = v - 0.5f;

    const float cos_w = cosf(camera.w_fov * u_offset);
    const float sin_w = sinf(camera.w_fov * u_offset);

    const float3 image_point1 =
        (camera.forward * (camera.width * cos_w)) +
        (camera.right   * (camera.width * sin_w)) +
        (camera.up      * (camera.height * camera.w_fov * v_offset));

    const float cos_h = cosf(camera.h_fov * v_offset);
    const float sin_h = sinf(camera.h_fov * v_offset);

    const float3 image_point2 =
        (camera.right   * (camera.width * camera.h_fov * u_offset)) +
        (camera.forward * (camera.height * cos_h)) +
        (camera.up      * (camera.height * sin_h));

    const float3 image_point = (image_point1 + image_point2) * 0.5f;

    const float inv_len = rsqrtf(length_squared(image_point));
    return image_point * inv_len;
}

//
// -----------------------------------------------------------------------------
// Scene intersection
// -----------------------------------------------------------------------------
//

__device__ __forceinline__ static void update_intersection_data(IntersectionObjectData* data, float distance, int object_index) {
    data->closest_object_distance = distance;
    data->closest_object_index = object_index;
}


__device__ static float get_object_distance(const float3 ray_origen, const float3 ray_direction, Object object) {
    switch (object.type) {
        case Spherical: {
            const Sphere sphere = scene.spheres.data[object.index];
            return sphere.get_intersection_distance(ray_origen, ray_direction);
        }
        case Triangular: {
            const Triangle triangle = scene.triangles.data[object.index];
            return triangle.get_intersection_distance(ray_origen, ray_direction);
        }
        case Boxed: {
            const Box box = scene.boxes.data[object.index];
            return box.get_intersection_distance(ray_origen, ray_direction);
        }
        case Cylindrical: {
            const Cylinder cylinder = scene.cylinders.data[object.index];
            return cylinder.get_intersection_distance(ray_origen, ray_direction);
        }
    }
    return -1.0;
}

__device__ static void add_object_information(IntersectionObjectData* data, const float3 ray_origen, const float3 ray_direction) {
    data->intersection = ray_origen + ray_direction * data->closest_object_distance;

    Object intersection_object = scene.objects.data[data->closest_object_index];

    switch (intersection_object.type) {
        case Spherical: {
            const Sphere sphere = scene.spheres.data[intersection_object.index];
            data->normal = sphere.compute_surface_normal(data->intersection);
            data->material = scene.materials.data[sphere.material_index];
            break;
        }

        case Triangular: {
            const Triangle triangle = scene.triangles.data[intersection_object.index];
            data->normal = triangle.normal;
            data->material = scene.materials.data[triangle.material_index];
            break;
        }

        case Boxed: {
            const Box box = scene.boxes.data[intersection_object.index];
            data->normal = box.compute_surface_normal(data->intersection);
            data->material = scene.materials.data[box.material_index];
            break;
        }

        case Cylindrical:
        {
            const Cylinder cylinder = scene.cylinders.data[intersection_object.index];
            data->normal = cylinder.compute_surface_normal(data->intersection);
            data->material = scene.materials.data[cylinder.material_index];
            break;
        }
    }

    if (dot(data->normal, ray_direction) > 0.0f)
        data->normal = -1 * data->normal;
}

__device__ static IntersectionObjectData get_intersection(const float3 ray_origen, const float3 ray_direction, const IntersectionObjectData last_hit) {
    IntersectionObjectData best;
    best.closest_object_distance = FLT_MAX;
    best.closest_object_index = SIZE_MAX;

    const int STACK_SIZE = 64;
    int stack[STACK_SIZE];
    int stack_ptr = 0;

    stack[stack_ptr++] = 0; // root
    int total_calls = 0;
    while (stack_ptr > 0) {
        int node_index = stack[--stack_ptr];

        const NodeGPU node = scene.bvh.nodes.data[node_index];

        // -------------------------
        // Leaf
        // -------------------------
        const bool is_leaf = (2 * (uint32_t)node_index) + 1 >= scene.bvh.nodes.size;
        if (is_leaf) {
            uint32_t start = node.start_index;
            uint32_t end   = start + node.amount_objects;

            for (uint32_t i = start; i < end; i++) {
                uint32_t object_index = scene.bvh.object_indexes.data[i];
                Object object = scene.objects.data[object_index];
                float d = get_object_distance(ray_origen, ray_direction, object);
                total_calls += 1;
                bool same_object = (last_hit.closest_object_index == object_index && scene.objects.data[last_hit.closest_object_index].type == object.type);
                if (d > 0.0f && d < best.closest_object_distance && !same_object) {
                    best.closest_object_distance = d;
                    best.closest_object_index = object_index;
                }
            }
            continue;
        }

        // -------------------------
        // Internal node
        // -------------------------

        uint32_t child1_index = (2 * node_index) + 1;
        uint32_t child2_index = (2 * node_index) + 2;
        
        const NodeGPU child1 = scene.bvh.nodes.data[child1_index];
        const NodeGPU child2 = scene.bvh.nodes.data[child2_index];
        
        float distance1 = child1.aabb.get_intersection_distance(ray_origen, ray_direction);
        total_calls += 1;
        float distance2 = child2.aabb.get_intersection_distance(ray_origen, ray_direction);
        total_calls += 1;
        bool hit1 = (distance1 >= 0.0f);
        bool hit2 = (distance2 >= 0.0f);
        
        
        if (!hit1 && !hit2)
            continue;
        
        const bool inside1 = child1.aabb.has_inside(ray_origen);
        const bool inside2 = child2.aabb.has_inside(ray_origen);
        const bool check_child1 = (distance1 < best.closest_object_distance || (child1.aabb.has_inside(ray_origen) && child1.aabb.has_inside(ray_origen + distance1 * ray_direction)));
        const bool check_child2 = (distance2 < best.closest_object_distance || (child2.aabb.has_inside(ray_origen) && child2.aabb.has_inside(ray_origen + distance2 * ray_direction)));
        if (hit1 && !hit2) {
            if (check_child1)
                stack[stack_ptr++] = (int)child1_index;
            continue;
        }

        if (hit2 && !hit1) {
            if (check_child2)
                stack[stack_ptr++] = (int)child2_index;
            continue;
        }

        if (distance1 == distance2) { // revisar
            if ((inside1 && inside2) || (!inside1 && !inside2)) {
                if (check_child2)
                    stack[stack_ptr++] = (int)child2_index;

                if (check_child1)
                    stack[stack_ptr++] = (int)child1_index;
                continue;
            }
            if (inside1) {
                if (check_child2)
                    stack[stack_ptr++] = (int)child2_index;

                if (check_child1)
                    stack[stack_ptr++] = (int)child1_index;
                continue;
            } 
            if (inside2) {
                if (check_child2)
                    stack[stack_ptr++] = (int)child1_index;

                if (check_child1)
                    stack[stack_ptr++] = (int)child2_index;
                continue;
            }
        }
        if (distance1 < distance2) {
            if (check_child2)
                stack[stack_ptr++] = (int)child2_index;

            if (check_child1)
                stack[stack_ptr++] = (int)child1_index;
        }
        if (distance2 < distance1) {
            if (check_child1)
                stack[stack_ptr++] = (int)child1_index;

            if (check_child2)
                stack[stack_ptr++] = (int)child2_index;
        }
    }
    if (total_calls >= 1000) {
        printf("total_calls = %i\n", total_calls);
        printf("best.closest_object_distance = %f\n", best.closest_object_distance);
    }
    if (best.closest_object_distance < FLT_MAX)
        add_object_information(&best, ray_origen, ray_direction);
    return best;

}


__device__ static IntersectionObjectData get_intersection2(const float3 ray_origen, const float3 ray_direction, const IntersectionObjectData last_hit) {
    //__device__ static IntersectionObjectData get_intersection2(const float3 ray_origen, const float3 ray_direction, const IntersectionObjectData last_hit, int bounce, uint32_t x, uint32_t y) {
        typedef struct {
            float closest_object_distance;
            bool visited;
        } BasicIntersectionData;
        
        typedef struct {
            uint32_t node_index;
            BasicIntersectionData child_1;
            BasicIntersectionData child_2;
        } NodeInfo;
    IntersectionObjectData best;
    best.closest_object_distance = FLT_MAX;
    best.closest_object_index = SIZE_MAX;

    const int N = 32;
    NodeInfo nodes[N] = {0};
    NodeInfo emptyNode = {0};
    int current_index = 0;

    nodes[current_index] = {0}; // for final result, it will be in child_2.
    current_index += 1;

    nodes[current_index] = {0}; // root
    int total_calls = 0;
    while (current_index > 0) {
        NodeInfo* node_info = &nodes[current_index];
        
        BasicIntersectionData* father_node_my_info;
        const uint32_t node_index = node_info->node_index;
        if (node_index % 2 == 1) {
            father_node_my_info = &nodes[current_index - 1].child_1;
        }
        else {
            father_node_my_info = &nodes[current_index - 1].child_2;
        }
        father_node_my_info->visited = true;
        
        const NodeGPU node = scene.bvh.nodes.data[node_index];
        // -------------------------
        // Leaf
        // -------------------------
        const bool is_leaf = (2 * node_index) + 1 >= scene.bvh.nodes.size;
        if (is_leaf) {
            uint32_t start = node.start_index;
            uint32_t end   = start + node.amount_objects;
            
            uint32_t closest_object_index;
            float closest_object_distance = FLT_MAX;
            for (uint32_t i = start; i < end; i++) {
                uint32_t object_index = scene.bvh.object_indexes.data[i];
                Object object = scene.objects.data[object_index];
                float d = get_object_distance(ray_origen, ray_direction, object);
                total_calls += 1;
                bool same_object = (last_hit.closest_object_index == object_index && scene.objects.data[last_hit.closest_object_index].type == object.type);
                if (d > 0.0f && d < closest_object_distance && !same_object) {
                    closest_object_distance = d;
                    best.closest_object_index = object_index;
                }
            }
            father_node_my_info->closest_object_distance = closest_object_distance;
            *node_info = {0};
            current_index -= 1;
            continue;
        }
        
        // -------------------------
        // Internal node
        // -------------------------
        
        uint32_t child1_index = (2 * node_index) + 1;
        uint32_t child2_index = (2 * node_index) + 2;
        
        const NodeGPU child1 = scene.bvh.nodes.data[child1_index];
        const NodeGPU child2 = scene.bvh.nodes.data[child2_index];
        
        float distance1 = child1.aabb.get_intersection_distance(ray_origen, ray_direction);
        
        total_calls += 1;
        float distance2 = child2.aabb.get_intersection_distance(ray_origen, ray_direction);
        total_calls += 1;
        
        bool hit1 = (distance1 >= 0.0f);
        bool hit2 = (distance2 >= 0.0f);

        if (!hit1 && !hit2) {
            father_node_my_info->closest_object_distance = FLT_MAX;
            *node_info = {0};
            current_index -= 1;
            continue;
        }

        // -------------------------
        // PRUNING EXACTO
        // -------------------------

        if (hit1 && !hit2) {
            if (node_info->child_1.visited) {
                father_node_my_info->closest_object_distance = node_info->child_1.closest_object_distance;
                *node_info = {0};
                current_index -= 1;
            } else {
                current_index += 1;
                nodes[current_index].node_index = child1_index;
            }
            continue;
        }

        if (hit2 && !hit1) {
            if (node_info->child_2.visited) {
                father_node_my_info->closest_object_distance = node_info->child_2.closest_object_distance;
                *node_info = {0};
                current_index -= 1;
            } else {
                current_index += 1;
                nodes[current_index].node_index = child2_index;
            }
            continue;
        }

        if (distance1 == distance2) {
            bool inside1 = child1.aabb.has_inside(ray_origen);
            bool inside2 = child2.aabb.has_inside(ray_origen);

            if ((inside1 && inside2) || (!inside1 && !inside2)) {
                if (!node_info->child_1.visited) {
                    current_index += 1;
                    nodes[current_index].node_index = child1_index;
                    continue;
                }
                if (!node_info->child_2.visited) {
                    current_index += 1;
                    nodes[current_index].node_index = child2_index;
                    continue;
                }
                if (node_info->child_1.closest_object_distance <= node_info->child_2.closest_object_distance) {
                    father_node_my_info->closest_object_distance = node_info->child_1.closest_object_distance;
                } else {
                    father_node_my_info->closest_object_distance = node_info->child_2.closest_object_distance;
                }
                *node_info = {0};
                current_index -= 1;
                continue;
            }
            
            if (inside1) {
                if (!node_info->child_1.visited) {
                    current_index += 1;
                    nodes[current_index].node_index = child1_index;
                    continue;
                }
                if (node_info->child_1.closest_object_distance <= distance2) {
                    father_node_my_info->closest_object_distance = node_info->child_1.closest_object_distance;
                    *node_info = {0};
                    current_index -= 1;
                    continue;
                } else {
                    if (!node_info->child_2.visited) {
                        current_index += 1;
                        nodes[current_index].node_index = child2_index;
                        continue;
                    }
                    father_node_my_info->closest_object_distance = node_info->child_2.closest_object_distance;
                    *node_info = {0};
                    current_index -= 1;
                    continue;
                }
            }
            
            if (inside2) {
                if (!node_info->child_2.visited) {
                    current_index += 1;
                    nodes[current_index].node_index = child2_index;
                    continue;
                }
                if (node_info->child_2.closest_object_distance <= distance1) {
                    father_node_my_info->closest_object_distance = node_info->child_2.closest_object_distance;
                    *node_info = {0};
                    current_index -= 1;
                    continue;
                } else {
                    if (!node_info->child_1.visited) {
                        current_index += 1;
                        nodes[current_index].node_index = child1_index;
                        continue;
                    }
                    father_node_my_info->closest_object_distance = node_info->child_1.closest_object_distance;
                    *node_info = {0};
                    current_index -= 1;
                    continue;
                }
            }
        }
        if (distance1 < distance2) {
            if (!node_info->child_1.visited) {
                current_index += 1;
                nodes[current_index].node_index = child1_index;
                continue;
            }
            if (node_info->child_1.closest_object_distance <= distance2) {
                father_node_my_info->closest_object_distance = node_info->child_1.closest_object_distance;
                *node_info = {0};
                current_index -= 1;
                continue;
            } else {
                if (!node_info->child_2.visited) {
                    current_index += 1;
                    nodes[current_index].node_index = child2_index;
                    continue;
                }
                father_node_my_info->closest_object_distance = node_info->child_2.closest_object_distance;
                *node_info = {0};
                current_index -= 1;
                continue;
            }
        }
        if (distance2 < distance1) {
            if (!node_info->child_2.visited) {
                current_index += 1;
                nodes[current_index].node_index = child2_index;
                continue;
            }
            if (node_info->child_2.closest_object_distance <= distance1) {
                father_node_my_info->closest_object_distance = node_info->child_2.closest_object_distance;
                *node_info = {0};
                current_index -= 1;
                continue;
            } else {
                if (!node_info->child_1.visited) {
                    current_index += 1;
                    nodes[current_index].node_index = child1_index;
                    continue;
                }
                father_node_my_info->closest_object_distance = node_info->child_1.closest_object_distance;
                *node_info = {0};
                current_index -= 1;
                continue;
            }
        }
    }
    best.closest_object_distance = nodes[0].child_2.closest_object_distance;
    //if (total_calls >= 2500 && total_calls % 100 == 0) {
    //    printf("total_calls = %i\n", total_calls);
    //    printf("best.closest_object_distance = %f\n", best.closest_object_distance);
    //}
    
    if (best.closest_object_distance < FLT_MAX)
        add_object_information(&best, ray_origen, ray_direction);
    return best;

}


__device__ static IntersectionObjectData get_intersection3(const float3 ray_origen, const float3 ray_direction, const IntersectionObjectData last_hit) {
//__device__ static IntersectionObjectData get_intersection3(const float3 ray_origen, const float3 ray_direction, const IntersectionObjectData last_hit, int bounce, uint32_t x, uint32_t y) {
    
    typedef struct {
        bool visited;
    } BasicIntersectionData;
    
    typedef struct {
        uint32_t node_index;
        BasicIntersectionData child_1;
        BasicIntersectionData child_2;
    } NodeInfo;

    IntersectionObjectData best;
    best.closest_object_distance = FLT_MAX;
    best.closest_object_index = SIZE_MAX;

    const int STACK_SIZE = 32;
    NodeInfo stack[STACK_SIZE];
    int current_index = 0;

    stack[current_index] = {0}; // for final result, it will be in child_2.
    current_index += 1;

    stack[current_index] = {0}; // root
    int double_inside = 0;
    int double_outside = 0;
    int total_calls = 0;
    while (current_index > 0) {
        total_calls += 1;
        NodeInfo* node_info = &stack[current_index];
        BasicIntersectionData* father_node_my_info;
        const uint32_t node_index = node_info->node_index;
        if (node_index % 2 == 1) {
            father_node_my_info = &stack[current_index - 1].child_1;
        }
        else {
            father_node_my_info = &stack[current_index - 1].child_2;
        }
        father_node_my_info->visited = true;

        const NodeGPU node = scene.bvh.nodes.data[node_index];
        // -------------------------
        // Leaf
        // -------------------------
        const bool is_leaf = (2 * node_index) + 1 >= scene.bvh.nodes.size;
        if (is_leaf) {
            uint32_t start = node.start_index;
            uint32_t end   = start + node.amount_objects;

            uint32_t closest_object_index;
            float closest_object_distance = FLT_MAX;
            for (uint32_t i = start; i < end; i++) {
                uint32_t object_index = scene.bvh.object_indexes.data[i];
                Object object = scene.objects.data[object_index];
                float d = get_object_distance(ray_origen, ray_direction, object);
                bool same_object = (last_hit.closest_object_index == object_index && scene.objects.data[last_hit.closest_object_index].type == object.type);
                if (d > 0.0f && d < closest_object_distance && !same_object) {
                    closest_object_distance = d;
                    best.closest_object_index = object_index;
                }
            }
            best.closest_object_distance = closest_object_distance;
            *node_info = {0};
            current_index -= 1;
            continue;
        }

        // -------------------------
        // Internal node
        // -------------------------

        uint32_t child1_index = (2 * node_index) + 1;
        uint32_t child2_index = (2 * node_index) + 2;

        const NodeGPU child1 = scene.bvh.nodes.data[child1_index];
        const NodeGPU child2 = scene.bvh.nodes.data[child2_index];

        float distance1 = child1.aabb.get_intersection_distance(ray_origen, ray_direction);
        float distance2 = child2.aabb.get_intersection_distance(ray_origen, ray_direction);

        bool hit1 = (distance1 >= 0.0f);
        bool hit2 = (distance2 >= 0.0f);

        if (!hit1 && !hit2) {
            *node_info = {0};
            current_index -= 1;
            continue;
        }

        // -------------------------
        // PRUNING EXACTO
        // -------------------------

        if (hit1 && !hit2) {
            if (node_info->child_1.visited) {
                *node_info = {0};
                current_index -= 1;
            } else {
                current_index += 1;
                stack[current_index].node_index = child1_index;
            }
            continue;
        }

        if (hit2 && !hit1) {
            if (node_info->child_2.visited) {
                *node_info = {0};
                current_index -= 1;
            } else {
                current_index += 1;
                stack[current_index].node_index = child2_index;
            }
            continue;
        }

        if (distance1 == distance2) {
            bool inside1 = child1.aabb.has_inside(ray_origen);
            bool inside2 = child2.aabb.has_inside(ray_origen);

            if (inside1 && inside2 || (!inside1 && !inside2)) {
                if (!node_info->child_1.visited) {
                    current_index += 1;
                    stack[current_index].node_index = child1_index;
                    continue;
                }
                if (!node_info->child_2.visited) {
                    current_index += 1;
                    stack[current_index].node_index = child2_index;
                    continue;
                }
                *node_info = {0};
                current_index -= 1;
                continue;
            }
            
            if (inside1) {
                if (!node_info->child_1.visited) {
                    current_index += 1;
                    stack[current_index].node_index = child1_index;
                    continue;
                }
                if (best.closest_object_distance <= distance2) {
                    *node_info = {0};
                    current_index -= 1;
                    continue;
                } else {
                    if (!node_info->child_2.visited) {
                        current_index += 1;
                        stack[current_index].node_index = child2_index;
                        continue;
                    }
                    *node_info = {0};
                    current_index -= 1;
                    continue;
                }
            }
            
            if (inside2) {
                if (!node_info->child_2.visited) {
                    current_index += 1;
                    stack[current_index].node_index = child2_index;
                    continue;
                }
                if (best.closest_object_distance <= distance1) {
                    *node_info = {0};
                    current_index -= 1;
                    continue;
                } else {
                    if (!node_info->child_1.visited) {
                        current_index += 1;
                        stack[current_index].node_index = child1_index;
                        continue;
                    }
                    *node_info = {0};
                    current_index -= 1;
                    continue;
                }
            }
        }
        if (distance1 < distance2) {
            if (!node_info->child_1.visited) {
                current_index += 1;
                stack[current_index].node_index = child1_index;
                continue;
            }
            if (best.closest_object_distance <= distance2) {
                *node_info = {0};
                current_index -= 1;
                continue;
            } else {
                if (!node_info->child_2.visited) {
                    current_index += 1;
                    stack[current_index].node_index = child2_index;
                    continue;
                }
                *node_info = {0};
                current_index -= 1;
                continue;
            }
        }
        if (distance2 < distance1) {
            if (!node_info->child_2.visited) {
                current_index += 1;
                stack[current_index].node_index = child2_index;
                continue;
            }
            if (best.closest_object_distance <= distance1) {
                *node_info = {0};
                current_index -= 1;
                continue;
            } else {
                if (!node_info->child_1.visited) {
                    current_index += 1;
                    stack[current_index].node_index = child1_index;
                    continue;
                }
                *node_info = {0};
                current_index -= 1;
                continue;
            }
        }
    }
    if (best.closest_object_distance < FLT_MAX)
        add_object_information(&best, ray_origen, ray_direction);
    return best;

}

//
// -----------------------------------------------------------------------------
// Cosine-weighted hemisphere sampling (Frisvad)
// -----------------------------------------------------------------------------
//

__device__ static float3 get_new_direction(float3 N, uint32_t& seed) {
    float3 T, B;

    if (N.z >= 0.0f) {
        float a = 1.0f / (1.0f + N.z);
        float b = -N.x * N.y * a;

        T = make_float3(1.0f - N.x * N.x * a, b, -N.x);
        B = make_float3(b, 1.0f - N.y * N.y * a, -N.y);
    } else {
        float a = 1.0f / (1.0f - N.z);
        float b = N.x * N.y * a;

        T = make_float3(1.0f - N.x * N.x * a, -b, N.x);
        B = make_float3(b, N.y * N.y * a - 1.0f, -N.y);
    }

    const float u = rand01(seed);
    const float v = rand01(seed);

    const float r = sqrtf(1.0f - u);
    const float z = sqrtf(u);
    const float theta = 2.0f * M_PI * v;

    float3 dir =
        (r * cosf(theta)) * T +
        (r * sinf(theta)) * B +
        z * N;

    return dir * rsqrt(length_squared(dir));

}

//
// -----------------------------------------------------------------------------
// Path tracing
// -----------------------------------------------------------------------------
//

__device__ static float3 get_color(Camera camera, float3 ray_origen, float3 ray_direction, uint32_t& seed, uint32_t x, uint32_t y) {
    IntersectionObjectData hit = {};
    hit.closest_object_index = SIZE_MAX;

    const float3 sky_radiance = {1.0f, 1.f, 1.0f};

    float3 throughput = make_float3(1.0f, 1.0f, 1.0f);  // throughput
    float3 emision = make_float3(0.0f, 0.0f, 0.0f);

    const int max_bounces = 100;
    const int rr_start = 5;

    for (int i = 0; i < max_bounces; i++) {

        //hit = get_intersection2(ray_origen, ray_direction, hit, i, x, y);
        hit = get_intersection2(ray_origen, ray_direction, hit);

        if (hit.closest_object_distance == FLT_MAX) {
            emision = emision + hadamard(sky_radiance, throughput);
            break;
        }

        // Emisión
        emision = emision + hadamard(hit.material.radiance, throughput);

        // Actualizar throughput (Lambert cosine-weighted)
        throughput = hadamard(throughput, hit.material.albedo);

        // Russian Roulette
        if (i >= rr_start) {

            float p = fmaxf(throughput.x, fmaxf(throughput.y, throughput.z));
            p = fminf(p, 0.95f);

            float r = rand01(seed);

            if (r > p)
                break;

            throughput = throughput / p;  // compensación para mantener unbiased
        }

        // Nuevo rebote
        ray_direction = get_new_direction(hit.normal, seed);
        ray_origen = hit.intersection;
    }

    return emision;
}


//
// -----------------------------------------------------------------------------
// Kernel
// -----------------------------------------------------------------------------
//

__global__ void raygen_kernel(half4* __restrict__ pixels, float4* __restrict__ accumulation, uint32_t frame_index, Camera camera)   // <-- nuevo argumento
{
    const uint32_t x = blockIdx.x * blockDim.x + threadIdx.x;
    const uint32_t y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x >= camera.width || y >= camera.height) return;
    const uint32_t pixel_index = (camera.height - 1 - y) * camera.width + x;
    uint32_t seed = x + y * camera.width + frame_index * 9781u;
    seed = wang_hash(seed);

    const float u = ((float)x + rand01(seed) - 0.5) / (float)camera.width;
    const float v = ((float)y + rand01(seed) - 0.5) / (float)camera.height;

    // Reemplaza `camera.` por `camera.` en compute_ray_direction o
    // pasa `camera` como parámetro a compute_ray_direction
    float3 ray_direction = compute_ray_direction(camera, u, v);
    float3 color = get_color(camera, camera.position, ray_direction, seed, x, y);

    const float saved_frames = (float) camera.saved_frames;
    if (saved_frames == 0) {
        accumulation[pixel_index].x = color.x;
        accumulation[pixel_index].y = color.y;
        accumulation[pixel_index].z = color.z;
    }
    else {
        accumulation[pixel_index].x += color.x;
        accumulation[pixel_index].y += color.y;
        accumulation[pixel_index].z += color.z;
    }

    pixels[pixel_index].x = accumulation[pixel_index].x / (saved_frames + 1);
    pixels[pixel_index].y = accumulation[pixel_index].y / (saved_frames + 1);
    pixels[pixel_index].z = accumulation[pixel_index].z / (saved_frames + 1);

    if (x == camera.x && y == camera.y && frame_index % 100 == 0)
    {
        printf("RGB: ");
        printf("%f, ", (float) pixels[pixel_index].x);
        printf("%f, ", (float) pixels[pixel_index].y);
        printf("%f\n", (float) pixels[pixel_index].z);
    }
    
}

//
// -----------------------------------------------------------------------------
// Host launcher
// -----------------------------------------------------------------------------
//

void execute_raygen_kernel(CudaRenderTarget* render_target, uint32_t frame_index, const Camera* camera, SceneGPU* scene_gpu) {
    // La escena solo se copia si cambia (ver cambio 2)
    if (scene_gpu->is_dirty) {
        CUDA_CHECK(cudaMemcpyToSymbol(scene, scene_gpu, sizeof(SceneGPU)));
    }
    scene_gpu->is_dirty = false;

    dim3 block_size(16, 16);
    dim3 grid_size(
        (render_target->width  + block_size.x - 1) / block_size.x,
        (render_target->height + block_size.y - 1) / block_size.y
    );

    raygen_kernel<<<grid_size, block_size>>>(
        (half4*)render_target->device_ptr,
        (float4*)render_target->accumulation_buffer,
        frame_index,
        *camera
    );


    CUDA_CHECK(cudaGetLastError());
}

#ifdef __cplusplus
}
#endif
