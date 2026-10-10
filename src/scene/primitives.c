#include "primitives.h"
#include <math.h>
#include <stdio.h>

static inline void push_triangle(Primitives* primitives, float3 a, float3 b, float3 c, int material_index) {
    Triangle t;
    triangle_create(&t, a, b, c, material_index);
    Triangle_list_push(&primitives->triangle_list, t);
}

void add_head(Primitives* primitives, float3 c, float r) {
    float3 top    = {c.x, c.y, c.z + r};
    float3 bottom = {c.x, c.y, c.z - r};

    float3 v[4] = {
        {c.x + r, c.y,     c.z},
        {c.x - r, c.y,     c.z},
        {c.x,     c.y + r, c.z},
        {c.x,     c.y - r, c.z},
    };

    for (int i = 0; i < 4; i++) {
        push_triangle(primitives, top, v[i], v[(i + 1) % 4], 3);
        push_triangle(primitives, bottom, v[(i + 1) % 4], v[i], 4);
    }
}

void add_box(Primitives* primitives, float3 c, float3 s) {
    float3 v[8] = {
        {c.x - s.x, c.y - s.y, c.z - s.z},
        {c.x + s.x, c.y - s.y, c.z - s.z},
        {c.x + s.x, c.y + s.y, c.z - s.z},
        {c.x - s.x, c.y + s.y, c.z - s.z},
        {c.x - s.x, c.y - s.y, c.z + s.z},
        {c.x + s.x, c.y - s.y, c.z + s.z},
        {c.x + s.x, c.y + s.y, c.z + s.z},
        {c.x - s.x, c.y + s.y, c.z + s.z},
    };

    int f[12][3] = {
        {0,2,1},{0,3,2}, // abajo
        {4,5,6},{4,6,7}, // arriba
        {0,1,5},{0,5,4},
        {1,2,6},{1,6,5},
        {2,3,7},{2,7,6},
        {3,0,4},{3,4,7}
    };

    for (int i = 0; i < 12; i++)
        push_triangle(primitives, v[f[i][0]], v[f[i][1]], v[f[i][2]], 3);
}



void add_cube(Primitives* primitives, float3 center, float size) {
    float s = size * 0.5f;

    float3 offset = (float3) {s, s, s};
    float3 A = subtract(center, offset);
    float3 B = add(center, offset);

    Box box;
    box_create(&box, A, B, 7);
    Box_list_push(&primitives->box_list, box);
}

void add_pyramid(Primitives* primitives, float3 center, float base, float height) {
    float s = base * 0.5f;

    float3 b0 = {center.x - s, center.y - s, center.z};
    float3 b1 = {center.x + s, center.y - s, center.z};
    float3 b2 = {center.x + s, center.y + s, center.z};
    float3 b3 = {center.x - s, center.y + s, center.z};

    float3 top = {center.x, center.y, center.z + height};

    // Base (mirando hacia abajo)
    push_triangle(primitives, b0, b1, b2, 4);
    push_triangle(primitives, b0, b2, b3, 4);

    // Caras laterales
    push_triangle(primitives, b0, b1, top, 4);
    push_triangle(primitives, b1, b2, top, 4);
    push_triangle(primitives, b2, b3, top, 4);
    push_triangle(primitives, b3, b0, top, 4);
}

void add_floor(Primitives* primitives, float z, float size, int material_index) {
    float s = size * 0.5f;

    float3 a = {-s, -s, z};
    float3 b = { s, -s, z};
    float3 c = { s,  s, z};
    float3 d = {-s,  s, z};

    // Normal apuntando +Z
    push_triangle(primitives, a, b, c, material_index);
    push_triangle(primitives, a, c, d, material_index);
}

// -----------------------------------------------------------------------------
// Nueva función: genera una cuadrícula 3D de esferas
// -----------------------------------------------------------------------------
void add_sphere_grid(Primitives* primitives, int grid_size, float spacing, float radius) {
    // Centrar la cuadrícula en el origen
    float offset = (float)(grid_size - 1) * spacing * 0.5f;

    for (int i = 0; i < grid_size; i++) {
        for (int j = 0; j < grid_size; j++) {
            for (int k = 0; k < grid_size; k++) {
                float x = (float)i * spacing - offset;
                float y = (float)j * spacing - offset;
                float z = (float)k * spacing - offset;

                // Variar ligeramente el radio y el material para dar variedad
                float r = radius * (0.8f + 0.4f * ((i + j + k) % 5) / 4.0f);
                int mat = (i * 7 + j * 13 + k * 23) % 6; // 9 materiales disponibles

                Sphere sphere;
                sphere_create(&sphere, x, y, z, r, mat);
                Sphere_list_push(&primitives->sphere_list, sphere);
            }
        }
    }
}

// -----------------------------------------------------------------------------
// Icosphere: esfera generada proceduralmente a base de triángulos.
//
// Empieza con un icosaedro (20 triángulos, 12 vértices) y subdivide cada
// triángulo en 4 recursivamente. Cada nuevo vértice se proyecta a la esfera.
//
// Triangle count = 20 * 4^subdivisions
//   subdivisions = 0  →    20 triángulos (icosaedro)
//   subdivisions = 1  →    80 triángulos
//   subdivisions = 2  →   320 triángulos
//   subdivisions = 3  →  1280 triángulos
//   subdivisions = 4  →  5120 triángulos
//   subdivisions = 5  → 20480 triángulos
// -----------------------------------------------------------------------------

