#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "application.h"
#include "../core/time.h"
#include "../core/input.h"
#include "../core/window.h"
#include "../renderer/renderer.h"
#include "../scene/scene.h"
#include "../scene/scene_gpu.h"
#include "../camera/camera.h"
#include "../compute/compute.h"


static void app_update(Application* app) {
    if (app->window.should_close) {
        app_request_exit(app);
    }
    if (update_camera(&app->camera, &app->input, (float) app->delta_time_s)) {
        app->scene_gpu.is_dirty = true;
    }
    if (input_mouse_went_down(&app->input, 0)) {
        int x;
        int y;
        input_get_mouse_position(&app->input, &x, &y);
        printf("CLICK! Mouse at: %i, %i\n", x, y);
        app->camera.x = x;
        app->camera.y = y;
    }
    
}

static void app_render(Application* app) {
    if (app->window.resized && app->window.width > 0 && app->window.height > 0) {
        printf("Window resized to %ux%u, recreating swapchain...\n", app->window.width, app->window.height);
        renderer_recreate_swapchain(&app->renderer);
        app->camera.width = app->window.width;
        app->camera.height = app->window.height;
        app->camera.saved_frames = 0;
        app->scene_gpu.is_dirty = true;
    }
    
    uint32_t image_index;
    
    if (!renderer_begin_frame(&app->renderer, &image_index, &app->camera, &app->scene_gpu)) {
        return;
    }
    renderer_draw_frame(&app->renderer, image_index);
    
    renderer_end_frame(&app->renderer, image_index);
    app->camera.saved_frames += 1;
}


void app_init(Application* app) {
    app->is_running = false;
    app->target_fps = 144;
    app->last_time_s = get_current_time_seconds();
    app->delta_time_s = 0.0;
    input_init(&app->input);

    // Backend de cómputo (CUDA en Win32, Metal en macOS).
    if (!compute_init()) {
        fprintf(stderr, "Failed to initialize compute backend.\n");
        exit(EXIT_FAILURE);
    }

    if (!window_init(&app->window, 800, 600, "C Application Window", &app->input)) {
        fprintf(stderr, "Failed to initialize window.\n");
        exit(EXIT_FAILURE);
    }
    if (!scene_innit(&app->scene)) {
        fprintf(stderr, "Failed to initialize scene.\n");
        exit(EXIT_FAILURE);
    }

    if (!scene_gpu_create(&app->scene_gpu, &app->scene)) {
        fprintf(stderr, "Failed to create scene GPU.\n");
        exit(EXIT_FAILURE);
    };

    camera_innit(&app->camera, app->window.width, app->window.height);

    renderer_init(&app->renderer, &app->window);

    printf("Application initialized.\n");
}

void app_run(Application* app) {
    const double frame_delay_s = 1.0 / (double) app->target_fps;

    app->is_running = true;
    printf("Application is running...\n");

    // Estadísticas acumuladas durante la ventana de medición.
    const double stats_window_s = 10.0;
    double stats_window_start_s = get_current_time_seconds();
    double frames_time_sum_s    = 0.0;
    uint64_t frames_count       = 0;

    while (app->is_running) {
        double frame_start_s = get_current_time_seconds();
        
        app->delta_time_s = (frame_start_s - app->last_time_s);
        app->last_time_s = frame_start_s;
        input_begin_frame(&app->input);
        window_poll_events(&app->window);
        app_update(app);
        app_render(app);

        double frame_time = get_current_time_seconds() - frame_start_s;

        if (frame_time < frame_delay_s) {
            sleep_s(frame_delay_s - frame_time);
        }

        double end_frame = get_current_time_seconds();
        frames_time_sum_s += (end_frame - frame_start_s);
        frames_count += 1;

        // ¿Pasó la ventana de medición?
        if (end_frame - stats_window_start_s >= stats_window_s) {
            const double avg_s = frames_time_sum_s / (double)frames_count;
            printf("Average frame time over last %.0f s (%llu frames): %.2f ms (%.2f FPS)\n",
                   stats_window_s,
                   (unsigned long long)frames_count,
                   1000.0 * avg_s,
                   1.0 / avg_s);

            // Resetear la ventana.
            stats_window_start_s = end_frame;
            frames_time_sum_s    = 0.0;
            frames_count         = 0;
        }
    }
}

void app_shutdown(Application* app) {
    printf("Application shutting down.\n");
    renderer_shutdown(&app->renderer);
    scene_destroy(&app->scene);
    scene_gpu_destroy(&app->scene_gpu);
    window_destroy(&app->window);
    compute_shutdown();
}

void app_request_exit(Application* app) {
    app->is_running = false;
}