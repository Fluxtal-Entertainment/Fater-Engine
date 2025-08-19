#include "core/f_memory.h"
#include "core/f_string.h"
#include <string.h>

u64 string_length(const char * str)
{
    return strlen(str);
}

char* string_duplicate(const char* str)
{
    u64 length = string_length(str);
    char* copy = mem_allocate(length + 1, MEMORY_TAG_STRING);
    mem_copy(copy, str, length + 1);
    return copy;
}