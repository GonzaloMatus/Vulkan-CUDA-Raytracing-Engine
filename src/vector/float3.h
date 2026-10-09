#pragma once
#include <stdbool.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef __cplusplus
typedef struct {
    union {
        struct {
            float x, y, z;
        };
        float data[3];
    };
} float3;

// Operaciones básicas (return nuevo vector)
float dot(float3 a, float3 b);
float length_squared(float3 v);
float length(float3 v);
float3 add(float3 a, float3 b);
float3 subtract(float3 a, float3 b);
float3 multiply(float3 v, float scalar);
float3 divide(float3 v, float scalar);
float3 normalized(float3 v);
float3 cross(float3 a, float3 b);

// Operaciones que modifican el vector original
void increase(float3* a, float3 b);
void decrease(float3* a, float3 b);
void scale(float3* v, float scalar);
void shrink(float3* v, float scalar);
void normalize(float3* v);

// Verificaciones
bool is_zero_vector(float3 v);
bool is_near_zero(float3 v, float epsilon);
#endif

// Funciones específicas para CUDA (declaraciones e implementaciones)
#ifdef __CUDACC__
static __device__ __forceinline__ inline float dot(float3 a, float3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static __device__ __forceinline__ inline float length_squared(float3 v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

static __device__ __forceinline__ inline float3 cross(float3 a, float3 b) {
    float3 c;
    c.x = a.y * b.z - a.z * b.y;
    c.y = a.z * b.x - a.x * b.z;
    c.z = a.x * b.y - a.y * b.x;
    return c;
}

static __device__ __forceinline__ inline float3 hadamard(float3 a, float3 b) {
    float3 r = {a.x * b.x, a.y * b.y, a.z * b.z};
    return r;
}

static __device__ __forceinline__ inline float3 operator+(float3 a, float3 b) {
    float3 r = {a.x + b.x, a.y + b.y, a.z + b.z};
    return r;
}

static __device__ __forceinline__ inline float3 operator-(float3 a, float3 b) {
    float3 r = {a.x - b.x, a.y - b.y, a.z - b.z};
    return r;
}

static __device__ __forceinline__ inline float3 operator*(float3 v, float scalar) {
    float3 r = {v.x * scalar, v.y * scalar, v.z * scalar};
    return r;
}

static __device__ __forceinline__ inline float3 operator/(float3 v, float scalar) {
    float3 r = {v.x / scalar, v.y / scalar, v.z / scalar};
    return r;
}

static __device__ __forceinline__ inline float3 operator*(float scalar, float3 v) {
    float3 r = {v.x * scalar, v.y * scalar, v.z * scalar};
    return r;
}
#endif