#include "fmemory.h"
#include "core/logger.h"
#include "platform/platform.h"

struct memory_stats
{
    u64 total_allocated;
    u64 tagged_allocations[MEMORY_TAG_MAX_TAGS];
};
static struct memory_stats mem_stats;

void initialize_memory()
{
    platform_zero_memory(&mem_stats, sizeof(mem_stats));
}

void* fmem_allocate(u64 size, memory_tag mem_tag)
{
    if(mem_tag == MEMORY_TAG_UNKNOWN)
    {
        WARN("Unknown memory tag called fallocate function. ")
    }
}