#pragma once

#include <libcpp/base.hpp>
#include <libcpp/memory/allocator.hpp>

namespace libcpp {

    template <typename T>
    struct Array {
        Allocator *allocator;
        T *values;
        Usize count;
        Usize capacity;

        _inline T &operator[](Usize index) {
            assert(index < count);
            return values[index];
        }

        _inline const T &operator[](Usize index) const {
            assert(index < count);
            return values[index];
        }
    };


    // interfaces.
    template <typename T> bool eq(const Array<T> &left, const Array<T> &right);
    //template <typename T> Usize hash(const Array<T> &key);

    // lifecycle.
    template <typename T> Array<T> create_array(
        Allocator &allocator = default_allocator,
        Usize initial_capacity = 0
    );
    template <typename T> void destroy(Array<T> &array);

    // capacity manipulation.
    template <typename T> void set_capacity(Array<T> &array, Usize new_capacity);
    template <typename T> void reserve(Array<T> &array, Usize count);
    template <typename T> void grow(Array<T> &array, Usize count);
    template <typename T> void grow_by(Array<T> &array, Usize delta);

    // count manipulation.
    template <typename T> void set_count(Array<T> &array, Usize count, T new_value = {});
    template <typename T> void set_min_count(Array<T> &array, Usize min_count, T new_value = {});
    template <typename T> void add_count(Array<T> &array, Ssize delta, T new_value = {});

    // querying.
    template <typename T> Usize get_first_index(const Array<T> &array, const T &value);
    template <typename T> bool has(const Array<T> &array, const T &value);

    // addition.
    template <typename T> void push(Array<T> &array, const T &value);
    template <typename T> void push(Array<T> &array, const Array<T> &other);
    template <typename T> void push_at(Array<T> &array, Usize index, const T &value);
    template <typename T> void push_at(Array<T> &array, Usize index, const Array<T> &other);
    template <typename T> void push_front(Array<T> &array, const T &value);
    template <typename T> void push_front(Array<T> &array, const Array<T> &other);
    template <typename T> bool push_unique(Array<T> &array, T value);
    template <typename T> T &push(Array<T> &array);

    // removal.
    template <typename T> T pop(Array<T> &array);
    template <typename T> void remove_at(Array<T> &array, Usize index);
    template <typename T> void remove_swap(Array<T> &array, Usize index);
    template <typename T> void remove_leading(Array<T> &array, Usize count);

    // util.
    template <typename T> void clear(Array<T> &array);
    template <typename T> void reset(Array<T> &array);
    template <typename T> Array<T> duplicate(const Array<T> &array, Allocator &allocator);
    template <typename T> Array<T> duplicate(const Array<T> &array);
    template <typename T> T &last(Array<T> &array, Usize offset = 0);
    template <typename T> Array<T> sub_array(Array<T> &array, Usize begin, Usize count);
    template <typename T> Usize remaining_capacity(Array<T> &array);
    template <typename T> Usize data_size(Array<T> &array);
    template <typename T> void make_space_at(Array<T> &array, Usize index, Usize amount);
    template <typename T> void reverse(Array<T> &array);

}

#include "array.inl"

