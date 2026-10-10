#include <metal_stdlib>
using namespace metal;

// -----------------------------------------------------------------------------
// Constantes
// -----------------------------------------------------------------------------
constant float FLT_MAX_METAL = 3.402823466e+38f;

// -----------------------------------------------------------------------------
// Structs compatibles byte a byte con C.
// -----------------------------------------------------------------------------

struct AABB {
    packed_float3 A;
    packed_float3 B;
};

struct Sphere {
    int           material_index;
    packed_float3 center;
    float         radius;
};

struct Triangle {
    int           material_index;
    packed_float3 A;
    packed_float3 E1;
    packed_float3 E2;
    packed_float3 normal;
};

struct Box {
    int           material_index;
    packed_float3 A;
    packed_float3 B;
};

struct Cylinder {
    int           material_index;
    packed_float3 C;
    float         radius;
    float         height;
};

struct Material {
    packed_float3 albedo;
    packed_float3 radiance;
};

struct Object {
    int    type;    // 0=Spherical, 1=Triangular, 2=Boxed, 3=Cylindrical
    ulong  index;
    AABB   aabb;
};

struct NodeGPU {
    ulong start_index;
    ulong amount_objects;
    AABB  aabb;
};

struct Camera {
    ulong         saved_frames;
    packed_float3 position;
    float         yaw;
    float         pitch;
    float         w_fov;
    packed_float3 velocity;
    float         h_fov;
    float         plane_distance;
    uint          width;
    uint          height;
    packed_float3 forward;
    packed_float3 right;
    packed_float3 up;
    int           x, y;
    uint          max_bounces;
    uint          rr_start;
};

struct SceneCounts {
    ulong materials_size;
    ulong objects_size;
    ulong bvh_object_indexes_size;
    ulong bvh_nodes_size;
    ulong spheres_size;
    ulong triangles_size;
    ulong boxes_size;
    ulong cylinders_size;
};

// Verificación de layout.
static_assert(sizeof(AABB)     == 24,  "AABB layout");
static_assert(sizeof(Sphere)   == 20,  "Sphere layout");
static_assert(sizeof(Triangle) == 52,  "Triangle layout");
static_assert(sizeof(Box)      == 28,  "Box layout");
static_assert(sizeof(Cylinder) == 24,  "Cylinder layout");
static_assert(sizeof(Material) == 24,  "Material layout");
static_assert(sizeof(Object)   == 40,  "Object layout");
static_assert(sizeof(NodeGPU)  == 40,  "NodeGPU layout");
static_assert(sizeof(Camera) == 112, "Camera layout");


// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------
inline float3 f3(packed_float3 v) { return float3(v.x, v.y, v.z); }


// -----------------------------------------------------------------------------
// Hash & random
// -----------------------------------------------------------------------------
inline uint32_t wang_hash(thread uint32_t& seed) {
    seed = (seed ^ 61u) ^ (seed >> 16);
    seed *= 9u;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2du;
    seed = seed ^ (seed >> 15);
    return seed;
}

inline float rand01(thread uint32_t& seed) {
    return (wang_hash(seed) & 0x00FFFFFF) * (1.0f / 16777216.0f);
}


// -----------------------------------------------------------------------------
// Ray direction
// -----------------------------------------------------------------------------
inline float3 compute_ray_direction(constant Camera& camera, float u, float v) {
    const float u_offset = u - 0.5f;
    const float v_offset = v - 0.5f;

    const float cos_w = cos(camera.w_fov * u_offset);
    const float sin_w = sin(camera.w_fov * u_offset);

    const float3 fwd   = f3(camera.forward);
    const float3 right = f3(camera.right);
    const float3 up    = f3(camera.up);

    const float3 image_point1 =
        (fwd   * (camera.width * cos_w)) +
        (right * (camera.width * sin_w)) +
        (up    * (camera.height * camera.w_fov * v_offset));

    const float cos_h = cos(camera.h_fov * v_offset);
    const float sin_h = sin(camera.h_fov * v_offset);

    const float3 image_point2 =
        (right * (camera.width * camera.h_fov * u_offset)) +
        (fwd   * (camera.height * cos_h)) +
        (up    * (camera.height * sin_h));

    const float3 image_point = (image_point1 + image_point2) * 0.5f;

    const float inv_len = rsqrt(length_squared(image_point));
    return image_point * inv_len;
}


