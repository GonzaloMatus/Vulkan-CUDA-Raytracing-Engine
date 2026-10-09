#pragma once
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

bool compute_init(void);
void compute_shutdown(void);

bool compute_alloc(void** ptr, size_t size);
bool compute_free(void* ptr);

bool compute_memcpy_to(void* dst_device, const void* src_host, size_t size);
bool compute_memcpy_from(void* dst_host, const void* src_device, size_t size);
bool compute_memset(void* ptr, int value, size_t size);

bool compute_sync(void);

// Devuelve el handle nativo del buffer de cómputo, para pasarlo al backend
// de cómputo como MTLBuffer (Mac) o como HANDLE (Win32, no usado).
void* compute_get_native_buffer(void* device_ptr);

#ifdef __cplusplus
}
#endif