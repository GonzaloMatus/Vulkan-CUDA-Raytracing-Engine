#pragma once
#include "../vector/float3.h"
#include "../core/input.h"
#include <stdint.h>



typedef struct {
    uint64_t saved_frames;
    float3 position;
    float yaw;
    float pitch;
    float w_fov;
    float3 velocity;

    // Cached Data:
    float h_fov;
    float plane_distance;
    uint32_t width;
    uint32_t height;
    float3 forward;
    float3 right;
    float3 up;

    int x, y;
} Camera;


void camera_innit(Camera* camera, uint32_t width, uint32_t height);
void update_camera(Camera* camera, Input* input, float delta_time_miliseconds);
void camera_update_vectors(Camera* camera);
float3 get_forward(Camera* camera);
float3 get_up(Camera* camera);
float3 get_right(Camera* camera);
