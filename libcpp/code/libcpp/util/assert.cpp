#include <libcpp/util/assert.hpp>

namespace libcpp {

    static Usize assert_stack_count = 1;
    static constexpr Usize assert_stack_capacity = 64;
    static Proc_assert *assert_stack[assert_stack_capacity] = { _default_assert };

    void _current_assert(String condition, String source_location) {
        assert_stack[assert_stack_count - 1](condition, source_location);
    }

    void push_assert(Proc_assert *proc_assert) {
        default_assert(assert_stack_count < assert_stack_capacity);

        assert_stack[assert_stack_count] = proc_assert;
        assert_stack_count += 1;
    }

    void pop_assert() {
        default_assert(assert_stack_count > 1);

        assert_stack_count -= 1;
    }

}

