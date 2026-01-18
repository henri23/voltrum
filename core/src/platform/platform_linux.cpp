#include "defines.hpp"

#ifdef PLATFORM_LINUX

#    include "platform.hpp"
#    include <sys/mman.h>
#    include <unistd.h>

void *platform_virtual_reserve(u64 size) {
    auto block = mmap(NULL, size, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (block == MAP_FAILED) {
        return nullptr;
    }
    return block;
}

void platform_virtual_release(void *block, u64 size) {
    if (block) {
        munmap(block, size);
    }
}

u64 platform_query_page_size() { return (u64)sysconf(_SC_PAGESIZE); }

#endif