// -----------------------------------------------------------------------------
// AABB
// -----------------------------------------------------------------------------
inline float aabb_intersect(AABB aabb, float3 ro, float3 rd) {
    const float3 A = f3(aabb.A);
    const float3 B = f3(aabb.B);

    // Protección contra NaN: si rd es 0 y el AABB es plano en ese eje,
    // 0/0 daría NaN y el slab se descartaría silenciosamente.
    const float3 safe_rd = float3(
        copysign(fmax(fabs(rd.x), 1e-20f), rd.x),
        copysign(fmax(fabs(rd.y), 1e-20f), rd.y),
        copysign(fmax(fabs(rd.z), 1e-20f), rd.z)
    );
    const float3 inv_rd = 1.0f / safe_rd;

    const float3 t0 = (A - ro) * inv_rd;
    const float3 t1 = (B - ro) * inv_rd;

    const float3 tmin = fmin(t0, t1);
    const float3 tmax = fmax(t0, t1);

    const float t_enter = fmax(fmax(tmin.x, tmin.y), tmin.z);
    const float t_exit  = fmin(fmin(tmax.x, tmax.y), tmax.z);

    // Sin intersección.
    if (t_enter > t_exit) return -FLT_MAX_METAL;

    // AABB completamente detrás del rayo.
    if (t_exit < 0.0f) return -FLT_MAX_METAL;

    // Rayo empieza dentro del AABB: devolvemos 0 (distancia mínima).
    if (t_enter < 0.0f) return 0.0f;

    // Rayo entra desde afuera.
    return t_enter;
}

inline bool aabb_has_inside(AABB aabb, float3 p) {
    const float3 A = f3(aabb.A);
    const float3 B = f3(aabb.B);
    return (A.x <= p.x && p.x <= B.x &&
            A.y <= p.y && p.y <= B.y &&
            A.z <= p.z && p.z <= B.z);
}


// -----------------------------------------------------------------------------
// Primitivas
// -----------------------------------------------------------------------------
inline float sphere_intersect(Sphere s, float3 ro, float3 rd) {
    const float3 center = f3(s.center);
    const float3 oc = center - ro;
    const float b = dot(oc, rd);
    const float c = dot(oc, oc) - s.radius * s.radius;
    const float disc = b * b - c;
    if (disc < 0.0f) return -1.0f;
    const float sqrt_d = sqrt(disc);
    const float t0 = b - sqrt_d;
    if (t0 > 0.0f) return t0;
    const float t1 = b + sqrt_d;
    if (t1 > 0.0f) return t1;
    return -1.0f;
}

inline float3 sphere_normal(Sphere s, float3 p) {
    return (p - f3(s.center)) / s.radius;
}

inline float triangle_intersect(Triangle tri, float3 ro, float3 rd) {
    const float3 A  = f3(tri.A);
    const float3 E1 = f3(tri.E1);
    const float3 E2 = f3(tri.E2);

    const float3 P   = cross(rd, E2);
    const float  det = dot(E1, P);
    if (fabs(det) < 1e-8f) return -1.0f;

    const float inv_det = 1.0f / det;
    const float3 T = ro - A;

    const float u = dot(T, P) * inv_det;
    if (u < 0.0f || u > 1.0f) return -1.0f;

    const float3 Q = cross(T, E1);
    const float v = dot(rd, Q) * inv_det;
    if (v < 0.0f || (u + v) > 1.0f) return -1.0f;

    const float t = dot(E2, Q) * inv_det;
    return (t > 0.0f) ? t : -1.0f;
}

