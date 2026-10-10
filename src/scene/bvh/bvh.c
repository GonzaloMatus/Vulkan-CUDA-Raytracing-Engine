#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <float.h>
#include <string.h>
#include <math.h>
#include "../../vector/float3.h"
#include "bvh.h"
#include "../primitives.h"

#define MAX_OBJECTS_PER_LEAF 2
#define MAX_DEPTH 17

typedef struct {
    float3 centroid;
    AABB   aabb;
    size_t object_index;
} SAHEntry;

static float aabb_surface_area(AABB aabb) {
    const float dx = aabb.B.x - aabb.A.x;
    const float dy = aabb.B.y - aabb.A.y;
    const float dz = aabb.B.z - aabb.A.z;
    return 2.0f * (dx * dy + dy * dz + dz * dx);
}

static AABB aabb_union(AABB a, AABB b) {
    AABB r;
    r.A.x = fminf(a.A.x, b.A.x);
    r.A.y = fminf(a.A.y, b.A.y);
    r.A.z = fminf(a.A.z, b.A.z);
    r.B.x = fmaxf(a.B.x, b.B.x);
    r.B.y = fmaxf(a.B.y, b.B.y);
    r.B.z = fmaxf(a.B.z, b.B.z);
    return r;
}

static int cmp_centroid_x(const void* a, const void* b) {
    const float ca = ((const SAHEntry*)a)->centroid.x;
    const float cb = ((const SAHEntry*)b)->centroid.x;
    return (ca > cb) - (ca < cb);
}
static int cmp_centroid_y(const void* a, const void* b) {
    const float ca = ((const SAHEntry*)a)->centroid.y;
    const float cb = ((const SAHEntry*)b)->centroid.y;
    return (ca > cb) - (ca < cb);
}
static int cmp_centroid_z(const void* a, const void* b) {
    const float ca = ((const SAHEntry*)a)->centroid.z;
    const float cb = ((const SAHEntry*)b)->centroid.z;
    return (ca > cb) - (ca < cb);
}

static void compute_aabb_from_entries(const SAHEntry* entries, size_t n, AABB* out) {
    AABB r;
    r.A = (float3){FLT_MAX, FLT_MAX, FLT_MAX};
    r.B = (float3){-FLT_MAX, -FLT_MAX, -FLT_MAX};
    for (size_t i = 0; i < n; i++) {
        r.A.x = fminf(r.A.x, entries[i].aabb.A.x);
        r.A.y = fminf(r.A.y, entries[i].aabb.A.y);
        r.A.z = fminf(r.A.z, entries[i].aabb.A.z);
        r.B.x = fmaxf(r.B.x, entries[i].aabb.B.x);
        r.B.y = fmaxf(r.B.y, entries[i].aabb.B.y);
        r.B.z = fmaxf(r.B.z, entries[i].aabb.B.z);
    }
    *out = r;
}

static float evaluate_sah(SAHEntry* entries, size_t n,
                          AABB* prefix, AABB* suffix,
                          size_t* out_best_split)
{
    prefix[0] = entries[0].aabb;
    for (size_t i = 1; i < n; i++) {
        prefix[i] = aabb_union(prefix[i - 1], entries[i].aabb);
    }

    suffix[n - 1] = entries[n - 1].aabb;
    for (size_t i = n - 1; i-- > 0; ) {
        suffix[i] = aabb_union(suffix[i + 1], entries[i].aabb);
    }

    // Coste del nodo padre sin partir.
    const float sa_parent = aabb_surface_area(prefix[n - 1]);
    if (sa_parent <= 0.0f) {
        // Padre degenerado (AABB plano). Forzar split por la mitad.
        *out_best_split = n / 2;
        return FLT_MAX;
    }

    // Coste de traversal, relativo al SA. Es el término constante que
    // faltaba: sin él, SAH siempre parte, produciendo árboles muy profundos
    // con hojas cuyo AABB no aporta pruning.
    const float traverse_cost = 1.0f;

    float best_cost = FLT_MAX;
    size_t best_split = 0;

    for (size_t i = 0; i + 1 < n; i++) {
        const float sa_l = aabb_surface_area(prefix[i]);
        const float sa_r = aabb_surface_area(suffix[i + 1]);
        const size_t n_l = i + 1;
        const size_t n_r = n - n_l;
        // Coste normalizado por SA_parent + coste de traversal.
        const float cost = traverse_cost +
                           (sa_l * (float)n_l + sa_r * (float)n_r) / sa_parent;
        if (cost < best_cost) {
            best_cost = cost;
            best_split = n_l;
        }
    }

    *out_best_split = best_split;
    return best_cost;
}

