#include "defines.hpp"

#ifdef PLATFORM_LINUX

#include "platform.hpp"
#include <sys/mman.h>
#include <unistd.h>

Virtual_Memory_Block platform_virtual_reserve(u64 size) {
    Virtual_Memory_Block block = {};
    // MAP_ANONYMOUS: OS lazily backs pages on first access
    // Pages are guaranteed zero-initialized by the kernel
    block.base = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (block.base != MAP_FAILED) {
        block.size = size;
    } else {
        block.base = nullptr;
    }
    return block;
}

void platform_virtual_release(Virtual_Memory_Block* block) {
    if (block && block->base) {
        munmap(block->base, block->size);
        block->base = nullptr;
        block->size = 0;
    }
}

u64 platform_get_page_size() {
    return (u64)sysconf(_SC_PAGESIZE);
}

#endif
