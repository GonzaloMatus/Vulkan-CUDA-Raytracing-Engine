#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <float.h>
#include <math.h>
#include "../../vector/float3.h"
#include "bvh.h"
#include "../primitives.h"

void set_aabb(Node* node, const Object_list* object_list) {
    AABB none_restrictive_aabb;
    none_restrictive_aabb.A = (float3) {FLT_MAX, FLT_MAX, FLT_MAX};
    none_restrictive_aabb.B = (float3) {-FLT_MAX, -FLT_MAX, -FLT_MAX};
    node->aabb = none_restrictive_aabb;
    for (size_t i = 0; i < node->object_indexes.size; i++) {
        Object* object = &object_list->data[node->object_indexes.data[i]];
        node->aabb.A.x = fminf(node->aabb.A.x, object->aabb.A.x);
        node->aabb.A.y = fminf(node->aabb.A.y, object->aabb.A.y);
        node->aabb.A.z = fminf(node->aabb.A.z, object->aabb.A.z);

        node->aabb.B.x = fmaxf(node->aabb.B.x, object->aabb.B.x);
        node->aabb.B.y = fmaxf(node->aabb.B.y, object->aabb.B.y);
        node->aabb.B.z = fmaxf(node->aabb.B.z, object->aabb.B.z);   
    }
    
}


static int get_direction(Node* node) {
    const int X = 0;
    const int Y = 1;
    const int Z = 2;
    float3 difference = subtract(node->aabb.B, node->aabb.A);
    const float max_dif = fmaxf(fmaxf(difference.x, difference.y), difference.z);

    if (max_dif == difference.x) {
        return X;
    } else if (max_dif == difference.y) {
        return Y;
    } else if (max_dif == difference.z) {
        return Z;
    }
    return X;
}

int compare_by_high_x(const void* a, const void* b) {
    const Object* oa = (const Object*)a;
    const Object* ob = (const Object*)b;
    return (oa->aabb.B.x > ob->aabb.B.x) - (oa->aabb.B.x < ob->aabb.B.x);
}

int compare_by_high_y(const void* a, const void* b) {
    const Object* oa = (const Object*)a;
    const Object* ob = (const Object*)b;
    return (oa->aabb.B.y > ob->aabb.B.y) - (oa->aabb.B.y < ob->aabb.B.y);
}

int compare_by_high_z(const void* a, const void* b) {
    const Object* oa = (const Object*)a;
    const Object* ob = (const Object*)b;
    return (oa->aabb.B.z > ob->aabb.B.z) - (oa->aabb.B.z < ob->aabb.B.z);
}


static void sort_objects_by_high_in_direction(Object_list* object_list, int direction) {
    switch (direction) {
    case 0:
        qsort(object_list->data, object_list->size, sizeof(Object), compare_by_high_x);
        break;
    case 1:
        qsort(object_list->data, object_list->size, sizeof(Object), compare_by_high_y);
        break;
    case 2:
        qsort(object_list->data, object_list->size, sizeof(Object), compare_by_high_z);
        break;
    return;
    }
}

static void print_enclosure(AABB aabb) {
    printf("A = ");
    printf("%f, ", aabb.A.x);
    printf("%f, ", aabb.A.y);
    printf("%f\n", aabb.A.z);

    printf("B = ");
    printf("%f, ", aabb.B.x);
    printf("%f, ", aabb.B.y);
    printf("%f\n", aabb.B.z);
}

static void print_volume(const AABB aabb) {
    float dx = aabb.B.x - aabb.A.x;
    float dy = aabb.B.y - aabb.A.y;
    float dz = aabb.B.z - aabb.A.z;
    printf("Volume: %f\n", dx * dy * dz);
}


static inline void swap(float* a, float* b) {
    float tmp = *a;
    *a = *b;
    *b = tmp;
}

static int partition(float* arr, int left, int right) {
    float pivot = arr[right];  // pivote simple
    int i = left;

    for (int j = left; j < right; j++) {
        if (arr[j] < pivot) {
            swap(&arr[i], &arr[j]);
            i++;
        }
    }

    swap(&arr[i], &arr[right]);
    return i;
}