inline float box_intersect(Box b, float3 ro, float3 rd) {
    const float3 A = f3(b.A);
    const float3 B = f3(b.B);

    const float min_x = fmin(A.x, B.x), max_x = fmax(A.x, B.x);
    const float min_y = fmin(A.y, B.y), max_y = fmax(A.y, B.y);
    const float min_z = fmin(A.z, B.z), max_z = fmax(A.z, B.z);

    float t_low  = -FLT_MAX_METAL;
    float t_high =  FLT_MAX_METAL;

    const float tx_low  = ((rd.x >= 0.0f ? min_x : max_x) - ro.x) / rd.x;
    const float tx_high = ((rd.x >= 0.0f ? max_x : min_x) - ro.x) / rd.x;
    t_low  = fmax(t_low,  tx_low);
    t_high = fmin(t_high, tx_high);

    const float ty_low  = ((rd.y >= 0.0f ? min_y : max_y) - ro.y) / rd.y;
    const float ty_high = ((rd.y >= 0.0f ? max_y : min_y) - ro.y) / rd.y;
    t_low  = fmax(t_low,  ty_low);
    t_high = fmin(t_high, ty_high);

    const float tz_low  = ((rd.z >= 0.0f ? min_z : max_z) - ro.z) / rd.z;
    const float tz_high = ((rd.z >= 0.0f ? max_z : min_z) - ro.z) / rd.z;
    t_low  = fmax(t_low,  tz_low);
    t_high = fmin(t_high, tz_high);

    if (t_low > t_high) return -1.0f;
    if (t_low >= 0.0f)  return t_low;
    if (t_high >= 0.0f) return t_high;
    return -1.0f;
}

inline float3 box_normal(Box b, float3 p) {
    const float3 A = f3(b.A);
    const float3 B = f3(b.B);

    const float min_x = fmin(A.x, B.x), max_x = fmax(A.x, B.x);
    const float min_y = fmin(A.y, B.y), max_y = fmax(A.y, B.y);
    const float min_z = fmin(A.z, B.z), max_z = fmax(A.z, B.z);

    const float eps = 1e-4f;
    if (fabs(p.x - min_x) < eps) return float3(-1, 0, 0);
    if (fabs(p.x - max_x) < eps) return float3( 1, 0, 0);
    if (fabs(p.y - min_y) < eps) return float3(0, -1, 0);
    if (fabs(p.y - max_y) < eps) return float3(0,  1, 0);
    if (fabs(p.z - min_z) < eps) return float3(0, 0, -1);
    if (fabs(p.z - max_z) < eps) return float3(0, 0,  1);
    return float3(0, 0, 0);
}

inline float cylinder_intersect(Cylinder cyl, float3 ro, float3 rd) {
    const float3 C = f3(cyl.C);
    const float3 Q = C - ro;
    float closest_distance = FLT_MAX_METAL;

    const float b = rd.x * Q.x + rd.y * Q.y;
    const float a = (1.0f - rd.z * rd.z);
    const float radius_squared = cyl.radius * cyl.radius;
    const float c = Q.x * Q.x + Q.y * Q.y - radius_squared;
    const float determinant = (b * b) - (a * c);

    if (determinant >= 0.0f) {
        const float sqrt_d = sqrt(determinant);
        const float t1 = (b - sqrt_d) / a;
        const float z1 = (rd.z * t1) - Q.z;
        if (t1 > 0.0f && 0.0f <= z1 && z1 <= cyl.height) {
            closest_distance = t1;
        } else {
            const float t2 = (b + sqrt_d) / a;
            const float z2 = (rd.z * t2) - Q.z;
            if (t2 > 0.0f && 0.0f <= z2 && z2 <= cyl.height)
                closest_distance = t2;
        }
    }

    if (fabs(rd.z) > 1e-8f) {
        const float t3 = Q.z / rd.z;
        const float x3 = rd.x * t3 - Q.x;
        const float y3 = rd.y * t3 - Q.y;
        if (0.0f < t3 && x3 * x3 + y3 * y3 <= radius_squared) {
            closest_distance = fmin(closest_distance, t3);
        }

        const float t4 = (cyl.height + Q.z) / rd.z;
        const float x4 = rd.x * t4 - Q.x;
        const float y4 = rd.y * t4 - Q.y;
        if (0.0f < t4 && x4 * x4 + y4 * y4 <= radius_squared) {
            closest_distance = fmin(closest_distance, t4);
        }
    }

    return (closest_distance == FLT_MAX_METAL) ? -1.0f : closest_distance;
}

