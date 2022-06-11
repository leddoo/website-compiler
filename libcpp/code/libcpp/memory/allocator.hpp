#pragma once

#include <libcpp/base.hpp>
#include <libcpp/memory/util.hpp>

namespace libcpp {

    //
    // RANGE allocator interface.
    //

    struct Allocator {
        typedef void *(Proc_allocate)(Allocator *data, Usize size, Usize alignment);
        typedef void  (Proc_free    )(Allocator *data, void *allocation);

        Proc_allocate *allocate;
        Proc_free     *free;
    };

    extern Allocator &default_allocator;


    //
    // RANGE basic Allocator call wrappers.
    //

    _inline void *allocate_uninitialized(
        Usize size, Usize alignment,
        Allocator &allocator = default_allocator
    ) {
        assert(allocator.allocate != NULL);
        auto result = allocator.allocate(&allocator, size, alignment);
        return result;
    }

    template <typename T>
    _inline void free(
        T *&allocation,
        Allocator &allocator = default_allocator
    ) {
        assert(allocator.free != NULL);
        allocator.free(&allocator, (void *)allocation);
        allocation = NULL;
    }


    //
    // RANGE single value allocation.
    //

    template <typename T>
    _inline T *allocate_uninitialized(
        Allocator &allocator = default_allocator
    ) {
        auto result = (T *)allocate_uninitialized(
            sizeof(T), alignof(T),
            allocator
        );
        return result;
    }

    template <typename T>
    _inline T *allocate(
        Allocator &allocator = default_allocator,
        T value = {}
    ) {
        auto result = allocate_uninitialized<T>(allocator);
        *result = value;
        return result;
    }


    //
    // RANGE array allocation.
    //

    template <typename T>
    _inline T *allocate_array_uninitialized(
        Usize count,
        Allocator &allocator = default_allocator
    ) {
        auto result = (T *)allocate_uninitialized(
            count*sizeof(T), alignof(T),
            allocator
        );
        return result;
    }

    template <typename T>
    _inline T *allocate_array(
        Usize count,
        Allocator &allocator = default_allocator,
        T value = {}
    ) {
        auto result = allocate_array_uninitialized<T>(
            count,
            allocator
        );
        set_values(result, value, count);
        return result;
    }


    //
    // RANGE malloc allocator.
    //

#if LIBCPP_USE_LIBC
    extern Allocator malloc_allocator;
#endif

}

