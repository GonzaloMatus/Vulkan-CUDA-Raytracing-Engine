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
    set(CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE}   /O2 /fp:fast /arch:AVX2")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /fp:fast /arch:AVX2")
    set(CMAKE_C_FLAGS_DEBUG     "${CMAKE_C_FLAGS_DEBUG}     /arch:AVX2")
    set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG}   /arch:AVX2")

    set(CMAKE_CUDA_FLAGS_RELEASE "${CMAKE_CUDA_FLAGS_RELEASE} --use_fast_math -Xcompiler=/O2,/fp:fast,/arch:AVX2")
    set(CMAKE_CUDA_FLAGS_DEBUG   "${CMAKE_CUDA_FLAGS_DEBUG}   -Xcompiler=/arch:AVX2")

elseif(APPLE)
    set(CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE}   -O3 -ffast-math -march=native")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -ffast-math -march=native")
    set(CMAKE_C_FLAGS_DEBUG     "${CMAKE_C_FLAGS_DEBUG}     -march=native")
    set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG}   -march=native")

else()
    set(CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE}   -O3 -ffast-math -march=native")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -ffast-math -march=native")
    set(CMAKE_C_FLAGS_DEBUG     "${CMAKE_C_FLAGS_DEBUG}     -march=native")
    set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG}   -march=native")

    set(CMAKE_CUDA_FLAGS_RELEASE "${CMAKE_CUDA_FLAGS_RELEASE} --use_fast_math -Xcompiler=-O3,-ffast-math,-march=native")
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
    src/scene/primitives_gpu.c
    src/scene/bvh/bvh_gpu.c
    src/scene/bvh/node_gpu.c
)

# --- Fuentes específicas de plataforma ---
if(WIN32)
    list(APPEND ENGINE_SOURCES
        src/core/window_win32.c
        src/renderer/vulkan/vk_platform_win32.c
        # Compute backend (CUDA)
        src/compute/compute_cuda.c
        src/compute/raytracer_cuda.cu
        src/cuda_api/cuda_api.cu
        src/renderer/raygen_kernel/raygen_kernel.cu
    )

elseif(APPLE)
    list(APPEND ENGINE_SOURCES
        src/core/window_cocoa.m
        src/renderer/vulkan/vk_platform_cocoa.m
        # Compute backend (Metal)
        src/compute/compute_metal.m
        src/compute/raytracer_metal.m
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
if(APPLE AND DEFINED ENV{VULKAN_SDK})
    set(VULKAN_SDK "$ENV{VULKAN_SDK}" CACHE PATH "Vulkan SDK" FORCE)
    set(CMAKE_PREFIX_PATH "$ENV{VULKAN_SDK}" ${CMAKE_PREFIX_PATH})
endif()

find_package(Vulkan REQUIRED)
target_link_libraries(engine PRIVATE Vulkan::Vulkan)

# =============================================================================
# Directorios de shaders
# =============================================================================
# Los .spv (SPIR-V, GLSL) y los .metallib (Metal) van a build/shaders/.
# Los .metal fuente viven junto al código que los usa (raygen_kernel/).

set(SHADER_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SHADER_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)

file(MAKE_DIRECTORY ${SHADER_BIN_DIR})

# =============================================================================
# Shaders SPIR-V (GLSL) — comunes a Windows y macOS
# =============================================================================

set(TONEMAP_COMP ${SHADER_SRC_DIR}/tonemap.comp)
set(TONEMAP_SPV  ${SHADER_BIN_DIR}/tonemap.spv)

find_program(GLSLC_EXECUTABLE glslc
    HINTS ENV VULKAN_SDK
    PATH_SUFFIXES bin
)

if(NOT GLSLC_EXECUTABLE)
    message(FATAL_ERROR "glslc no encontrado. Configura VULKAN_SDK o añade su bin/ al PATH.")
endif()

add_custom_command(
    OUTPUT  ${TONEMAP_SPV}
    COMMAND ${GLSLC_EXECUTABLE} ${TONEMAP_COMP} -o ${TONEMAP_SPV}
    DEPENDS ${TONEMAP_COMP}
    COMMENT "Compilando tonemap.comp → tonemap.spv"
    VERBATIM
)

add_custom_target(spirv_shaders ALL DEPENDS ${TONEMAP_SPV})
add_dependencies(engine spirv_shaders)

# =============================================================================
# Metal kernels — solo en macOS
# =============================================================================
# La fuente .metal vive en src/renderer/raygen_kernel/, junto a su hermano
# raygen_kernel.cu. Los .air y .metallib van a build/shaders/.

if(APPLE)
    set(RAYGEN_METAL    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/raygen_kernel/raygen.metal)
    set(RAYGEN_AIR      ${SHADER_BIN_DIR}/raygen.air)
    set(RAYGEN_METALLIB ${SHADER_BIN_DIR}/raygen.metallib)

    find_program(XCRUN_EXECUTABLE xcrun)
    if(NOT XCRUN_EXECUTABLE)
        message(FATAL_ERROR "xcrun no encontrado (necesario para compilar Metal)")
    endif()

    add_custom_command(
        OUTPUT  ${RAYGEN_AIR}
        COMMAND ${XCRUN_EXECUTABLE} -sdk macosx metal
                -c ${RAYGEN_METAL} -o ${RAYGEN_AIR}
        DEPENDS ${RAYGEN_METAL}
        COMMENT "Compilando raygen.metal → raygen.air"
        VERBATIM
    )

    add_custom_command(
        OUTPUT  ${RAYGEN_METALLIB}
        COMMAND ${XCRUN_EXECUTABLE} -sdk macosx metallib
                ${RAYGEN_AIR} -o ${RAYGEN_METALLIB}
        DEPENDS ${RAYGEN_AIR}
        COMMENT "Enlazando raygen.air → raygen.metallib"
        VERBATIM
    )

    add_custom_target(metal_shaders ALL DEPENDS ${RAYGEN_METALLIB})
    add_dependencies(engine metal_shaders)
endif()

# =============================================================================
# Copiar todos los shaders compilados junto al ejecutable
# =============================================================================

if(APPLE)
    set(ALL_SHADER_OUTPUTS ${TONEMAP_SPV} ${RAYGEN_METALLIB})
else()
    set(ALL_SHADER_OUTPUTS ${TONEMAP_SPV})
endif()

add_custom_command(TARGET engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:engine>/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${ALL_SHADER_OUTPUTS}
            $<TARGET_FILE_DIR:engine>/shaders/
    COMMENT "Copiando shaders junto al ejecutable"
)

# Ruta absoluta de los shaders compilados, embebida en el binario.
# La usan tonemap_pipeline.c y raytracer_metal.m para cargar sus archivos.
target_compile_definitions(engine PRIVATE
    ENGINE_SHADER_DIR="$<TARGET_FILE_DIR:engine>/shaders"
)

# --- Frameworks nativos de macOS ---
if(APPLE)
    target_link_libraries(engine PRIVATE
        "-framework Cocoa"
        "-framework QuartzCore"
        "-framework Metal"
        "-framework MetalKit"
    )
endif()

# --- Includes ---
target_include_directories(engine PRIVATE
    src
)