inline float3 cylinder_normal(Cylinder cyl, float3 p) {
    const float3 C = f3(cyl.C);
    const float eps = 1e-4f;
    if (fabs(C.z + cyl.height - p.z) < eps) return float3(0, 0, 1);
    if (fabs(C.z - p.z) < eps)             return float3(0, 0, -1);
    return float3((p.x - C.x) / cyl.radius, (p.y - C.y) / cyl.radius, 0.0f);
}


// -----------------------------------------------------------------------------
// HitData y estado del traversal
// -----------------------------------------------------------------------------
struct HitData {
    float    distance;
    ulong    object_index;
    float3   normal;
    float3   intersection;
    Material material;
};

struct BasicIntersectionData {
    float closest_object_distance;
    bool  visited;
};

struct NodeInfo {
    uint node_index;
    BasicIntersectionData child_1;
    BasicIntersectionData child_2;
};


// -----------------------------------------------------------------------------
// Dispatch por tipo
// -----------------------------------------------------------------------------
inline float get_object_distance(
    device Sphere*   spheres,
    device Triangle* triangles,
    device Box*      boxes,
    device Cylinder* cylinders,
    Object obj, float3 ro, float3 rd)
{
    switch (obj.type) {
        case 0: return sphere_intersect(spheres[obj.index], ro, rd);
        case 1: return triangle_intersect(triangles[obj.index], ro, rd);
        case 2: return box_intersect(boxes[obj.index], ro, rd);
        case 3: return cylinder_intersect(cylinders[obj.index], ro, rd);
    }
    return -1.0f;
}


// -----------------------------------------------------------------------------
// fill_hit
// -----------------------------------------------------------------------------
inline void fill_hit(
    device Object*   objects,
    device Material* materials,
    device Sphere*   spheres,
    device Triangle* triangles,
    device Box*      boxes,
    device Cylinder* cylinders,
    thread HitData& hit, float3 rd)
{
    const Object obj = objects[hit.object_index];
    switch (obj.type) {
        case 0: {
            const Sphere s = spheres[obj.index];
            hit.normal   = sphere_normal(s, hit.intersection);
            hit.material = materials[s.material_index];
            break;
        }
        case 1: {
            const Triangle t = triangles[obj.index];
            hit.normal   = f3(t.normal);
            hit.material = materials[t.material_index];
            break;
        }
        case 2: {
            const Box b = boxes[obj.index];
            hit.normal   = box_normal(b, hit.intersection);
            hit.material = materials[b.material_index];
            break;
        }
        case 3: {
            const Cylinder c = cylinders[obj.index];
            hit.normal   = cylinder_normal(c, hit.intersection);
            hit.material = materials[c.material_index];
            break;
        }
    }

    if (dot(hit.normal, rd) > 0.0f)
        hit.normal = -hit.normal;
}


// -----------------------------------------------------------------------------
// Frisvad cosine-weighted hemisphere sampling
// -----------------------------------------------------------------------------
inline float3 get_new_direction(float3 N, thread uint32_t& seed) {
    float3 T, B;

    if (N.z >= 0.0f) {
        const float a = 1.0f / (1.0f + N.z);
        const float b = -N.x * N.y * a;
        T = float3(1.0f - N.x * N.x * a, b, -N.x);
        B = float3(b, 1.0f - N.y * N.y * a, -N.y);
    } else {
        const float a = 1.0f / (1.0f - N.z);
        const float b = N.x * N.y * a;
        T = float3(1.0f - N.x * N.x * a, -b, N.x);
        B = float3(b, N.y * N.y * a - 1.0f, -N.y);
    }

    const float u = rand01(seed);
    const float v = rand01(seed);

    const float r     = sqrt(1.0f - u);
    const float z     = sqrt(u);
    const float theta = 2.0f * M_PI_F * v;

    const float3 dir = (r * cos(theta)) * T +
                       (r * sin(theta)) * B +
                       z * N;

    return dir * rsqrt(length_squared(dir));
}

