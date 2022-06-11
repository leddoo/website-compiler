#include <libcpp/memory/heap.hpp>
#include <libcpp/util/math.hpp>

namespace libcpp {

//
// RANGE helpers.
//

    static Usize block_size_overhead(Usize chunk_count) {
        auto result =
              sizeof(Heap::Block)
            + sizeof(Heap::Chunk)*chunk_count;
        return result;
    }

    constexpr Usize block_empty_alignment = alignof(Heap::Chunk);

    static Heap::Block *create_block(Usize size, Usize chunk_count, Allocator &allocator) {
        static_assert(alignof(Heap::Block) == alignof(Heap::Chunk), "");
        auto block_offset = (Usize)0;
        auto chunks_offset = block_offset + sizeof(Heap::Block);
        auto start_offset = chunks_offset + sizeof(Heap::Chunk)*chunk_count;

        assert(size > start_offset);

        auto memory = (U8 *)allocate_uninitialized(size, alignof(Heap::Block), allocator);
        auto remaining = size - start_offset;

        auto block_pointer  = (Heap::Block *)(memory + block_offset);
        auto chunks_pointer = (Heap::Chunk *)(memory + chunks_offset);

        // NOTE(llw): Initialize chunks.
        set_values(chunks_pointer, {}, chunk_count);

        auto first_free = &chunks_pointer->free;
        first_free->address = memory + start_offset;
        first_free->size = remaining;

        Heap::Chunk *idle_list;
        {
            auto begin = chunks_pointer + 1;
            auto end   = chunks_pointer + chunk_count;
            idle_list = begin;

            for(auto current = begin + 1; current < end; current += 1) {
                current[-1].next_idle = current;
            }
        }

        // NOTE(llw): Initialize block.
        auto &block = *block_pointer;
        block = {};
        block.first_free_by_address = first_free;
        block.idle_list_by_address = idle_list;
        block.chunk_count = chunk_count;
        block.used = start_offset;
        block.size = size;
        return &block;
    }


    static void insert_into_idle_list(
        Heap::Chunk::Free *free_chunk, Heap::Block *block
    ) {
        auto chunk = (Heap::Chunk *)free_chunk;
        *chunk = {};

        // NOTE(llw): Store idle chunks sorted by address to ensure new
        // used/free chunks will be dense in memory (near the front of the
        // designated region).
        auto current = block->idle_list_by_address;
        auto previous = current;
        while(current != NULL && chunk > current) {
            previous = current;
            current = current->next_idle;
        }

        // NOTE(llw): Reached insertion point.
        if(previous != current) {
            previous->next_idle = chunk;
            chunk->next_idle = current;
        }
        else {
            chunk->next_idle = block->idle_list_by_address;
            block->idle_list_by_address = chunk;
        }
    }

    static void insert_into_free_list(
        Heap::Chunk::Free *chunk, Heap::Block *block
    ) {
        /* NOTE(llw): Merging.
            - Invariant: Consecutive chunks in free list are sorted by
              increasing start address, cannot be merged, and don't overlap
              (for obvious reasons).
            - Only have to check if current and chunk (and potentially
              current->next) can be merged. Previous was current in the
              preceeding iteration or is equal to current in the first
              iteration.
            - current->next only needs to be checked if chunk is on the right.
              Otherwise the invariant ensures that current and next cannot be
              merged.
        */
        auto current = block->first_free_by_address;
        auto previous = current;
        while(current != NULL) {

            //    [ chunk ][ current ] # [ next ]
            // -> [ current          ] # [ next ]
            if(chunk->address + chunk->size == current->address) {
                current->address = chunk->address;
                current->size += chunk->size;

                insert_into_idle_list(chunk, block);
                return;
            }

            //    [ current ][ chunk ] ? [ next ]
            // -> [ current          ] # [ next ]
            // or [ current                     ]
            if(current->address + current->size == chunk->address) {

                // NOTE(llw): Merge chunk into current.
                current->size += chunk->size;
                insert_into_idle_list(chunk, block);

                // NOTE(llw): Check if enlarged current can be merged with next.
                auto next = current->next;
                if(    next != NULL
                    && (current->address + current->size == next->address)
                ) {
                    current->size += next->size;
                    current->next = next->next;
                    insert_into_idle_list(next, block);
                }

                return;
            }

            if(current->address > chunk->address) {
                break;
            }

            previous = current;
            current = current->next;
        }

        // NOTE(llw): Reached end of list or insertion point (without being
        // able to merge).
        if(previous != current) {
            previous->next = chunk;
            chunk->next = current;
        }
        else {
            chunk->next = block->first_free_by_address;
            block->first_free_by_address = chunk;
        }
    }


