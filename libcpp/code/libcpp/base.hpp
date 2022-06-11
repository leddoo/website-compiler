#pragma once

#if   defined(_MSC_VER)
    #define LIBCPP_MSVC
#elif defined(__clang__)
    #define LIBCPP_CLANG
#else
    #error "Unsupported compiler."
#endif

#include <stdint.h>

#if   defined(LIBCPP_MSVC)

    #define _inline       __forceinline
    #define _inline_maybe __inline
    #define _inline_never __declspec(noinline)

    #define UNUSED(x) (void)x

#elif defined(LIBCPP_CLANG)

    #define _inline       __attribute__((always_inline)) inline
    #define _inline_maybe inline
    #define _inline_never __attribute__((never_inline))

    #define UNUSED(x) (void)x

    #define NULL 0

#else
    #error "Unimplemented."
#endif

#define _LIBCPP_STRINGIFY(x) #x
#define LIBCPP_STRINGIFY(x) _LIBCPP_STRINGIFY(x)
#define _LIBCPP_CONCAT(a, b) a##b
#define LIBCPP_CONCAT(a, b) _LIBCPP_CONCAT(a, b)

#define KIBI(n) ((Usize)1024*(Usize)(n))
#define MEBI(n) ((Usize)1024*KIBI(n))
#define GIBI(n) ((Usize)1024*MEBI(n))

#include "config.hpp"


namespace libcpp {

    using S8  = int8_t;
    using S16 = int16_t;
    using S32 = int32_t;
    using S64 = int64_t;
    using U8  = uint8_t;
    using U16 = uint16_t;
    using U32 = uint32_t;
    using U64 = uint64_t;

    using Ssize = intptr_t;
    using Usize = uintptr_t;

    using Signed   = S64;
    using Unsigned = U64;

    using F32 = float;
    using F64 = double;

}

