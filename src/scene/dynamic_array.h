#pragma once

#include <stdlib.h>
#include <string.h>


#define DynamicArray(T) \
typedef struct { \
    T* data; \
    size_t size; \
    size_t capacity; \
} T##_list; \
\
static T##_list T##_list_create(size_t initial_capacity) { \
    T##_list da = {0}; \
    if (initial_capacity == 0) initial_capacity = 1; \
    da.data = (T*)malloc(sizeof(T) * initial_capacity); \
    if (da.data) da.capacity = initial_capacity; \
    return da; \
} \
\
static void T##_list_free(T##_list* da) { \
    free(da->data); \
    da->data = NULL; \
    da->size = da->capacity = 0; \
} \
\
static int T##_list_push(T##_list* da, T item) { \
    if (da->size >= da->capacity) { \
        size_t new_capacity = da->capacity * 2; \
        T* new_data = (T*)realloc(da->data, sizeof(T) * new_capacity); \
        if (!new_data) return 0; \
        da->data = new_data; \
        da->capacity = new_capacity; \
    } \
    da->data[da->size++] = item; \
    return 1; \
} \
\
static T T##_list_pop(T##_list* da) { \
    if (da->size > 0) return da->data[--da->size]; \
    T tmp; memset(&tmp, 0, sizeof(T)); \
    return tmp; \
} \
\
static T* T##_list_at(T##_list* da, size_t index) { \
    return (index < da->size) ? &da->data[index] : NULL; \
} \
\
static size_t T##_list_size(const T##_list* da) { \
    return da->size; \
} \
\
static size_t T##_list_capacity(const T##_list* da) { \
    return da->capacity; \
} \
\
static int T##_list_empty(const T##_list* da) { \
    return da->size == 0; \
} \
\
static void T##_list_clear(T##_list* da) { \
    da->size = 0; \
} \
\
static int T##_list_reserve(T##_list* da, size_t new_capacity) { \
    if (new_capacity <= da->capacity) return 1; \
    T* new_data = (T*)realloc(da->data, sizeof(T) * new_capacity); \
    if (!new_data) return 0; \
    da->data = new_data; \
    da->capacity = new_capacity; \
    return 1; \
} \
\
static void T##_list_shrink_to_fit(T##_list* da) { \
    if (da->size == 0) { \
        free(da->data); \
        da->data = NULL; \
        da->capacity = 0; \
    } else if (da->size < da->capacity) { \
        T* new_data = (T*)realloc(da->data, sizeof(T) * da->size); \
        if (new_data) { \
            da->data = new_data; \
            da->capacity = da->size; \
        } \
    } \
} \
\
static T* T##_list_begin(T##_list* da) { \
    return da->data; \
} \
\
static T* T##_list_end(T##_list* da) { \
    return da->data + da->size; \
} \
\
static const T* T##_list_cbegin(const T##_list* da) { \
    return da->data; \
} \
\
static const T* T##_list_cend(const T##_list* da) { \
    return da->data + da->size; \
}