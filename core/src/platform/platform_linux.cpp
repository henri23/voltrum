#include "defines.hpp"

#ifdef PLATFORM_LINUX

#    include "platform.hpp"
#    include <sys/mman.h>
#    include <unistd.h>

void *
platform_virtual_memory_reserve(u64 size)
{
    auto block = mmap(NULL, size, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (block == MAP_FAILED)
    {
        return nullptr;
    }
    return block;
}

b8
platform_virtual_memory_commit(void *block, u64 size)
{
    if (mprotect(block, size, PROT_READ | PROT_WRITE) != 0)
    {
        return false;
    }
    return true;
}

void
platform_virtual_memory_release(void *block, u64 size)
{
    if (block)
    {
        munmap(block, size);
    }
}

void
platform_virtual_memory_decommit(void *block, u64 size)
{
    if (block)
    {
        madvise(block, size, MADV_DONTNEED);
        mprotect(block, size, PROT_NONE);
    }
}

Platform_System_Info
platform_query_system_info()
{
    Platform_System_Info info = {};
    info.logical_processor_count = (u32)sysconf(_SC_NPROCESSORS_ONLN);
    info.page_size               = (u64)sysconf(_SC_PAGESIZE);
    info.large_page_size         = 0;
    info.allocation_granularity  = info.page_size;

    return info;
}

#endif
