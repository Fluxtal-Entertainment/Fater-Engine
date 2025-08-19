#pragma once
#include "defines.h"

/*
Memory layout
u64 capacity <- number of elements that can be held
u64 length <- number of elements that are currently contained
u64 stride <- size of each element in bytes
void* elements
*/

enum{
    DYNAMIC_ARRAY_CAPACITY,
    DYNAMIC_ARRAY_LENGTH,
    DYNAMIC_ARRAY_STRIDE,
    DYNAMIC_ARRAY_FIELD_LENGTH
};

FAPI void* _dynamic_array_create(u64 length, u64 stride);
FAPI void _dynamic_array_destroy(void* array);
FAPI u64 _dynamic_array_field_get(void* array, u64 field);
FAPI void _dynamic_array_field_set(void* array, u64 field, u64 value);
FAPI void* _dynamic_array_resize(void* array);
FAPI void* _dynamic_array_push(void* array, const void* value_ptr);
FAPI void _dynamic_array_pop(void* array, void* dest);
FAPI void* _dynamic_array_pop_at(void* array, u64 index, void* dest);
FAPI void* _dynamic_array_insert_at(void* array, u64 index, void* value_ptr);

#define DYNAMIC_ARRAY_DEFAULT_CAPACITY 1
#define DYNAMIC_ARRAY_RESIZE_FACTOR 2
#define dynamic_array_create(type) _dynamic_array_create(DYNAMIC_ARRAY_DEFAULT_CAPACITY, sizeof(type))
#define dynamic_array_reserve(type, capacity) _dynamic_array_create(capacity, sizeof(type))
#define dynamic_array_destroy(array) _dynamic_array_destroy(array)
#define dynamic_array_push(array, value) \
    {\
        typeof(value) temp = value;\
        array = _dynamic_array_push(array, &temp);\
    }
#define dynamic_array_pop(array, value_ptr) _dynamic_array_pop(array, value_ptr)
#define dynamic_array_insert_at(array, index, value)\
    {\
        typeof(value) temp = value;\
        array = _dynamic_array_insert_at(array, index, &temp);\
    }
#define dynamic_array_pop_at(array, index, value_ptr) _dynamic_array_pop_at(array, index, value_ptr)
#define dynamic_array_clear(array) _dynamic_array_field_set(array, DYNAMIC_ARRAY_LENGTH, 0)
#define dynamic_array_capacity(array) _dynamic_array_field_get(array, DYNAMIC_ARRAY_CAPACITY)
#define dynamic_array_length(array) _dynamic_array_field_get(array, DYNAMIC_ARRAY_LENGTH)
#define dynamic_array_stride(array) _dynamic_array_field_get(array, DYNAMIC_ARRAY_STRIDE)
#define dynamic_array_length_set(array, value) _dynamic_array_field_set(array, DYNAMIC_ARRAY_LENGTH, value)
