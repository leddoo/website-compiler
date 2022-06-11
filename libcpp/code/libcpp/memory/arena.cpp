#include <libcpp/memory/arena.hpp>
#include <libcpp/util/math.hpp>

namespace libcpp {

    static void remove_extra_allocations(Arena &arena, void *until_base = NULL);

    //
    // RANGE create/destroy.
    //

    Arena create_arena(Allocator &backing, Usize block_size) {
        auto result = Arena {};
        result.allocate   = arena_allocate;
        result.free       = arena_free;
        result.allocator  = &backing;
        result.block_size = block_size;

        return result;
    }

    void destroy(Arena &arena) {
        remove_extra_allocations(arena);

        if(arena.base != NULL) {
            free(arena.base, *arena.allocator);
        }

        arena = {};
    }


    //
    // RANGE allocate/free.
    //

    struct Arena_Marker {
        void *base;
        Usize capacity;
    };

    static void grow(Arena &arena, Usize capacity) {
        capacity = max(arena.block_size, capacity + sizeof(Arena_Marker));

        auto marker = Arena_Marker {
            arena.base,
            arena.capacity,
        };

        arena.capacity = capacity;
        arena.base = allocate_uninitialized(
            capacity, alignof(Arena_Marker),
            *arena.allocator
        );
        arena.used = sizeof(Arena_Marker);

        *(Arena_Marker *)arena.base = marker;
    }

    void *arena_allocate(Allocator *data, Usize size, Usize alignment) {
        assert(size > 0);
        assert(is_power_of_two(alignment));

        auto &arena = *(Arena *)data;

        auto offset = alignment_offset((Usize)arena.base + arena.used, alignment);
        auto effective_size = size + offset;

        // NOTE(llw): Grow if necessary
        if(arena.used + effective_size > arena.capacity) {
            grow(arena, size);

            offset = 0;
            effective_size = size;
        }

        #if LIBCPP_SLOW
            assert(arena.used + effective_size <= arena.capacity);
        #endif

        auto result = (Usize)arena.base + arena.used + offset;
        arena.used += effective_size;

        #if LIBCPP_SLOW
            // NOTE(llw): Make sure next allocation cannot overlap this one.
            //  (Might be redundant, just to make sure)
            assert((Usize)arena.base + arena.used >= result + size);

            // NOTE(llw): Check alignment
            assert(result % alignment == 0);
        #endif

        auto allocation = (void *)result;
        return allocation;
    }

    void arena_free(Allocator *data, void *allocation) {
        // NOTE(llw): NOP.
        UNUSED(data);
        UNUSED(allocation);
    }


    //
    // RANGE reset.
    //

    static void remove_extra_allocations(Arena &arena, void *until_base) {
        if(arena.base == NULL) {
            return;
        }

        while(true) {
            auto marker = *(Arena_Marker *)arena.base;
            if(arena.base == until_base || marker.base == NULL) {
                break;
            }

            free(arena.base, *arena.allocator);

            arena.base     = marker.base;
            arena.capacity = marker.capacity;
        }

        arena.used = sizeof(Arena_Marker);
    }

    void reset(Arena &arena, Arena_State old_state) {
        auto used = max(old_state.used, sizeof(Arena_Marker));
        auto base = old_state.base;

        if(arena.base != base) {
            remove_extra_allocations(arena, base);
        }

        arena.used = used;
    }

}

