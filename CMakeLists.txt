cmake_minimum_required(VERSION 3.21)

# Declaramos solo los lenguajes que existen en TODAS las plataformas.
# CUDA y OBJC se habilitan condicionalmente más abajo.
project(engine
    VERSION 0.1
    LANGUAGES C CXX
)

# --- Lenguajes específicos por plataforma ---
if(WIN32)
    enable_language(CUDA)
elseif(APPLE)
    enable_language(OBJC)
endif()

# --- Estándares ---
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    set(CMAKE_CUDA_STANDARD 14)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
    set(CMAKE_CUDA_ARCHITECTURES native)
endif()

# --- Banderas de compilación por compilador / plataforma ---
if(MSVC)
    # MSVC (Windows)
    set(CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE}   /O2 /fp:fast /arch:AVX2")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /fp:fast /arch:AVX2")
    set(CMAKE_C_FLAGS_DEBUG     "${CMAKE_C_FLAGS_DEBUG}     /arch:AVX2")
    set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG}   /arch:AVX2")

    set(CMAKE_CUDA_FLAGS_RELEASE "${CMAKE_CUDA_FLAGS_RELEASE} --use_fast_math -Xcompiler=/O2,/fp:fast,/arch:AVX2")
    set(CMAKE_CUDA_FLAGS_DEBUG   "${CMAKE_CUDA_FLAGS_DEBUG}   -Xcompiler=/arch:AVX2")

elseif(APPLE)
    # Clang en macOS (Apple Silicon e Intel)
    set(CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE}   -O3 -Ofast -ffast-math -march=native")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -Ofast -ffast-math -march=native")
    set(CMAKE_C_FLAGS_DEBUG     "${CMAKE_C_FLAGS_DEBUG}     -march=native")
    set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG}   -march=native")
    # No hay CUDA en macOS, así que no hay flags CMAKE_CUDA_*

else()
    # GCC/Clang en Linux
    set(CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE}   -O3 -Ofast -ffast-math -march=native")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -Ofast -ffast-math -march=native")
    set(CMAKE_C_FLAGS_DEBUG     "${CMAKE_C_FLAGS_DEBUG}     -march=native")
    set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG}   -march=native")

    set(CMAKE_CUDA_FLAGS_RELEASE "${CMAKE_CUDA_FLAGS_RELEASE} --use_fast_math -Xcompiler=-O3,-Ofast,-ffast-math,-march=native")
    set(CMAKE_CUDA_FLAGS_DEBUG   "${CMAKE_CUDA_FLAGS_DEBUG}   -Xcompiler=-march=native")
endif()

# --- Fuentes comunes a todas las plataformas ---
set(ENGINE_SOURCES
    src/main.c
    src/app/application.c
    src/core/time.c
    src/core/input.c
    src/renderer/renderer.c
    src/renderer/vulkan/vk_context.c
    src/renderer/swapchain/swapchain.c
    src/renderer/command_buffers/command_buffers.c
    src/renderer/sync/sync.c
    src/renderer/render_target/render_target.c
    src/renderer/gpu_render_buffer/gpu_render_buffer.c
    src/renderer/render_target_hdr/render_target_hdr.c
    src/renderer/tonemap_pipeline/tonemap_pipeline.c
    src/scene/scene.c
    src/scene/sphere/sphere.c
    src/vector/float3.c
    src/camera/camera.c
    src/scene/scene_gpu.c
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

# --- Fuentes específicas de plataforma ---
if(WIN32)
    list(APPEND ENGINE_SOURCES
        src/core/window_win32.c
        src/renderer/vulkan/vk_platform_win32.c
        # Backend CUDA
        src/renderer/cuda_renderer/cuda_renderer.c
        src/cuda_api/cuda_api.cu
        src/renderer/raygen_kernel/raygen_kernel.cu
    )

elseif(APPLE)
    list(APPEND ENGINE_SOURCES
        src/core/window_cocoa.m
        src/renderer/vulkan/vk_platform_cocoa.m
        # Backend Metal (a crear)
        # src/renderer/metal_renderer/metal_renderer.m
        # src/renderer/raygen_kernel/raygen_kernel.metal
    )

else()
    message(FATAL_ERROR "Plataforma no soportada todavía (solo Windows y macOS).")
endif()

# --- Ejecutable ---
add_executable(engine ${ENGINE_SOURCES})

# --- CUDA: solo en Windows ---
if(WIN32)
    set_target_properties(engine PROPERTIES
        CUDA_SEPARABLE_COMPILATION ON
    )
endif()

# --- Vulkan / MoltenVK ---
# En Windows, find_package(Vulkan) usa el SDK de LunarG.
# En macOS, requiere VULKAN_SDK apuntando al SDK de MoltenVK.
find_package(Vulkan REQUIRED)
target_link_libraries(engine PRIVATE Vulkan::Vulkan)

# --- Frameworks nativos de macOS ---
if(APPLE)
    target_link_libraries(engine PRIVATE
        "-framework Cocoa"
        "-framework QuartzCore"
        "-framework Metal"
        "-framework MetalKit"
    )

    # Opcional: empaquetar como .app real (Dock, menú, foco correcto).
    # Descomenta si quieres el bundle. Afecta la ruta del ejecutable final.
    # set_target_properties(engine PROPERTIES
    #     MACOSX_BUNDLE TRUE
    #     MACOSX_BUNDLE_GUI_IDENTIFIER "com.tuempresa.engine"
    #     MACOSX_BUNDLE_BUNDLE_VERSION "0.1"
    #     MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1"
    # )
endif()

# --- Includes ---
target_include_directories(engine PRIVATE
    src
)