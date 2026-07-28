#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "../core/window.h"
#include "../core/input.h"
#include "../renderer/renderer.h"
#include "../scene/scene.h"
#include "../scene/scene_gpu.h"
#include "../camera/camera.h"

typedef struct {
    Input    input;
    Renderer renderer;
    Window   window;
    Scene    scene;
    SceneGPU scene_gpu;
    Camera camera;
    double last_time_s;
    double   delta_time_s;
    uint16_t target_fps;
    bool     is_running;
} Application;


void app_init(Application* app);
void app_run(Application* app);
void app_shutdown(Application* app);
void app_request_exit(Application* app);
