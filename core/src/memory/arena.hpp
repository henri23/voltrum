#pragma once

#include "defines.hpp"

// Conservative number that ensures the Arena header struct can fit inside this
// size
constexpr u64 ARENA_HEADER_SIZE = 128;

struct Arena
{
    u64   committed_memory;
    u64   reserved_memory;
    u64   commit_granularity; // How many bytes for each commit
    u64   offset;
    void *memory;

    // Debug - No need to overkill with ifdef clause to disable this
    const char *allocation_file;
    int         allocation_line;
};

STATIC_ASSERT(sizeof(Arena) <= ARENA_HEADER_SIZE,
              "Arena header should be smaller that 128 bytes!");

constexpr u64 ARENA_DEFAULT_RESERVE_SIZE = 64 * MiB;
constexpr u64 ARENA_DEFAULT_COMMIT_SIZE  = 64 * KiB;

#define arena_create(...)                                                      \
    _arena_create(__FILE__, __LINE__ __VA_OPT__(, ) __VA_ARGS__)
Arena *_arena_create(const char *file,
                     s32         line,
                     u64         reserve_size = ARENA_DEFAULT_RESERVE_SIZE,
                     u64         commit_size  = ARENA_DEFAULT_COMMIT_SIZE);

void arena_release(Arena *arena);

#define push_struct(arena, type)                                               \
    (type *)_arena_push((arena), sizeof(type), MAX(8, alignof(type)))

#define push_array(arena, type, count)                                         \
    (type *)_arena_push((arena), sizeof(type) * count, MAX(8, alignof(type)))

#define push_array_aligned(arena, type, count, align)                          \
    (type *)_arena_push((arena), sizeof(type) * count, align)

void *_arena_push(Arena *arena, u64 size, u64 align, b8 should_zero = true);

// Popping functions
void arena_pop(Arena *arena, u64 size);
void arena_pop_to(Arena *arena, u64 position);

void arena_clear(Arena *arena);

// Temporary arenas
struct Scratch_Arena
{
    Arena *arena;
    u64    position;
};

// Temporary arena scope
Scratch_Arena arena_scratch_begin(Arena *arena);

void arena_scratch_end(Scratch_Arena scratch);
