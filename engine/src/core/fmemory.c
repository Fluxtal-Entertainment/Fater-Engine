#include "core/fmemory.h"
#include "core/logger.h"
#include "platform/platform.h"
#include <string.h> //NOTE: Will be replaced by custom string library in the future
#include <stdio.h>

struct memory_stats
{
    u64 total_allocated;
    u64 tagged_allocations[MEMORY_TAG_MAX_TAGS];
};
static const char* memory_tag_strings[MEMORY_TAG_MAX_TAGS] = {"UNKNOWN", "ARRAY", "DARRAY", "DICT", "RING_QUEUE", "BST", "STRING", "APPLICATION", "JOB", "TEXTURE", "MATERIAL_INSTANCE", "RENDERER", "GAME", "TRANSFORM", "ENTITY", "ENTITY_NODE" , "SCENE"};
static struct memory_stats mem_stats;

void initialize_memory()
{
    platform_zero_memory(&mem_stats, sizeof(mem_stats));
}

void shutdown_memory()
{

}

void* mem_allocate(u64 size, memory_tag mem_tag)
{
    if(mem_tag == MEMORY_TAG_UNKNOWN)
    {
        WARN("Unknown memory tag called mem_allocate(). Re-class this allocation.")
    }
    mem_stats.total_allocated += size;
    mem_stats.tagged_allocations[mem_tag] += size;
    //TODO: Add memory alignment here
    void* block = platform_allocate(size, FALSE);
    platform_zero_memory(block, size);
    return block;
}

void mem_free(void* block, u64 size, memory_tag mem_tag)
{
    if(mem_tag == MEMORY_TAG_UNKNOWN)
    {
        WARN("Unknown memory tag called mem_free(). Re-class this allocation.")
    }
    mem_stats.total_allocated -= size;
    mem_stats.tagged_allocations[mem_tag] -= size;
    //TODO: Add memory alignment here
    platform_free(block, FALSE);
}

void* mem_zero(void* block, u64 size)
{
    return platform_zero_memory(block, size);
}

void* mem_copy(void* dest, const void* source, u64 size)
{
    return platform_copy_memory(dest, source, size);
}

void* mem_set(void* dest, i32 value, u64 size)
{
    return platform_set_memory(dest, value, size);
}

char* get_memory_usage_string()
{
    const u64 kib = 1024;
    const u64 mib = 1048576;//1024 * 1024
    const u64 gib = 1073741824;//NOTE: Literally 1024 * 1024 * 1024 
    char buffer[8000] = "System memory use (tagged):\n";
    u64 offset = strlen(buffer);
    for(u32 i = 0; i < MEMORY_TAG_MAX_TAGS; i++)
    {
        char unit[4] = "?iB";
        float amount = 1.0f;
        if(mem_stats.tagged_allocations[i] >= gib)
        {
            unit[0] = 'G';
            amount = mem_stats.tagged_allocations[i] / (float)gib;
        }
        else if(mem_stats.tagged_allocations[i] >= mib)
        {
            unit[0] = 'M';
            amount = mem_stats.tagged_allocations[i] / (float)mib;
        }
        else if(mem_stats.tagged_allocations[i] >= kib)
        {
            unit[0] = 'K';
            amount = mem_stats.tagged_allocations[i] / (float)kib;
        }
        else
        {
            unit[0] = 'B';
            unit[1] = 0;
            amount = (float)mem_stats.tagged_allocations[i];
        }
        i32 length = snprintf(buffer + offset, 8000, " %s: %.2f%s\n", memory_tag_strings[i], amount, unit);
        offset += length;
    }
    char* out_string = _strdup(buffer);
    return out_string;
}