    static void *use_chunk(
        Heap::Block *block,
        Heap::Chunk::Free *previous, Heap::Chunk::Free *chunk,
        Usize size, Usize alignment_offset,
        Usize split_threshold
    ) {
        assert(alignment_offset + size <= chunk->size);

        // NOTE(llw): Remove chunk from free list.
        if(previous != NULL) {
            previous->next = chunk->next;
        }
        else {
            block->first_free_by_address = chunk->next;
        }


        auto used_address = chunk->address;
        auto used_size    = chunk->size;
        auto result       = used_address + alignment_offset;

        // NOTE(llw): Attempt to split chunk.
        auto min_size  = (alignment_offset + size);
        auto remaining = used_size - min_size;
        if(    block->idle_list_by_address != NULL
            && remaining >= split_threshold
        ) {
            auto &idle_list = block->idle_list_by_address;

            auto split = (Heap::Chunk::Free *)idle_list;
            idle_list = idle_list->next_idle;

            *split = {};
            split->address = used_address + min_size;
            split->size = remaining;

            insert_into_free_list(split, block);

            used_size -= remaining;
        }

        // NOTE(llw): Update block size.
        block->used += used_size;

        // NOTE(llw): Convert chunk to used chunk.
        auto used = (Heap::Chunk::Used *)chunk;
        *used = {};
        used->address = used_address;
        used->size    = used_size;
        used->offset  = alignment_offset;

        // NOTE(llw): Put at front of used list.
        used->next = block->first_used_lru;
        block->first_used_lru = used;

        return result;
    }



//
// RANGE Heap api.
//


    Heap create_heap(
        Allocator &backing,
        Usize default_block_size,
        Usize default_chunk_count,
        Usize split_threshold
    ) {
        auto result = Heap {};
        result.allocate = heap_allocate;
        result.free = heap_free;
        result.allocator = &backing;
        result.blocks = NULL;
        result.default_block_size = default_block_size;
        result.default_chunk_count = default_chunk_count;
        result.split_threshold = split_threshold;
        return result;
    }

    void destroy(Heap &heap) {
        auto block = heap.blocks;
        while(block != NULL) {
            auto next = block->next;
            free(block, *heap.allocator);
            block = next;
        }

        heap = {};
    }


    bool check(Heap &heap, Heap_Statistics *statistics) {
        if(statistics) {
            *statistics = {};
        }

        auto result = true;

        auto block = heap.blocks;
        while(block != NULL) {
            auto used_chunk_count = (Usize)0;
            auto free_chunk_count = (Usize)0;
            auto idle_chunk_count = (Usize)0;

            auto total_used = (Usize)0;
            auto total_free = (Usize)0;

            auto used = block->first_used_lru;
            while(used != NULL) {
                used_chunk_count += 1;
                total_used += used->size;

                used = used->next;
            }

            auto free = block->first_free_by_address;
            auto free_last_address = (U8 *)NULL;
            auto free_last_size    = (Usize)0;
            while(free != NULL) {
                free_chunk_count += 1;
                total_free += free->size;

                if(    free_last_address >= free->address
                    || free_last_address + free_last_size >= free->address
                ) {
                    result = false;
                }

                free_last_address = free->address;
                free_last_size    = free->size;

                free = free->next;
            }

            auto idle = block->idle_list_by_address;
            auto last_idle = (Heap::Chunk *)NULL;
            while(idle != NULL) {
                idle_chunk_count += 1;

                if(last_idle >= idle) {
                    result = false;
                }

                last_idle = idle;

                idle = idle->next_idle;
            }


            auto chunk_count = used_chunk_count + free_chunk_count + idle_chunk_count;
            if(chunk_count != block->chunk_count) {
                result = false;
            }

            auto overhead = block_size_overhead(block->chunk_count);
            total_used += overhead;
            if(total_used != block->used) {
                result = false;
            }

            auto total_size = total_used + total_free;
            if(total_size != block->size) {
                result = false;
            }

            if(statistics) {
                statistics->block_count += 1;
                statistics->total_overhead += overhead;
                statistics->total_used += total_used;
                statistics->total_free += total_free;
                statistics->total_allocated += total_size;

                statistics->used_chunk_count += used_chunk_count;
                statistics->free_chunk_count += free_chunk_count;
                statistics->total_chunk_count += chunk_count;
            }

            block = block->next;
        }

        return result;
    }


