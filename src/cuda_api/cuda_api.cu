#include "cuda_api.h"
#include <cuda_runtime.h>
#include <stdio.h>
#include <cuda.h>
#include <cuda_fp16.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CUDA_CHECK(x) do { \
    cudaError_t err = x; \
    if (err != cudaSuccess) { \
        printf("CUDA error %s:%d: %s\n", __FILE__, __LINE__, cudaGetErrorString(err)); \
    } \
} while (0)

#ifdef __cplusplus
extern "C" {
#endif

bool cuda_alloc(void** ptr, size_t size) {
    return cudaMalloc(ptr, size) == cudaSuccess;
}

bool cuda_memset(void *devPtr, int value, size_t count) {
    return cudaMemset(devPtr, value, count) == cudaSuccess;
}

bool cuda_free(void* ptr) {
    return cudaFree(ptr) == cudaSuccess;
}

bool cuda_memcpy_to_device(void* dst, const void* src, size_t size) {
    return cudaMemcpy(dst, src, size, cudaMemcpyHostToDevice) == cudaSuccess;
}

bool cuda_memcpy_to_host(void* dst, const void* src, size_t size) {
    return cudaMemcpy(dst, src, size, cudaMemcpyDeviceToHost) == cudaSuccess;
}

void cudaDeviceSynchronize_wrapper(void) {
    CUDA_CHECK(cudaDeviceSynchronize());
}

#ifdef __cplusplus
}
#endif

bool cuda_import_external_memory_win32(CudaExternalMemory* out_mem, void* win32_handle, size_t size) {
    cudaExternalMemoryHandleDesc mem_desc = {};
    mem_desc.type = cudaExternalMemoryHandleTypeOpaqueWin32;
    mem_desc.handle.win32.handle = win32_handle;
    mem_desc.size = size;

    cudaExternalMemory_t ext_mem = nullptr;
    CUDA_CHECK(cudaImportExternalMemory(&ext_mem, &mem_desc));

    out_mem->handle = ext_mem;
    return true;
}

bool cuda_external_memory_get_buffer(void** device_ptr, CudaExternalMemory mem, size_t size) {
    cudaExternalMemoryBufferDesc buffer_desc = {};
    buffer_desc.offset = 0;
    buffer_desc.size   = size;

    CUDA_CHECK(cudaExternalMemoryGetMappedBuffer(
        device_ptr,
        (cudaExternalMemory_t)mem.handle,
        &buffer_desc
    ));

    return true;
}

void cuda_external_memory_destroy(CudaExternalMemory* mem) {
    if (mem->handle) {
        cudaDestroyExternalMemory((cudaExternalMemory_t)mem->handle);
        mem->handle = nullptr;
    }
}
