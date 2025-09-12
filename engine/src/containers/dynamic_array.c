#include "containers/dynamic_array.h"
#include "core/f_memory.h"
#include "core/logger.h"

void* _dynamic_array_create(u64 length, u64 stride)
{
    u64 header_size = DYNAMIC_ARRAY_FIELD_LENGTH * sizeof(u64);
    u64 array_size = length * stride;
    u64* new_array = mem_allocate(header_size + array_size, MEMORY_TAG_DYNAMIC_ARRAY);
    mem_set(new_array, 0, header_size + array_size);
    new_array[DYNAMIC_ARRAY_CAPACITY] = length;
    new_array[DYNAMIC_ARRAY_LENGTH] = 0;
    new_array[DYNAMIC_ARRAY_STRIDE] = stride;
    return (void*)(new_array + DYNAMIC_ARRAY_FIELD_LENGTH);
}

void _dynamic_array_destroy(void* array)
{
    u64* header = (u64*)array - DYNAMIC_ARRAY_FIELD_LENGTH;
    u64 header_size = DYNAMIC_ARRAY_FIELD_LENGTH * sizeof(u64);
    u64 total_size = header_size + header[DYNAMIC_ARRAY_CAPACITY] * header[DYNAMIC_ARRAY_STRIDE];
    mem_free(header, total_size, MEMORY_TAG_DYNAMIC_ARRAY);
}

u64 _dynamic_array_field_get(void* array, u64 field)
{
    u64* header = (u64*)array - DYNAMIC_ARRAY_FIELD_LENGTH;
    return header[field];
}

void _dynamic_array_field_set(void* array, u64 field, u64 value)
{
    u64* header = (u64*)array - DYNAMIC_ARRAY_FIELD_LENGTH;
    header[field] = value;
}

void* _dynamic_array_resize(void* array)
{
    u64 length = dynamic_array_length(array);
    u64 stride = dynamic_array_stride(array);
    void* temp = _dynamic_array_create((DYNAMIC_ARRAY_RESIZE_FACTOR * dynamic_array_capacity(array)), stride);
    mem_copy(temp, array, length * stride);
    _dynamic_array_field_set(temp, DYNAMIC_ARRAY_LENGTH, length);
    _dynamic_array_destroy(array);
    return temp;
}

void* _dynamic_array_push(void* array, const void* value_ptr)
{
    u64 length = dynamic_array_length(array);
    u64 stride = dynamic_array_stride(array);
    if(length >= dynamic_array_capacity(array))
    {
        array = _dynamic_array_resize(array);
    }
    u64 address = (u64)array;
    address += (length * stride);
    mem_copy((void*)address, value_ptr, stride);
    _dynamic_array_field_set(array, DYNAMIC_ARRAY_LENGTH, length + 1);
    return array;
}

void _dynamic_array_pop(void* array, void* dest)
{
    u64 length = dynamic_array_length(array);
    u64 stride = dynamic_array_stride(array);
    u64 address = (u64)array;
    address += ((length - 1) * stride);
    mem_copy(dest, (void*)address, stride);
    _dynamic_array_field_set(array, DYNAMIC_ARRAY_LENGTH, length -1);
}

void* _dynamic_array_pop_at(void* array, u64 index, void* dest)
{
    u64 length = dynamic_array_length(array);
    u64 stride = dynamic_array_stride(array);
    if(index >= length)
    {
        ERROR_LOG("Index out of bounds of this array!!! Length: %i, index: %index", length, index);
        return array;
    }
    u64 address = (u64)array;
    mem_copy(dest, (void*)(address + (index * stride)), stride);
    //If not on the last element, snip out the entry and copy the rest inward
    if(index != length - 1)
    {
        mem_copy((void*)(address + (index * stride)), (void*)(address + ((index + 1) * stride)), stride * (length - index));
    }
    _dynamic_array_field_set(array, DYNAMIC_ARRAY_LENGTH, length - 1);
    return array;
}

void* _dynamic_array_insert_at(void* array, u64 index, void* value_ptr)
{
    u64 length = dynamic_array_length(array);
    u64 stride = dynamic_array_stride(array);
    if(index >= length)
    {
        ERROR_LOG("Index out of bounds of this array!!! Length: %i, index: %index", length, index);
        return array;
    }
    if(length >= dynamic_array_capacity(array))
    {
        array = _dynamic_array_resize(array);
    }
    u64 address = (u64)array;
    //If not on the last element, copy the rest outward
    if(index != length - 1)
    {
        mem_copy((void*)(address + ((index + 1) * stride)), (void*)(address + (index * stride)), stride * (length - index));
    }
    //Sets the value at the index
    mem_copy((void*)(address + (index * stride)), value_ptr, stride);
    _dynamic_array_field_set(array, DYNAMIC_ARRAY_LENGTH, length + 1);
    return array;
}