float quickselect_iterative(float* arr, int n, int k) {
    int left = 0;
    int right = n - 1;

    while (left <= right) {
        int pivot_index = partition(arr, left, right);

        if (pivot_index == k)
            return arr[pivot_index];

        if (pivot_index > k)
            right = pivot_index - 1;
        else
            left = pivot_index + 1;
    }

    return FLT_MAX; // error
}


void bvh_create(BVH* bvh, const Object_list* object_list) {
    const int max_objects = 2;
    size_t levels = (size_t) floor(log2(fmin((double) (object_list->size + 1), 2.0 * (double) object_list->size / (double) max_objects)));
    printf("levels= %zu\n", levels);
    bvh->amount_nodes = (1ULL << levels) - 1;
    printf("Amount nodes: %zu\n",bvh->amount_nodes);

    bvh->nodes = calloc(bvh->amount_nodes, sizeof(Node));

    bvh->nodes[0].object_indexes = size_t_list_create(object_list->size);

    for (size_t i = 0; i < object_list->size; i++) {
        size_t_list_push(&bvh->nodes[0].object_indexes, i);
    }
    
    set_aabb(&bvh->nodes[0], object_list);
    float* border_position_list;
    border_position_list = malloc(2 * object_list->size * sizeof(float)); // 2 bordes por objeto
    for (int node_index = 0; node_index < bvh->amount_nodes / 2; node_index++) {
        Node* node = &bvh->nodes[node_index];
        //print_enclosure(node->aabb);
        int direction = get_direction(node);
        for (size_t i = 0; i < node->object_indexes.size; i++) {
            AABB aabb = object_list->data[node->object_indexes.data[i]].aabb;
            border_position_list[2 * i] = aabb.A.data[direction];
            border_position_list[(2 * i) + 1] = aabb.B.data[direction];
        }
        //float median_low = quickselect_iterative(border_position_list, 2 * node->object_indexes.size, node->object_indexes.size -1);
        //float median_high = quickselect_iterative(border_position_list, 2 * node->object_indexes.size, node->object_indexes.size);
        //float median = (median_low + median_high) * 0.5f;
        float median = quickselect_iterative(border_position_list, 2 * node->object_indexes.size, (2 * ((node->object_indexes.size - 1) / 2)) + 1 );
        
        Node* high_child = &bvh->nodes[(2 * node_index) + 2];
        Node* low_child = &bvh->nodes[(2 * node_index) + 1];
        
        size_t minimum_amount_of_objects_per_child = (node->object_indexes.size + 1) / 2;
        high_child->object_indexes = size_t_list_create(minimum_amount_of_objects_per_child);
        low_child->object_indexes = size_t_list_create(minimum_amount_of_objects_per_child);
        
        for (size_t i = 0; i < node->object_indexes.size; i++) {
            Object object = object_list->data[node->object_indexes.data[i]];
            
            if (object.aabb.A.data[direction] <= median) {
                size_t_list_push(&low_child->object_indexes, node->object_indexes.data[i]);
            }
            if (median <= object.aabb.B.data[direction]) {
                size_t_list_push(&high_child->object_indexes, node->object_indexes.data[i]);
            }
        }
        
        // Update child low aabb
        set_aabb(low_child, object_list);
        low_child->aabb = aabb_intersect(low_child->aabb, node->aabb);
        low_child->aabb.B.data[direction] = fminf(low_child->aabb.B.data[direction], median);

        set_aabb(high_child, object_list);
        high_child->aabb = aabb_intersect(high_child->aabb, node->aabb);
        high_child->aabb.A.data[direction] = fmaxf(high_child->aabb.A.data[direction], median);

        size_t_list_free(&node->object_indexes);
    }
    free(border_position_list);
    printf("Amount objects: %zu\n", object_list->size);
    printf("Optimal Average objects per leaf: %f\n", (double) object_list->size / (double) ((bvh->amount_nodes + 1) / 2));
}