typedef struct { float3 a, b, c; } IcosphereTri;

static void add_icosphere(Primitives* primitives, float3 center, float radius,
                          int subdivisions, int material_index)
{
    // Proporción áurea.
    const float t = (1.0f + sqrtf(5.0f)) * 0.5f;
    const float inv_len = 1.0f / sqrtf(1.0f + t * t);

    // 12 vértices del icosaedro base.
    float3 base[12] = {
        {-1,  t,  0}, { 1,  t,  0}, {-1, -t,  0}, { 1, -t,  0},
        { 0, -1,  t}, { 0,  1,  t}, { 0, -1, -t}, { 0,  1, -t},
        { t,  0, -1}, { t,  0,  1}, {-t,  0, -1}, {-t,  0,  1}
    };
    for (int i = 0; i < 12; i++) {
        base[i] = multiply(base[i], inv_len);
    }

    // 20 caras del icosaedro.
    static const int faces[20][3] = {
        { 0, 11,  5}, { 0,  5,  1}, { 0,  1,  7}, { 0,  7, 10}, { 0, 10, 11},
        { 1,  5,  9}, { 5, 11,  4}, {11, 10,  2}, {10,  7,  6}, { 7,  1,  8},
        { 3,  9,  4}, { 3,  4,  2}, { 3,  2,  6}, { 3,  6,  8}, { 3,  8,  9},
        { 4,  9,  5}, { 2,  4, 11}, { 6,  2, 10}, { 8,  6,  7}, { 9,  8,  1}
    };

    // Triángulos actuales. Empiezan siendo las 20 caras del icosaedro.
    size_t tri_capacity = 20u * (size_t)(1u << (2 * subdivisions));
    IcosphereTri* tris = malloc(tri_capacity * sizeof(IcosphereTri));
    size_t tri_count = 0;

    for (int i = 0; i < 20; i++) {
        tris[tri_count++] = (IcosphereTri){
            base[faces[i][0]],
            base[faces[i][1]],
            base[faces[i][2]]
        };
    }

    // Subdividir recursivamente.
    for (int s = 0; s < subdivisions; s++) {
        IcosphereTri* new_tris = malloc(tri_count * 4 * sizeof(IcosphereTri));
        size_t new_count = 0;

        for (size_t i = 0; i < tri_count; i++) {
            const IcosphereTri tri = tris[i];

            // Puntos medios, proyectados a la esfera unitaria.
            const float3 ab = normalized(add(tri.a, tri.b));
            const float3 bc = normalized(add(tri.b, tri.c));
            const float3 ca = normalized(add(tri.c, tri.a));

            new_tris[new_count++] = (IcosphereTri){ tri.a, ab, ca };
            new_tris[new_count++] = (IcosphereTri){ tri.b, bc, ab };
            new_tris[new_count++] = (IcosphereTri){ tri.c, ca, bc };
            new_tris[new_count++] = (IcosphereTri){ ab,    bc, ca };
        }

        free(tris);
        tris = new_tris;
        tri_count = new_count;
    }

    // Emitir todos los triángulos escalados y trasladados.
    for (size_t i = 0; i < tri_count; i++) {
        const float3 a = add(center, multiply(tris[i].a, radius));
        const float3 b = add(center, multiply(tris[i].b, radius));
        const float3 c = add(center, multiply(tris[i].c, radius));
        push_triangle(primitives, a, b, c, material_index);
    }

    free(tris);

    printf("Icosphere: %zu triangles (%d subdivisions, radius %.1f)\n",
           tri_count, subdivisions, radius);
}

void load_primitives(Primitives* primitives) {
    const int GRID_SIZE = 30;
    const float SPACING = 10.0f;
    const float RADIUS  = 0.4f;

    add_sphere_grid(primitives, GRID_SIZE, SPACING, RADIUS);

    Sphere light;
    sphere_create(&light, 0, 0, (float)GRID_SIZE * SPACING * 0.8f, 20.0f, 6);
    Sphere_list_push(&primitives->sphere_list, light);

    add_floor(primitives, -(float)GRID_SIZE * SPACING * 0.8f, (float)GRID_SIZE * SPACING * 2.0f, 0);

    // NUEVO: un par de icospheres para probar el pipeline de triángulos.
    // 3 subdivisiones = 1280 triángulos cada uno.
    add_icosphere(primitives, (float3){  50.0f, 0.0f, 0.0f }, 30.0f, 3, 1);  // Red
    add_icosphere(primitives, (float3){ -50.0f, 0.0f, 0.0f }, 30.0f, 4, 2);  // Green
    add_icosphere(primitives, (float3){   0.0f, 50.0f, 50.0f }, 20.0f, 2, 4); // Blue
}

bool primitives_innit(Primitives* primitives) {
    primitives->sphere_list   = Sphere_list_create(1024);   // capacidad inicial grande
    primitives->triangle_list = Triangle_list_create(1024);
    primitives->box_list      = Box_list_create(1024);
    primitives->cylinder_list = Cylinder_list_create(1024);

    load_primitives(primitives);

    return true;
}

void primitives_destroy(Primitives* primitives) {
    Sphere_list_free(&primitives->sphere_list);
    Triangle_list_free(&primitives->triangle_list);
    Box_list_free(&primitives->box_list);
    Cylinder_list_free(&primitives->cylinder_list);
}