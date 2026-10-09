#include "camera.h"
#include <math.h>
#include "../core/input.h"
#include "../vector/float3.h"
#include <stdio.h>
#include <stdint.h>

#ifndef M_E
#define M_E 2.7182818284590452354
#endif

float get_plane_distance(Camera* camera) {
    return (((float) camera->width) * 0.5f) / tanf(camera->w_fov * 0.5);
}

float get_h_fov(Camera* camera) {
    const float plane_distance = (camera->width * 0.5f) / tanf(camera->w_fov * 0.5f);
    float h_fov = 2*atanf(camera->height / (2.0f * plane_distance));
    if (h_fov <= 0) {
        h_fov += 2*M_PI;
    }
    return h_fov;
}

void camera_innit(Camera* camera, uint32_t width, uint32_t height) {
    camera->saved_frames = 0;
    camera->width = width;
    camera->height = height;
    camera->yaw = 0.0f;
    camera->pitch = 0.0f;
    camera->position.x = -2.0f;
    camera->position.y = 1.0f;
    camera->position.z = -1.0f;
    camera->w_fov = 1.4f;
    camera->plane_distance = get_plane_distance(camera);
    camera->h_fov = get_h_fov(camera);
    camera->velocity = (float3) {0.0f, 0.0f, 0.0f};
    // Initialize cached vectors
    camera->forward = get_forward(camera);
    camera->up = get_up(camera);
    camera->right = get_right(camera);
}

static float clamp(float m, float x, float M) {
    return (float) fmax(m, fmin(x, M));
}


bool update_camera(Camera* camera, Input* input, float time_s) {
    const float acceleration = 500.0f + 1500.0f * ((float) input_is_down(input, 0x11));
    const float friction = 10.0f;

    bool adjust_fov = input->mouse.current.wheel != 0.0f;
    if (adjust_fov) {
        camera->w_fov -= 0.1f * ((float) input->mouse.current.wheel);
        camera->w_fov = clamp(0.7f, camera->w_fov, 2.0f);
        camera->h_fov = get_h_fov(camera);
    }
    
    bool adjust_rotation = input->mouse.current.delta_x != 0 || input->mouse.current.delta_y != 0;
    if (adjust_rotation) {
        camera->yaw -= 0.01f * (float) input->mouse.current.delta_x;
        camera->pitch -= 0.01f * (float) input->mouse.current.delta_y;
        camera->pitch = clamp((float) -M_PI * 0.5f, camera->pitch, (float) M_PI * 0.5f);

        // Update cached direction vectors when rotation changes
        camera->forward = get_forward(camera);
        camera->up = get_up(camera);
        camera->right = get_right(camera);
    }

    float3 movement_direction = {0, 0, 0}; // Forward, Right, Up

    movement_direction.x -= (float) input_is_down(input, 'S');
    movement_direction.x += (float) input_is_down(input, 'W');

    movement_direction.y -= (float) input_is_down(input, 'A');
    movement_direction.y += (float) input_is_down(input, 'D');

    movement_direction.z -= (float) input_is_down(input, 16);
    movement_direction.z += (float) input_is_down(input, 32);

    bool accelerate = !is_zero_vector(movement_direction);
    bool move_camera = accelerate || !is_zero_vector(camera->velocity);
    
    if (move_camera) {
        normalize(&movement_direction);
        float3 acceleration_direction = add(add(multiply((float3) {cosf(camera->yaw), sinf(camera->yaw), 0.0f}, movement_direction.x), multiply((float3) {sinf(camera->yaw), -cosf(camera->yaw), 0.0f}, movement_direction.y)), multiply((float3) {0.0f, 0.0f, 1.0f}, movement_direction.z));

        float e = expf(-friction * time_s);

        increase(&camera->position, add( multiply(camera->velocity, time_s), multiply(acceleration_direction, acceleration * (time_s - (1 - e)/friction) / friction) ) );
        camera->velocity = add(multiply(camera->velocity, e), multiply(acceleration_direction, acceleration * (1 - e) / friction));
        if (is_near_zero(camera->velocity, 1.0f)) {
            camera->velocity = (float3) {0.0f, 0.0f, 0.0f};
        }
        
    }

    bool changed = adjust_fov || adjust_rotation || move_camera;
    if (changed) {
        camera->saved_frames = 0;
    }

    return changed;
}

// Utility to explicitly refresh cached vectors (if needed)
void camera_update_vectors(Camera* camera) {
    camera->forward = get_forward(camera);
    camera->up = get_up(camera);
    camera->right = get_right(camera);
}

float3 get_forward(Camera* camera) { // D
    float3 direction;
    direction.x = cos(camera->pitch)*cos(camera->yaw);
    direction.y = cos(camera->pitch)*sin(camera->yaw);
    direction.z = sin(camera->pitch);
    return direction;
}

float3 get_up(Camera* camera) { // H
    float3 direction;
    direction.x = -sin(camera->pitch)*cos(camera->yaw);
    direction.y = -sin(camera->pitch)*sin(camera->yaw);
    direction.z = cos(camera->pitch);
    return direction;
}

float3 get_right(Camera* camera) { // W
    float3 direction;
    direction.x = sin(camera->yaw);
    direction.y = -cos(camera->yaw);
    direction.z = 0;
    return direction;
}