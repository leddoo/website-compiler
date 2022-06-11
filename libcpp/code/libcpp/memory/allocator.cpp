#include <libcpp/memory/allocator.hpp>

#if LIBCPP_USE_LIBC

using libcpp::Allocator;
using libcpp::Usize;

#include <stdlib.h>

// NOTE(llw): In global namespace because I'm scared "free" won't link
// correctly and cause infinite recursion by calling libcpp::free.

static void *malloc_allocate(Allocator *data, Usize size, Usize alignment) {
    UNUSED(data);

    auto result = malloc(size);
    assert(result != NULL);
    assert((Usize)result % alignment == 0);

    return result;
}

static void malloc_free(Allocator *data, void *allocation) {
    UNUSED(data);
    free(allocation);
}


namespace libcpp {

    Allocator malloc_allocator = { malloc_allocate, malloc_free };

    Allocator &default_allocator = malloc_allocator;

}

#endif

