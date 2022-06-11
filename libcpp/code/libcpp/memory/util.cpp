#include <libcpp/memory/util.hpp>

void *memset(void *dest, libcpp::U8 src, libcpp::Usize count) {
    auto write = (libcpp::U8 *)dest;
    auto end = write + count;
    while(write < end) {
        *write = src;
        write += 1;
    }
    return dest;
}

void memcpy(void *dest, const void *src, libcpp::Usize count) {
    auto write = (libcpp::U8 *)dest;
    auto read = (libcpp::U8 *)src;
    auto end = write + count;
    while(write < end) {
        *write = *read;
        write += 1;
        read += 1;
    }
}