static void make_leaf(Node* node, const SAHEntry* entries, size_t count) {
    node->object_indexes = size_t_list_create(count);
    for (size_t i = 0; i < count; i++) {
        size_t_list_push(&node->object_indexes, entries[i].object_index);
    }
}

static void build_sah_recursive(
    BVH* bvh, size_t node_index,
    SAHEntry* entries, size_t count,
    SAHEntry* scratch, AABB* prefix, AABB* suffix,
    size_t node_capacity, int depth)
{
    if (count == 0) return;

    if (node_index >= node_capacity) {
        fprintf(stderr,
                "BVH overflow at node %zu (capacity %zu, depth %d)\n",
                node_index, node_capacity, depth);
        return;
    }

    Node* node = &bvh->nodes[node_index];
    node->object_indexes.data = NULL;
    node->object_indexes.size = 0;

    compute_aabb_from_entries(entries, count, &node->aabb);

    if (bvh->amount_nodes <= node_index) {
        bvh->amount_nodes = node_index + 1;
    }

    if (count <= MAX_OBJECTS_PER_LEAF || depth >= MAX_DEPTH) {
        make_leaf(node, entries, count);
        return;
    }

    float  best_cost  = FLT_MAX;
    size_t best_split = 0;

    for (int axis = 0; axis < 3; axis++) {
        memcpy(scratch, entries, count * sizeof(SAHEntry));

        int (*cmp)(const void*, const void*) =
            axis == 0 ? cmp_centroid_x :
            axis == 1 ? cmp_centroid_y :
                        cmp_centroid_z;
        qsort(scratch, count, sizeof(SAHEntry), cmp);

        size_t split;
        const float cost = evaluate_sah(scratch, count, prefix, suffix, &split);

        if (cost < best_cost && split > 0 && split < count) {
            best_cost = cost;
            best_split = split;
            memcpy(entries, scratch, count * sizeof(SAHEntry));
        }
    }

    if (best_split == 0 || best_split >= count) {
        make_leaf(node, entries, count);
        return;
    }

    build_sah_recursive(bvh, 2 * node_index + 1,
                        entries, best_split,
                        scratch, prefix, suffix,
                        node_capacity, depth + 1);
    build_sah_recursive(bvh, 2 * node_index + 2,
                        entries + best_split, count - best_split,
                        scratch, prefix, suffix,
                        node_capacity, depth + 1);
}

void bvh_create(BVH* bvh) {
    const size_t N = bvh->objects.size;
    if (N == 0) {
        bvh->nodes = NULL;
        bvh->amount_nodes = 0;
        return;
    }

    const size_t node_capacity = (size_t)1 << (MAX_DEPTH + 1);

    bvh->nodes = calloc(node_capacity, sizeof(Node));
    bvh->amount_nodes = 0;

    SAHEntry* entries = malloc(N * sizeof(SAHEntry));
    SAHEntry* scratch = malloc(N * sizeof(SAHEntry));
    AABB* prefix      = malloc(N * sizeof(AABB));
    AABB* suffix      = malloc(N * sizeof(AABB));

    for (size_t i = 0; i < N; i++) {
        const Object* obj = &bvh->objects.data[i];
        entries[i].aabb         = obj->aabb;
        entries[i].object_index = i;
        entries[i].centroid.x = (obj->aabb.A.x + obj->aabb.B.x) * 0.5f;
        entries[i].centroid.y = (obj->aabb.A.y + obj->aabb.B.y) * 0.5f;
        entries[i].centroid.z = (obj->aabb.A.z + obj->aabb.B.z) * 0.5f;
    }

    build_sah_recursive(bvh, 0, entries, N,
                        scratch, prefix, suffix,
                        node_capacity, 0);

    free(entries);
    free(scratch);
    free(prefix);
    free(suffix);

    printf("BVH built with SAH: %zu nodes for %zu objects\n",
           bvh->amount_nodes, N);
}

void bvh_destroy(BVH* bvh) {
    if (bvh->nodes) {
        for (size_t i = 0; i < bvh->amount_nodes; i++) {
            if (bvh->nodes[i].object_indexes.data) {
                size_t_list_free(&bvh->nodes[i].object_indexes);
            }
        }
        free(bvh->nodes);
        bvh->nodes = NULL;
    }
    if (bvh->objects.data) {
        free(bvh->objects.data);
        bvh->objects.data = NULL;
        bvh->objects.size = 0;
    }
    bvh->amount_nodes = 0;
}