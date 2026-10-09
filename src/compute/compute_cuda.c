#include "compute.h"
#include "../cuda_api/cuda_api.h"

bool compute_init(void)  { return true; }
void compute_shutdown(void) {}

bool compute_alloc(void** ptr, size_t size) {
    if (size == 0) {
        *ptr = NULL;
        return true;
    }
    return cuda_alloc(ptr, size);
}
bool compute_free(void* ptr)                             { return cuda_free(ptr); }
bool compute_memcpy_to(void* d, const void* s, size_t n) { return cuda_memcpy_to_device(d, s, n); }
bool compute_memcpy_from(void* d, const void* s, size_t n) { return cuda_memcpy_to_host(d, s, n); }
bool compute_memset(void* p, int v, size_t n)            { return cuda_memset(p, v, n); }
bool compute_sync(void)                                  { return cuda_sync(); }