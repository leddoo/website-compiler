#pragma once

#include <libcpp/base.hpp>
#include <libcpp/util/assert.hpp>
#include <libcpp/memory/string.hpp>

#if LIBCPP_USE_LIBC
    #include <cstring>
#else
    extern "C" void *memset(void *dest, libcpp::U8 src, libcpp::Usize count);
    extern "C" void memcpy(void *dest, const void *src, libcpp::Usize count);
#endif


namespace libcpp {

    //
    // Range array utils.
    //

    _inline void set_bytes(void *dest, U8 src, Usize count) {
        memset(dest, src, count);
    }

    template <typename T>
    void set_values(T *dest, const T &src, Usize count) {
        for(Usize i = 0; i < count; i += 1) {
            dest[i] = src;
        }
    }

    template <typename T>
    void set_values(T *begin, T *end, const T &src) {
        if(begin < end) {
            set_values(begin, src, (Usize)(end - begin));
        }
    }


    _inline void copy_bytes(void *dest, const void *src, Usize count) {
        assert(( ((U8 *)src + count) <= (U8 *)dest )
            || ( ((U8 *)dest + count) <= (U8 *)src )
        );
        memcpy(dest, src, count);
    }

    template <typename T>
    _inline void copy_values_ltr(T *dest, T *src, Usize count) {
        for(Usize i = 0; i < count; i += 1) {
            dest[i] = src[i];
        }
    }

    template <typename T>
    _inline void copy_values_rtl(T *dest, T *src, Usize count) {
        for(Usize i = count; i > 0; i -= 1) {
            dest[i - 1] = src[i - 1];
        }
    }

    template <typename T>
    _inline void copy_values(T *dest, const T *src, Usize count) {
        assert(src + count <= dest
            || dest + count <= src
        );
        copy_values_ltr(dest, (T *)src, count);
    }


    _inline void copy_bytes_ltr(void *dest, void *src, Usize count) {
        return copy_values_ltr((U8 *)dest, (U8 *)src, count);
    }

    _inline void copy_bytes_rtl(void *dest, void *src, Usize count) {
        return copy_values_rtl((U8 *)dest, (U8 *)src, count);
    }


    //
    // RANGE semantic equality.
    //

    template <typename Key>
    _inline bool eq(Key left, Key right) {
        auto result = left == right;
        return result;
    }

    template <typename T>
    bool array_eq(
        const T *left,  Usize left_count,
        const T *right, Usize right_count
    ) {
        if(left_count != right_count) {
            return false;
        }
        else {
            for(Usize i = 0; i < left_count; i += 1) {
                if(!eq(left[i], right[i])) {
                    return false;
                }
            }
        }

        return true;
    }

    template <>
    _inline bool eq(String left, String right) {
        return array_eq(
            left.values,  left.size,
            right.values, right.size
        );
    }



    //
    // RANGE other memory utils.
    //

    template <typename T>
    _inline void swap(T &left, T &right) {
        T temp = left;
        left = right;
        right = temp;
    }


    _inline Usize alignment_offset(Usize pointer, Usize alignment) {
        auto aligned = ( pointer + (alignment - 1) ) & ~(alignment - 1);
        auto offset  = aligned - pointer;
        return offset;
    }

}

