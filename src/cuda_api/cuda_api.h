#pragma once
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void* handle;   // cudaExternalMemory_t
} CudaExternalMemory;

bool cuda_alloc(void** ptr, size_t size);
bool cuda_free(void* ptr);
bool cuda_memcpy_to_device(void* dst, const void* src, size_t size);
bool cuda_memcpy_to_host(void* dst, const void* src, size_t size);
bool cuda_memset(void *devPtr, int value, size_t count);

bool cuda_import_external_memory_win32(CudaExternalMemory* out_mem, void* win32_handle, size_t size);

bool cuda_external_memory_get_buffer(void** device_ptr, CudaExternalMemory mem, size_t size);

void cuda_external_memory_destroy(CudaExternalMemory* mem);

#ifdef __cplusplus
}
#endif
