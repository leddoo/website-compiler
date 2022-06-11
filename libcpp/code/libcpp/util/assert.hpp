#pragma once

#include <libcpp/base.hpp>
#include <libcpp/memory/string.hpp>

namespace libcpp {

    typedef void(Proc_assert)(String, String);

    void _default_assert(String condition, String source_location);
    void _current_assert(String condition, String source_location);

    #if LIBCPP_ENABLE_ASSERT

        #define assert_with(the_assert, condition)                          \
            if(!(condition)) {                                              \
                the_assert(                                                 \
                    STRING(#condition),                                     \
                    STRING(__FILE__ ":" LIBCPP_STRINGIFY(__LINE__))         \
                );                                                          \
            }

        #define assert(condition)                                           \
            assert_with(_current_assert, condition)

        #define default_assert(condition)                                   \
            assert_with(_default_assert, condition)

    #else

        #define assert(condition)                                           \
            auto LIBCPP_CONCAT(__assert_, __LINE__) = condition;            \
            UNUSED(LIBCPP_CONCAT(__assert_, __LINE__))

        #define default_assert(condition) assert(condition)

    #endif

    void push_assert(Proc_assert *proc_assert);
    void pop_assert();

}

