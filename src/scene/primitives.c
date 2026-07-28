#include "primitives.h"
#include <math.h>

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

void add_person(Primitives* primitives, float3 p) {
    Sphere head;

    // Piernas
    add_box(primitives, (float3){p.x - 0.35f, p.y, 1.0f}, (float3){0.2f, 0.2f, 1.0f});
    add_box(primitives, (float3){p.x + 0.35f, p.y, 1.0f}, (float3){0.2f, 0.2f, 1.0f});

    // Torso
    add_box(primitives, (float3){p.x, p.y, 2.6f}, (float3){0.6f, 0.3f, 0.8f});

    // Brazos
    add_box(primitives, (float3){p.x - 1.0f, p.y, 2.6f}, (float3){0.4f, 0.15f, 0.15f});
    add_box(primitives, (float3){p.x + 1.0f, p.y, 2.6f}, (float3){0.4f, 0.15f, 0.15f});

    // Cabeza (esfera)
    sphere_create(&head, p.x, p.y, 3.9f, 0.35f, 2);
    Sphere_list_push(&primitives->sphere_list, head);
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


void load_primitives(Primitives* primitives) {
    // Suelo
    add_floor(primitives, -2.0f, 50.0f, 0);

    // Luz en el techo
    Sphere light;
    sphere_create(&light, 0, 0, 5, 2.5f, 6);
    Sphere_list_push(&primitives->sphere_list, light);

    // Esfera blanca
    Sphere sphere;
    sphere_create(&sphere, -8, 0, 0, 2.2f, 0);
    Sphere_list_push(&primitives->sphere_list, sphere);

    // Esfera roja
    sphere_create(&sphere, -3, 3, 0, 1.3f, 1);
    Sphere_list_push(&primitives->sphere_list, sphere);

    // Esfera verde
    sphere_create(&sphere, 3, -2, 0, 1.6f, 2);
    Sphere_list_push(&primitives->sphere_list, sphere);

    // Cubo
    add_cube(primitives, (float3){8,0,0},3);

    // Pirámide
    add_pyramid(primitives,(float3){13,2,-2},4,4);

    // Cilindro
    Cylinder cyl;
    cylinder_create(&cyl, (float3){-13,-1,-2}, 1.5, 5, 4);
    Cylinder_list_push(&primitives->cylinder_list,cyl);


}

bool primitives_innit(Primitives* primitives) {
    primitives->sphere_list = Sphere_list_create(1);
    primitives->triangle_list = Triangle_list_create(1);
    primitives->box_list = Box_list_create(1);
    primitives->cylinder_list = Cylinder_list_create(1);

    load_primitives(primitives);

    return true;
}



void primitives_destroy(Primitives* primitives) {
    Sphere_list_free(&primitives->sphere_list);
    Triangle_list_free(&primitives->triangle_list);
    Box_list_free(&primitives->box_list);
    Cylinder_list_free(&primitives->cylinder_list);
}