// Traversal simple con stack de enteros. Sin pruning exacto.
// Corta cuando el AABB de un nodo está más lejos que el mejor hit actual.
inline HitData get_intersection(
    device Object*   objects,
    device Material* materials,
    device Sphere*   spheres,
    device Triangle* triangles,
    device Box*      boxes,
    device Cylinder* cylinders,
    device ulong*    bvh_object_indexes,
    device NodeGPU*  bvh_nodes,
    ulong            bvh_nodes_size,
    float3 ro, float3 rd,
    ulong            last_hit_index)
{
    HitData best;
    best.distance     = FLT_MAX_METAL;
    best.object_index = (ulong)-1;

    const int STACK_SIZE = 64;
    int stack[STACK_SIZE];
    int stack_ptr = 0;

    stack[stack_ptr++] = 0;   // root

    while (stack_ptr > 0) {
        const int node_index = stack[--stack_ptr];
        const NodeGPU node = bvh_nodes[node_index];

        const bool is_leaf = node.amount_objects > 0;

        if (is_leaf) {
            const ulong start = node.start_index;
            const ulong end   = start + node.amount_objects;

            for (ulong i = start; i < end; i++) {
                const ulong object_index = bvh_object_indexes[i];
                const Object object = objects[object_index];
                const float d = get_object_distance(
                    spheres, triangles, boxes, cylinders, object, ro, rd);

                const bool same_object =
                    (last_hit_index != (ulong)-1) &&
                    (last_hit_index == object_index) &&
                    ((int)objects[last_hit_index].type == object.type);

                if (d > 0.0f && d < best.distance && !same_object) {
                    best.distance     = d;
                    best.object_index = object_index;
                }
            }
            continue;
        }

        const ulong child1_index = 2ul * (ulong)node_index + 1ul;
        const ulong child2_index = 2ul * (ulong)node_index + 2ul;

        const NodeGPU child1 = bvh_nodes[child1_index];
        const NodeGPU child2 = bvh_nodes[child2_index];

        const float distance1 = aabb_intersect(child1.aabb, ro, rd);
        const float distance2 = aabb_intersect(child2.aabb, ro, rd);

        const bool hit1 = (distance1 >= 0.0f);
        const bool hit2 = (distance2 >= 0.0f);

        // Pruning por mejor-hit-actual: si el AABB está más lejos, no lo visitamos.
        if (!hit1 && !hit2) continue;

        if (hit1 && !hit2) {
            if (distance1 < best.distance)
                stack[stack_ptr++] = (int)child1_index;
            continue;
        }
        if (hit2 && !hit1) {
            if (distance2 < best.distance)
                stack[stack_ptr++] = (int)child2_index;
            continue;
        }

        // Ambos hits: visitar el más cercano primero (mejor pruning).
        if (distance1 < distance2) {
            if (distance2 < best.distance) stack[stack_ptr++] = (int)child2_index;
            if (distance1 < best.distance) stack[stack_ptr++] = (int)child1_index;
        } else {
            if (distance1 < best.distance) stack[stack_ptr++] = (int)child1_index;
            if (distance2 < best.distance) stack[stack_ptr++] = (int)child2_index;
        }
    }

    if (best.distance < FLT_MAX_METAL) {
        best.intersection = ro + rd * best.distance;
        fill_hit(objects, materials, spheres, triangles, boxes, cylinders, best, rd);
    }

    return best;
}

