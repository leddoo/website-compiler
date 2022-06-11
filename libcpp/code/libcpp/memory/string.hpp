#pragma once

#include <libcpp/base.hpp>

namespace libcpp {

    struct String {
        U8 *values;
        Usize size;
    };

    #define STRING(string) ::libcpp::String { (::libcpp::U8 *)string, sizeof(string) - 1 }

}