    void *heap_allocate(Allocator *data, Usize allocation_size, Usize alignment) {
        assert(is_power_of_two(alignment));

        auto &heap = *(Heap *)data;

        // NOTE(llw): Try to find chunk in existing blocks.
        auto block = heap.blocks;
        auto previous_block = block;
        while(block != NULL) {
            if(block->used + allocation_size <= block->size) {
                auto chunk    = block->first_free_by_address;
                auto previous = (Heap::Chunk::Free *)NULL;

                auto best_chunk         = (Heap::Chunk::Free *)NULL;
                auto best_previous      = (Heap::Chunk::Free *)NULL;
                auto best_fragmentation = (Usize)0;
                auto best_offset        = (Usize)0;

                // NOTE(llw): Find best-fit chunk.
                while(chunk != NULL) {
                    auto offset = alignment_offset((Usize)chunk->address, alignment);
                    auto required = allocation_size + offset;
                    auto fragmentation = chunk->size - required;

                    if(    required <= chunk->size
                        && (best_chunk == NULL || fragmentation < best_fragmentation)
                    ){
                        best_chunk         = chunk;
                        best_previous      = previous;
                        best_fragmentation = fragmentation;
                        best_offset        = offset;
                    }

                    chunk = chunk->next;
                }

                if(best_chunk != NULL) {
                    auto result = use_chunk(
                        block,
                        best_previous, best_chunk,
                        allocation_size, best_offset,
                        heap.split_threshold
                    );
                    return result;
                }
            }

            previous_block = block;
            block = block->next;
        }

        // NOTE(llw): No block found, create a new one.
        {
            auto block_size  = heap.default_block_size;
            auto chunk_count = heap.default_chunk_count;

            // NOTE(llw): Consider alignment.
            auto required_size = allocation_size;
            if(alignment > block_empty_alignment) {
                required_size += alignment - 1;
            }

            // NOTE(llw): Large block.
            auto overhead = block_size_overhead(chunk_count);
            if(allocation_size + overhead > block_size) {
                overhead = block_size_overhead(1);
                block_size = required_size + overhead;
                chunk_count = 1;
            }

            block = create_block(block_size, chunk_count, *heap.allocator);

            if(previous_block != NULL) {
                previous_block->next = block;
            }
            else {
                heap.blocks = block;
            }
        }

        // NOTE(llw): Allocate from new block.
        auto chunk = block->first_free_by_address;
        auto offset = alignment_offset((Usize)chunk->address, alignment);
        auto result = use_chunk(
            block,
            NULL, chunk,
            allocation_size, offset,
            heap.split_threshold
        );
        return result;
    }

    void heap_free(Allocator *data, void *allocation) {
        auto &heap = *(Heap *)data;

        auto block = heap.blocks;
        auto previous_block = block;
        while(block != NULL) {

            // NOTE(llw): Bounds check on block.
            auto begin = (U8 *)block + block_size_overhead(block->chunk_count);
            auto end   = (U8 *)block + block->size;
            if(allocation >= begin && allocation < end) {

                // NOTE(llw): Walk used list.
                auto current = block->first_used_lru;
                auto previous = current;
                while(current != NULL) {
                    if(current->address + current->offset == allocation) {

                        // NOTE(llw): Remove from used list.
                        if(previous != current) {
                            previous->next = current->next;
                        }
                        else {
                            block->first_used_lru = current->next;
                        }

                        auto address = current->address;
                        auto size    = current->size;

                        // NOTE(llw): Free chunk.
                        auto free = (Heap::Chunk::Free *)current;
                        *free = {};
                        free->address = address;
                        free->size    = size;

                        insert_into_free_list(free, block);

                        // NOTE(llw): Update block.
                        block->used -= size;

                        /* TODO(llw): Freeing blocks.
                            - Don't free immediately, that can get costly when
                              "alloc, free, alloc, free" is repeatedly called
                              and blocks are created and destroyed on every
                              call.
                            - Instead keep one empty block around, only free
                              secondary free blocks and large blocks.
                            - Does this actually work, or does it just move
                              the problem to another case?
                        */

                        return;
                    }

                    previous = current;
                    current = current->next;
                }

                // NOTE(llw): No chunk found in block.
                assert(false);
            }

            previous_block = block;
            block = block->next;
        }

        // NOTE(llw): No block found.
        assert(false);
    }
}