// -----------------------------------------------------------------------------
// Path tracing
// -----------------------------------------------------------------------------
inline float3 get_color(
    constant Camera& camera,
    device Object*   objects,
    device Material* materials,
    device Sphere*   spheres,
    device Triangle* triangles,
    device Box*      boxes,
    device Cylinder* cylinders,
    device ulong*    bvh_object_indexes,
    device NodeGPU*  bvh_nodes,
    ulong            bvh_nodes_size,
    float3 ro, float3 rd, thread uint32_t& seed)
{
    HitData hit;
    hit.object_index = (ulong)-1;

    const float3 sky_radiance = float3(0.0f, 0.0f, 0.0f);

    // -------------------------------------------------------------------------
    // Frame 0: preview rápido. Un solo rayo, sin path tracing.
    // Da una imagen limpia (sin ruido) al instante, que el path tracer
    // irá refinando en los frames siguientes.
    // -------------------------------------------------------------------------
    if (camera.saved_frames == 0) {
        hit = get_intersection(
            objects, materials, spheres, triangles, boxes, cylinders,
            bvh_object_indexes, bvh_nodes, bvh_nodes_size,
            ro, rd, (ulong)-1);

        if (hit.distance == FLT_MAX_METAL) {
            return sky_radiance;
        }
        return f3(hit.material.radiance) + f3(hit.material.albedo);
    }

    // -------------------------------------------------------------------------
    // Frames siguientes: path tracing acumulativo normal.
    // -------------------------------------------------------------------------
    float3 throughput = float3(1.0f, 1.0f, 1.0f);
    float3 emission   = float3(0.0f, 0.0f, 0.0f);

    const int max_bounces = (int)camera.max_bounces;
    const int rr_start    = (int)camera.rr_start;

    ulong last_hit_index = (ulong)-1;

    for (int i = 0; i < max_bounces; i++) {
        hit = get_intersection(
            objects, materials, spheres, triangles, boxes, cylinders,
            bvh_object_indexes, bvh_nodes, bvh_nodes_size,
            ro, rd, last_hit_index);

        if (hit.distance == FLT_MAX_METAL) {
            emission += sky_radiance * throughput;
            break;
        }

        last_hit_index = hit.object_index;

        emission   += f3(hit.material.radiance) * throughput;
        throughput *= f3(hit.material.albedo);

        if (i >= rr_start) {
            float p = fmax(throughput.x, fmax(throughput.y, throughput.z));
            p = fmin(p, 0.95f);
            if (rand01(seed) > p) break;
            throughput /= p;
        }

        rd = get_new_direction(hit.normal, seed);
        ro = hit.intersection;
    }

    return emission;
}


// -----------------------------------------------------------------------------
// Kernel principal
// -----------------------------------------------------------------------------
kernel void raygen(
    device half4*      pixels             [[buffer(0)]],
    constant Camera&   camera             [[buffer(1)]],
    constant uint32_t& frame_index        [[buffer(2)]],
    device Material*   materials          [[buffer(3)]],
    device Object*     objects            [[buffer(4)]],
    device ulong*      bvh_object_indexes [[buffer(5)]],
    device NodeGPU*    bvh_nodes          [[buffer(6)]],
    device Sphere*     spheres            [[buffer(7)]],
    device Triangle*   triangles          [[buffer(8)]],
    device Box*        boxes              [[buffer(9)]],
    device Cylinder*   cylinders          [[buffer(10)]],
    constant SceneCounts& counts          [[buffer(11)]],
    device float4*     accumulation       [[buffer(12)]],
    uint2              gid                [[thread_position_in_grid]])
{
    const uint32_t x = gid.x;
    const uint32_t y = gid.y;

    if (x >= camera.width || y >= camera.height) return;

    const uint32_t pixel_index = (camera.height - 1 - y) * camera.width + x;
    uint32_t seed = x + y * camera.width + frame_index * 9781u;
    seed = wang_hash(seed);

    const float u = ((float)x + rand01(seed) - 0.5f) / (float)camera.width;
    const float v = ((float)y + rand01(seed) - 0.5f) / (float)camera.height;

    const float3 ray_origin    = f3(camera.position);
    const float3 ray_direction = compute_ray_direction(camera, u, v);

    const float3 color = get_color(
        camera,
        objects, materials, spheres, triangles, boxes, cylinders,
        bvh_object_indexes, bvh_nodes, counts.bvh_nodes_size,
        ray_origin, ray_direction, seed);

    // Acumulación progresiva.
    const float saved_frames = (float)camera.saved_frames;

    if (saved_frames == 0.0f) {
        accumulation[pixel_index].x = color.x;
        accumulation[pixel_index].y = color.y;
        accumulation[pixel_index].z = color.z;
    } else {
        accumulation[pixel_index].x += color.x;
        accumulation[pixel_index].y += color.y;
        accumulation[pixel_index].z += color.z;
    }

    const float inv_frames = 1.0f / (saved_frames + 1.0f);

    half4 out;
    out.x = half(accumulation[pixel_index].x * inv_frames);
    out.y = half(accumulation[pixel_index].y * inv_frames);
    out.z = half(accumulation[pixel_index].z * inv_frames);
    out.w = half(1.0f);

    pixels[pixel_index] = out;
}