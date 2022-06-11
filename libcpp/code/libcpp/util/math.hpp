#pragma once

#include <libcpp/base.hpp>

namespace libcpp {

    template <typename T>
    T min(T left, T right) { return left < right ? left : right; }

    template <typename T>
    T max(T left, T right) { return left > right ? left : right; }

    template <typename T>
    T clamp(T x, T low, T high) { return max(min(x, high), low); }

    template <typename T>
    T abs(T x) { return x >= (T)0 ? x : -x; }

    inline U64 leading_zeros(U64 value) {
        U64 result = 0;
        U64 mask = 1ull << 63;

        for(Usize i = 0; i < 64; i += 1) {
            if(value & mask) {
                break;
            }
            result += 1;
            mask >>= 1;
        }

        return result;
    }

    template <typename T>
    bool is_power_of_two(T number) {
        auto result = (number != 0) && ( (number & (number - 1)) == 0 );
        return result;
    }

    template <typename T>
    _inline T next_power_of_two(T value) {
        auto result = (T)1;
        if(value > (T)1) {
            result = (T)1 << ( (T)(sizeof(T)*8) - (T)leading_zeros((U64)(value - 1)));
        }
        return result;
    }
}

