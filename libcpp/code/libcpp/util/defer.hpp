#pragma once

#include <libcpp/base.hpp>

namespace libcpp {

    template <typename F>
    struct _Defer {
        F f;

        _Defer(F f) : f(f) {}
        ~_Defer() { f(); }
    };

    struct _Defer_Maker {
        template<typename F>
        _Defer<F> operator+(F &&f)
        {
            return _Defer<F>(f);
        }
    };

    #define defer auto LIBCPP_CONCAT(_defer, __LINE__) = ::libcpp::_Defer_Maker {} + [&]()

}

