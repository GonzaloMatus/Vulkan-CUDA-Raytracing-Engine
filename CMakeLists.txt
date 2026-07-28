cmake_minimum_required(VERSION 3.21)

project(engine
    VERSION 0.1
    LANGUAGES C CXX CUDA
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CUDA_STANDARD 14)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_ARCHITECTURES native)

add_executable(engine
    src/main.c
    src/app/application.c
    src/core/time.c
    src/core/input.c
    src/core/window.c
    src/renderer/renderer.c
    src/renderer/vulkan/vk_context.c
    src/renderer/swapchain/swapchain.c
    src/renderer/command_buffers/command_buffers.c
    src/renderer/sync/sync.c
    src/renderer/render_target/render_target.c
    src/renderer/cuda_renderer/cuda_renderer.c
    src/renderer/gpu_render_buffer/gpu_render_buffer.c
    src/renderer/render_target_hdr/render_target_hdr.c
    src/renderer/tonemap_pipeline/tonemap_pipeline.c
    src/scene/scene.c
    src/scene/sphere/sphere.c
    src/vector/float3.c
    src/camera/camera.c
    src/scene/scene_gpu.c
    src/cuda_api/cuda_api.cu
    src/renderer/raygen_kernel/raygen_kernel.cu
    src/scene/triangle/triangle.c
    src/scene/box/box.c
    src/scene/cylinder/cylinder.c
    src/scene/material/material.c
    src/scene/bvh/object.c
    src/scene/bvh/node.c
    src/scene/bvh/bvh.c
    src/scene/aabb/aabb.c
    src/scene/primitives.c
    src/scene/bvh/bvh_gpu.c
    src/scene/bvh/node_gpu.c
)

set_target_properties(engine PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
)

find_package(Vulkan REQUIRED)
target_link_libraries(engine Vulkan::Vulkan)

target_include_directories(engine PRIVATE
    src
)
