#pragma once

#include <libcpp/base.hpp>
#include <libcpp/memory/allocator.hpp>
#include <libcpp/util/defer.hpp>

namespace libcpp {

    struct Arena : public Allocator {
        Allocator *allocator;
        void *base;
        Usize used;
        Usize capacity;
        Usize block_size;
    };

    Arena create_arena(
        Allocator &backing = default_allocator,
        Usize block_size = LIBCPP_ARENA_DEFAULT_BLOCK_SIZE
    );
    void destroy(Arena &arena);

    void *arena_allocate(Allocator *data, Usize size, Usize alignment);
    void arena_free(Allocator *data, void *allocation);

    struct Arena_State {
        void *base;
        Usize used;
    };

    _inline Arena_State get_state(const Arena &arena) {
        auto result = Arena_State { arena.base, arena.used };
        return result;
    }
    void reset(Arena &arena, Arena_State old_state);

    #define TEMP_SCOPE(arena)                                               \
        auto __old_arena_state = get_state(arena);                          \
        defer { ::libcpp::reset((arena), __old_arena_state); }

}

