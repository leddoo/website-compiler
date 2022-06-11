#pragma once

#include <libcpp/base.hpp>
#include <libcpp/memory/allocator.hpp>

namespace libcpp {

    struct Heap : public Allocator {

        union Chunk {
            Chunk *next_idle;

            struct Used {
                Used *next;
                U8   *address;
                Usize offset;
                Usize size;
            } used;

            struct Free {
                Free *next;
                U8   *address;
                Usize size;
            } free;
        };

        struct Block {
            Block *next;
            Chunk::Used *first_used_lru;
            Chunk::Free *first_free_by_address;
            Chunk *idle_list_by_address;
            Usize chunk_count;
            Usize used, size;
        };


        Allocator *allocator;
        Block *blocks;
        Usize default_block_size;
        Usize default_chunk_count;
        Usize split_threshold;
    };

    struct Heap_Statistics {
        Usize block_count;
        Usize total_overhead;
        Usize total_used;
        Usize total_free;
        Usize total_allocated;

        Usize used_chunk_count;
        Usize free_chunk_count;
        Usize total_chunk_count;
    };


    Heap create_heap(
        Allocator &backing = default_allocator,
        Usize default_block_size = LIBCPP_HEAP_DEFAULT_BLOCK_SIZE,
        Usize default_chunk_count = LIBCPP_HEAP_DEFAULT_CHUNK_COUNT,
        Usize split_threshold = LIBCPP_HEAP_DEFAULT_SPLIT_THRESHOLD
    );
    void destroy(Heap &heap);

    bool check(Heap &heap, Heap_Statistics *statistics = NULL);

    void *heap_allocate(Allocator *data, Usize size, Usize alignment);
    void heap_free(Allocator *data, void *allocation);

}

